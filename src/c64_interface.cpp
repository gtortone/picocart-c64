
#include "pico/stdlib.h"

#include "c64_interface.h"

void c64_hold_reset(int ms) {
   gpio_put(RESET,1);
   sleep_ms(ms);
}

void c64_release_reset(void) {
   gpio_put(RESET,0);
}

void c64_reset(void) {
   c64_hold_reset();
   sleep_ms(250);
   c64_release_reset();
}
