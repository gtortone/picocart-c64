
#include "pico/stdlib.h"

#include "board.h"

void c64_reset(void) {

   gpio_put(RESET, 0);
   sleep_ms(2000);
   gpio_put(RESET, 1);
}

void c64_set_exrom_game(bool exrom, bool game) {

   gpio_put(EXROM, exrom);
   gpio_put(GAME, exrom);
}
