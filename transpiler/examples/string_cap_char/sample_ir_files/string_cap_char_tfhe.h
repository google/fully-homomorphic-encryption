#ifndef STRING_CAP_CHAR_TFHE_H_
#define STRING_CAP_CHAR_TFHE_H_

#include "absl/status/status.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"

absl::Status my_package(LweSample* result, LweSample* st, LweSample* c,
                        const TFheGateBootstrappingCloudKeySet* bk);
#endif  // STRING_CAP_CHAR_TFHE_H_
