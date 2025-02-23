#include "util_tap.h"
#include "util_sd.h"
#include <stdint.h>
#include <string.h>
#include <zx_emu/z80.h>
#include "pico/stdlib.h"
#include <ps2.h>
#include "zx_emu/zx_machine.h"
#include "screen_util.h"
#include <math.h>
#include "stdbool.h"

//#define ZX_RAM_PAGE_SIZE 0x4000
#define BUFF_PAGE_SIZE 0x1000

extern volatile z80 cpu;
extern bool im_z80_stop;
extern bool im_ready_loading;
extern uint8_t RAM[ZX_RAM_PAGE_SIZE*8]; //Реальная память куском 128Кб
extern uint8_t sd_buffer[SD_BUFFER_SIZE];
extern char temp_msg[60];
extern uint8_t* zx_cpu_ram[4];
#ifndef DEBUG_DISABLE_LOADERS
extern uint8_t temp_buffer_y[TEMP_BUFF_SIZE];
#endif

#include "zx_emu/zx_machine.h"
#include "screen_util.h"



/*
typedef struct TapeBlock{
	uint16_t Size;
	uint8_t Flag;
	uint8_t DataType;
	char NAME[11];
	uint32_t FPos;
} __attribute__((packed)) TapeBlock;
*/

uint16_t tapBlocksCount=0;

uint8_t TapeStatus;
uint8_t SaveStatus;
uint8_t RomLoading;

int tape_file_status =-1; //tape file descriptor
uint8_t blockChecksum=0;
size_t bytesRead;
size_t bytesToRead;

static uint8_t tapePhase;
static uint64_t tapeStart;
static uint32_t tapePulseCount;
static uint16_t tapeBitPulseLen;   
static uint8_t tapeBitPulseCount;	 
static uint32_t tapebufByteCount;
static uint32_t tapeBlockByteCount;
static uint16_t tapeHdrPulses;
static uint32_t tapeBlockLen;
static uint8_t* tape;
static uint8_t tapeEarBit;
static uint8_t tapeBitMask; 

int tapeCurrentBlock;
uint32_t tapeFileSize;
uint32_t tapeTotByteCount;

char tapeBHbuffer[20]; //tape block header buffer


