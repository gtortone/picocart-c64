#include <stdio.h>      // printf
#include <cstdlib>      // malloc
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/structs/sio.h"

#include "cartridge.h"
#include "board.h"
#include "c64_interface.h"
#include "crt.h"

extern CRTHandler crt;
extern uint8_t crt_buf[CRT_BUFFER_SIZE];

#define wait_until(t) { for(int i=0; i<t; i++) asm volatile("nop\n"); } // 4ns

#define CRT_BANK(bank)          (crt_buf + (uint32_t)(16 * 1024 * bank))

// Fast look-up of 16k ROM bank address   (320 kb)
uint8_t * const crt_banks[20] = {
   CRT_BANK(0),
   CRT_BANK(1),
   CRT_BANK(2),
   CRT_BANK(3),
   CRT_BANK(4),
   CRT_BANK(5),
   CRT_BANK(6),
   CRT_BANK(7),
   CRT_BANK(8),
   CRT_BANK(9),
   CRT_BANK(10),
   CRT_BANK(11),
   CRT_BANK(12),
   CRT_BANK(13),
   CRT_BANK(14),
   CRT_BANK(15),
   CRT_BANK(16),
   CRT_BANK(17),
   CRT_BANK(18),
   CRT_BANK(19),
};

#define CORE1_STACK_SIZE 4096
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
      printf("cart: 8K, 16K, Ultimax\n");
      multicore_launch_core1_with_stack(run_cart_normal, core1_stack, CORE1_STACK_SIZE);
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

void __time_critical_func(run_cart_normal)(void) {

   volatile uint32_t control;
   volatile uint32_t addr;

   uint32_t irqstatus = save_and_disable_interrupts();

   //wait_high(PHI2);

   SET_DATA_MODE_IN
   while(1) {

      GPIO_GET_LOW_32(control);
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if( !(control & ROML_MASK) || ( !(control & ROMH_MASK) ) ) {

         DATA_OUT(crt_buf[addr & 0x3FFF]);
         SET_DATA_MODE_OUT
         // 16 nop
         wait_until(16);
         //wait_high(ROML);
         //wait_high(ROMH);
         SET_DATA_MODE_IN
      }
   } // end loop
}

//

void __time_critical_func(run_cart_magic_desk)(void) {

   volatile uint32_t control;
   volatile uint32_t addr;
   volatile uint8_t data;
   uint8_t *crt_ptr = crt_banks[0];

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      GPIO_GET_LOW_32(control);
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if (control & RW_MASK) {
         if( !(control & ROML_MASK) ) {

            DATA_OUT(crt_ptr[addr & 0x1FFF]);
            SET_DATA_MODE_OUT
            wait_high(ROML);
            SET_DATA_MODE_IN
         }

      } else {
         
         SET_DATA_MODE_IN
         data = DATA_IN;
         if( !(control & IO1_MASK) && !(addr & 0xFF) ) {
            if ( !(data & 0x80)) {
               c64_set_exrom_game(0, 1);
               crt_ptr = crt_banks[(data >> 1) & 0x3f];
               if (data & 0x01) {
                  // Use ROMH location for odd banks
                  crt_ptr += 0x2000;
               }
            } else {
               c64_set_exrom_game(1, 1);
            }
         }
         // test PRG
         //if( !(control & IO2_MASK) && (addr == 0xDF1C) )
         //   printf("D: %d\n", data);
      }
   }  // end loop
}

//

void __time_critical_func(run_cart_ocean)(void) {

   volatile uint32_t control;
   volatile uint32_t addr;
   volatile uint8_t data;
   uint8_t *crt_ptr = crt_banks[0];

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      GPIO_GET_LOW_32(control);
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if (control & RW_MASK) {
         if( !(control & ROML_MASK) || !(control & ROMH_MASK) ) {

            DATA_OUT(crt_ptr[addr & 0x3FFF]);
            SET_DATA_MODE_OUT
            wait_high(ROML);
            wait_high(ROMH);
            SET_DATA_MODE_IN
         }

      } else {
         
         SET_DATA_MODE_IN
         data = DATA_IN;
         if( !(control & IO1_MASK) && (addr == 0xDE00) )
            crt_ptr = crt_banks[data & 0x3f];
      }
   }  // end loop
}

