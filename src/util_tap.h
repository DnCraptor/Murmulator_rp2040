#pragma once
#include "inttypes.h"
#include "stdbool.h"
#include <pico/stdlib.h>
#include "util_sd.h"

#define TAPE_OFF			(0)
#define TAPE_INTERNAL_MANU	(1<<1)
#define TAPE_INTERNAL_AUTO	(1<<2)
#define TAPE_INTERNAL_ROM	(1<<3)
#define TAPE_ROM_READY		(1<<4)
#define TAPE_EXTERNAL		(1<<5)


#define TAPE_FILE_FREE 0xFF

// Tape status definitions
#define TAPE_STOPPED	(0)
#define TAPE_LOADING	(1)
#define TAPE_PAUSED		(2)
#define TAPE_LOADED		(3)

// Saving status
#define SAVE_STOPPED 0
#define TAPE_SAVING 1

// Tape phases
#define TAPE_PHASE_SYNC 1
#define TAPE_PHASE_SYNC1 2
#define TAPE_PHASE_SYNC2 3
#define TAPE_PHASE_DATA 4
#define TAPE_PHASE_PAUSE 5
#define TAPE_PHASE_NEED_STOP 6

// Tape sync phases lenght in microseconds
#define TAPE_SYNC_LEN 2168 // 620 microseconds for 2168 tStates (48K)
#define TAPE_SYNC1_LEN 667 // 190 microseconds for 667 tStates (48K)
#define TAPE_SYNC2_LEN 735 // 210 microseconds for 735 tStates (48K)

#define TAPE_HDR_LONG 8063   // Header sync lenght in pulses
#define TAPE_HDR_SHORT 3223  // Data sync lenght in pulses

#define TAPE_BIT0_PULSELEN 855 // tstates = 244 ms, lenght of pulse for bit 0
#define TAPE_BIT1_PULSELEN 1710 // tstates = 488 ms, lenght of pulse for bit 1

//#define TAPE_BLK_PAUSELEN 15000000UL // 1 second of pause between blocks
//#define TAPE_BLK_PAUSELEN  3500000UL // 1 second of pause between blocks
#define TAPE_BLK_PAUSELEN  1750000UL // 1/2 second of pause between blocks
//#define TAPE_BLK_PAUSELEN    87500UL // 1/4 second of pause between blo


#define TAPE_BLK_SIZE (TEMP_BUFF_SIZE_Y/20)-2
#define TAPE_MAX_NAMES 78

typedef struct TapeBlock{
	uint16_t Size;
	uint8_t Flag;
	uint8_t DataType;
	char NAME[11];
	uint32_t FPos;
} __attribute__((packed)) TapeBlock;


extern uint8_t  tap_loader_active;
extern uint16_t tap_block_position;

//char tapeFileName[160];
extern uint8_t TapeStatus;
extern uint8_t SaveStatus;


//size_t file_pos;
//TapeBlock tap_blocks[TAPE_BLK_SIZE];

extern int tapeCurrentBlock;
extern uint16_t tapBlocksCount;
extern uint32_t tapeFileSize;
extern uint32_t tapeTotByteCount;

uint8_t __not_in_flash_func(TAP_Read)();
void __not_in_flash_func(TAP_Play)();
void Init();
bool TAP_Load(char *file_name);
void TAP_Rewind();
bool TAP_NextBlock();
bool TAP_PrevBlock();
void TAP_Eject();
bool LoadScreenFromTap(char *file_name,bool find_screen);

bool FastOpenTAP(char *file_name);
bool FastLoadTAP(uint16_t blockType,uint16_t ramAddress,uint16_t ramLen);
void FastCloseTAP();
