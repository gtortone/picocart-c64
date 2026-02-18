
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/stdio_uart.h"
#include "hardware/clocks.h"

#include "board.h"
#include "c64_interface.h"
#include "utils.h"

#include "cart.h"

#define CMD_BUFFER_SIZE 64

#define COMPILER_BARRIER() asm volatile("" ::: "memory")

void run_cart(void);

int main() {
   
   char cmd_buffer[CMD_BUFFER_SIZE];
   uint8_t cmd_index = 0;

   board_setup();

   // init multicore
   multicore_reset_core1();

   printf("START\n");

   c64_set_exrom_game(0, 0);
   c64_reset();
   
   multicore_launch_core1(run_cart);

   printf("\n\n-- PicoCart-64 shell --\n\n");
   printf("> ");
   while(1) {
      while (uart_is_readable(UART_ID)) {
         char c = uart_getc(UART_ID);

         // echo
         if (c == '\r') {
            uart_putc(UART_ID, c);
            uart_putc(UART_ID, '\n');
         } else uart_putc(UART_ID, c);

         if (c == '\r' || c == '\n') {
            cmd_buffer[cmd_index] = '\0';
            trim(cmd_buffer);

            if (strcmp(cmd_buffer, "reset") == 0) {
               printf("C64 reset\n");
               c64_reset();
            } else if (strlen(cmd_buffer) == 0) {
               printf("> ");
               continue;
            } else {
               printf("unknown command\n");
            }
            cmd_index = 0;
            printf("> ");
         } else if (cmd_index < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_index++] = c;
         }
      }
   }
}

//

void __time_critical_func(run_cart)(void) {

   volatile uint64_t all, control;
   volatile uint16_t addr;

   uint32_t irqstatus = save_and_disable_interrupts();

   while(1) {

      while((gpio_get(ROML) == 1) && (gpio_get(ROMH) == 1)) {
         tight_loop_contents();
      }

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if (control & RW_MASK) {
            //DATA_OUT(cart[addr & 0x1FFF]); // 8K
            DATA_OUT(cart[addr & 0x3FFF]); // 16K
            SET_DATA_MODE_OUT
            while(gpio_get(ROML) == 0)
               tight_loop_contents();
            while(gpio_get(ROMH) == 0)
               tight_loop_contents();
            SET_DATA_MODE_IN
      } else {
         DATA_IN;                 
      }
   }
}
