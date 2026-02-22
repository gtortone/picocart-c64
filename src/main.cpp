
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/stdio_uart.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"

#include "board.h"
#include "c64_interface.h"
#include "utils.h"

// 8K cartridges
//#include "carts/tenpins-cart.h"           // 8K
//#include "carts/destest-cart.h"

// 16K cartridges
//#include "carts/toybizarre-cart.h"
//#include "carts/centipede-cart.h"
//#include "carts/galaxian-cart.h"
//#include "carts/moonpatrol-cart.h"

// Magic Desk cartridges
//#include "carts/arkanoid-cart.h"
//#include "carts/attack-cart.h"
//#include "carts/mission-cart.h"
#include "carts/zynaps-cart.h"

#define CMD_BUFFER_SIZE 64

#define COMPILER_BARRIER() asm volatile("" ::: "memory")

void run_cart_8k(void);
void run_cart_16k(void);
void run_cart_magic_desk(void);

int main() {
   
   char cmd_buffer[CMD_BUFFER_SIZE];
   uint8_t cmd_index = 0;

   board_setup();

   // init multicore
   multicore_reset_core1();

   printf("START\n");

   //c64_set_exrom_game(0, 0);         // run_cart_16k
   c64_set_exrom_game(0, 1);           // run_cart_8k / run_cart_magic_desk
   //c64_set_exrom_game(1, 0);         // run_cart_ultimax 
   //c64_set_exrom_game(1, 1);         // <no cartridge>
   c64_reset();
   
   //multicore_launch_core1(run_cart_8k);
   //multicore_launch_core1(run_cart_16k);
   multicore_launch_core1(run_cart_magic_desk);

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

         // process command
         if (c == '\r' || c == '\n') {
            cmd_buffer[cmd_index] = '\0';
            trim(cmd_buffer);
            
            char *token = strtok(cmd_buffer, " ");

            // reset command
            if (strcmp(token, "reset") == 0) {
               // parameter: [cpu], [c64]
               token = strtok(NULL, " ");
               if (strcmp(token, "c64") == 0) {
                  printf("C64 reset\n");
                  c64_reset();
               } else if (strcmp(token, "pico") == 0) {
                  printf("PICO reset\n");
                  watchdog_enable(100, 0);
               } else {
                  printf("reset [c64 | pico]\n");
               }
            } else if (strcmp(token, "test") == 0) {
               printf("test\n");
            } else if (strlen(cmd_buffer) == 0) {
               printf("> ");
               continue;
            } else {
               printf("unknown command\n");
            }
            cmd_index = 0; printf("> ");
         } else if (cmd_index < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_index++] = c;
         }
      }
   }
}

//

void __time_critical_func(run_cart_8k)(void) {

   volatile uint64_t control;
   volatile uint32_t addr;

   uint32_t irqstatus = save_and_disable_interrupts();

   while(1) {

      while(gpio_get(PHI2) == 0)
         tight_loop_contents();

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      //if( !(control & ROML_MASK) || !(control & ROMH_MASK) ) {
      if( !(control & ROML_MASK) ) {

         if (control & RW_MASK) {
               DATA_OUT(cart[addr & 0x1FFF]);
               SET_DATA_MODE_OUT
               while(gpio_get(PHI2) == 1)
                  tight_loop_contents();
               SET_DATA_MODE_IN
         } 
      }
   } // end loop
}

//

void __time_critical_func(run_cart_16k)(void) {

   volatile uint64_t control;
   volatile uint32_t addr;

   uint32_t irqstatus = save_and_disable_interrupts();

   while(1) {

      while(gpio_get(PHI2) == 0)
         tight_loop_contents();

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if( !(control & ROML_MASK) || !(control & ROMH_MASK) ) {

         if (control & RW_MASK) {
               DATA_OUT(cart[addr & 0x3FFF]);
               SET_DATA_MODE_OUT
               while(gpio_get(PHI2) == 1)
                  tight_loop_contents();
               SET_DATA_MODE_IN
         } 
      }
   } // end loop
}

//

void __time_critical_func(run_cart_magic_desk)(void) {

   volatile uint64_t control;
   volatile uint32_t addr;
   volatile uint8_t data;
   uint8_t bank = 0;

   uint32_t irqstatus = save_and_disable_interrupts();

   while(1) {

      while(gpio_get(PHI2) == 0)
         tight_loop_contents();

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if (control & RW_MASK) {
         if( !(control & ROML_MASK) ) {

            addr -= 0x8000;
            addr += (bank * 0x2000);
            DATA_OUT(cart[addr]);
            SET_DATA_MODE_OUT
            while(gpio_get(PHI2) == 1)
               tight_loop_contents();
            SET_DATA_MODE_IN
         }

      } else {
         
         SET_DATA_MODE_IN
         data = DATA_IN;
         if( !(control & IO1_MASK) && (addr == 0xDE00) ) {
            if ( !(data & 0x80)) {
               c64_set_exrom_game(0, 1);
               bank = data;
            } else {
               c64_set_exrom_game(1, 1);
            }
         }
      }
   }  // end loop
}
