#ifndef CRT_H_
#define CRT_H_

#include <cstdint>
#include "ff.h"

#define CRT_SIGNATURE      "C64 CARTRIDGE   "
#define CHIP_SIGNATURE     "CHIP"

typedef enum {
   FILE_OK = 0,
   FILE_ERR_NOT_VALID = 1,
   FILE_ERR_FORMAT = 2,
   FILE_ERR_COUNT
} CRTFileError;

extern const char* CRTFileErrorStrings[FILE_ERR_COUNT];

typedef enum {
   BANK_TYPE_ROM = 0,
   BANK_TYPE_RAM,
   BANK_TYPE_FLASH,
   BANK_TYPE_COUNT
} BankType;

extern const char* BankTypeStrings[BANK_TYPE_COUNT];

typedef struct {

   uint32_t offset;
   uint16_t length;
   BankType type;
   uint8_t number;
   uint16_t load_addr;
   uint16_t size;
   uint8_t *data;

} CRTBank;

typedef struct {

   char filename[128];
   FIL fil;
   uint8_t type;
   bool exrom;
   bool game;
   char name[32];

   CRTBank bank[128];
   uint8_t nbanks;
   uint32_t size;

} CRTHandler;

CRTFileError crt_file_open(CRTHandler *crt, const char *filename);
CRTFileError crt_file_close(CRTHandler *crt);
CRTFileError crt_file_parse(CRTHandler *crt);
void crt_to_bin(CRTHandler crt, uint8_t *data);

#endif
