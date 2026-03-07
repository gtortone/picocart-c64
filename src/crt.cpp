
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>

#include "crt.h"

bool debug = false;

const char* CRTFileErrorStrings[FILE_ERR_COUNT] = {
   "OK",
   "open error",
   "format not valid"
};

const char* BankTypeStrings[BANK_TYPE_COUNT] = {
   "RAM",
   "ROM",
   "FLASH"
};

CRTFileError crt_file_open(CRTHandler *crt, const char *filename) {

   FRESULT fr = f_open(&crt->fil, filename, FA_READ);

   if(fr != FR_OK)
      return FILE_ERR_NOT_VALID;

   UINT br;
   DWORD file_size = f_size(&crt->fil);

   fr = f_read(&crt->fil, crt->rawdata, file_size, &br);

   if(fr != FR_OK)
      return FILE_ERR_NOT_VALID;      

   printf("crt_file_open: read ok\n");

   return crt_build_banks(crt);
}

CRTFileError crt_file_close(CRTHandler *crt) {

   f_close(&crt->fil);

   return FILE_OK;
}

void crt_clear(CRTHandler *crt) {
   for(int i=0; i<128; i++) {
      crt->bank[i].init = false;
      crt->bank[i].offset = 0;
      crt->bank[i].length = 0;
      crt->bank[i].type = (BankType) 0;
      crt->bank[i].number = 0;
      crt->bank[i].load_addrl = crt->bank[i].load_addrh = 0;
      crt->bank[i].datal = crt->bank[i].datah = NULL;
   }
   //memset(crt->rawdata, 0, sizeof(crt->rawdata));
   crt->nbanks = 0;
   crt->size = 0;
}

CRTFileError crt_build_banks(CRTHandler *crt) {

   uint8_t buf[64];
   UINT br;
   uint8_t *bp = crt->rawdata;
   int pos = 0;

   // clear
   crt->nbanks = 0;
   crt->size = 0;

   // header

   if(strncmp((char *)bp, CRT_SIGNATURE, strlen(CRT_SIGNATURE)))
      return FILE_ERR_FORMAT;

   pos += strlen(CRT_SIGNATURE);
   bp += strlen(CRT_SIGNATURE);

   uint32_t header_length = ((uint32_t)bp[0] << 24) |
    ((uint32_t)bp[1] << 16) |
    ((uint32_t)bp[2] << 8)  |
    ((uint32_t)bp[3]);

   pos += 4;
   bp += 4;

   pos += 2;
   bp += 2;

   crt->type = (bp[0] << 8) | (bp[1]);

   pos += 2;
   bp += 2;

   crt->exrom = bool(bp[0]);
   pos++;
   bp++;

   crt->game = bool(bp[0]);
   pos++;
   bp++;

   // fix Ocean bad EXROM/GAME
   if(crt->type == 5)
      crt->exrom = crt->game = 0;

   pos += 6;
   bp += 6;

   strncpy(crt->name, (char *)bp, 32);

   pos += 32;
   bp += 32;
   
   // chip packets

   uint8_t i = 0;
   bool merge_banks = false;
   uint32_t offset;
   uint16_t length;
   BankType type;
   uint16_t load_addr;
   uint16_t size;

   while(true) {

      offset = pos;

      if(strncmp((char *)bp, CHIP_SIGNATURE, strlen(CHIP_SIGNATURE)))
         break;

      pos += strlen(CHIP_SIGNATURE);
      bp += strlen(CHIP_SIGNATURE);

      length = (bp[0] << 24) | (bp[1] << 16) | (bp[2] << 8) | (bp[3]);

      pos += 4;
      bp += 4;

      type = (BankType) ((bp[0] << 8) | bp[1]);

      pos += 2;
      bp += 2;

      uint8_t bnum = ((bp[0] << 8) | bp[1]);

      pos += 2;
      bp += 2;

      load_addr = ((bp[0] << 8) | bp[1]);

      pos += 2;
      bp += 2;

      size = ((bp[0] << 8) | bp[1]);

      pos += 2;
      bp += 2;

      if(crt->bank[bnum].init == true) {
         if(load_addr > 0x8000) {
            crt->bank[bnum].load_addrh = load_addr;
            crt->bank[bnum].datah = bp;
         } else {
            crt->bank[bnum].load_addrl = load_addr;
            crt->bank[bnum].datal = bp;
         }
         crt->bank[bnum].size += size;
      } else {
         crt->bank[bnum].init = true;
         crt->bank[bnum].offset = offset;
         crt->bank[bnum].length = length;
         crt->bank[bnum].type = type;
         crt->bank[bnum].number = bnum;
         crt->bank[bnum].size = size;
         if(load_addr > 0x8000) {
            crt->bank[bnum].load_addrh = load_addr;
            crt->bank[bnum].datah = bp;
         } else {
            crt->bank[bnum].load_addrl = load_addr;
            crt->bank[bnum].datal = bp;
         }
         crt->nbanks++;
      }

      pos += size;
      bp += size;
      
      crt->size += size;
   }

   return FILE_OK;
}

void crt_print(CRTHandler *crt) {

   printf("name: %s\n", crt->name);
   printf("hardware type: %d\n", crt->type);
   printf("exrom: %d, game: %d\n", crt->exrom, crt->game);

   for(int i=0; i<MAX_BANKS_NUM; i++) {

      if(crt->bank[i].init) {
         printf("bank: %d\n", crt->bank[i].number);
         printf("\toffset: 0x%X\n", crt->bank[i].offset);
         printf("\tlength: 0x%X\n", crt->bank[i].length);
         printf("\ttype: %s\n", BankTypeStrings[crt->bank[i].type]);
         printf("\tload address L: 0x%X\n", crt->bank[i].load_addrl);
         printf("\tload address H: 0x%X\n", crt->bank[i].load_addrh);
         printf("\tsize: 0x%X\n", crt->bank[i].size);
      }
   }

   printf("Total banks: %d\n", crt->nbanks);
}
