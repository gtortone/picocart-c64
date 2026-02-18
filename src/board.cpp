
#include "pico/stdlib.h"
#include "hardware/clocks.h"

#include "board.h"

void board_setup(void) {

   // overclock
   set_sys_clock_khz(250000, true);

   gpio_set_function(UART_TX, UART_FUNCSEL_NUM(UART_ID, UART_TX));
   gpio_set_function(UART_RX, UART_FUNCSEL_NUM(UART_ID, UART_RX));
   uart_init(UART_ID, 115200);
   stdio_uart_init_full(UART_ID, 115200, UART_TX, UART_RX);

   gpio_init(PHI2);
   gpio_set_dir(PHI2, GPIO_IN);

   gpio_init(ROML);
   gpio_set_dir(ROML, GPIO_IN);

   gpio_init(ROMH);
   gpio_set_dir(ROMH, GPIO_IN);

   gpio_init(IO1);
   gpio_set_dir(IO1, GPIO_IN);

   gpio_init(IO2);
   gpio_set_dir(IO2, GPIO_IN);

   gpio_init(RW);
   gpio_set_dir(RW, GPIO_IN);

   gpio_init(LED);
   gpio_set_dir(LED, GPIO_OUT);

   gpio_init(EXROM);
   gpio_set_dir(EXROM, GPIO_OUT);

   gpio_init(GAME);
   gpio_set_dir(GAME, GPIO_OUT);

   gpio_init(RESET);
   gpio_set_dir(RESET, GPIO_OUT);

   gpio_init_mask(ADDR_GPIO_MASK | DATA_GPIO_MASK);
   gpio_set_dir_in_masked(ADDR_GPIO_MASK | DATA_GPIO_MASK);
}
