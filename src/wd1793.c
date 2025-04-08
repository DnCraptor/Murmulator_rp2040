/*
	Terms:
	  * Sector - a tr-dos sector is 256 bytes length, has number 1-16
	  * Block - minimum amount of bytes on SD/TF card that can be read or written is 512 bytes

*/
//#define  DEBUG_TRD

int null_printf(const char *str, ...);

#ifdef DEBUG_TRD
	#define debug_printf  printf
#else 
	#define debug_printf  null_printf
#endif


#include <pico.h>
#include "pico/time.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdbool.h"
#include "util_sd.h"
#include "screen_util.h"
#include "wd1793.h"

#define  SECTORLENGTH		256U
#define  SECTORPERTRACK		16U
#define  DISKSIDES			2U
#define  S_SIZE_TO_CODE(x)  ((((x)>>8)|(((x)&1024)>>9)|(((x)&1024)>>10))&3)
#define  S_CODE_TO_SIZE(x)  (1<<(7+(x)))

#define  BUFFERSIZE 0x4000//512U
#define  BUFFERSIZEFACTOR (BUFFERSIZE/SECTORLENGTH)
#define  DEFAULT_DISK_POS 0xFFFFU


//#define  REVOLUTION_TIME 6U //6U in .h
//#define  TIME_PER_SECTOR 1U //1U

//Один такт Z80 = 0.2857142857142857 us

/*
#define  BYTE_READ_TIME 24//       32uS    112U тактов Z80 64U
#define  STEP_TIME   3000U // 3ms 3000us 10500U тактов Z80 6000U
#define  SEEK_DELAY 800 // 1400
#define  INDEX_COUNTER_TIME 	65536 //116666  //100000  //    28570uS 100000 тактов Z80
*/

#define  BYTE_READ_TIME 12U//       32uS    112U тактов Z80 64U
#define  STEP_TIME   1500U // 3ms 3000us 10500U тактов Z80 6000U
#define  SEEK_DELAY 300U // 1400
#define  INDEX_COUNTER_TIME 40000U	//	40000//65536 //116666  //100000  //    28570uS 100000 тактов Z80
#define  REV_PER_SECS  20000U //58752 //20000


//#define  MIN(a,b) ((a<b)?(a):(b))

bool TRDOS_mode; 		// Информационный сигнал Текущий режим - ROM TRDOS или стандартный ROM 48k
bool TRDOS_disabled; 	// Управляющий сигнал Запрет входить в TRDOS
uint8_t WD1793_Status;
uint8_t Requests;
uint8_t wd1793_PortFF;
WD1793_struct WD1793;
uint8_t NewCommandReceived;


const uint8_t Turbo = 1; //0;
uint32_t Delay; //in timer ticks

volatile uint32_t TCNT1=0;
volatile uint8_t  IndexCounter=0;

volatile uint32_t passed=0;

uint32_t TimerValue;
uint16_t BufferPos = 0;
uint16_t SectorPos = 0;
uint16_t pos;
uint8_t NoDisk = 1;
uint8_t BufferUpdated = 0;
uint16_t FormatCounter = 0;
uint8_t NewDrive = 5;
//const uint8_t ControlDrive = 3; // D:
//uint8_t SelectedDrive = 0; // default drive A:
//char * SelectedImage = NULL;
uint8_t SelectedDrive;
char* SelectedImage;

#ifndef DEBUG_DISABLE_LOADERS
	extern uint8_t temp_buffer_y[TEMP_BUFF_SIZE_Y];
#endif

extern char dir_path[];
char list_path[128] = "";

char disk[4][160];

FRESULT res;
uint8_t FilesCount = 0;
size_t TrdFileSize;
//uint8_t buf[BUFFERSIZE];
extern uint8_t sd_buffer[SD_BUFFER_SIZE];
extern char temp_msg[60];
uint br;
uint8_t CmdType = 1;
uint16_t CurrentDiskPos = DEFAULT_DISK_POS; // Big enough to be outside of disk

WD1793_struct WD1793;

uint8_t Requests = 0; // выдает состояние контроллера 
uint8_t SIDE = 0;
uint8_t DRV = 5;
uint8_t Prevwd1793_PortFF = 0xFF;

void WD1793_Cmd_StartIdle();

uint8_t NewCommandReceived = 0;
typedef void (*Command) ();
Command CurrentCommand = WD1793_Cmd_StartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
Command NextCommand = NULL; 
Command CommandTable[16] = {
		WD1793_Cmd_Restore, // 00 Restore 	Восстановление 					- %0000 hvrr - +
		WD1793_Cmd_Seek, 	// 10 Seak		Поиск/Позиционирование 			- %0001 hvrr |
		WD1793_Cmd_Step, 	// 20 Step		Шаг в предыдущем направлении 	- %001t hvrr + - 
		WD1793_Cmd_Step, 	// 30 Step		Шаг в предыдущем направлении 	- %001t hvrr + - 
		WD1793_Cmd_Step, 	// 40 Step In	Шаг вперед                   	- %010t hvrr |
		WD1793_Cmd_Step, 	// 50 Step In	Шаг вперед                   	- %010t hvrr |
		WD1793_Cmd_Step, 	// 60 Step Out	Шаг назад                    	- %011t hvrr - +
		WD1793_Cmd_Step, 	// 70 Step Out	Шаг назад                    	- %011t hvrr - +
		WD1793_Cmd_ReadSector, // 80		Чтение сектора               	- %100m seca
		WD1793_Cmd_ReadSector, // 90		Чтение сектора               	- %100m seca
		WD1793_Cmd_WriteSector, // A0		Запись сектора               	- %101m sec0
		WD1793_Cmd_WriteSector, // B0		Запись сектора               	- %101m sec0
		WD1793_Cmd_ReadAddress, // C0		Чтение адреса                	- %1100 0e00
		WD1793_Cmd_StartIdle, 	// D0		Принудительное прерывание   	- %1101 iiii
		WD1793_Cmd_ReadTrack, 	// E0		Чтение дорожки               	- %1110 0e00
		WD1793_Cmd_WriteTrack 	// F0		Запись дорожки/форматир-е   	- %1111 0e00
	};
