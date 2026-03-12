#ifndef C64_INTERFACE_
#define C64_INTERFACE_

#include "board.h"

void c64_hold_reset(int ms=250);
void c64_release_reset(void);
void c64_reset(void);

inline void c64_set_exrom_game(bool exrom, bool game) {
   gpio_put(EXROM, !exrom);
   gpio_put(GAME, !game);
}

inline void wait_low(int line) {
   while(sio_hw->gpio_in & (1u << line))
      tight_loop_contents();
}

inline void wait_high(int line) {
   while(!(sio_hw->gpio_in & (1u << line)))
      tight_loop_contents();
}

#endif