uint8_t __not_in_flash_func(TAP_Read)(){
#ifndef DEBUG_DISABLE_LOADERS
//uint8_t TAP_Read(){
	if(TapeStatus!=TAPE_LOADING) return false;
	uint64_t tapeCurrent = cpu.cyc - tapeStart;
	////printf("Tape PHASE:%X\n",tapePhase);
	switch (tapePhase) {
	case TAPE_PHASE_SYNC:
		if (tapeCurrent > TAPE_SYNC_LEN) {
			tapeStart=cpu.cyc;
			tapeEarBit ^= 1;
			tapePulseCount++;
			if (tapePulseCount>tapeHdrPulses) {
				tapePulseCount=0;
				tapePhase=TAPE_PHASE_SYNC1;
			}
		}
		break;
	case TAPE_PHASE_SYNC1:
		if (tapeCurrent > TAPE_SYNC1_LEN) {
			tapeStart=cpu.cyc;
			tapeEarBit ^= 1;
			tapePhase=TAPE_PHASE_SYNC2;
		}
		break;
	case TAPE_PHASE_SYNC2:
		if (tapeCurrent > TAPE_SYNC2_LEN) {
			tapeStart=cpu.cyc;
			tapeEarBit ^= 1;
			if (tape[tapebufByteCount] & tapeBitMask) tapeBitPulseLen=TAPE_BIT1_PULSELEN; else tapeBitPulseLen=TAPE_BIT0_PULSELEN;			
			tapePhase=TAPE_PHASE_DATA;
		}
		break;
	case TAPE_PHASE_DATA:
		if (tapeCurrent >= tapeBitPulseLen) {
			tapeStart=cpu.cyc;
			tapeEarBit ^= 1;
			tapeBitPulseCount++;
			if (tapeBitPulseCount==2) {
				tapeBitPulseCount=0;
				tapeBitMask = (tapeBitMask >>1 | tapeBitMask <<7);
				if (tapeBitMask==0x80) {
					tapebufByteCount++;
					tapeBlockByteCount++;
					tapeTotByteCount++;
					////printf("BUF:%d BLOCK:%d TOTAL:%d\n",tapebufByteCount,tapeBlockByteCount,tapeTotByteCount);
					if(tapebufByteCount>=BUFF_PAGE_SIZE){
						////printf("Read next buffer\n");
						im_z80_stop = true;
						tape_file_status = sd_read_file(&sd_file,sd_buffer,BUFF_PAGE_SIZE,&bytesRead);
						im_z80_stop = false;
						////printf("bytesRead=%d\n",bytesRead);
						if (tape_file_status!=FR_OK){
							////printf("Error read SD\n");
							sd_close_file(&sd_file);
							tap_loader_active = false;
							tapebufByteCount=0;
							//im_z80_stop = false;	   
							TapeStatus=TAPE_STOPPED;
							return false;
						}
						//im_z80_stop = false;
						tapebufByteCount=0;
					}
					if(tapeBlockByteCount==(tapeBlockLen-2)){//(tapeBlockLen-2)
						//tapeTotByteCount+=sizeof(tap_blocks[tapeCurrentBlock]);
						tapeTotByteCount+=2;
						//printf("Wait next block: %d\n",tapeTotByteCount);
						tapebufByteCount=0;
						tapePhase=TAPE_PHASE_PAUSE;
						tapeEarBit=false;
						break;
					}
					if (tapeTotByteCount >= tapeFileSize){
						////printf("tapeTotByteCount:%d  tapeFileSize:%d \n",tapeTotByteCount,tapeFileSize);
						////printf("Full Read TAPE_STOPPED 2\n");
						//TapeStatus=TAPE_STOPPED;
						tapePhase=TAPE_PHASE_PAUSE;
						TapeStatus=TAPE_LOADED;
						//TAP_Rewind();
						return false;
					}
				}
				if (tape[tapebufByteCount] & tapeBitMask) tapeBitPulseLen=TAPE_BIT1_PULSELEN; else tapeBitPulseLen=TAPE_BIT0_PULSELEN;
			}
		}
		break;
	case TAPE_PHASE_PAUSE:
		////printf("Tape PHASE:%X\n",tapePhase);
		if (tapeTotByteCount <= tapeFileSize) {
			if (tapeCurrent > TAPE_BLK_PAUSELEN) {
				tapeCurrentBlock++;
				TapeBlock* tblock = (TapeBlock*)&temp_buffer_y[sizeof(TapeBlock)*tapeCurrentBlock];
				sd_seek_file(&sd_file,tblock->FPos);
				tapeBlockLen=tblock->Size + 2;
				bytesToRead = tapeBlockLen<BUFF_PAGE_SIZE ? tapeBlockLen : BUFF_PAGE_SIZE;
				tape_file_status = sd_read_file(&sd_file,sd_buffer,bytesToRead,&bytesRead);
				if (tape_file_status != FR_OK){
					////printf("Error read SD\n");
					sd_close_file(&sd_file);
					tap_loader_active = false;
					tapebufByteCount=0;
					TapeStatus=TAPE_STOPPED;
					return false;
				}
				////printf("Block:%d Seek:%d Length:%d Read:%d\n",tapeCurrentBlock,tblock->FPos,tapeBlockLen,bytesRead);
				tapeStart=cpu.cyc;
				tapePulseCount=0;
				tapePhase=TAPE_PHASE_SYNC;
				tapebufByteCount=2;
				tapeBlockByteCount=0;
				////printf("Flag:%X, DType:%X \n",tblock->Flag,tblock->DataType);
				if (tblock->Flag) tapeHdrPulses=TAPE_HDR_SHORT; else tapeHdrPulses=TAPE_HDR_LONG;
				//printf("tapeTotByteCount:%d  tapeFileSize:%d \n",tapeTotByteCount,tapeFileSize);
			}
		}
		if (tapeTotByteCount >= tapeFileSize){
			printf("Full Read TAPE_STOPPED\n");
			TapeStatus=TAPE_LOADED;
			tap_loader_active=TAPE_OFF;
			break;
		}
		return false;
	} 
  
	return tapeEarBit;
#endif
}