//

void __time_critical_func(run_cart_fun_play)(void) {

   volatile uint32_t control;
   volatile uint32_t addr;
   volatile uint8_t data;
   uint8_t bank = 0;

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      GPIO_GET_LOW_32(control);
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

   volatile uint32_t control;
   volatile uint32_t addr;
   volatile uint8_t data;
   uint8_t *crt_ptr = crt_banks[0];
   bool disable = false;

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      GPIO_GET_LOW_32(control);
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if (control & RW_MASK) {

         if( !(control & ROML_MASK) || !(control & ROMH_MASK) ) {

            DATA_OUT(crt_ptr[addr & 0x3FFF]);
            SET_DATA_MODE_OUT
            wait_high(ROML);
            wait_high(ROMH);
            SET_DATA_MODE_IN
         } 

      } else {
         
         SET_DATA_MODE_IN
         data = DATA_IN;
         if( !(control & IO2_MASK) && !disable) {
            crt_ptr = crt_banks[data & 0x03];
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
   
   volatile uint32_t control;
   volatile uint32_t addr;
   volatile uint8_t data;
   uint8_t *crt_ptr = crt_banks[0];
   uint8_t *ram_buf = (uint8_t *) malloc(255);
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

   while(1) {

      GPIO_GET_LOW_32(control);
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      SET_DATA_MODE_IN
      if (control & RW_MASK) {

         if ((control & (ROML_MASK|ROMH_MASK)) != (ROML_MASK|ROMH_MASK)) {

            SET_DATA_MODE_OUT
            DATA_OUT(crt_ptr[addr & 0x3FFF]);
            wait_high(ROML);
            wait_high(ROMH);

         } else if ( !(control & IO2_MASK) ) {
            
            SET_DATA_MODE_OUT
            DATA_OUT(ram_buf[addr & 0xFF]);
            wait_high(IO2);
         }

      } else {
         
         data = DATA_IN;
         if( !(control & IO1_MASK) ) {

            switch (addr & 0xff) {

               case 0x00:
                  crt_ptr = crt_banks[data & 0x3f];
                  break;

               case 0x02:
                  mode = (((data >> 5) & 0x04) | (data & 0x02) | (((data >> 2) & 0x01) & ~data)) & 0x07;
                  c64_set_exrom_game(ef_control[mode][0], ef_control[mode][1]);
            }
         }

         if (!(control & IO2_MASK)) {
            ram_buf[addr & 0xff] = data;
         }
      }
   }  // end loop
}

//

void __time_critical_func(run_cart_dinamic)(void) {

   volatile uint32_t control;
   volatile uint32_t addr;
   volatile uint8_t data;
   uint8_t *crt_ptr = crt_banks[0];

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      GPIO_GET_LOW_32(control);
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if( !(control & ROML_MASK) ) {

         DATA_OUT(crt_ptr[addr & 0x1fff]);
         SET_DATA_MODE_OUT
         wait_high(ROML);
         SET_DATA_MODE_IN
      } 

      if( !(control & IO1_MASK) )
         crt_ptr = crt_banks[addr & 0x3F];

   }  // end loop
}

//

void __time_critical_func(run_cart_zaxxon)(void) {

   volatile uint32_t control;
   volatile uint32_t addr;
   uint8_t *data;
   uint8_t *crt_ptr;
   uint8_t *crt_rom_ptr = crt_banks[0];

   uint32_t irqstatus = save_and_disable_interrupts();

   SET_DATA_MODE_IN
   while(1) {

      GPIO_GET_LOW_32(control);
      addr = (control & ADDR_GPIO_MASK);
      COMPILER_BARRIER();

      if( !(control & ROML_MASK) ) {

         DATA_OUT(crt_rom_ptr[addr & 0x0fff]);
         SET_DATA_MODE_OUT
         wait_high(ROML);
         SET_DATA_MODE_IN

         crt_ptr = crt_banks[addr & 0x1000 ? 1 : 0];

      } else if( !(control & ROMH_MASK) ) {

         DATA_OUT(crt_ptr[addr & 0x3fff]);
         SET_DATA_MODE_OUT
         wait_high(ROMH);
         SET_DATA_MODE_IN
      }

   }  // end loop
}

//
