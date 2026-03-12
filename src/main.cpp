
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/stdio_uart.h"
#include "pico/i2c_slave.h"
#include "hardware/clocks.h"
#include "hardware/flash.h"
#include "hardware/watchdog.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"

#include "board.h"
#include "i2c_handler.h"
#include "c64_interface.h"
#include "utils.h"
#include "ff.h"
#include "cartridge.h"

#define CMD_BUFFER_SIZE 64

void run_shell(void);
void run_read_test(void);

int main(void) {
   
   board_setup();

   c64_set_exrom_game(1, 1);         // <no cartridge>
   c64_reset();

   // mount SD
   FATFS fs;
   FRESULT fr = f_mount(&fs, "", 1);

   if(fr != FR_OK)
      printf("E: SD mount failed\n");

   // setup I2C irq handler
   i2c_slave_init(i2c0, I2C_ADDR, &i2c_slave_handler);
   i2c_init_regspace();

   run_shell();
   //run_read_test();

   return 0;
}

void run_shell(void) {

   const int cmd_buffer_size = 64;
   char cmd_buffer[cmd_buffer_size];
   uint8_t cmd_index = 0;
   uint8_t rc;
   extern bool skip;

   printf("\n\n-- PicoCart-64 shell --\n\n");
   printf("> ");
   while(1) {
      while (uart_is_readable(UART_ID)) {
         char c = uart_getc(UART_ID);

         if (c == '\r') {     // cr + lf
            uart_putc(UART_ID, c);
            uart_putc(UART_ID, '\n');
         } else if (c == '\b') {   // backspace or del
            if (cmd_index > 0) {
               cmd_index--;
               uart_putc(UART_ID, '\b');
               uart_putc(UART_ID, ' ');
               uart_putc(UART_ID, '\b');
            } 
            continue;
         } else uart_putc(UART_ID, c);    // echo

         // process command
         if (c == '\r' || c == '\n') {
            cmd_buffer[cmd_index] = '\0';
            trim(cmd_buffer);
            
            char *token = strtok(cmd_buffer, " ");

            if (strcmp(token, "skip") == 0) {
              skip = !skip;
              printf("skip: %d\n", skip);
            } else if (strcmp(token, "reset") == 0) {
               // reset command
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
            } else if (strcmp(token, "load") == 0) {
               // load file
               // parameter: <filename>
               token = strtok(NULL, " ");
               run_cart(token);
            } else if (strcmp(token, "ls") == 0) {
               // list files/directories
               FRESULT res;
               DIR dir;
               FILINFO fno;
               res = f_opendir(&dir, "/");
               if (res == FR_OK) {
                  while (1) {
                     if ( (f_readdir(&dir, &fno) != FR_OK) || (fno.fname[0] == 0) )
                        break;
                     if(fno.fattrib & (AM_HID | AM_SYS))
                        continue;
                     char *name = *fno.fname ? fno.fname : fno.fname;
                     if (fno.fattrib & AM_DIR)
                        printf("[DIR] %s\n", name);
                     else
                        printf("[FILE] %s (%lu bytes)\n", name, fno.fsize);
                  }
                  f_closedir(&dir);
               } else {
                  printf("E: ls error %d\n", res);
               }
            } else if (strcmp(token, "test") == 0) {
               //
            } else if (strcmp(token, "info") == 0) {
               extern char __data_start__;
               extern char __data_end__;
               extern char __bss_start__;
               extern char __bss_end__;
               extern char __StackLimit;
               extern char __StackTop;
               uint32_t data_size = &__data_end__ - &__data_start__;
               uint32_t bss_size  = &__bss_end__ - &__bss_start__;
               uint32_t stack_size = &__StackTop - &__StackLimit;
               printf("DATA:  %u bytes\n", data_size);
               printf("BSS:   %u bytes\n", bss_size);
               printf("STACK: %u bytes reserved\n", stack_size);
               printf("CPU frequency: %.0f MHz\n", clock_get_hz(clk_sys) / 1e6);
               printf("flash size: %ld bytes\n", PICO_FLASH_SIZE_BYTES);
               printf("flash sector size: %ld bytes\n", FLASH_SECTOR_SIZE);
               printf("flash page size: %ld bytes\n", FLASH_PAGE_SIZE);
               printf("flash area address: 0x%X\n", FLASH_AREA_OFFSET);
               printf("XIP_BASE address: 0x%X\n", XIP_BASE);
            } else if (strcmp(token, "run") == 0) {
               run_cart(NULL, false);
            } else if (strlen(cmd_buffer) == 0) {
               printf("> ");
               continue;
            } else {
               printf("%s: unknown command\n", cmd_buffer);
            }
            cmd_index = 0;
            printf("> ");
         } else if (cmd_index < cmd_buffer_size - 1) {
            cmd_buffer[cmd_index++] = c;
         }
      }
   }
}

void run_read_test(void) {

   volatile uint32_t control;
   volatile uint16_t addr;
   volatile uint8_t data;

   uint8_t arr[300];
   int i = 0;

   SET_DATA_MODE_IN
   while(1) {

      GPIO_GET_LOW_32(control);
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if (control & RW_MASK) {

      } else {

         // 0xDF1C  --  POKE 57116,2
         // 10 FOR X=1 to 255
         // 20 POKE 57116, X
         // 30 NEXT X
         // 40 POKE 57117, 1

         if ( !(control & IO2_MASK) ) {
            if (addr == 0xDF1C) {
               arr[i++] = DATA_IN;
            } else if (addr == 0xDF1D) {
               for(int b=0; b<i; b++)
                  printf("%d) %d\n", b, arr[b]);
               i = 0;
            }
            wait_high(IO2);
         }
      }
   } // end loop
}