void TAP_Play(){
#ifndef DEBUG_DISABLE_LOADERS
	//printf("1>Tape STATUS:%X\n",TapeStatus);
	switch (TapeStatus) {
	case TAPE_STOPPED:
		//TAP_Load(activefilename);
	   	tapePhase=TAPE_PHASE_SYNC;
	   	tapePulseCount=0;
	   	tapeEarBit=false;
	   	tapeBitMask=0x80;
	   	tapeBitPulseCount=0;
	   	tapeBitPulseLen=TAPE_BIT0_PULSELEN;
	   	tapeHdrPulses=TAPE_HDR_LONG;
		TapeBlock* tblock = (TapeBlock*)&temp_buffer_y[sizeof(TapeBlock)*tapeCurrentBlock];
		sd_seek_file(&sd_file,tblock->FPos);

		tapeTotByteCount = tblock->FPos;
		tapeBlockLen=tblock->Size + 2;
		bytesToRead = tapeBlockLen<BUFF_PAGE_SIZE ? tapeBlockLen : BUFF_PAGE_SIZE;
		tape_file_status = sd_read_file(&sd_file,sd_buffer,bytesToRead,&bytesRead);
		if (tape_file_status != FR_OK){sd_close_file(&sd_file);break;}
		//printf("Block:%d Seek:%d Length:%ld Read:%d\n",tapeCurrentBlock,tblock->FPos,tapeBlockLen,bytesRead);
	   	tapebufByteCount=2;
		tapeBlockByteCount=0;
	   	tapeStart=cpu.cyc;
	   	TapeStatus=TAPE_LOADING;
	   	break;
	case TAPE_LOADING:
	   	TapeStatus=TAPE_PAUSED;
	   	break;
	case TAPE_PAUSED:
		tapeStart=cpu.cyc;
		TapeStatus=TAPE_LOADING;
		break;
	case TAPE_LOADED:
		return;
		break;
	}
	//printf("2>Tape STATUS:%X\n",TapeStatus);
#endif
}


void Init(){
	TapeStatus = TAPE_STOPPED;
	SaveStatus = SAVE_STOPPED;
}

bool TAP_Load(char *file_name){
#ifndef DEBUG_DISABLE_LOADERS
	//printf("  Tap Load begin\n");
	TapeStatus = TAPE_STOPPED;
	//printf("  Tap FN:%s\n",file_name);
	tape_file_status = sd_open_file(&sd_file,file_name,FA_READ);
	////printf("sd_open_file=%d\n",tape_file_status);
	if (tape_file_status!=FR_OK){sd_close_file(&sd_file);return false;}
   	tapeFileSize = sd_file_size(&sd_file);
	//printf("  .TAP Filesize %lu bytes\n", tapeFileSize);
	tapBlocksCount=0;
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	while (tapeTotByteCount<=tapeFileSize){
		tape_file_status = sd_read_file(&sd_file,tapeBHbuffer,14,&bytesRead);
		if (tape_file_status != FR_OK){sd_close_file(&sd_file);return false;}
		////printf(" Readbuf:%d\n", bytesRead);
		////printf(" pos:%d\n", tapeTotByteCount);
		TapeBlock* block = (TapeBlock*) &tapeBHbuffer;
		TapeBlock* tblock = (TapeBlock*)&temp_buffer_y[sizeof(TapeBlock)*tapBlocksCount];
		memcpy(tblock,block,sizeof(TapeBlock));
		tblock->FPos=tapeTotByteCount;
		//printf("TL>block:%d, size:%d, fpos:%lu \n",tapBlocksCount,tblock->Size,tblock->FPos);
		tapeTotByteCount+=block->Size+2;
		sd_seek_file(&sd_file,tapeTotByteCount);
		if(tapeTotByteCount>=tapeFileSize){
			break;
		}
		if(tapBlocksCount==TAPE_BLK_SIZE){
			break;
		}
		tapBlocksCount++;
	}
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	tapeCurrentBlock=0;
	sd_seek_file(&sd_file,0);
	tape = (uint8_t*)&sd_buffer;

	for(uint8_t j=0;j<tapBlocksCount;j++){
		TapeBlock* tblock = (TapeBlock*)&temp_buffer_y[sizeof(TapeBlock)*j];
		//printf(" block:%d, size:%d \n",j,tblock->Size);
	}
	//printf("ut>tap_loader_active:[%02X][%02X][%08lX]\n",tap_loader_active,TapeStatus,tapeFileSize);
	return true;
#endif
}

