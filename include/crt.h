#ifndef CRT_H_
#define CRT_H_

#include <cstdint>
#include "ff.h"

#include "board.h"

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

typedef enum {
   CRT_NORMAL_CARTRIDGE = 0x00,
   CRT_ACTION_REPLAY,
   CRT_KCS_POWER_CARTRIDGE,
   CRT_FINAL_CARTRIDGE_III,
   CRT_SIMONS_BASIC,
   CRT_OCEAN_TYPE_1,
   CRT_EXPERT_CARTRIDGE,
   CRT_FUN_PLAY_POWER_PLAY,
   CRT_SUPER_GAMES,
   CRT_ATOMIC_POWER,
   CRT_EPYX_FASTLOAD,
   CRT_WESTERMANN_LEARNING,
   CRT_REX_UTILITY,
   CRT_FINAL_CARTRIDGE_I,
   CRT_MAGIC_FORMEL,
   CRT_C64_GAME_SYSTEM_SYSTEM_3,
   CRT_WARP_SPEED,
   CRT_DINAMIC,
   CRT_ZAXXON_SUPER_ZAXXON,
   CRT_MAGIC_DESK_DOMARK_HES_AUSTRALIA,
   CRT_SUPER_SNAPSHOT_V5,
   CRT_COMAL_80,
   CRT_STRUCTURED_BASIC,
   CRT_ROSS,
   CRT_DELA_EP64,
   CRT_DELA_EP7X8,
   CRT_DELA_EP256,
   CRT_REX_EP256,
   CRT_MIKRO_ASSEMBLER,
   CRT_FINAL_CARTRIDGE_PLUS,
   CRT_ACTION_REPLAY_4,
   CRT_STARDOS,
   CRT_EASYFLASH,
   CRT_EASYFLASH_XBANK,
   CRT_CAPTURE,
   CRT_ACTION_REPLAY_3,
   CRT_RETRO_REPLAY,
   CRT_MMC64,
   CRT_MMC_REPLAY,
   CRT_IDE64,
   CRT_SUPER_SNAPSHOT_V4,
   CRT_IEEE_488,
   CRT_GAME_KILLER,
   CRT_PROPHET64,
   CRT_EXOS,
   CRT_FREEZE_FRAME,
   CRT_FREEZE_MACHINE,
   CRT_SNAPSHOT64,
   CRT_SUPER_EXPLODE_V5_0,
   CRT_MAGIC_VOICE,
   CRT_ACTION_REPLAY_2,
   CRT_MACH_5,
   CRT_DIASHOW_MAKER,
   CRT_PAGEFOX,
   CRT_KINGSOFT,
   CRT_SILVERROCK_128K_CARTRIDGE,
   CRT_FORMEL_64,
   CRT_RGCD,
   CRT_RR_NET_MK3,
   CRT_EASYCALC,
   CRT_GMOD2,
   CRT_MAX_BASIC,
   CRT_GMOD3,
   CRT_ZIPP_CODE_48,
   CRT_BLACKBOX_V8,
   CRT_BLACKBOX_V3,
   CRT_BLACKBOX_V4,
   CRT_REX_RAM_FLOPPY,
   CRT_BIS_PLUS,
   CRT_SD_BOX,
   CRT_MULTIMAX,
   CRT_BLACKBOX_V9,
   CRT_LT_KERNAL_HOST_ADAPTOR,
   CRT_RAMLINK,
   CRT_DREAN,
   CRT_IEEE_FLASH_64,
   CRT_TURTLE_GRAPHICS_II,
	CRT_FREEZE_FRAME_MK2,
   CRT_PARTNER_64,

} CRTType;

extern const char* BankTypeStrings[BANK_TYPE_COUNT];

typedef struct {

   bool init = false;
   uint32_t offset;
   uint16_t length;
   BankType type;
   uint8_t number;

   uint16_t load_addrh;
   uint16_t load_addrl;

   uint8_t *datah;
   uint8_t *datal;

   uint16_t size;

} CRTBank;

#define MAX_BANKS_NUM   128

typedef struct {

   char filename[128];
   FIL fil;
   uint8_t rawdata[ROM_SIZE];
   uint8_t type;
   bool exrom;
   bool game;
   char name[32];

   CRTBank bank[MAX_BANKS_NUM];
   uint8_t nbanks;
   uint32_t size;

} CRTHandler;

CRTFileError crt_file_open(CRTHandler *crt, const char *filename);
CRTFileError crt_file_close(CRTHandler *crt);
CRTFileError crt_build_banks(CRTHandler *crt);
void crt_print(CRTHandler *crt);
void crt_init(CRTHandler *crt);
void crt_clear_buffer(CRTHandler *crt);

#endif
