
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
   memset(crt->rawdata, 0, sizeof(crt->rawdata));
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

   debug && printf("header length: %d\n", header_length);

   pos += 4;
   bp += 4;

   debug && printf("cartridge version: %d.%d\n", bp[0], bp[1]);

   pos += 2;
   bp += 2;

   crt->type = (bp[0] << 8) | (bp[1]);
   debug && printf("hardware type: %d\n", crt->type);

   pos += 2;
   bp += 2;

   crt->exrom = bool(bp[0]);
   pos++;
   bp++;

   crt->game = bool(bp[0]);
   pos++;
   bp++;

   debug && printf("exrom: %d, game: %d\n", crt->exrom, crt->game);
   
   // fix Ocean bad EXROM/GAME
   if(crt->type == 5)
      crt->exrom = crt->game = 0;

   pos += 6;
   bp += 6;

   strncpy(crt->name, (char *)bp, 32);
   debug && printf("name: %s\n", crt->name);

   pos += 32;
   bp += 32;
   
   // chip packets

   uint8_t i = 0;

   while(true) {

      uint32_t offset = pos;

      if(strncmp((char *)bp, CHIP_SIGNATURE, strlen(CHIP_SIGNATURE)))
         break;

      debug && printf("bank: %d\n", i);

      crt->bank[i].offset = offset;
      debug && printf("\toffset: 0x%X\n", crt->bank[i].offset);

      pos += strlen(CHIP_SIGNATURE);
      bp += strlen(CHIP_SIGNATURE);

      crt->bank[i].length = (bp[0] << 24) | (bp[1] << 16) | (bp[2] << 8) | (bp[3]);

      pos += 4;
      bp += 4;

      debug && printf("\tlength: 0x%X\n", crt->bank[i].length);

      crt->bank[i].type = (BankType) ((bp[0] << 8) | bp[1]);
      debug && printf("\ttype: %s\n", BankTypeStrings[crt->bank[i].type]);

      pos += 2;
      bp += 2;

      crt->bank[i].number = ((bp[0] << 8) | bp[1]); 
      debug && printf("\tnumber: %d\n", crt->bank[i].number);

      pos += 2;
      bp += 2;

      crt->bank[i].load_addr = ((bp[0] << 8) | bp[1]);
      debug && printf("\tload address: 0x%X\n", crt->bank[i].load_addr);

      pos += 2;
      bp += 2;

      crt->bank[i].size = ((bp[0] << 8) | bp[1]);
      debug && printf("\tsize: 0x%X\n", crt->bank[i].size);

      pos += 2;
      bp += 2;

      // keep track bank data
      crt->bank[i].data = bp;

      pos += crt->bank[i].size;
      bp += crt->bank[i].size;
      
      crt->size += crt->bank[i].size;

      i++;
   }

   crt->nbanks = i;
   debug && printf("Total banks: %d\n", crt->nbanks);

   return FILE_OK;
}

CRTFileError crt_file_parse(CRTHandler *crt) {

   uint8_t buf[64];
   UINT br;

   // header

   f_read(&crt->fil, buf, strlen(CRT_SIGNATURE), &br);
   buf[strlen(CRT_SIGNATURE)] = '\0';

   if(strcmp((char *)buf, CRT_SIGNATURE))
      return FILE_ERR_FORMAT;

   f_read(&crt->fil, buf, 4, &br);
   uint32_t header_length = ((uint32_t)buf[0] << 24) |
    ((uint32_t)buf[1] << 16) |
    ((uint32_t)buf[2] << 8)  |
    ((uint32_t)buf[3]);

   debug && printf("header length: %d\n", header_length);

   f_read(&crt->fil, buf, 2, &br);
   debug && printf("cartridge version: %d.%d\n", buf[0], buf[1]);

   f_read(&crt->fil, buf, 2, &br);
   crt->type = (buf[0] << 8) | (buf[1]);
   debug && printf("hardware type: %d\n", crt->type);

   f_read(&crt->fil, buf, 1, &br);
   crt->exrom = bool(buf[0]);
   f_read(&crt->fil, buf, 1, &br);
   crt->game = bool(buf[0]);
   debug && printf("exrom: %d, game: %d\n", crt->exrom, crt->game);
   
   // fix Ocean bad EXROM/GAME
   if(crt->type == 5)
      crt->exrom = crt->game = 0;

   f_read(&crt->fil, buf, 6, &br);

   f_read(&crt->fil, buf, 32, &br);
   strcpy(crt->name, (char *)buf);
   debug && printf("name: %s\n", crt->name);

   // chip packets

   uint8_t i = 0;

   while(!f_eof(&crt->fil)) {

      uint32_t offset = f_tell(&crt->fil);

      f_read(&crt->fil, buf, strlen(CHIP_SIGNATURE), &br);
      buf[strlen(CHIP_SIGNATURE)] = '\0';

      if(strcmp((char *)buf, CHIP_SIGNATURE))
         return FILE_ERR_FORMAT;

      debug && printf("bank: %d\n", i);

      crt->bank[i].offset = offset;
      debug && printf("\toffset: 0x%X\n", crt->bank[i].offset);

      f_read(&crt->fil, buf, 4, &br);

      crt->bank[i].length = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3]);

      debug && printf("\tlength: 0x%X\n", crt->bank[i].length);

      f_read(&crt->fil, buf, 2, &br);
      crt->bank[i].type = (BankType) ((buf[0] << 8) | buf[1]);
      debug && printf("\ttype: %s\n", BankTypeStrings[crt->bank[i].type]);

      f_read(&crt->fil, buf, 2, &br);
      crt->bank[i].number = ((buf[0] << 8) | buf[1]); 
      debug && printf("\tnumber: %d\n", crt->bank[i].number);

      f_read(&crt->fil, buf, 2, &br);
      crt->bank[i].load_addr = ((buf[0] << 8) | buf[1]);
      debug && printf("\tload address: 0x%X\n", crt->bank[i].load_addr);

      f_read(&crt->fil, buf, 2, &br);
      crt->bank[i].size = ((buf[0] << 8) | buf[1]);
      debug && printf("\tsize: 0x%X\n", crt->bank[i].size);
      
      // skip data
      f_lseek(&crt->fil, f_tell(&crt->fil) + crt->bank[i].size);

      crt->size += crt->bank[i].size;

      i++;
   }

   crt->nbanks = i;
   debug && printf("Total banks: %d\n", crt->nbanks);

   return FILE_OK;
}

