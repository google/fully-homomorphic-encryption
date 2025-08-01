#include "transpiler/examples/redact_ssn/redact_ssn.h"

#pragma hls_top
void RedactSsn(char my_string[MAX_LENGTH]) {
  // pattern 1: "ddddddddd"
  int consecutive_digits_p1 = 0;

  // pattern 2: "ddd-dd-dddd"
  int consecutive_digits_p2 = 0;

#pragma hls_unroll yes
  for (int i = 0; i < MAX_LENGTH && my_string[i] != '\0'; i++) {
    if (my_string[i] >= '0' && my_string[i] <= '9') {
      consecutive_digits_p1++;
      consecutive_digits_p2++;
    } else if (my_string[i] == '-' &&
               (consecutive_digits_p2 == 3 || consecutive_digits_p2 == 6)) {
      consecutive_digits_p2++;
      consecutive_digits_p1 = 0;
    } else {
      consecutive_digits_p1 = 0;
      consecutive_digits_p2 = 0;
    }

    if (i < MAX_LENGTH - 1 &&
        (consecutive_digits_p1 == 9 || consecutive_digits_p2 == 11) &&
        ((my_string[i + 1] >= '0' && my_string[i + 1] <= '9') ||
         my_string[i + 1] == '-')) {
      consecutive_digits_p1 = 0;
      consecutive_digits_p2 = 0;
      continue;
    }

    if (consecutive_digits_p1 == 9) {
      my_string[i] = '*';
      my_string[i - 1] = '*';
      my_string[i - 2] = '*';
      my_string[i - 3] = '*';
      my_string[i - 4] = '*';
      my_string[i - 5] = '*';
      my_string[i - 6] = '*';
      my_string[i - 7] = '*';
      my_string[i - 8] = '*';
      consecutive_digits_p1 = 0;
    }

    if (consecutive_digits_p2 == 11) {
      my_string[i] = '*';
      my_string[i - 1] = '*';
      my_string[i - 2] = '*';
      my_string[i - 3] = '*';
      my_string[i - 5] = '*';
      my_string[i - 6] = '*';
      my_string[i - 8] = '*';
      my_string[i - 9] = '*';
      my_string[i - 10] = '*';
      consecutive_digits_p2 = 0;
    }
  }
}