void TAP_Rewind(){
	//printf("Tape Rewind\n");
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeCurrentBlock=0;	
	tapeTotByteCount=0;	
	sd_seek_file(&sd_file,0);
	tape = (uint8_t*)&sd_buffer;
	TapeStatus=TAPE_STOPPED;
};

bool TAP_NextBlock(){
	//printf("Tape NextBlock\n");
	TapeStatus=TAPE_STOPPED;
	tapeCurrentBlock++;
	if ((tapeCurrentBlock>=0)&&(tapeCurrentBlock<tapBlocksCount)){
		return true;
	}
	if (tapeCurrentBlock>tapBlocksCount){
		TAP_Rewind();
		return true;
	}
	return false;
};

bool TAP_PrevBlock(){
	//printf("Tape PrevBlock\n");
	TapeStatus=TAPE_STOPPED;	
	tapeCurrentBlock--;
	if ((tapeCurrentBlock>=0)&&(tapeCurrentBlock<tapBlocksCount)){
		return true;
	}
	if (tapeCurrentBlock<0){
		TAP_Rewind();
		return true;
	}	
	return false;
};

void TAP_Eject(){
#ifndef DEBUG_DISABLE_LOADERS
	printf("Tape Eject\n");
	TapeStatus = TAPE_STOPPED;
	//printf("  Tap FN:%s\n",file_name);
	if(tape_file_status!=TAPE_FILE_FREE){
		sd_close_file(&sd_file);
		tapeFileSize = 0;
		tapBlocksCount = 0;
		tapebufByteCount = 0;
		tapeBlockByteCount = 0;
		tapeTotByteCount = 0;
		memset(temp_buffer_y,0,TEMP_BUFF_SIZE);
		tape_file_status = TAPE_FILE_FREE;
	}
#endif
};


/*

#define coef		   (0.990) //0.993->
#define PILOT_TONE	 (2168*coef) //2168
#define PILOT_SYNC_HI  (667*coef)  //667
#define PILOT_SYNC_LOW (735*coef)  //735
#define LOG_ONE		(1710*coef) //1710
#define LOG_ZERO	   (855*coef)  //855

	#define PILOT_TONE	 (2168) //2168
	#define PILOT_SYNC_HI  (667)  //667
	#define PILOT_SYNC_LOW (735)  //735
	#define LOG_ONE		(1710) //1710
	#define LOG_ZERO	   (855)  //855
*/