/*
где rr - скорость позиционирования головки: 00 - 6мс, 01 - 12мс, 10 - 20мс, 11 - 30мс (1Mhz).
	v - проверка номера дорожки после позиционирования.
	h - загрузка головки (всегда должен быть в 1!).
	t - изменение номера дорожки в регистре дорожки после каждого шага.
	a - тип адресной метки (0 - #fb, стирание сектора запрещено 1 - #F8, стирание сектора разрешено).
	c - проверка номера стороны диска при идентификации индексной области.
	e - задержка после загрузки головки на 30мс (1Mhz).
	s - сторона диска (0 - верх/1 - низ).
	m - мультисекторная операция.
	i - условие прерывания:
	i0 - после перехода сигнала cprdy из 0 в 1 (готов);
	i1 - после перехода сигнала cprdy из 1 в 0 (не готов);
	i2 - при поступлении индексного импульса;
	i3 - немедленное прерывание команды, при:
	i=0 - intrq не выдается
	i=1 - intrq выдается.
*/

/*void init_buffers(void){
	for (uint8_t i=0;i<3;i++){
		disk_buffers[i]=&sd_buffer[i*0x1000];
	}
}*/


bool load_image_TRDOS(char *file_name,uint8_t drid){
	debug_printf("[154]Selected TRDOS image: %s\n",file_name);
	memcpy(disk[drid],file_name,160);
	//memcpy(Path,file_name,3);
	//debug_printf("[143]Sets Path: [%s]\n",Path);
	//SelectedDrive=drid;
	//sd_close_file(&sd_file);
	DRV = 5;
	WD1793_Status=0;
	return true;
}

bool free_image_TRDOS(uint8_t drid){
	memset(disk[drid],0,160);
	DRV = 5;
	WD1793_Status=0;
	return true;
}

bool is_image_TRDOS_empty(uint8_t drid){
	if(strlen(disk[drid])==0){
		return true;
	}
	return false;
}


