#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_UTIL_STRING_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_UTIL_STRING_H_

#include "absl/strings/string_view.h"

namespace fully_homomorphic_encryption {

std::string ToSnakeCase(absl::string_view input);

}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_UTIL_STRING_H_