bool LoadScreenFromTap(char *file_name,bool find_screen){
#ifndef DEBUG_DISABLE_LOADERS
	bool screen_found = false;
	uint8_t* bufferOut;

	memset(sd_buffer, 0, sizeof(sd_buffer));
	memset(temp_buffer_y, 0, sizeof(temp_buffer_y));

	/*
	for(uint8_t i=0;i<TAPE_BLK_SIZE;i++){
		tap_blocks[i].DataType=0;
		tap_blocks[i].Flag=0;
		tap_blocks[i].FPos=0;
		tap_blocks[i].Size=0;
		tap_blocks[i].NAME[0]=0;
	}
	*/
	

	tape_file_status = sd_open_file(&sd_file,file_name,FA_READ);
	//printf("sd_open_file=%d\n",tape_file_status);
	if (tape_file_status!=FR_OK){sd_close_file(&sd_file);return false;}
   	tapeFileSize = sd_file_size(&sd_file);
	//printf(".TAP Filesize %lu bytes\n", tapeFileSize);
	tapBlocksCount=0;
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	while (tapeTotByteCount<=tapeFileSize){
		tape_file_status = sd_read_file(&sd_file,tapeBHbuffer,14,&bytesRead);
		if (tape_file_status != FR_OK){sd_close_file(&sd_file);return false;}
		////printf(" Readbuf:%d\n", bytesRead);
		////printf(" pos:%d\n", tapeTotByteCount);
		
		TapeBlock* block = (TapeBlock*)&tapeBHbuffer;
		TapeBlock* tblock = (TapeBlock*)&temp_buffer_y[sizeof(TapeBlock)*tapBlocksCount];
		memcpy(tblock,block,sizeof(TapeBlock));
		tblock->FPos = tapeTotByteCount;
		//printf(">block:%d, size:[%08lX], fpos:[%08lX] \n",tapBlocksCount,tblock->Size,tblock->FPos);
		tapeTotByteCount+=block->Size+2;
		sd_seek_file(&sd_file,(FSIZE_t)tapeTotByteCount);
		if(tapeTotByteCount>tapeFileSize){
			printf("exit:size [%08lX] tapeTotByteCount:[%08lX]\n",tapeFileSize,tapeTotByteCount);
			break;
		}
		if(tapBlocksCount==TAPE_BLK_SIZE){
			printf("exit:size [%08lX] tapeTotByteCount:[%08lX]\n",tapeFileSize,tapeTotByteCount);
			break;
		}
		tapBlocksCount++;
	}
	printf(">>>tapBlocksCount:%d\n", tapBlocksCount-1);
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	tapeCurrentBlock=0;
	sd_seek_file(&sd_file,0);
	if(find_screen){
		for (int16_t i=0;i<tapBlocksCount;i++){
			TapeBlock* tblock = (TapeBlock*)&temp_buffer_y[sizeof(TapeBlock)*i];
			//printf("block:%d, flag:[%02X], size:[%08lX], fpos:[%08lX] \n",i,tblock->Flag,tblock->Size,tblock->FPos);
			if (tblock->Flag>0){
				if((tblock->Size>=0x1AFE)&&(tblock->Size<=0x1B02)){
					sd_seek_file(&sd_file,tblock->FPos);
					bufferOut = (SD_BUFFER_SIZE>0x2000) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
					memset(sd_buffer, 0, sizeof(sd_buffer));
					tape_file_status = sd_read_file(&sd_file,bufferOut,tblock->Size,&bytesRead);
					//printf("bytesRead=%d\n",bytesRead);
					if (tape_file_status!=FR_OK){sd_close_file(&sd_file);return false;}
					ShowScreenshot(bufferOut,PREVIEW_POS_X,PREVIEW_POS_Y);
					screen_found = true;
					break;
				}
			}			
		}
	}
	if (!screen_found){
		draw_text_len(PREVIEW_POS_X,PREVIEW_POS_Y,"File contents:",COLOR_TEXT,COLOR_BACKGOUND,14);
		for (uint8_t i = 0; i < tapBlocksCount; i++){ //78
			TapeBlock* tblock = (TapeBlock*)&temp_buffer_y[sizeof(TapeBlock)*i];
			if (tblock->Size>0){
				if (tblock->Flag==0){
					memset(temp_msg, 0, sizeof(temp_msg));
					sprintf(temp_msg,"%s",tblock->NAME);
				} else{
					memset(temp_msg, 0, sizeof(temp_msg));
					sprintf(temp_msg," DB %dKb",(tblock->Size/1024));
				}
				if(i<26){
					draw_text5x7_len(PREVIEW_POS_X,PREVIEW_POS_Y+FONT_H+(FONT_5x7_H)*i,temp_msg,COLOR_TEXT,COLOR_PIC_BG,12);	
				} else 
				if((i>25)&&(i<52)){
					draw_text5x7_len(PREVIEW_POS_X+(FONT_5x7_W*FILE_NAME_LEN),PREVIEW_POS_Y+FONT_H+(FONT_5x7_H)*(i-26),temp_msg,COLOR_TEXT,COLOR_PIC_BG,12);	
				} else 
				if((i>51)&&(i<78)){
					draw_text5x7_len(PREVIEW_POS_X+(FONT_5x7_W*(FILE_NAME_LEN*2)),PREVIEW_POS_Y+FONT_H+(FONT_5x7_H)*(i-52),temp_msg,COLOR_TEXT,COLOR_PIC_BG,12);	
				};
			}
		}
	}
	memset(temp_msg, 0, sizeof(temp_msg));
	sprintf(temp_msg,"Type:.TAP");
	draw_text_len(PREVIEW_POS_X,SCREEN_H-(FONT_H*4), temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);
	memset(temp_msg, 0, sizeof(temp_msg));
	sprintf(temp_msg,"FSize: %ldKb",(long)sd_file_size(&sd_file)/1024);
	draw_text_len(PREVIEW_POS_X,SCREEN_H-(FONT_H*3), temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);
	memset(temp_msg, 0, sizeof(temp_msg));
	//strncpy(file_name,"A:/",3);
	//strncpy(temp_msg,file_name,22);
	//draw_text_len(PREVIEW_POS,224, temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);

	sd_close_file(&sd_file);
	//printf("\nAll loaded\n");
	memset(temp_msg, 0, sizeof(temp_msg));
	return true;
#endif
}

