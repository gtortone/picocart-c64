
#include "pico/stdlib.h"

#include "board.h"

void c64_hold_reset(int ms=250) {
   gpio_put(RESET,0);
   sleep_ms(ms);
}

void c64_release_reset(void) {
   gpio_put(RESET,1);
}

void c64_reset(void) {
   c64_hold_reset();
   sleep_ms(250);
   c64_release_reset();
}


void c64_set_exrom_game(int exrom, int game) {

   gpio_put(EXROM, exrom);
   gpio_put(GAME, game);
}
