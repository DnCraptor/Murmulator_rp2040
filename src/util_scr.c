#include "util_z80.h"
#include "util_sd.h"
#include <string.h>
#include "zx_emu/z80.h"
#include "zx_emu/aySoundSoft.h"
#include "zx_emu/zx_machine.h"
#include "screen_util.h"


extern uint8_t RAM[ZX_RAM_PAGE_SIZE*8]; //Реальная память куском 128Кб
extern uint8_t zx_Border_color;

extern uint8_t sd_buffer[SD_BUFFER_SIZE];
extern int last_error;


bool LoadScreenshot(char *file_name, bool open_file){
	size_t bytesRead;
	file_descr=0;
	char fileinfo[30];
	UINT bytesToRead;
	uint8_t* bufferIn;
	uint8_t* bufferOut;
	uint16_t pageSize = 0;
	int8_t pageNumber = 0;
	uint16_t destSize=0;
	uint16_t usedBytes=0;

	bufferIn = (uint8_t*)&sd_buffer;

	bufferOut = (sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
	if (open_file){bufferOut = &RAM[5*ZX_RAM_PAGE_SIZE];}
	memset(bufferOut, 0, 0x1B00);

	pageSize = 0x1B00;
	file_descr = sd_open_file(&sd_file,file_name,FA_READ);
	////printf("sd_open_file=%d\n",file_descr);
	if (file_descr!=FR_OK){sd_close_file(&sd_file);return false;}	
	
	do{
		if (pageSize>sizeof(sd_buffer)){
			bytesToRead = sizeof(sd_buffer);
		} else {
			bytesToRead = pageSize;
		}
		file_descr = sd_read_file(&sd_file,bufferIn,bytesToRead,&bytesRead);
		if (file_descr != FR_OK){sd_close_file(&sd_file);return false;}
		////printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
		if (bytesRead != bytesToRead){sd_close_file(&sd_file);return false;}
		memcpy(bufferOut,bufferIn,bytesRead);
		bufferIn = (uint8_t*)&sd_buffer;
		bufferOut += bytesRead;
		pageSize-=bytesRead;
	}while (pageSize>0);
	if (!open_file){
		bufferOut = (sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
		ShowScreenshot(bufferOut,PREVIEW_POS_X,PREVIEW_POS_Y);
		sprintf(fileinfo,"Type:.SCR - ZX Screen");
		draw_text_len(PREVIEW_POS_X,SCREEN_H-(FONT_H*4), fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);
		memset(fileinfo, 0, sizeof(fileinfo));
		sprintf(fileinfo,"FSize: %ldKb",(long)sd_file_size(&sd_file)/1024);
		draw_text_len(PREVIEW_POS_X,SCREEN_H-(FONT_H*3), fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);
	}
	zx_Border_color=0;
	//last_out_7ffd = 0x30;
	//zx_video_ram  = zx_ram_bank[5];
	sd_close_file(&sd_file);
	return true;
}