/*---------Fast load tap-----------*/

bool FastSkipTAPBytes(uint16_t Length){
	size_t bytesRead;
	////printf("FSTB> Len>%04x\n",Length);
	while (Length>0){
		if (Length<=SD_BUFFER_SIZE) {
			//memset(sd_buffer, 0, sizeof(sd_buffer));
			tape_file_status = sd_read_file(&sd_file,sd_buffer,Length,&bytesRead);
			if (tape_file_status != FR_OK){sd_close_file(&sd_file);return false;}
			for (uint16_t i = 0; i < Length; i++){
				blockChecksum=(blockChecksum^sd_buffer[i]);
			}
			break;
		} else {
			Length=Length-SD_BUFFER_SIZE;
			memset(sd_buffer, 0, sizeof(sd_buffer));
			tape_file_status = sd_read_file(&sd_file,sd_buffer,SD_BUFFER_SIZE,&bytesRead);
			for (uint16_t i = 0; i < SD_BUFFER_SIZE; i++){
				blockChecksum=(blockChecksum^sd_buffer[i]);
			}
			continue;
		}
	}
	return true;
}

uint8_t read_z80(uint16_t addr){
	if (addr<16384) return zx_cpu_ram[0][addr];
	if (addr<32768) return zx_cpu_ram[1][addr-16384];
	if (addr<49152) return zx_cpu_ram[2][addr-32768];
	return zx_cpu_ram[3][addr-49152];
}

void write_z80(uint16_t addr, uint8_t val){
	if (addr<16384) return;//запрещаем писать в ПЗУ
	if (addr<32768) {zx_cpu_ram[1][addr-16384]=val;return;};
	if (addr<49152) {zx_cpu_ram[2][addr-32768]=val;return;};
	zx_cpu_ram[3][addr-49152]=val;
}


bool FastReadTAPBlock(uint16_t tpStart, uint16_t tpLen) {
	////printf("FRTB> Start>%08x  Len>%04x\n",tpStart,tpLen);
	memset(sd_buffer, 0, sizeof(sd_buffer));
	//uint16_t addr;
	uint8_t byte;
	for (uint16_t i = 0; i < tpLen; i++){
		tape_file_status = sd_read_file(&sd_file,&byte,1,&bytesRead);
		if (tape_file_status != FR_OK){sd_close_file(&sd_file);return false;}
		write_z80(tpStart+i, byte);
		blockChecksum=(blockChecksum^byte);
		////printf("RP>%04x  ram>%02x  срu>%02x buff>%02x\n",tpStart+i,RAM[tpStart+i],read_z80(tpStart+i),sd_buffer[i]);
	}
	
	return true;
}

