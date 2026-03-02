
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
#include "ff.h"

// Ultimax cartridges
#include "carts/kickman-cart.h"
//#include "carts/slalom-cart.h"
//#include "carts/music-cart.h"
//#include "carts/visible-cart.h"

// 8K cartridges
//#include "carts/tenpins-cart.h"
//#include "carts/moondust-cart.h"
//#include "carts/destest-cart.h"
//#include "carts/diag-cart.h"

// 16K cartridges
//#include "carts/toybizarre-cart.h"
//#include "carts/centipede-cart.h"
//#include "carts/galaxian-cart.h"
//#include "carts/moonpatrol-cart.h"

// Magic Desk cartridges
//#include "carts/arkanoid-cart.h"
//#include "carts/attack-cart.h"
//#include "carts/mission-cart.h"
//#include "carts/zynaps-cart.h"

#define CMD_BUFFER_SIZE 64

#define COMPILER_BARRIER() asm volatile("" ::: "memory")

void run_cart_8k(void);
void run_cart_16k(void);
void run_cart_ultimax(void);
void run_cart_magic_desk(void);

int main(void) {
   
   char cmd_buffer[CMD_BUFFER_SIZE];
   uint8_t cmd_index = 0;

   board_setup();

   // mount SD
   FATFS fs;
   FRESULT fr = f_mount(&fs, "", 1);

   // init multicore
   multicore_reset_core1();

   //c64_set_exrom_game(0, 0);         // run_cart_16k
   //c64_set_exrom_game(0, 1);         // run_cart_8k / run_cart_magic_desk
   c64_set_exrom_game(1, 0);         // run_cart_ultimax 
   //c64_set_exrom_game(1, 1);         // <no cartridge>
   
   c64_hold_reset();

   //multicore_launch_core1(run_cart_8k);
   //multicore_launch_core1(run_cart_16k);
   multicore_launch_core1(run_cart_ultimax);
   //multicore_launch_core1(run_cart_magic_desk);

   c64_release_reset();

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

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();
     
      if( !(control & ROML_MASK) ) {
         if (control & RW_MASK) {
            DATA_OUT(cart[addr & 0x1FFF]);
            SET_DATA_MODE_OUT
            wait_high(ROML);
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

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if( !(control & ROML_MASK) || !(control & ROMH_MASK) ) {

         if (control & RW_MASK) {
            DATA_OUT(cart[addr & 0x3FFF]);
            SET_DATA_MODE_OUT
            wait_high(ROML);
            wait_high(ROMH);
            SET_DATA_MODE_IN
         } 
      }
   } // end loop
}

//

/*
 * 0x8000      1000  0000  0000  0000        ROML  (8K)
 * 0x9FFF      1001  1111  1111  1111
 *
 * 0xE000      1110  0000  0000  0000        ROMH  (8K)
 * 0xFFFF      1111  1111  1111  1111
 */

void __time_critical_func(run_cart_ultimax)(void) {

   volatile uint32_t control;
   volatile uint32_t addr;
   int mask = 0x1FFF;

   if(sizeof(cart) > 0x2000)
      mask = 0x3FFF;

   uint32_t irqstatus = save_and_disable_interrupts();

   while(1) {

      control = gpio_get_all();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if( !(control & ROML_MASK) ) {

            DATA_OUT(cart[addr & mask]);
            SET_DATA_MODE_OUT
            wait_high(ROML);
            SET_DATA_MODE_IN

      } else if( !(control & ROMH_MASK) ) {

            DATA_OUT(cart[addr & mask]);
            SET_DATA_MODE_OUT
            wait_high(ROMH);
            SET_DATA_MODE_IN
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

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if (control & RW_MASK) {
         if( !(control & ROML_MASK) ) {

            addr -= 0x8000;
            addr += (bank * 0x2000);
            DATA_OUT(cart[addr]);
            SET_DATA_MODE_OUT
            wait_high(ROML);
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
