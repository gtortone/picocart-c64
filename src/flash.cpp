

#include "hardware/flash.h"
#include "pico/multicore.h"

#include "board.h"

void __not_in_flash_func(flash_buffer_at)(uint8_t* buffer, uint32_t buffer_size, uint32_t flash_address) {

   static uint16_t parts;
   static uint32_t irqstatus;

   parts = buffer_size / FLASH_SECTOR_SIZE;

   if(parts == 0)
      parts = 1;     // erase/program at least one sector
   
   int i = 0;
   while(i < parts) {
      irqstatus = save_and_disable_interrupts();
         flash_range_erase(flash_address, FLASH_SECTOR_SIZE);
         flash_range_program(flash_address, buffer + (i * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
      restore_interrupts(irqstatus);
      flash_address += FLASH_SECTOR_SIZE;
      i++;
   }
}
