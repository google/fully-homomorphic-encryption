#include "transpiler/util/string.h"

#include "absl/strings/ascii.h"

namespace fully_homomorphic_encryption {

// From
// https://github.com/grpc-ecosystem/proto-converter/blob/d77ff301f48bf2e7a0f8935315e847c1a8e00017/src/google/protobuf/util/converter/utility.cc#L283
std::string ToSnakeCase(absl::string_view input) {
  bool was_not_underscore = false;  // Initialize to false for case 1 (below)
  bool was_not_cap = false;
  std::string result;
  result.reserve(input.size() << 1);

  for (size_t i = 0; i < input.size(); ++i) {
    if (absl::ascii_isupper(input[i])) {
      // Consider when the current character B is capitalized:
      // 1) At beginning of input:   "B..." => "b..."
      //    (e.g. "Biscuit" => "biscuit")
      // 2) Following a lowercase:   "...aB..." => "...a_b..."
      //    (e.g. "gBike" => "g_bike")
      // 3) At the end of input:     "...AB" => "...ab"
      //    (e.g. "GoogleLAB" => "google_lab")
      // 4) Followed by a lowercase: "...ABc..." => "...a_bc..."
      //    (e.g. "GBike" => "g_bike")
      if (was_not_underscore &&                     //            case 1 out
          (was_not_cap ||                           // case 2 in, case 3 out
           (i + 1 < input.size() &&                 //            case 3 out
            absl::ascii_islower(input[i + 1])))) {  // case 4 in
        // We add an underscore for case 2 and case 4.
        result.push_back('_');
      }
      result.push_back(absl::ascii_tolower(input[i]));
      was_not_underscore = true;
      was_not_cap = false;
    } else {
      result.push_back(input[i]);
      was_not_underscore = input[i] != '_';
      was_not_cap = true;
    }
  }
  return result;
}

}  // namespace fully_homomorphic_encryption
