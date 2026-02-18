
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"

#include "board.h"
#include "cart.h"
#include "c64_roml.pio.h"

int __not_in_flash_func(main)() {

   // overclock
   // set_sys_clock_khz(250000, true);

   //

   gpio_init(EXROM);
   gpio_set_dir(EXROM, GPIO_OUT);
   gpio_put(EXROM, 0);

   //

   gpio_init(GAME);
   gpio_set_dir(GAME, GPIO_OUT);
   gpio_put(GAME, 1);

   //

   gpio_init(RESET);
   gpio_set_dir(RESET, GPIO_OUT);

   gpio_put(RESET, 0);
   sleep_ms(2000);
   gpio_put(RESET, 1);

   //

   PIO pio = pio0;
   uint sm = 0;

   uint offset = pio_add_program(pio, &c64_roml_program);

   pio_sm_config c = c64_roml_program_get_default_config(offset);

   sm_config_set_in_pins(&c, 0);       // A0 base
   for(int i=0; i<=15; i++)
      pio_gpio_init(pio, i);
   pio_sm_set_consecutive_pindirs(pio, sm, 0, 16, GPIO_IN);

   //sm_config_set_out_pins(&c, 16, 8);  // D0-D7
   //for(int i=16; i<=23; i++)
   //   pio_gpio_init(pio, i);
   //pio_sm_set_consecutive_pindirs(pio, sm, 16, 8, GPIO_OUT);
   gpio_init_mask(DATA_GPIO_MASK);
   gpio_set_dir_in_masked(DATA_GPIO_MASK);

   sm_config_set_jmp_pin(&c, ROML);    // ROML
   pio_gpio_init(pio, ROML);
   pio_sm_set_consecutive_pindirs(pio, sm, ROML, 1, GPIO_IN);

   // opzionale: regola clock divider per rallentare SM se serve più tempo
   // sm_config_set_clkdiv(&c, 1.0f);

   // load SM
   pio_sm_init(pio, sm, offset, &c);
   pio_sm_set_enabled(pio, sm, true);

   //uint32_t irqstatus = save_and_disable_interrupts();

   while(1) {
      uint32_t addr_word = pio_sm_get_blocking(pio, sm); // contains 14 bits in LSBs
      DATA_OUT(cart[addr_word & 0x1FFF]); // 8K
      SET_DATA_MODE_OUT;
      asm inline("nop;");
      SET_DATA_MODE_IN;
      pio_sm_put_blocking(pio, sm, cart[addr_word & 0x1FFF]);

      //if (!pio_sm_is_rx_fifo_empty(pio, sm)) {
      //   uint16_t addr = pio_sm_get(pio, sm) & 0x1FFF; // A0-A12
      //   uint8_t data = cart[addr];                     // leggi byte ROM
      //   // rimanda al PIO tramite FIFO
      //   pio_sm_put_blocking(pio, sm, data);
      //}
   }

   return 0;
}
