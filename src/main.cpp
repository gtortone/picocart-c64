
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
#include "crt.h"

#define CMD_BUFFER_SIZE 64

#define COMPILER_BARRIER() asm volatile("" ::: "memory")

#define CORE1_STACK_SIZE 2048
static uint32_t core1_stack[CORE1_STACK_SIZE];

void run_cart_8k(void);
void run_cart_16k(void);
void run_cart_ultimax(void);
void run_cart_magic_desk(void);
void run_cart_ocean(void);
void run_cart_fun_play(void);
void run_cart_super_games(void);
void run_cart_easyflash(void);

CRTHandler crt;

//

int main(void) {
   
   char cmd_buffer[CMD_BUFFER_SIZE];
   uint8_t cmd_index = 0;
   uint8_t rc;

   board_setup();

   c64_set_exrom_game(1, 1);         // <no cartridge>
   c64_reset();

   // mount SD
   FATFS fs;
   FRESULT fr = f_mount(&fs, "", 1);

   sleep_ms(1500);

   if(fr != FR_OK)
      printf("E: SD mount failed\n");

   // setup I2C irq handler
   i2c_slave_init(i2c0, I2C_ADDR, &i2c_slave_handler);
   i2c_init_regspace();

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

            if (strcmp(token, "reset") == 0) {
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
               multicore_reset_core1();
               crt_clear(&crt);
               // build raw cart
               if((rc = crt_file_open(&crt, token)) == FILE_OK) {
                  crt_file_close(&crt);
                  // debug
                  //crt_print(&crt);
                  // configure C64
                  printf("EXROM: %d, GAME: %d\n", crt.exrom, crt.game);
                  c64_set_exrom_game(crt.exrom, crt.game);
                  printf("CRT size: %d\n", crt.size);
                  if(crt.type == 0) {
                     // normal cartridge (0)
                     if(crt.exrom == 1) {
                        // Ultimax
                        printf("cart: Ultimax\n");
                        multicore_launch_core1_with_stack(run_cart_ultimax, core1_stack, CORE1_STACK_SIZE);
                     } else {
                        // 8K / 16K
                        if(crt.size > 8192) {
                           printf("cart: normal 16K\n");
                           multicore_launch_core1_with_stack(run_cart_16k, core1_stack, CORE1_STACK_SIZE);
                        } else {
                           printf("cart: normal 8K\n");
                           multicore_launch_core1_with_stack(run_cart_8k, core1_stack, CORE1_STACK_SIZE);
                        }
                     }
                  } else if(crt.type == 19) {
                     // Magic Desk (19)
                     printf("cart: Magic Desk\n");
                     multicore_launch_core1_with_stack(run_cart_magic_desk, core1_stack, CORE1_STACK_SIZE);
                  } else if(crt.type == 5) {
                     // Ocean (5)
                     printf("cart: Ocean\n");
                     multicore_launch_core1_with_stack(run_cart_ocean, core1_stack, CORE1_STACK_SIZE);
                  } else if(crt.type == 7) {
                     // Fun Play (7)
                     printf("cart: Fun Play\n");
                     multicore_launch_core1_with_stack(run_cart_fun_play, core1_stack, CORE1_STACK_SIZE);
                  } else if(crt.type == 8) {
                     // Super Games (8)
                     printf("cart: Super Games\n");
                     multicore_launch_core1_with_stack(run_cart_super_games, core1_stack, CORE1_STACK_SIZE);
                  } else if(crt.type == 32) {
                     // EasyFlash (32)
                     printf("cart: EasyFlash\n");
                     multicore_launch_core1_with_stack(run_cart_easyflash, core1_stack, CORE1_STACK_SIZE);
                  }
                  c64_reset();
                  printf("done\n");
               } else {
                  printf("E: %s\n", CRTFileErrorStrings[rc]);
               }
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
               printf("CPU frequency: %.0f MHz\n", clock_get_hz(clk_sys) / 1e6);
               printf("flash size: %ld bytes\n", PICO_FLASH_SIZE_BYTES);
               printf("flash sector size: %ld bytes\n", FLASH_SECTOR_SIZE);
               printf("flash page size: %ld bytes\n", FLASH_PAGE_SIZE);
               printf("flash area address: 0x%X\n", FLASH_AREA_OFFSET);
               printf("XIP_BASE address: 0x%X\n", XIP_BASE);
            } else if (strcmp(token, "run") == 0) {
               multicore_reset_core1();
               crt_clear(&crt);
               // build raw cart
               if((rc = crt_build_banks(&crt)) == FILE_OK) {
                  // debug
                  //crt_print(&crt);
                  // configure C64
                  printf("EXROM: %d, GAME: %d\n", crt.exrom, crt.game);
                  c64_set_exrom_game(crt.exrom, crt.game);
                  printf("CRT size: %d\n", crt.size);
                  if(crt.type == 0) {
                     // normal cartridge (0)
                     if(crt.exrom == 1) {
                        // Ultimax
                        printf("cart: Ultimax\n");
                        multicore_launch_core1_with_stack(run_cart_ultimax, core1_stack, CORE1_STACK_SIZE);
                     } else {
                        // 8K / 16K
                        if(crt.size > 8192) {
                           printf("cart: normal 16K\n");
                           multicore_launch_core1_with_stack(run_cart_16k, core1_stack, CORE1_STACK_SIZE);
                        } else {
                           printf("cart: normal 8K\n");
                           multicore_launch_core1_with_stack(run_cart_8k, core1_stack, CORE1_STACK_SIZE);
                        }
                     }
                  } else if(crt.type == 19) {
                     // Magic Desk (19)
                     printf("cart: Magic Desk\n");
                     multicore_launch_core1_with_stack(run_cart_magic_desk, core1_stack, CORE1_STACK_SIZE);
                  } else if(crt.type == 5) {
                     // Ocean (5)
                     printf("cart: Ocean\n");
                     multicore_launch_core1_with_stack(run_cart_ocean, core1_stack, CORE1_STACK_SIZE);
                  } else if(crt.type == 7) {
                     // Fun Play (7)
                     printf("cart: Fun Play\n");
                     multicore_launch_core1_with_stack(run_cart_fun_play, core1_stack, CORE1_STACK_SIZE);
                  } else if(crt.type == 8) {
                     // Super Games (8)
                     printf("cart: Super Games\n");
                     multicore_launch_core1_with_stack(run_cart_super_games, core1_stack, CORE1_STACK_SIZE);
                  } else if(crt.type == 32) {
                     // EasyFlash (32)
                     printf("cart: EasyFlash\n");
                     multicore_launch_core1_with_stack(run_cart_easyflash, core1_stack, CORE1_STACK_SIZE);
                  }
                  c64_reset();
                  printf("done\n");
               } else {
                  printf("E: %s\n", CRTFileErrorStrings[rc]);
               }
            } else if (strlen(cmd_buffer) == 0) {
               printf("> ");
               continue;
            } else {
               printf("%s: unknown command\n", cmd_buffer);
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

void __time_critical_func(run_cart_8k)(void) {

   volatile uint64_t control;
   volatile uint32_t addr;

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();
     
      if( !(control & ROML_MASK) ) {
         DATA_OUT(crt.bank[0].datal[(addr - crt.bank[0].load_addrl) & 0x1FFF]);
         SET_DATA_MODE_OUT
         wait_high(ROML);
         SET_DATA_MODE_IN
      }
   } // end loop
}

//

void __time_critical_func(run_cart_16k)(void) {

   volatile uint64_t control;
   volatile uint32_t addr;
   uint8_t *datal, *datah;
   uint16_t load_addrl, load_addrh;

   datal = crt.bank[0].datal;
   load_addrl = crt.bank[0].load_addrl;

   if(crt.bank[0].datah != NULL) {
      datah = crt.bank[0].datah;
      load_addrh = crt.bank[0].load_addrh;
   } else {
      datah = crt.bank[0].datal;
      load_addrh = crt.bank[0].load_addrl;
   }

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if( !(control & ROML_MASK) ) {

         DATA_OUT(datal[(addr - crt.bank[0].load_addrl) & 0x3FFF]);
         SET_DATA_MODE_OUT
         wait_high(ROML);
         SET_DATA_MODE_IN

      } else if( !(control & ROMH_MASK) ) {

         DATA_OUT(datah[(addr - crt.bank[0].load_addrh) & 0x3FFF]);
         SET_DATA_MODE_OUT
         wait_high(ROMH);
         SET_DATA_MODE_IN
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
   uint8_t *datal, *datah;
   uint16_t load_addrl, load_addrh;

   datal = crt.bank[0].datal;
   load_addrl = crt.bank[0].load_addrl;

   if(crt.bank[0].datah != NULL) {
      datah = crt.bank[0].datah;
      load_addrh = crt.bank[0].load_addrh;
   } else {
      datah = crt.bank[0].datal;
      load_addrh = crt.bank[0].load_addrl;
   }

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      control = gpio_get_all();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if( !(control & ROML_MASK) ) {

         DATA_OUT(datal[(addr - crt.bank[0].load_addrl) & 0x3FFF]);
         SET_DATA_MODE_OUT
         wait_high(ROML);
         SET_DATA_MODE_IN

      } else if( !(control & ROMH_MASK) ) {

         DATA_OUT(datah[(addr - crt.bank[0].load_addrh) & 0x3FFF]);
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

   SET_DATA_MODE_IN
   while(1) {

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if (control & RW_MASK) {
         if( !(control & ROML_MASK) ) {

            DATA_OUT(crt.bank[bank].datal[(addr - crt.bank[bank].load_addrl)]);
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

//

void __time_critical_func(run_cart_ocean)(void) {

   volatile uint64_t control;
   volatile uint32_t addr;
   volatile uint8_t data;
   uint8_t bank = 0;

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if (control & RW_MASK) {
         if( !(control & ROML_MASK) ) {

            DATA_OUT(crt.bank[bank].datal[(addr - crt.bank[bank].load_addrl) & 0x3FFF]);
            SET_DATA_MODE_OUT
            wait_high(ROML);
            SET_DATA_MODE_IN

         } else if( !(control & ROMH_MASK) ) {

            DATA_OUT(crt.bank[bank].datah[(addr - crt.bank[bank].load_addrh) & 0x3FFF]);
            SET_DATA_MODE_OUT
            wait_high(ROMH);
            SET_DATA_MODE_IN
         }

      } else {
         
         SET_DATA_MODE_IN
         data = DATA_IN;
         if( !(control & IO1_MASK) && (addr == 0xDE00) )
            bank = data & 0x3F;
      }
   }  // end loop
}

//

void __time_critical_func(run_cart_fun_play)(void) {

   volatile uint64_t control;
   volatile uint32_t addr;
   volatile uint8_t data;
   uint8_t bank = 0;

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if (control & RW_MASK) {
         if( !(control & ROML_MASK) ) {

            DATA_OUT(crt.bank[bank].datal[(addr - crt.bank[bank].load_addrl)]);
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
               //bank = ( (data >> 3) & 0x07 ) | ( (data & 0x01) << 3);
               bank = data & 0x3F; 
            } else {
               c64_set_exrom_game(1, 1);
            }
         }
      }
   }  // end loop
}

//

void __time_critical_func(run_cart_super_games)(void) {

   volatile uint64_t control;
   volatile uint32_t addr;
   volatile uint8_t data;
   uint8_t bank = 0;
   bool disable = false;

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if (control & RW_MASK) {

         if( !(control & ROML_MASK) || !(control & ROMH_MASK) ) {

            DATA_OUT(crt.bank[bank].datal[(addr - crt.bank[bank].load_addrl)]);
            SET_DATA_MODE_OUT
            wait_high(ROML);
            wait_high(ROMH);
            SET_DATA_MODE_IN
         } 

      } else {
         
         SET_DATA_MODE_IN
         data = DATA_IN;
         if( !(control & IO2_MASK) && !disable) {
            bank = data & 0x03;
            if( (data & 0x04) ) {
               c64_set_exrom_game(1, 1);
            } else {
               c64_set_exrom_game(0, 0);
            }

            if(data & 0x08)
               disable = true;
         }
      }
   }  // end loop
}

//

__attribute__((optimize("O3"), hot))
void __time_critical_func(run_cart_easyflash)(void) {
   
   volatile uint64_t control;
   volatile uint32_t addr;
   volatile uint8_t data;
   uint8_t ram_buf[255];
   uint8_t bank = 0;
   uint8_t mode = 0;
   uint8_t ef_control[8][2] = {
      {1, 0},  // Ultimax
      {1, 1},  // none
      {0, 0},  // 16K
      {0, 1},  // 8K
      {1, 0},  // Ultimax
      {1, 1},  // none
      {0, 0},  // 16K
      {0, 1}   // 8K
   };

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      SET_DATA_MODE_IN
      if (control & RW_MASK) {

         if( !(control & ROML_MASK) ) {

            DATA_OUT(crt.bank[bank].datal[(addr - crt.bank[bank].load_addrl) & 0x3FFF]);
            SET_DATA_MODE_OUT
            wait_high(ROML);

         } else if( !(control & ROMH_MASK) ) {

            DATA_OUT(crt.bank[bank].datah[(addr - crt.bank[bank].load_addrh) & 0x3FFF]);
            SET_DATA_MODE_OUT
            wait_high(ROMH);

         } else if ( !(control & IO2_MASK) ) {
            
            DATA_OUT(ram_buf[addr & 0xFF]);
            SET_DATA_MODE_OUT
            wait_high(IO2);
         }

      } else {
         
         data = DATA_IN;
         if( !(control & IO1_MASK) && (addr == 0xDE00) ) {
            bank = data & 0x3f;
         } else if( !(control & IO1_MASK) && (addr == 0xDE02) ) {
            mode = (((data >> 5) & 0x04) | (data & 0x02) | (((data >> 2) & 0x01) & ~data)) & 0x07;
            c64_set_exrom_game(ef_control[mode][0], ef_control[mode][1]);
         } else if( !(control & IO2_MASK) ) {
            ram_buf[addr & 0xFF] = data;
         }
      }
   }  // end loop
}

//

