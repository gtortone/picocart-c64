#ifndef C64_INTERFACE_
#define C64_INTERFACE_

void c64_hold_reset(int ms=250);
void c64_release_reset(void);
void c64_reset(void);

void c64_set_exrom_game(int exrom, int game);

inline void wait_low(int line) {
   while (gpio_get(line))
      tight_loop_contents();
}

inline void wait_high(int line) {
   while (!(gpio_get(line)))
      tight_loop_contents();
}

#endif