bool LoadScreenFromTRD(char *file_name){
#ifndef DEBUG_DISABLE_LOADERS
	size_t bytesRead;
	size_t bytesToRead;
	TRDFNames trd_file;
	//uint8_t disk_buff[0x800];

	memset(temp_buffer_y, 0, sizeof(temp_buffer_y));

	res = sd_open_file(&sd_file,file_name,FA_READ);
    ////debug_printf("sd_open_file=%d\n",res);
	if (res!=FR_OK){sd_close_file(&sd_file);return false;}
   	TrdFileSize = sd_file_size(&sd_file);
    ////debug_printf(".TRD Filesize %u bytes\n", TrdFileSize);
	res = sd_read_file(&sd_file,temp_buffer_y,TEMP_BUFF_SIZE_Y,&bytesRead);
	uint16_t idx=0;
	for(uint8_t i=0;i<TRD_BLK_SIZE;i++){
		TRDFNames* trd_file = (TRDFNames*) &temp_buffer_y[(i*0x10)];
		if(trd_file->Name[0]!=0){
			idx++;
		}
	}
	draw_text_len(PREVIEW_POS_X,PREVIEW_POS_Y,"File contents:",COLOR_TEXT,COLOR_BACKGOUND,14);
	memset(temp_msg, 0, sizeof(temp_msg));
	sprintf(temp_msg,"Total files:%d",idx);
	draw_text_len(PREVIEW_POS_X,SCREEN_H-(FONT_H*4), temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);	
	memset(temp_msg, 0, sizeof(temp_msg));
	sprintf(temp_msg,"File size:%dk",(TrdFileSize/1024));
	draw_text_len(PREVIEW_POS_X,SCREEN_H-(FONT_H*3), temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);
	for (uint8_t i = 0; i < 78; i++){
		TRDFNames* trd_file = (TRDFNames*) &temp_buffer_y[(i*0x10)];
		if(trd_file->Name[0]!=0){
			memset(temp_msg, 0, sizeof(temp_msg));
			char file_name[14];
			memset(file_name, 0, sizeof(file_name));
			if(((trd_file->Ext[0]==0x42)||(trd_file->Ext[0]==0x43)||(trd_file->Ext[0]==0x44)||(trd_file->Ext[0]==0x23))||((trd_file->Ext[1]>0x80)||(trd_file->Ext[1]<0x30))){ //||(trd_file->Ext[1]>0x80)||(trd_file->Ext[1]<0x21)
				strncpy(&file_name[0],trd_file->Name,8);
				strncpy(&file_name[8],".",1);
				strncpy(&file_name[9],trd_file->Ext,1);
				file_name[10]=0;
			} else {
				strncpy(&file_name[0],trd_file->Name,8);
				strncpy(&file_name[8],".",1);
				strncpy(&file_name[9],trd_file->Ext,3);
			}
			sprintf(temp_msg,"%s",file_name); 
			debug_printf("file> %s \n",temp_msg);
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
	sd_close_file(&sd_file);
	return true;
	#endif
}


uint8_t GetWD1793_Status(){
	return WD1793_Status;
}

uint8_t GetWD1793_Drive(){
	return DRV;
}
uint8_t GetWD1793_Drive_Load(uint8_t drive){
	return strlen(disk[drive]);
}

void WD1793_Write(uint8_t Address, uint8_t Value){ // Z80 write to port 
	debug_printf("[252]WrPort %02X, %02X\n", Address & 0x03, Value);
	// 0 - 0x1F  1 - 0x3F  2 - 0x5F  3 - 0x7F  
	switch (Address & 0x03){
		case 0 : // 0x1F Write to Command Register
				Address = 4;
				NewCommandReceived = 1;
				break;
		case 2 :
				debug_printf("[260]5F<S:%d \n",Value);
				break;
		case 3 : // 0x7F Write to Data Register 
				 // задание номера логического трека, для команды позиционирования или запись байта если была какая-либо команда на запись.
				WD1793.StatusRegister &= ~_BV(stsDRQ); // Запрос данных
				Requests &= ~_BV(rqDRQ); //When !WR
				break;
	}
	WD1793.Regs[Address] = Value; // 1 - 0x3F 2-0x5F 3-0x7F 4-0x1F
}

uint8_t WD1793_Read(uint8_t Address){ // Z80 read from port 
	Address &= 0x03;
	switch (Address){
		case 0 : // 0x1F Read from Status Register
				 // выдает состояние контроллера, где A:
				Requests &= ~_BV(rqINTRQ);
		break;
		case 3 : // 0x7F Read from Data Register
				 // чтение байта
				WD1793.StatusRegister &= ~_BV(stsDRQ); // Запрос данных
				Requests &= ~_BV(rqDRQ); //When !RD
				break;
	}
	debug_printf("[284]RdA %02X D: %02X\n", Address & 0x03,WD1793.Regs[Address]);
	return WD1793.Regs[Address];
}

void WD1793_timer(uint32_t dtcpu){ // Вызывается из другого ядра эмуляции Таймер для TR-DOS
	//printf("time pass %d \n",time_us_32()-passed);
	TCNT1 += 1;//(time_us_32()-passed); //dtcpu / 1.7;
    if (TCNT1 >= INDEX_COUNTER_TIME){ //INDEX_COUNTER_TIME) //100000
		//debug_
		printf("time pass %d \n",(time_us_32()-passed));
		passed=time_us_32();
		printf("[299]TCNT OVF %d \n",TCNT1 - INDEX_COUNTER_TIME);
		TCNT1 = TCNT1 - INDEX_COUNTER_TIME;
		if((++IndexCounter > REVOLUTION_TIME)){
			printf("[303]IndexCounter %d \n",IndexCounter);
			IndexCounter=0;
		}
		if(IndexCounter == REVOLUTION_TIME){
			printf("[307]IndexCounter %d \n",IndexCounter);
			TCNT1 = REV_PER_SECS; // correction for 5rps, 200 ms per revolution
		}
    }
	
}

uint32_t GetTimerValue(){ //Получаем значение счетчика
	//volatile	uint32_t value;
	//value = TCNT1;
	return TCNT1;///1.75; //value;
}

void StartTimePeriod(){ // Задание нового тамера с нуля
	//TimerValue = 0;
	//TCNT1=0;
	TimerValue = TCNT1;
}

bool HasTimePeriodExpired(uint16_t period){
	uint32_t t;
	t = GetTimerValue();
	if  (t - TimerValue < period){
		return false;
	}
	debug_printf("[322] Expire timer:[%d %d %d]\n",t - TimerValue, (t - TimerValue)-period, period);
	TimerValue = t;
	return true;	
}

void PrintTimePeriod(char * msg){
	uint16_t t = (GetTimerValue() - TimerValue)/2;
	/*if(msg != NULL){
		//printf(msg);
	}*/
	debug_printf("[332]Timer period %d\n", t);
}

uint8_t WD1793_GetCurrentSector(){ // узнать сектор по текущему времени
	uint8_t sector;
	uint8_t result;
	uint32_t get_time = GetTimerValue();
	//cli();
	sector = ((IndexCounter << 8) | (get_time>>8))/92U; //
	result = ((sector & 1) << 3) | ((sector >> 1) + 1);
	//debug_
	printf("[354]sector:%d GetCurrentSector: %d IndexCounter:%d GetTimerValue:%d\n", sector, result, IndexCounter, get_time);
	//sei();
	/*if(sector > 15){
		sector = 15;
	}*/
	//debug_printf("[360]GetCurrentSector: %d >> %d\n",sector, ((sector & 1) << 3) | ((sector >> 1) + 1));
	// 1,9,2,10,3,11,4,12,5,13,6,14,7,15,8,16
	//return ((sector & 1) << 3 | (sector >> 1)) + 1; 
	return result;
}

uint8_t WD1793_GetIndexMark(){
	uint8_t idx;
	uint32_t ttt = GetTimerValue();
	//cli();
	idx = IndexCounter == 0 && (ttt) < STEP_TIME; // 3ms 6000 ticks
	//sei();
	//debug_printf("[356]idx:%d IndC:%d GTV:%d\n", idx, IndexCounter, ttt);
	return idx; 
}


void printError(char* msg, uint8_t code){

	//printf(msg);
	switch(code){
		case FR_DISK_ERR:
		//printf("FR_DISK_ERR\n");
		break;
		case FR_NOT_READY:
		//printf("FR_NOT_READY\n");
		break;
		case FR_NO_FILE:
		//printf("FR_NO_FILE\n");
		break;
		//case FR_NOT_OPENED:
		debug_printf("FR_NOT_OPENED\n");
		//break;
		case FR_NOT_ENABLED:
		//printf("FR_NOT_ENABLED\n");
		break;
		case FR_NO_FILESYSTEM:
		//printf("FR_NO_FILESYSTEM\n");
		break;
	}
}

void WD1793_FlushBuffer(){
	if (BufferUpdated){ // flush partially filled buffer
		sd_seek_file(&sd_file, (uint32_t)CurrentDiskPos << 8); // return to start of sector on SD Card
		res = sd_write_file(&sd_file, &sd_buffer, BUFFERSIZE, &br);
		if (res!=FR_OK){
			printError("[391]Writing error: ", res);
			return;
		}
		sd_write_file(&sd_file, 0, 0, &br);
		if (res!=FR_OK){
			printError("[396]Writing error on finish: ", res);
			return;
		}
		BufferUpdated = 0;
		debug_printf("[400]Flushed buffer\n");
	}
	WD1793_Status=0;
}

// position is address/256
uint16_t ComputeDiskPosition(uint8_t Sector, uint8_t Track, uint8_t Side){
	uint16_t addr = (((uint16_t)Track * DISKSIDES + (uint16_t)Side) * SECTORPERTRACK + (uint16_t)Sector);
	debug_printf("[408] CompDiskPos Addr 0x%04X_%04X\n", (uint16_t)(addr>>8), (uint16_t)(addr<<8));
   return addr;
}

void SetDiskPosition(uint16_t pos){
	if(pos < CurrentDiskPos || pos >= (CurrentDiskPos + BUFFERSIZEFACTOR)){
		pos &= ~(BUFFERSIZEFACTOR-1); // round position to buffer size
		if(pos != (CurrentDiskPos + BUFFERSIZEFACTOR)){ // move position if only it is needed but it saves only 100 mcs
			sd_seek_file(&sd_file, (uint32_t)pos << 8);
		}
		res = sd_read_file(&sd_file, &sd_buffer, BUFFERSIZE, &br);
		if (res){
			printError("[420]Reading error: ", res);
			return;
		}
		debug_printf("\n[423]R:512 %d pos:%d\n",br,pos);
		CurrentDiskPos = pos;
	}
	debug_printf("[426]SetDiskPos %d\n",pos);
}

void WD1793_CmdIdle(){
	switch(CmdType){
		case 1:
		if (!NoDisk && WD1793_GetIndexMark()) { // Speccy checks rotation
			WD1793.StatusRegister |= _BV(stsIndexImpuls);
			debug_printf("[434]CmdIdle Speccy checks rotation\n");
		} else {
			WD1793.StatusRegister &= ~_BV(stsIndexImpuls);
			//debug_printf("[437]CmdIdle Speccy not checks rotation\n");
		}
		break;
	}//debug_printf("[439]CmdIdle\n");
}
void WD1793_Reset(uint8_t drive){
	debug_printf("[443] Drive: %c\n", 'A'+drive);
	WD1793_FlushBuffer();
	NoDisk = 1;
	CurrentDiskPos = DEFAULT_DISK_POS;
	sd_close_file(&sd_file);
	debug_printf("[448]Open File: %s",disk[drive]);
	if (strlen(disk[drive])>0){
		res = sd_open_file(&sd_file,disk[drive], FA_READ | FA_WRITE);
		if (res==FR_OK){
			IndexCounter=0;
			debug_printf(" Success \n");
			NoDisk = 0;
			return;
		}
	}
	debug_printf(" Error!!! \n");
	NoDisk = 1;
	memset(sd_buffer,0,BUFFERSIZE);
	CurrentDiskPos = DEFAULT_DISK_POS;
	CurrentCommand = WD1793_Cmd_StartIdle;
	//char strbuf[20]="\0";	//temp string buffer
	debug_printf("[464] Drive: %c\n",  'A'+drive);
	debug_printf("[465]%s %s\n",   disk);
}


void WD1793_Cmd_StartIdle(){  // D0		Принудительное прерывание   	- %1101 iiii
	WD1793.StatusRegister &= ~_BV(stsBusy); // clear bit
	Requests &= ~_BV(rqDRQ); // CmdStartIdle clear
	Requests |= _BV(rqINTRQ);
	
 	if(CmdType != 1 && WD1793.StatusRegister & _BV(stsLostData)){
 		debug_printf("---[475]Lost data, cmd=%02X\n", WD1793.CommandRegister);		
 	}
	
	WD1793_FlushBuffer();
	CurrentCommand = WD1793_CmdIdle;
}


void WD1793_CmdStartIdleForceInt0(){
	WD1793.StatusRegister &= ~_BV(stsBusy); // clear bit
	Requests &= ~_BV(rqDRQ); // CmdStartIdleForcent0 clear
	Requests &= ~_BV(rqINTRQ);
	
	WD1793_FlushBuffer();
	CurrentCommand = WD1793_CmdIdle;
		debug_printf("[490]CmdStartIdleF\n");
}

void WD1793_CmdType1Status(){

	if (WD1793.RealTrack == 0)
	WD1793.StatusRegister |= _BV(stsTrack0);
	
	if(WD1793.CommandRegister & 0x08){ // head load
		WD1793.StatusRegister |= _BV(stsLoadHead);
	}

	CurrentCommand = WD1793_Cmd_StartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
	debug_printf("[503]CmdType1Stat\n");
}

void WD1793_CmdDelay(){
	if (!HasTimePeriodExpired(Delay)) return;
	debug_printf("[508]CmdDelay\n");
	CurrentCommand = NextCommand;
}


void WD1793_CmdReadingSector(){
	WD1793_Status=1;
 	if(!HasTimePeriodExpired(BYTE_READ_TIME)){ // 32us 64
		return;
	} 

	if(SectorPos != 0 && Requests & _BV(rqDRQ)){
		debug_printf("[520]L Sec R: %d\n",WD1793.SectorRegister);
		WD1793.StatusRegister |= _BV(stsLostData);			
	}


	if (SectorPos >= SECTORLENGTH){
		if(WD1793.Multiple){
			if(BufferPos != 0 && !(BufferPos % BUFFERSIZE)){
				//res = sd_read_file(&sd_file, sd_buffer, BUFFERSIZE, &br);
				res = sd_read_file(&sd_file, &sd_buffer, BUFFERSIZE, &br);
				if (res){
					printError("[531]Reading error: ", res);		
				}
				debug_printf("[533]RdBufSize:%d\n",BUFFERSIZE);
			}
			SectorPos = 0;
			if(++WD1793.RealSector > SECTORPERTRACK){
				CurrentCommand = WD1793_Cmd_StartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
				return;
			}
			CurrentCommand = WD1793_CmdStartReadingSector;
			return;
		}
		CurrentCommand = WD1793_Cmd_StartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}

	WD1793.DataRegister = sd_buffer[BufferPos % BUFFERSIZE];
	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
	debug_printf("[550]BufferPos: %02X (%02X) \n", BufferPos,WD1793.DataRegister);

	BufferPos++;
	SectorPos++;
}

void WD1793_CmdWritingSector(){
	WD1793_Status=2;
	debug_printf("[558]CmdWrSec\n");
	if(!HasTimePeriodExpired(BYTE_READ_TIME*4)){ // 32us 64
		return;
	}		
	if(Requests & _BV(rqDRQ)){
		debug_printf("[563]L W Sec\n");
		WD1793.StatusRegister |= _BV(stsLostData);			
	}

	if(!(WD1793.StatusRegister & _BV(stsLostData))){
		sd_buffer[BufferPos % BUFFERSIZE] = WD1793.DataRegister;
	}
	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
	BufferUpdated = 1;
	BufferPos++;
	SectorPos++;
	        
	if (SectorPos >= SECTORLENGTH){
		CurrentCommand = WD1793_Cmd_StartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
	}
}

void WD1793_CmdWritingTrack(){ //WD1793_CmdStartWritingTrack() //Calling from WD1793_Cmd_WriteTrack() //F0 base command
	WD1793_Status=2;
	if(!HasTimePeriodExpired(BYTE_READ_TIME * 3)){ // 96us 64*3
		return;
	}

	if(Requests & _BV(rqDRQ)){
		debug_printf("[588]Lost data in writing track Pos=%d\n", BufferPos);
		WD1793.StatusRegister |= _BV(stsLostData);
		CurrentCommand = WD1793_Cmd_StartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}	

	debug_printf("[594]%02X \n", WD1793.DataRegister);
	
	if(WD1793.DataRegister == 0x4E){
		FormatCounter++;
	} else {
		FormatCounter  = 0;
	}
	
	if (FormatCounter  > 400){
		CurrentCommand = WD1793_Cmd_StartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}
	
	//sd_buffer[BufferPos % BUFFERSIZE] = WD1793.DataRegister;
	//BufferUpdated = 1;

	BufferPos++;
	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных

}
	        
void WD1793_CmdReadingAddress(){// <- [1076]
	WD1793_Status=1;
	//debug_printf("[618]CmdReadAddr\n");
 	if(!HasTimePeriodExpired(BYTE_READ_TIME)){ // 32us 64
		return;
	} 

	if(SectorPos != 0 && Requests & _BV(rqDRQ)){
		//if (err_timeout--) return;
		debug_printf("[625]  L Addr read\n");
		WD1793.StatusRegister |= _BV(stsLostData);
	}	 

	if (BufferPos >= 6){
		CurrentCommand = WD1793_Cmd_StartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}
		       
	switch(BufferPos){
		case 0: // N track (0-128)
		WD1793.DataRegister = WD1793.TrackRegister;
		WD1793.SectorRegister = WD1793.TrackRegister;
		break;
		       
		case 1: // N side always 0
		WD1793.DataRegister = 0;
		break;
		       
		case 2: // N sector (0-128)
		WD1793.DataRegister = WD1793.SectorRegister-1; //&&& WD1793_GetCurrentSector(); // узнать сектор по текущему времени
		break;
		       
		case 3: // sector size 0-128 1-256 2-512 3-1024
		WD1793.DataRegister = 1; // 256 bytes
		break;
		       
		case 4: // checksum
		case 5:
		WD1793.DataRegister = 0;
		break;
	}
#ifdef DEBUG	
	//printf("0x%02X ", WD1793.DataRegister);
#endif	
	BufferPos++;
	Requests |= _BV(rqDRQ); // Set rqDRQ
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных

}

void WD1793_Cmd_Restore(){ // 00 Restore 	Восстановление 					- %0000 hvrr - +
	debug_printf("[667]Restore\n");
	StartTimePeriod();
	WD1793.TrackRegister = 0;
	WD1793.RealTrack = 0;
	WD1793.Direction = 0;

	Delay = STEP_TIME; // 3ms 6000 in procedure WD1793_Cmd_Restore()
	NextCommand = WD1793_CmdType1Status;
	CurrentCommand = WD1793_CmdDelay;
	CmdType = 1;
}

void WD1793_Cmd_Seek(){// 10 Seak		Поиск/Позиционирование 			- %0001 hvrr |
	WD1793_Status=1;
	debug_printf("[681]Seek to track %i\n", WD1793.DataRegister);
	StartTimePeriod();
	if (WD1793.TrackRegister > WD1793.DataRegister)
	WD1793.Direction = 1;
	else
	WD1793.Direction = 0;

	Delay = abs(WD1793.TrackRegister - WD1793.DataRegister) * SEEK_DELAY;// 800;
	WD1793.TrackRegister = WD1793.DataRegister;
	WD1793.RealTrack = WD1793.TrackRegister;

	NextCommand = WD1793_CmdType1Status;
	CurrentCommand = WD1793_CmdDelay;
	CmdType = 1;
}

void WD1793_Cmd_Step(){// 20 30 40 50 60 70 Step		Шаг в предыдущем направлении 	- %001t hvrr + - 
	debug_printf("[698]CmdStep\n");
	StartTimePeriod();

	switch (WD1793.CommandRegister & 0xF0){ // Command Decoder
		case 0x40:
		case 0x50: // Step In
		WD1793.Direction = 0;
		break;

		case 0x60:
		case 0x70: // Step Out
		WD1793.Direction = 1;
		break;
	}
	
	if (WD1793.Direction == 0 && WD1793.TrackRegister < 80){
		WD1793.TrackRegister++;
	}
	if (WD1793.Direction == 1 && WD1793.TrackRegister > 0){
		WD1793.TrackRegister--;
	}
	debug_printf("[719]Step %s to track %i\n", WD1793.Direction ? "in" : "out", WD1793.TrackRegister);
	Delay = STEP_TIME; // 3ms 6000 in procedure WD1793_Cmd_Step()
	NextCommand = WD1793_CmdType1Status;
	CurrentCommand = WD1793_CmdDelay;
	CmdType = 1;
}

void WD1793_CmdStartReadingSector(){
	WD1793_Status=1;
	debug_printf("[728]CmdStartRdSec\n");
	if(!Turbo && WD1793_GetCurrentSector() != WD1793.RealSector){ // узнать сектор по текущему времени
		return;
	}
	CurrentCommand = WD1793_CmdReadingSector;
	StartTimePeriod();
}

void WD1793_Cmd_ReadSector(){  // 80	90	Чтение сектора               	- %100m seca
	WD1793_Status=1;
	debug_printf("[738]CmdRdSec\n");
	WD1793.Multiple = WD1793.CommandRegister & 0x10;
	pos = ComputeDiskPosition(WD1793.SectorRegister-1, WD1793.TrackRegister, SIDE);
	WD1793.RealSector = WD1793.SectorRegister;

	/*if(DRV == ControlDrive){
		if(pos >= 0 && pos < 0x8){
			memset(sd_buffer, 0, SECTORLENGTH);
			WD1793_ReadDir(pos);
		} else 
		if(pos >= 0x8 && pos < 0x9){
			memset(sd_buffer, 0, SECTORLENGTH);
			WD1793_ReadDiskInfo();
		} else {
			debug_printf("Look up file name\n");
			WD1793_LookUpFile((WD1793.TrackRegister<<1) | SIDE);
			WD1793_UpdateConfig();
		}
		BufferPos = 0;
	} else {*/
		BufferPos = (pos & (BUFFERSIZEFACTOR-1)) << 8; // Position in buffer
		SetDiskPosition(pos); //пример [332]R:512 512 pos:0
	//}
	SectorPos = 0;
	CurrentCommand = WD1793_CmdStartReadingSector;
	CmdType = 2;
	debug_printf("[764]TryRD[80-90] SEC %i, TRK %i, SIDE %i - %ld\n", WD1793.SectorRegister, WD1793.TrackRegister, SIDE, (uint32_t) (pos<<8));
}

void WD1793_CmdStartWritingSector(){
	WD1793_Status=2;
	if(!Turbo && WD1793_GetCurrentSector() != WD1793.SectorRegister){ // узнать сектор по текущему времени
		return;
	}
	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
	CurrentCommand = WD1793_CmdWritingSector;
	StartTimePeriod();
}

void WD1793_Cmd_WriteSector(){// A0 B0		Запись сектора               	- %101m sec0
	WD1793_Status=2;
	WD1793.Multiple = WD1793.CommandRegister & 0x10;
	CmdType = 2;
	
	/*if(DRV == ControlDrive){
		WD1793.StatusRegister |= _BV(stsWriteProtect);
		CurrentCommand = WD1793_Cmd_StartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}*/
	
	pos = ComputeDiskPosition(WD1793.SectorRegister-1, WD1793.TrackRegister, SIDE);
	BufferPos = (pos & (BUFFERSIZEFACTOR-1)) << 8;
	SetDiskPosition(pos);
	SectorPos = 0;
	
	Delay = BYTE_READ_TIME; // 32us 64
	NextCommand = WD1793_CmdStartWritingSector;
	CurrentCommand = WD1793_CmdDelay;
	CmdType = 2;	
	
	debug_printf("[799]WR SEC %i, TRK %i, SIDE %i - %ld\n", WD1793.SectorRegister, WD1793.TrackRegister, SIDE, (uint32_t) (pos<<8));

}

void WD1793_Cmd_ReadAddress(){// C0		Чтение адреса                	- %1100 0e00
	debug_printf("[804]Read Address\n");
	BufferPos = 0;
	CurrentCommand = WD1793_CmdReadingAddress; //->[854]
	CmdType = 3;
	StartTimePeriod();
}

void WD1793_Cmd_ReadTrack(){// E0		Чтение дорожки               	- %1110 0e00
	WD1793_Status=1;
	debug_printf("[813]RD TRACK %i, TRK %i, SIDE %i - %d\n", WD1793.SectorRegister, WD1793.TrackRegister, SIDE, ComputeDiskPosition(WD1793.SectorRegister - 1, WD1793.TrackRegister, SIDE) );

	CurrentCommand = WD1793_Cmd_StartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
	CmdType = 3;
	StartTimePeriod();
}

void WD1793_CmdStartWritingTrack(){ //Calling from WD1793_Cmd_WriteTrack() //F0 base command
	WD1793_Status=2;
	if(!Turbo && WD1793_GetIndexMark()){
		return;
	}
	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
	CurrentCommand = WD1793_CmdWritingTrack;
	StartTimePeriod();
}

void WD1793_Cmd_WriteTrack(){// F0		Запись дорожки/форматир-е   	- %1111 0e00
	WD1793_Status=2;
	debug_printf("[833]WR TRACK %i\n", WD1793.TrackRegister);
	BufferPos = 0;
	SectorPos = 0;
	FormatCounter = 0;
	CurrentCommand = WD1793_CmdStartWritingTrack;
	CmdType = 3;
}

void WD1793_CmdStartNewCommand(){
	debug_printf("[842]CmdStartNewCommand\n");
	uint8_t i = WD1793.CommandRegister >> 4;

	WD1793_FlushBuffer();

	if (DRV != NewDrive){ // Mount image only if drive is changed
		DRV = NewDrive;
		WD1793_Reset(DRV);
	}
	CommandTable[i]();
}

void WD1793_Execute(void){

	if(Prevwd1793_PortFF != wd1793_PortFF){
		Prevwd1793_PortFF = wd1793_PortFF;
/* 		
bit out(PortFF)
7 - x
6 - метод записи (1 - fm, 0 - mfm)
5 - x
4 - выбор магнитной головки (0 - верх, 1 - низ)
3 - загрузка головки (всегда должен быть в 1)
2 - аппаратный сброс микроконтроллера (если 0)
1 - номер дисковода (00 - a, 01 - b, 10 - c, 11 - d)
0 - -/-
 */
		NewDrive = GET_DRIVE();  // (wd1793_PortFF & 0b11)
		SIDE = GET_SIDE(); 		 // (~wd1793_PortFF & 0x000010000) >> 4
		debug_printf("[871]DRIVE %d > SIDE %d\n", NewDrive, SIDE);
		if(!GET_RES()){
			debug_printf("[873]Reset BDI\n");
			WD1793_Write(0, 0); // Restore command
		}
	}

	if(NewCommandReceived){
		debug_printf("[879]New Command Register: %02X\n",WD1793.CommandRegister);

		NewCommandReceived = 0; // try to avoid disabling interrupts
		if ((WD1793.CommandRegister & 0xF0) == 0xD0){ // Force Interrupt
			debug_printf("[883]Force Interrupt\n");	// In power up 2 demo can freeze 3d part at the end
			WD1793.StatusRegister &= ~_BV(stsBusy); // Cброс занятости
			CmdType = 1; // Exception
			CurrentCommand = WD1793.CommandRegister & 0x0F ? WD1793_CmdType1Status : WD1793_CmdStartIdleForceInt0;
		} else 
		if(!(WD1793.StatusRegister & _BV(stsBusy))){
			WD1793.StatusRegister = 0x01; // All bits clear but "Busy" set
			Requests = 0; // clear DRQ and INTRQ
			CurrentCommand = WD1793_CmdStartNewCommand;
		}
	}
   CurrentCommand();
}




/*
void WD1793_UpdateConfig(){
	uint16_t i, j, prev_space, start = BUFFERSIZE, end = 0;
	uint8_t d; // drive index in config
	
	if(SelectedImage == NULL){
		return;
	}
	
	WD1793_FlushBuffer();
	NoDisk = 1;
	CurrentDiskPos = DEFAULT_DISK_POS;
	debug_printf("[445]UpdateConfig] Mount FileSystem...\n");

/
	res = f_mount(&fss);
	if (res) 
	{
		//printError("Error mount: ", res);
		return;
	}
/

	res = sd_open_file(&sd_file,"0:/TRDOS/IMAGES.CFG",FA_READ);
	if (res){ // If can't open config looking for DISK<1-4>.TRD
		//printError("[455]Error open IMAGES.CFG: \n", res);
		return;
	}
	
	res = sd_read_file(&sd_file, sd_buffer, sizeof(sd_buffer), &br);
	if (res){
		//printError("[408]sd_read_file", res);
		return;
	}

	// parse config
	for(i=0, d=0, prev_space=BUFFERSIZE; i<br && d<4; i++){
		if(sd_buffer[i] != '\n' && sd_buffer[i] != '\r' && sd_buffer[i] != 0){
			if(d == SelectedDrive){
				if(start == BUFFERSIZE){
					start = i;
				}
				end = i;
			}
		} else {
			if(prev_space+1 != i){
				d++;
			}
			prev_space = i;
		}
		if(d > SelectedDrive){
			break;
		}
	}
	++end; // preserve space
	
	j = strlen(SelectedImage);
	if(j == 0){
		debug_printf("Empty image name\n");
		SelectedImage = NULL;
		return;
	}

    memmove(sd_buffer + start + j, sd_buffer + end, br - end);	//moving the rest of config to the position AFTER a new imagename to provide a gap
	memcpy(sd_buffer + start, SelectedImage, j);		//copying the new name into the gap prepared
	br = br + j - (end-start);			//calculating new block length


	debug_printf("[517]Write config\n");

	res = sd_write_file(&sd_file, sd_buffer, br, &br);
	if (res){
		//printError("[519]Error write config: ", res);
	}
	res = sd_write_file(&sd_file, 0, 0, &br);
	if (res){
		//printError("Error on finish write config: ", res);
	}	
	SelectedImage = NULL;

}


void WD1793_ReadDir(uint16_t pos){
	WD1793_Status=1;
    uint8_t i = pos ? 3 : 0; // file number 	
	uint16_t bufIndex = 0;
	uint16_t startFile = pos*16; 
	uint16_t endFile = startFile + 16;
	uint8_t len;
	const char ext = 'C';

	
	if(pos == 0){
		for (i=0; i<3; i++){ // Fake files as drives
			bufIndex = (i * 16) % SECTORLENGTH;
			memset(sd_buffer + bufIndex, ' ', 8);			
			sd_buffer[bufIndex] = 'A' + i;
			if(i == SelectedDrive){
				strncpy(sd_buffer + bufIndex + 1, " <--", 4);
			}
			bufIndex += 8;
			sd_buffer[bufIndex] = ext;			
			sd_buffer[++bufIndex] = 0; // start addr low byte
			sd_buffer[++bufIndex] = 0x40;  // start addr high byte
			sd_buffer[++bufIndex] = 1; // length
			sd_buffer[++bufIndex] = 0;
			sd_buffer[++bufIndex] = 1; // file length in sectors
			sd_buffer[++bufIndex] = 0; // Start sector
			sd_buffer[++bufIndex] = i + 1; // Start track, each track is a separate file
		}		
	}
	
	//memcpy(Path,"0:/TRD",3);
	//strcat(Path,"/");
	//uint8_t len = 
	debug_printf("[564]OpenDir '%s'\n",dir_path);
    res = sd_opendir(&dir, dir_path);
    if (res != FR_OK){
		printError("[565]Error open dir: ", res);
		FilesCount = 0;
	}
	
	while (i < 128){
		res = sd_readdir(&dir, &finfo);
		if (res != FR_OK || finfo.fname[0] == 0) break;
		if (!(finfo.fattrib & AM_DIR)){  // read files only
			//if(strstr(finfo.fname, ".TRD") != NULL){
				if(i >= startFile && i < endFile){
					bufIndex = (i * 16) % SECTORLENGTH;
					memset(sd_buffer + bufIndex, ' ', 8);
					len = strlen(finfo.fname);
					strncpy(sd_buffer + bufIndex, finfo.fname, MIN(len, 8));
					bufIndex += 8;
					sd_buffer[bufIndex] = ext;
					sd_buffer[++bufIndex] = 0; // start addr low byte
					sd_buffer[++bufIndex] = 0x40;  // start addr high byte
					sd_buffer[++bufIndex] = 1; // length
					sd_buffer[++bufIndex] = 0;
					sd_buffer[++bufIndex] = 1; // file length in sectors
					sd_buffer[++bufIndex] = 0; // Start sector
					sd_buffer[++bufIndex] = i + 1; // Start track, each track is a separate file		
				}
				i++;
			//}
		}
	}
	FilesCount = i;
}

uint8_t WD1793_GetFilesCount(){
	WD1793_Status=1;
	uint8_t i = 3; // file number

	if(FilesCount){
		return FilesCount;
	}
	//memcpy(Path,"0:/!TRD",3);
	debug_printf("[606]OpenDir '%s'\n",&dir_path[0]);
	res = sd_opendir(&dir, dir_path);
	if (res != FR_OK){
		printError("Error open dir: ", res);
		return 0;
	}
	while (i < 128){
		res = sd_readdir(&dir, &finfo);
		if (res != FR_OK || finfo.fname[0] == 0) break;
		if (!(finfo.fattrib & AM_DIR)){  // read files only
			if(strstr(finfo.fname, ".TRD") != NULL){
				i++;
			}
		}
	}
	return i;
}


void WD1793_ReadDiskInfo(){
	WD1793_Status=1;
	debug_printf("[652]Read disk info\n");

	uint8_t i = 225;
	uint16_t FreeSectorsCount = (160 - 4 - FilesCount) * 15;
	FilesCount = WD1793_GetFilesCount();
	sd_buffer[i++] = 0;
	sd_buffer[i++] = FilesCount + 1; // first free track
	sd_buffer[i++] = 0x1B;//0x16; // disk type 0x16
	sd_buffer[i++] = FilesCount; // files count
	sd_buffer[i++] = (uint8_t)FreeSectorsCount; // free sectors count
	sd_buffer[i++] = (uint8_t)(FreeSectorsCount >> 8); // free sectors count high byte
	sd_buffer[i++] = 16; // TR-DOS Id
	
	memset(sd_buffer + 234, 32, 8);
	memcpy(sd_buffer + 245, "SD Card", 7);
}

void WD1793_LookUpFile(uint8_t track){ // logical track 1-160
	uint8_t i = 3; // file number
	--track; // skip track 0
	debug_printf("[644]Read dir starting 0x%04X\n", track);
	if(track >= 0 && track < 3){
		SelectedDrive = track;
		debug_printf("[647]Selected drive %d:\n", SelectedDrive);
		SelectedImage = NULL;
		return;
	}
	
	//memcpy(Path,"0:/!TRD",3);
	debug_printf("OpenDir[655] '%s'\n",dir_path);
	res = sd_opendir(&dir, dir_path);
	if (res != FR_OK){
		printError("[656]Error open dir: ", res);
		FilesCount = 0;
	}
	
	while (i < 128){
		res = sd_readdir(&dir, &finfo);
		if (res != FR_OK || finfo.fname[0] == 0) break;
		if (!(finfo.fattrib & AM_DIR)) { // read files only
			if(strstr(finfo.fname, ".TRD") != NULL){
				if( i == track){
					debug_printf("[667]Selected file %s\n", finfo.fname);
					SelectedImage = finfo.fname;
					return;
				}
				i++;
			}
		}
	}
}
*/
