#include <unordered_map>

#include "absl/status/status.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"

absl::Status my_package(LweSample* result, LweSample* st, LweSample* c,
                        const TFheGateBootstrappingCloudKeySet* bk) {
  std::unordered_map<int, LweSample*> temp_nodes;

  temp_nodes[971] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCOPY(temp_nodes[971], &c[7], bk);

  temp_nodes[968] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCOPY(temp_nodes[968], &c[4], bk);

  temp_nodes[972] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[972], temp_nodes[971], bk);

  temp_nodes[973] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[973], temp_nodes[968], bk);

  temp_nodes[967] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCOPY(temp_nodes[967], &c[3], bk);

  temp_nodes[982] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[982], temp_nodes[972], temp_nodes[973], bk);

  temp_nodes[974] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[974], temp_nodes[967], bk);

  temp_nodes[966] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCOPY(temp_nodes[966], &c[2], bk);

  temp_nodes[970] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCOPY(temp_nodes[970], &c[6], bk);

  temp_nodes[969] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCOPY(temp_nodes[969], &c[5], bk);

  temp_nodes[983] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[983], temp_nodes[982], temp_nodes[974], bk);

  temp_nodes[975] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[975], temp_nodes[966], bk);

  temp_nodes[965] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCOPY(temp_nodes[965], &c[1], bk);

  temp_nodes[987] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[987], temp_nodes[970], temp_nodes[969], bk);

  temp_nodes[991] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[991], temp_nodes[970], temp_nodes[969], bk);

  temp_nodes[964] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCOPY(temp_nodes[964], &c[0], bk);

  temp_nodes[978] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsOR(temp_nodes[978], temp_nodes[971], temp_nodes[970], bk);

  temp_nodes[980] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsOR(temp_nodes[980], temp_nodes[971], temp_nodes[969], bk);

  temp_nodes[984] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[984], temp_nodes[983], temp_nodes[975], bk);

  temp_nodes[976] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[976], temp_nodes[965], bk);

  temp_nodes[988] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[988], temp_nodes[987], temp_nodes[968], bk);

  temp_nodes[992] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[992], temp_nodes[991], temp_nodes[968], bk);

  temp_nodes[977] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[977], temp_nodes[964], bk);

  temp_nodes[979] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[979], temp_nodes[978], bk);

  temp_nodes[981] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[981], temp_nodes[980], bk);

  temp_nodes[985] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[985], temp_nodes[984], temp_nodes[976], bk);

  temp_nodes[989] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[989], temp_nodes[988], temp_nodes[967], bk);

  temp_nodes[993] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[993], temp_nodes[992], temp_nodes[967], bk);

  temp_nodes[1009] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[1009], temp_nodes[977], temp_nodes[976], bk);

  temp_nodes[998] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsOR(temp_nodes[998], temp_nodes[979], temp_nodes[981], bk);

  temp_nodes[986] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[986], temp_nodes[985], temp_nodes[977], bk);

  temp_nodes[990] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[990], temp_nodes[989], temp_nodes[966], bk);

  temp_nodes[994] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[994], temp_nodes[993], temp_nodes[965], bk);

  temp_nodes[1010] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[1010], temp_nodes[1009], temp_nodes[975], bk);

  temp_nodes[999] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsOR(temp_nodes[999], temp_nodes[998], temp_nodes[986], bk);

  temp_nodes[1000] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsOR(temp_nodes[1000], temp_nodes[971], temp_nodes[990], bk);

  temp_nodes[995] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[995], temp_nodes[994], temp_nodes[964], bk);

  temp_nodes[1011] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[1011], temp_nodes[1010], temp_nodes[974], bk);

  temp_nodes[997] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCOPY(temp_nodes[997], &st[0], bk);

  temp_nodes[1002] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[1002], temp_nodes[999], bk);

  temp_nodes[1001] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsOR(temp_nodes[1001], temp_nodes[1000], temp_nodes[995], bk);

  temp_nodes[1012] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[1012], temp_nodes[1011], temp_nodes[973], bk);

  temp_nodes[1004] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[1004], temp_nodes[997], temp_nodes[1002], bk);

  temp_nodes[1003] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[1003], temp_nodes[1001], bk);

  temp_nodes[1013] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[1013], temp_nodes[1012], temp_nodes[969], bk);

  temp_nodes[1007] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[1007], temp_nodes[970], bk);

  temp_nodes[1005] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[1005], temp_nodes[1004], temp_nodes[1003], bk);

  temp_nodes[1014] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[1014], temp_nodes[1013], temp_nodes[1007], bk);

  temp_nodes[1006] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsNOT(temp_nodes[1006], temp_nodes[1005], bk);

  temp_nodes[1015] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[1015], temp_nodes[1014], temp_nodes[972], bk);

  temp_nodes[1008] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsAND(temp_nodes[1008], temp_nodes[969], temp_nodes[1006], bk);

  temp_nodes[960] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCONSTANT(temp_nodes[960], 1, bk);

  temp_nodes[961] = new_gate_bootstrapping_ciphertext(bk->params);
  bootsCONSTANT(temp_nodes[961], 0, bk);

  bootsCOPY(&result[7], temp_nodes[971], bk);
  bootsCOPY(&result[6], temp_nodes[970], bk);
  bootsCOPY(&result[5], temp_nodes[1008], bk);
  bootsCOPY(&result[4], temp_nodes[968], bk);
  bootsCOPY(&result[3], temp_nodes[967], bk);
  bootsCOPY(&result[2], temp_nodes[966], bk);
  bootsCOPY(&result[1], temp_nodes[965], bk);
  bootsCOPY(&result[0], temp_nodes[964], bk);
  bootsCOPY(&st[0], temp_nodes[1015], bk);
  for (auto pair : temp_nodes) {
    delete_gate_bootstrapping_ciphertext(pair.second);
  }
  return absl::OkStatus();
}
