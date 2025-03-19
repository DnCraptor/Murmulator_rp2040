

#ifndef _WD1793_H_
#define _WD1793_H_

#include <stdint.h>
#include "util_sd.h"


extern uint8_t sd_buffer[SD_BUFFER_SIZE];
extern char temp_msg[60];


// Status Bits
#define stsBusy            0
#define stsDRQ             1
#define stsIndexImpuls     1
#define stsTrack0          2
#define stsLostData        2 
#define stsCRCError        3
#define stsSeekError       4
#define stsRecordNotFound  4
#define stsLoadHead        5
#define stsRecordType      5
#define stsWriteError      5
#define stsWriteProtect    6
#define stsNotReady        7


#define rqDRQ   6
#define rqINTRQ 7

#define _BV(bit) (1 << (bit))

#define TRD_BLK_SIZE 128

typedef struct TRDFNames{ //sector 0-7 (256)
	char     Name[8]; //0-8
	char     Ext[3];  //9
	//uint16_t StartAddr; //10-11
	uint16_t FileLength;//12-13 
	uint8_t  SecCount;  //14
	uint8_t  StartSec;  //15
	uint8_t  StartTrk;  //16
} __attribute__((packed)) TRDFNames;


/*
typedef struct TRDVNames{ //sector 0-7 (256)
	char     Name[9]; //0-8
	char     Ext[4];  //9
	uint16_t StartAddr; //10-11
	uint16_t FileLength;//12-13 
	uint8_t  SecCount;  //14
	uint8_t  StartSec;  //15
	uint8_t  StartTrk;  //16
} __attribute__((packed)) TRDVNames;
*/

//TRDVNames trd_files[TRD_BLK_SIZE];

typedef struct{
	union {
		uint8_t Regs[5];
		struct {
			uint8_t StatusRegister;
			uint8_t TrackRegister; // 0x3F 	- in/out in - текущий физический трек       / out - задание нового физического трека дорожки 
			uint8_t SectorRegister;// 0x5F  - in/out in - выдает номер текущего сектора / out - задание нового сектора (нумерация с 1!).
			uint8_t DataRegister;  // 0x7F	- in/out in - чтение байта / out -  задание номера логического трека, для команды позиционирования или запись байта если была какая-либо команда на запись.
										
			uint8_t CommandRegister; // 0x1F in - выдает состояние контроллера, где A: / out - выполнение команды, где A - код команды (см. раздел команды).
		};
	};
	
	uint8_t RealSector;       // 
	uint8_t RealTrack;        // ������� �������, �� ������� ��������� �������
	uint8_t Direction;        // 0 = � ������, 1 = �� ������
	uint8_t Side;
	uint8_t Multiple;
}  WD1793_struct;


extern bool TRDOS_mode; 		// Информационный сигнал Текущий режим - ROM TRDOS или стандартный ROM 48k
extern bool TRDOS_disabled; 	// Управляющий сигнал Запрет входить в TRDOS
extern uint8_t WD1793_Status;
extern uint8_t Requests;
extern uint8_t wd1793_PortFF;
extern WD1793_struct WD1793;
extern uint8_t NewCommandReceived;



void WD1793_Reset(uint8_t drive);
void WD1793_Execute();




//extern uint8_t Requests;


inline uint8_t WD1793_GetRequests(){ // 7th bit - INTRQ, 6th - DRQ
	return Requests;
}


#define GET_DRIVE() (wd1793_PortFF & 0b11)
#define SIDE_PIN 4
#define GET_SIDE() ((~wd1793_PortFF & _BV(SIDE_PIN))>>SIDE_PIN)
#define RES_PIN 2
#define GET_RES()  (wd1793_PortFF & _BV(RES_PIN))

void WD1793_Cmd_Restore();
void WD1793_Cmd_Seek();
void WD1793_Cmd_Step();
void WD1793_Cmd_ReadSector();
void WD1793_Cmd_WriteSector();
void WD1793_Cmd_ReadAddress();
void WD1793_Cmd_ReadTrack();
void WD1793_Cmd_WriteTrack();
void WD1793_CmdStartReadingSector();



uint8_t WD1793_Read(uint8_t Address);
void WD1793_Write(uint8_t Address, uint8_t Value);
void WD1793_timer(uint32_t dtcpu);

//bool load_image_TRDOS(char *file_name);
bool load_image_TRDOS(char *file_name,uint8_t drid);
bool free_image_TRDOS(uint8_t drid);
bool is_image_TRDOS_empty(uint8_t drid);
bool LoadScreenFromTRD(char *file_name);
uint8_t GetWD1793_Status();
uint8_t GetWD1793_Drive();
uint8_t GetWD1793_Drive_Load(uint8_t drive);
void WD1793_Reset(uint8_t drive);
void WD1793_Write(uint8_t Address, uint8_t Value);
void WD1793_Cmd_StartIdle();

#define  REVOLUTION_TIME 6U //6U

#endif // _WD1793_H_
