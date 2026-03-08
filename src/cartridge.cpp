#include <stdio.h>      // printf
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "cartridge.h"
#include "board.h"
#include "c64_interface.h"
#include "crt.h"

extern CRTHandler crt;

#define CORE1_STACK_SIZE 2048
static uint32_t core1_stack[CORE1_STACK_SIZE];

uint8_t run_cart(char *filename, bool clear_buffer) {

   uint8_t rc;

   multicore_reset_core1();

   crt_init(&crt);
   if(clear_buffer)
      crt_clear_buffer(&crt);

   if(filename != NULL) {
      rc = crt_file_open(&crt, filename);
      if(rc != FILE_OK)
         return rc;
      crt_file_close(&crt);
   } else {
      crt_build_banks(&crt);
   }

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
   } else if(crt.type == 17) {
      // Dinamic (17)
      printf("cart: Dinamic\n");
      multicore_launch_core1_with_stack(run_cart_dinamic, core1_stack, CORE1_STACK_SIZE);
   } else if(crt.type == 18) {
      // Zaxxon (18)
      printf("cart: Zaxxon\n");
      multicore_launch_core1_with_stack(run_cart_zaxxon, core1_stack, CORE1_STACK_SIZE);
   }

   c64_reset();
   printf("done\n");

   return(FILE_OK);
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
   int mask = (crt.size - 1);

   crt_print(&crt);

   datal = crt.bank[0].datal;
   load_addrl = crt.bank[0].load_addrl;

   datah = crt.bank[0].datah;
   load_addrh = crt.bank[0].load_addrh;

   if(crt.bank[0].datal == NULL) {
      datal = crt.bank[0].datah;
      load_addrl = crt.bank[0].load_addrh;
   } else if(crt.bank[0].datah == NULL) {
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

         DATA_OUT(datal[(addr - load_addrl) & 0x3FFF]);
         SET_DATA_MODE_OUT
         wait_high(ROML);
         SET_DATA_MODE_IN

      } else if( !(control & ROMH_MASK) ) {

         DATA_OUT(datah[(addr - load_addrh) & 0x3FFF]);
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

void __time_critical_func(run_cart_dinamic)(void) {

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

      if( !(control & ROML_MASK) ) {

         DATA_OUT(crt.bank[bank].datal[(addr - crt.bank[bank].load_addrl) & 0x1FFF]);
         SET_DATA_MODE_OUT
         wait_high(ROML);
         SET_DATA_MODE_IN
      } 

      if( !(control & IO1_MASK) && (addr & 0xDE00) )
         bank = addr - 0xDE00;

   }  // end loop
}

//

void __time_critical_func(run_cart_zaxxon)(void) {

   volatile uint64_t control;
   volatile uint32_t addr;
   uint8_t *data;
   uint16_t load_addr;

   uint32_t irqstatus = save_and_disable_interrupts();

   //data = crt.bank[0].datal;
   //load_addr = crt.bank[0].load_addrl;

   SET_DATA_MODE_IN
   while(1) {

      control = gpio_get_all64();
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if( !(control & ROML_MASK) ) {

         DATA_OUT(crt.bank[0].datal[(addr - crt.bank[0].load_addrl) & 0x0FFF]);
         SET_DATA_MODE_OUT
         wait_high(ROML);
         SET_DATA_MODE_IN

         if( addr < 0x9000 ) {
            // 0x8000 - 0x8FFF   bank 0H
            data = crt.bank[0].datah;
            load_addr = crt.bank[0].load_addrh;    
         } else {
            // 0x9000 - 0x9FFF   bank 1H
            data = crt.bank[1].datah;
            load_addr = crt.bank[1].load_addrh;
         }

      } else if( !(control & ROMH_MASK) ) {

         DATA_OUT(data[(addr - load_addr) & 0x3FFF]);
         SET_DATA_MODE_OUT
         wait_high(ROMH);
         SET_DATA_MODE_IN
      }

   }  // end loop
}

//
