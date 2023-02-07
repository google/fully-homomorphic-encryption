#include "commons.h"

#include <stdio.h>

#include <iostream>
using namespace std;

void subsetImage(unsigned char input[(MAX_PIXELS + 2) * (MAX_PIXELS + 2)],
                 unsigned char window[9], int i, int j) {
  for (int iw = -1; iw < 2; ++iw)
    for (int jw = -1; jw < 2; ++jw)
      window[(iw + 1) * 3 + jw + 1] =
          input[(i + iw + 1) * (MAX_PIXELS + 2) + j + jw + 1];
}

int chooseFilterType() {
  int option;

  while (true) {
    cout << "Choose your filter: 1 - Sharpen filter, 2 - Gaussian blur, 3 - "
            "Ricker wavelet: ";
    cin >> option;

    if (option == 1 || option == 2 || option == 3) {
      return option;
    }

    cout << "Invalid option chosen. Please choose 1-3." << endl;
  }
}
