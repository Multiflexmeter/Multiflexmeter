#if defined(BOARD_MFM_V3_M1284P)

#include "board.h"
#include <avr/io.h>

void board_setup(void)
{
  // CLKPR = 1 << CLKPCE;
  // CLKPR = 0b0001;
}

#endif // BOARD_MFM_V3_M1284P