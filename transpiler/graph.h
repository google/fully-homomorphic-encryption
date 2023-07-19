// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_GRAPH_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_GRAPH_H_

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "xls/common/status/status_macros.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

template <typename K, typename V>
static absl::flat_hash_set<K> Keys(const absl::flat_hash_map<K, V>& map) {
  absl::flat_hash_set<K> result;
  for (const auto& pair : map) {
    result.insert(pair.first);
  }
  return result;
}

// A graph data structure.
//
// Parameter `V` is the vertex type, which is expected to be cheap to copy.
// Parameter `VW` is the type of vertex weights.
template <typename V, typename VW>
class Graph {
 public:
  // Adds a vertex to the graph with the given `weight` associated to it.
  void AddVertex(const V& vertex, const VW& weight) {
    vertex_weights_[vertex] = weight;
    if (!out_edges_.contains(vertex)) {
      out_edges_[vertex] = {};
    }
    if (!in_edges_.contains(vertex)) {
      in_edges_[vertex] = {};
    }
  }

  // Adds an edge from the given `source` to the given `target`. Returns false
  // if either the source or target is not a previously inserted vertex, and
  // returns true otherwise. The graph is unchanged if false is returned.
  bool AddEdge(const V& source, const V& target) {
    if (!(vertex_weights_.contains(source) &&
          vertex_weights_.contains(target))) {
      return false;
    }
    out_edges_.at(source).insert(target);
    in_edges_.at(target).insert(source);
    return true;
  }

  // Returns true iff the given vertex has previously been added to the graph
  // using `AddVertex`.
  bool Contains(const V& vertex) { return vertex_weights_.contains(vertex); }

  // Returns the set of vertices in the graph. If some set of vertices have been
  // identified, an arbitrary element of that set will be present in this list.
  std::vector<V> Vertices() {
    std::vector<V> result;
    for (const auto& pair : vertex_weights_) {
      result.push_back(pair.first);
    }
    // Note: The vertices are sorted to ensure determinism in the output.
    std::sort(result.begin(), result.end());
    return result;
  }

  // Returns the edges that point out of the given vertex, and their weights.
  std::vector<V> EdgesOutOf(const V& vertex) {
    if (vertex_weights_.contains(vertex)) {
      std::vector<V> result(out_edges_.at(vertex).begin(),
                            out_edges_.at(vertex).end());
      // Note: The vertices are sorted to ensure determinism in the output.
      std::sort(result.begin(), result.end());
      return result;
    }
    return {};
  }

  // Returns the edges that point into the given vertex, and their weights.
  std::vector<V> EdgesInto(const V& vertex) {
    if (vertex_weights_.contains(vertex)) {
      std::vector<V> result(in_edges_.at(vertex).begin(),
                            in_edges_.at(vertex).end());
      // Note: The vertices are sorted to ensure determinism in the output.
      std::sort(result.begin(), result.end());
      return result;
    }
    return {};
  }

  // Returns the weight of the given vertex.
  absl::StatusOr<VW> WeightOf(const V& vertex) {
    if (auto v = vertex_weights_.Find(vertex)) {
      return v.value().second;
    }
    return absl::InvalidArgumentError("Vertex not found");
  }

  // Returns a topological sort of the nodes in the graph if the graph is
  // acyclic, otherwise returns std::nullopt.
  absl::StatusOr<std::vector<V>> TopologicalSort() {
    std::vector<V> result;

    // Kahn's algorithm

    std::vector<V> active;
    absl::flat_hash_map<V, int64_t> edge_count;
    for (const V& vertex : Vertices()) {
      edge_count[vertex] = EdgesInto(vertex).size();
      if (edge_count.at(vertex) == 0) {
        active.push_back(vertex);
      }
    }

    while (!active.empty()) {
      V source = active.back();
      active.pop_back();
      result.push_back(source);
      for (const auto& target : EdgesOutOf(source)) {
        edge_count.at(target)--;
        if (edge_count.at(target) == 0) {
          active.push_back(target);
        }
      }
    }

    if (result.size() != Vertices().size()) {
      return absl::InvalidArgumentError(
          "A cycle was detected in the input graph");
    }

    return result;
  }

  // Find the level of each node in the graph, where the level
  // is the length of the longest path from any input node to that node.
  //
  // Note: this algorithm doesn't optimize for the most "balanced" levels.
  // Algorithms that result in better balancing of nodes across levels include
  // the Coffman-Graham algorithm.
  absl::StatusOr<std::vector<std::vector<V>>> SortGraphByLevels() {
    // Topologically sort the adjacency graph, then reverse it.
    XLS_ASSIGN_OR_RETURN(auto topo_order, TopologicalSort());
    std::reverse(topo_order.begin(), topo_order.end());
    absl::flat_hash_map<V, int> levels;

    // Assign levels to the nodes:
    // Traverse through the reversed topologically sorted nodes
    // (working backwards through the graph, starting from the outputs)
    // and assign the level of each node as 1 + the maximum of all the
    // destinations of that node and -1, such that the first node processed
    // (an output node) will have level = 0.
    int max_level = 0;
    int max_source_level = -1;
    for (const auto& vertex : topo_order) {
      max_source_level = -1;
      for (auto& edge : EdgesOutOf(vertex)) {
        max_source_level = std::max(max_source_level, levels[edge]);
      }
      levels[vertex] = 1 + max_source_level;
      max_level = std::max(levels.at(vertex), max_level);
    }

    // Output will be a vector of vectors of the nodes at each level.
    // Reverse the levels values, such that input nodes have smaller level
    // values.
    std::vector<std::vector<V>> output(max_level + 1);
    for (const auto& entry : levels) {
      output[max_level - entry.second].push_back(entry.first);
    }
    return output;
  }

 private:
  absl::flat_hash_map<V, VW> vertex_weights_;
  absl::flat_hash_map<V, absl::flat_hash_set<V>> out_edges_;
  absl::flat_hash_map<V, absl::flat_hash_set<V>> in_edges_;
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_GRAPH_H_