bool FastOpenTAP(char *file_name){
	//printf("  Tap FastLoad begin\n");
	if (tape_file_status==FR_OK) {
		sd_close_file(&sd_file);
		tape_file_status=TAPE_FILE_FREE;//0xFF
	}
	//printf("  FastTap FN:%s\n",file_name);
	tape_file_status = sd_open_file(&sd_file,file_name,FA_READ);
	////printf("sd_open_file=%d\n",tape_file_status);
	if (tape_file_status!=FR_OK){sd_close_file(&sd_file);return false;}
   	tapeFileSize = sd_file_size(&sd_file);
	//printf("  .TAP Filesize %lu bytes\n", tapeFileSize);
	TapeStatus = TAPE_STOPPED;
	return true;	
}

void FastCloseTAP() {
	if (tape_file_status==FR_OK) {
		sd_close_file(&sd_file);
		tape_file_status=TAPE_FILE_FREE;//0xFF
	}
}

bool FastLoadTAP(uint16_t blockType,uint16_t ramAddress,uint16_t ramLen){
	size_t bytesToRead;
	size_t bytesRead;
	
	uint8_t s[2]={0,0};
	uint16_t lBlockLen;
	uint16_t lBlockID;
	uint16_t lBlockChecksum;
	uint16_t de;
	
	im_z80_stop = true;
	bool res=false;
	
	if (tape_file_status==TAPE_FILE_FREE) {return res;}

	tape_file_status = sd_read_file(&sd_file,tapeBHbuffer,3,&bytesRead);
	TapeBlock* block = (TapeBlock*) &tapeBHbuffer;
	lBlockLen = block->Size-2;
	lBlockID = block->Flag;
	blockChecksum=lBlockID;

	//printf("Fast Tape Load Block>%02X - %04X:%04X\n",blockType,ramAddress,ramLen);

	if (lBlockID == blockType) {
		if (ramLen <= lBlockLen) {
			if(FastReadTAPBlock(ramAddress,ramLen)){///+1
				if (ramLen < lBlockLen) {
					FastSkipTAPBytes(lBlockLen - ramLen);
				}
			} else {
				res=false;
			}
			tape_file_status = sd_read_file(&sd_file,s,1,&bytesRead);
			lBlockChecksum=s[0];
			cpu.ix=(cpu.ix+ramLen)&(0xFFFF);
			cpu.d=0;
			cpu.e=0;
			if (blockChecksum == lBlockChecksum) {
				res=true;
			} else {
				printf("Fast Tape Load Checksum ERROR!!");
				res=false;
			};
			
		} else {
			if(FastReadTAPBlock(ramAddress,lBlockLen)){
				tape_file_status = sd_read_file(&sd_file,s,1,&bytesRead);
				lBlockChecksum=s[0];
				cpu.ix=(cpu.ix+ramLen)&(0xFFFF);
				de=((cpu.d * 256) | cpu.e);
				de=de-lBlockLen;
				cpu.d = (de & 0xFF00) >> 8;
				cpu.e = (de & 0xFF);
				if (blockChecksum == lBlockChecksum) {
					res=true;
				} else {
					printf("Fast Tape Load Checksum ERROR!!");
					res=false;
				};
			}
			//res=false;
		} // (lLength <= lBlockLen)
	} else {
		FastSkipTAPBytes(lBlockLen);
		tape_file_status = sd_read_file(&sd_file,s,1,&bytesRead);
		lBlockChecksum=s[0];
		res=false;
	}  //(lBlockID = blockType)
	if (sd_file_pos(&sd_file)>=sd_file_size(&sd_file)){
		printf("Full Fast Load\n");
		tap_loader_active = TAPE_OFF;
		sd_close_file(&sd_file);
		tapeFileSize = 0;
		tapBlocksCount = 0;
		tapebufByteCount = 0;
		tapeBlockByteCount = 0;
		tapeTotByteCount = 0;
		tape_file_status = TAPE_FILE_FREE;		
		//printf("Tape Full Load\n");
	}
	TapeStatus = TAPE_LOADED;
	im_z80_stop=false;
	return res;
}