CRTFileError crt_buffer_parse(CRTHandler *crt, uint8_t *buffer) {

   uint8_t buf[64];
   UINT br;
   uint8_t *bp = buffer;
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

   debug && printf("header length: %d\n", header_length);

   pos += 4;
   bp += 4;

   debug && printf("cartridge version: %d.%d\n", bp[0], bp[1]);

   pos += 2;
   bp += 2;

   crt->type = (bp[0] << 8) | (bp[1]);
   debug && printf("hardware type: %d\n", crt->type);

   pos += 2;
   bp += 2;

   crt->exrom = bool(bp[0]);
   pos++;
   bp++;

   crt->game = bool(bp[0]);
   pos++;
   bp++;

   debug && printf("exrom: %d, game: %d\n", crt->exrom, crt->game);
   
   // fix Ocean bad EXROM/GAME
   if(crt->type == 5)
      crt->exrom = crt->game = 0;

   pos += 6;
   bp += 6;

   strncpy(crt->name, (char *)bp, 32);
   debug && printf("name: %s\n", crt->name);

   pos += 32;
   bp += 32;
   
   // chip packets

   uint8_t i = 0;

   while(true) {

      uint32_t offset = pos;

      if(strncmp((char *)bp, CHIP_SIGNATURE, strlen(CHIP_SIGNATURE)))
         break;

      debug && printf("bank: %d\n", i);

      crt->bank[i].offset = offset;
      debug && printf("\toffset: 0x%X\n", crt->bank[i].offset);

      pos += strlen(CHIP_SIGNATURE);
      bp += strlen(CHIP_SIGNATURE);

      crt->bank[i].length = (bp[0] << 24) | (bp[1] << 16) | (bp[2] << 8) | (bp[3]);

      pos += 4;
      bp += 4;

      debug && printf("\tlength: 0x%X\n", crt->bank[i].length);

      crt->bank[i].type = (BankType) ((bp[0] << 8) | bp[1]);
      debug && printf("\ttype: %s\n", BankTypeStrings[crt->bank[i].type]);

      pos += 2;
      bp += 2;

      crt->bank[i].number = ((bp[0] << 8) | bp[1]); 
      debug && printf("\tnumber: %d\n", crt->bank[i].number);

      pos += 2;
      bp += 2;

      crt->bank[i].load_addr = ((bp[0] << 8) | bp[1]);
      debug && printf("\tload address: 0x%X\n", crt->bank[i].load_addr);

      pos += 2;
      bp += 2;

      crt->bank[i].size = ((bp[0] << 8) | bp[1]);
      debug && printf("\tsize: 0x%X\n", crt->bank[i].size);

      pos += 2;
      bp += 2;

      // skip data
      pos += crt->bank[i].size;
      bp += crt->bank[i].size;
      
      crt->size += crt->bank[i].size;

      i++;
   }

   crt->nbanks = i;
   debug && printf("Total banks: %d\n", crt->nbanks);

   return FILE_OK;

}

void crt_file_to_bin(CRTHandler crt, uint8_t *data) {

   int i = 0;
   UINT br;
   uint32_t ofs;

   f_rewind(&crt.fil);

   for(int b=0; b<crt.nbanks; b++) {
      //printf("%d) offset: 0x%X, length: 0x%X, size: 0x%X\n", 
      //      b, crt.bank[b].offset, crt.bank[b].length, crt.bank[b].size);
      ofs = crt.bank[b].offset + (crt.bank[b].length - crt.bank[b].size);

      f_lseek(&crt.fil, ofs);
      f_read(&crt.fil, data+i, crt.bank[b].size, &br);
      //printf("ofs: 0x%X, size: %ld, br: %d\n", ofs, crt.bank[b].size, br);

      i += crt.bank[b].size;
   }
}

void crt_buffer_to_bin(CRTHandler crt, uint8_t *buffer, uint8_t *data) {

   int i = 0;
   UINT br;
   uint32_t ofs;

   for(int b=0; b<crt.nbanks; b++) {
      //printf("%d) offset: 0x%X, length: 0x%X, size: 0x%X\n", 
      //      b, crt.bank[b].offset, crt.bank[b].length, crt.bank[b].size);
      ofs = crt.bank[b].offset + (crt.bank[b].length - crt.bank[b].size);

      memcpy((uint8_t *) data+i, (uint8_t *) buffer+ofs, crt.bank[b].size);
      //printf("ofs: 0x%X, size: %ld, br: %d\n", ofs, crt.bank[b].size, br);

      i += crt.bank[b].size;
   }

}
