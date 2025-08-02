"""Core functions for interacting with GCP TPUs."""

import functools
from typing import Optional

import google.auth
from google.cloud import tpu_v2


@functools.lru_cache()
def tpu_client():
  return tpu_v2.TpuClient()


@functools.lru_cache()
def default_project() -> str:
  _, project = google.auth.default()
  return project


class Core:
  """Core GCP TPU provisioningfunctionality.

  project: The GCP project to use.
  zone: The GCP zone to use.
    https://cloud.google.com/tpu/docs/regions-zones#europe
  """

  def __init__(self, project: Optional[str], zone: Optional[str]) -> None:
    if project is None:
      project = default_project()
    self.project = project

    if zone is None:
      zone = "us-central1-c"
    if zone not in self.available_zones():
      raise ValueError(
          f"{zone=} is not available in {project=}. Available zones are:"
          f" {self.available_zones().keys()}"
      )
    self.zone = zone
    self.region = zone[:-2]

    self.parent = f"projects/{self.project}/locations/{self.zone}"
    print(f"Using {project=} {zone=}")

  @staticmethod
  def add_args(parser) -> None:
    """Adds project and zone arguments to the parser."""
    parser.add_argument("--project", type=str, required=False)
    parser.add_argument("--zone", type=str, required=False)

  @staticmethod
  def from_args(args) -> "Core":
    """Creates a Core object from the given args."""
    return Core(project=args.project, zone=args.zone)

  @functools.lru_cache()
  def available_zones(self) -> dict[str, str]:
    client = tpu_v2.TpuClient()
    resp = client.list_locations(
        {"name": f"projects/{self.project}", "page_size": 1000}
    )
    return {l.location_id: l.name for l in resp.locations}

  def list_nodes(self, all_zones: bool = False) -> list[tpu_v2.Node]:
    """Returns all nodes in the given zones."""
    parents = (
        list(self.available_zones().values()) if all_zones else [self.parent]
    )

    nodes = []
    for parent in parents:
      nodes.extend(
          tpu_client()
          .list_nodes(request={"parent": parent, "page_size": 1000})
          .nodes
      )
    return nodes


def get_node(parent: str, node_id: str) -> tpu_v2.Node:
  return tpu_client().get_node(request={"name": f"{parent}/nodes/{node_id}"})


def start_node(node: tpu_v2.Node) -> tpu_v2.Node:
  """Starts a TPU node."""
  print(f"Starting {node.name} ...")
  node = tpu_client().start_node(request={"name": node.name}).result()
  print(f"Started {node.name}")
  return node


def stop_node(node: tpu_v2.Node) -> tpu_v2.Node:
  """Stops a TPU node."""
  print(f"Stopping {node.name} ...")
  node = tpu_client().stop_node(request={"name": node.name}).result()
  print(f"Stopped {node.name}")
  return node
