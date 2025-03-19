#include "aySoundSoft.h"
#include "stdbool.h"
#include "string.h"
#include "hardware/timer.h"
#include <pico.h>
#include <pico/stdlib.h>
// #include "AY_PIO_595.h"
#include "../util_cfg.h"
#include "PinSerialData_595.h"

/* send595() 0x4000 -  AY_Enable,Beep - 0x1000,,0x0800 выбор второго чипа АУ, 0x400 выбор первого чипа
				0x0200	BDIR pin AY, 0x0100 BC1 pin AY */
/*	SAA1099	*/	  
#define CS_SAA1099	(1<<15)
#define AY_Enable (1<<14)
#define SAVE (1<<13)
#define Beeper (1<<12)
#define CS_AY1 (1<<11)
#define CS_AY0	(1<<10)
#define BDIR (1<<9)
#define BC1 (1<<8)
static uint16_t control_bits = 0;
#define LOW(x) (control_bits &= ~(x))
#define HIGH(x) (control_bits |= (x))			 
/*	SAA1099	*/
#define MAX_AMPLS 5 

extern uint8_t cfg_tschip_order;

uint8_t N_sel_reg=0;

static uint8_t c_chip=0;
static uint8_t sound_mode=0;
static uint8_t ampls_mode=0;
uint8_t outs[6];

uint8_t ampls[MAX_AMPLS][16]={
	{	0,	2,	4,	5,	7,	8,	9,	11,	12,	14,	16,	19,	23,	28,	34,	40},//0//гибрид линейной и китайского чипа
	{	0,	1,	2,	3,	4,	5,	6,	8,	10,	12,	16,	19,	23,	28,	34,	40},//1//снятя с китайского чипа
	{	0,	1,	2,	2,	3,	3,	4,	6,	7,	9,	12,	15,	19,	25,	32,	41},//2//степенная зависимость	
	{	0,	3,	5,	8,	11,	13,	16,	19,	21,	24,	27,	29,	32,	35,	37,	40},//3//линейная
	{	0,	10,	15,	20,	23,	27,	29,	31,	32,	33,	35,	36,	38,	39,	40,	40},//4//выгнутая
};

typedef struct ay_regs_t {
	uint16_t ay_R1_R0;
	uint16_t ay_R3_R2;
	uint16_t ay_R5_R4;
	uint8_t ay_R6;
	uint8_t ay_R7;
	uint8_t ay_R8;
	uint8_t ay_R9;
	uint8_t ay_R10;
	uint16_t ay_R12_R11;
	uint32_t ay_R12_R11_sh;
	uint8_t ay_R13;
	uint8_t ay_R14;
	uint8_t ay_R15;
	bool is_envelope_begin;
	bool bools[4];
	uint32_t main_ay_count_env;
	uint32_t chA_count;
	uint32_t chB_count;
	uint32_t chC_count;
	uint32_t noise_ay_count;
	uint8_t ampl_ENV;
	bool is_env_inv_enum; //инверсия последовательности огибающей
	uint32_t envelope_ay_count;
} ay_regs_t;

static ay_regs_t chips[2];

void __not_in_flash_func(AY_to595Beep)(bool Beep){ 
	if (Beep) {send_to_595( HIGH(Beeper)) ;} else {send_to_595( LOW(Beeper));};
};

// SAA1099
void __not_in_flash_func(saa1099_write)(uint8_t addr, uint8_t byte) {
    // const uint16_t a0 = addr ? BC1 : 0;
	if(addr>0){HIGH(BC1);}else{LOW(BC1);}
    send_to_595(byte | LOW(CS_SAA1099)); //
    busy_wait_us(5);
    send_to_595(byte | HIGH(CS_SAA1099)); // 
}

void __not_in_flash_func(AY_select_reg)(uint8_t N_reg){
	N_sel_reg=N_reg;
	if(sound_mode>=3){
		if(cfg_tschip_order==0){
			if(N_reg==0xFE){c_chip=0;HIGH(CS_AY0);LOW(CS_AY1);}  
			if(N_reg==0xFF){c_chip=1;HIGH(CS_AY1);LOW(CS_AY0);}
		} else if(cfg_tschip_order==1){
			if(N_reg==0xFE){c_chip=1;HIGH(CS_AY1);LOW(CS_AY0);}  
			if(N_reg==0xFF){c_chip=0;HIGH(CS_AY0);LOW(CS_AY1);}
		}
		if (sound_mode == 4){ 
			send_to_595(HIGH (BDIR | BC1) | N_reg); 
			send_to_595( LOW (BDIR |BC1) | N_reg);
		}
		//stdio_printf("TS CS %d\n",c_chip);
		//AY_print_state_debug();
	} else {
		c_chip=0;
	}
};


void  __not_in_flash_func(AY_reset)(uint8_t s_mode){
	//stdio_printf("AY RST\n");
	sound_mode=s_mode;
	ay_regs_t chip;
	chip.ay_R1_R0=0;
	chip.ay_R3_R2=0;
	chip.ay_R5_R4=0;
	chip.ay_R6=0;
	chip.ay_R7=0xff;
	chip.ay_R8=0;
	chip.ay_R9=0;
	chip.ay_R10=0;
	chip.ay_R12_R11=0;
	chip.ay_R12_R11_sh=0;
	chip.ay_R13=0;
	chip.ay_R14=0xff;
	chip.ay_R15=0;
	chip.is_envelope_begin=false;
	chip.bools[0]=false;
	chip.bools[1]=false;
	chip.bools[2]=false;
	chip.bools[3]=false;
	chip.chA_count=0;
	chip.chB_count=0;
	chip.chC_count=0;
	chip.noise_ay_count=0;
	chip.main_ay_count_env=0;
	chip.ampl_ENV=0;
	chip.is_env_inv_enum=true;
	chip.envelope_ay_count=0;
	
	chips[0]=chip;
	chips[1]=chip;

	if (sound_mode == 4){
		send_to_595(LOW(AY_Enable));
		//  busy_wait_us(500);
		for (int i = 0; i < 0x20; i++){
			saa1099_write(1, i);
			saa1099_write(0, 0x00);
		}
		send_to_595(HIGH(AY_Enable | CS_SAA1099 | CS_AY0 | Beeper |CS_AY1 | BDIR | BC1 | (SAVE)));																						   
	}
};

void AY_print_state_debug(){
	stdio_printf("\n AY SEL %d\n",c_chip);
	ay_regs_t chip = chips[0];
	stdio_printf("\n AY CHIP 1\n");
	stdio_printf("R1_R0:[%04X]\n",chip.ay_R1_R0);
	stdio_printf("R3_R2:[%04X]\n",chip.ay_R3_R2);
	stdio_printf("R3_R2:[%04X]\n",chip.ay_R5_R4);
	stdio_printf("R6 :[%02X]\n",chip.ay_R6);
	stdio_printf("R7 :[%02X]\n",chip.ay_R7);
	stdio_printf("R8 :[%02X]\n",chip.ay_R8);
	stdio_printf("R9 :[%02X]\n",chip.ay_R9);
	stdio_printf("R10:[%02X]\n",chip.ay_R10);
	stdio_printf("R12_R11:[%04X]\n",chip.ay_R12_R11);
	stdio_printf("R13:[%02X]\n",chip.ay_R13);
	stdio_printf("R14:[%02X]\n",chip.ay_R14);
	stdio_printf("R15:[%02X]\n",chip.ay_R15);
	chip = chips[1];
	stdio_printf("\n AY CHIP 2\n");
	stdio_printf("R1_R0:[%04X]\n",chip.ay_R1_R0);
	stdio_printf("R3_R2:[%04X]\n",chip.ay_R3_R2);
	stdio_printf("R3_R2:[%04X]\n",chip.ay_R5_R4);
	stdio_printf("R6 :[%02X]\n",chip.ay_R6);
	stdio_printf("R7 :[%02X]\n",chip.ay_R7);
	stdio_printf("R8 :[%02X]\n",chip.ay_R8);
	stdio_printf("R9 :[%02X]\n",chip.ay_R9);
	stdio_printf("R10:[%02X]\n",chip.ay_R10);
	stdio_printf("R12_R11:[%04X]\n",chip.ay_R12_R11);
	stdio_printf("R13:[%02X]\n",chip.ay_R13);
	stdio_printf("R14:[%02X]\n",chip.ay_R14);
	stdio_printf("R15:[%02X]\n",chip.ay_R15);
	
};

uint8_t __not_in_flash_func(AY_get_reg)(){
	ay_regs_t chip = chips[c_chip];
	switch (N_sel_reg){
		case 0: return (chip.ay_R1_R0)&0xff;
		case 1: return (chip.ay_R1_R0>>8)&0xff;
		case 2: return (chip.ay_R3_R2)&0xff;
		case 3: return (chip.ay_R3_R2>>8)&0xff;
		case 4: return (chip.ay_R5_R4)&0xff;
		case 5: return (chip.ay_R5_R4>>8)&0xff;
		case 6: return chip.ay_R6;
		case 7: return chip.ay_R7;
		case 8: return chip.ay_R8;
		case 9: return chip.ay_R9;
		case 10: return chip.ay_R10;
		case 11: return (chip.ay_R12_R11)&0xff;
		case 12: return (chip.ay_R12_R11>>8)&0xff;
		case 13: return chip.ay_R13;
		case 14: return chip.ay_R14;
		case 15: return chip.ay_R15;
		default: return 0;
	}
	
};

void __not_in_flash_func(AY_set_reg)(uint8_t val){
	ay_regs_t chip = chips[c_chip];
	switch (N_sel_reg){
		case 0:
		chip.ay_R1_R0=(chip.ay_R1_R0&0xff00)|val;
		break;
		case 1:
		chip.ay_R1_R0=(chip.ay_R1_R0&0xff)|((val&0xf)<<8);
		break;
		case 2:
		chip.ay_R3_R2=(chip.ay_R3_R2&0xff00)|val;
		break;
		case 3:
		chip.ay_R3_R2=(chip.ay_R3_R2&0xff)|((val&0xf)<<8);
		break;
		case 4:
		chip.ay_R5_R4=(chip.ay_R5_R4&0xff00)|val;
		break;
		case 5:
		chip.ay_R5_R4=(chip.ay_R5_R4&0xff)|((val&0xf)<<8);
		break;
		case 6:
		chip.ay_R6=val&0x1f;
		break;
		case 7:
		chip.ay_R7=val;
		break;
		case 8:
		chip.ay_R8=val&0x1f;
		break;
		case 9:
		chip.ay_R9=val&0x1f;
		break;
		case 10:
		chip.ay_R10=val&0x1f;
		break;
		case 11:
		chip.ay_R12_R11=(chip.ay_R12_R11&0xff00)|val;
		chip.ay_R12_R11_sh=chip.ay_R12_R11<<1;
		break;
		case 12:
		chip.ay_R12_R11=(chip.ay_R12_R11&0xff)|(val<<8);
		chip.ay_R12_R11_sh=chip.ay_R12_R11<<1;
		break;
		case 13:
		chip.ay_R13=val&0xf;
		chip.is_envelope_begin=true;
		break;
		case 14:
		chip.ay_R14=val;
		break;
		case 15:
		chip.ay_R15=val;
		break;
		default:
		break;
	}
	chips[c_chip]=chip;
	//AY_print_state_debug();

	if (sound_mode == 4){
		send_to_595(LOW (BDIR) | val);
		send_to_595(HIGH(BDIR) | val);
		send_to_595(LOW (BDIR) | val);
	}

};

//------------------------
bool __not_in_flash_func(get_random)(){
	static uint32_t S = 0x00000001;
	if (S & 0x00000001) {
		S = ((S ^ 0xea000001) >> 1) | 0x80000000;
		return true;
	} else {
		S >>= 1;
		return false;
	};
}

void AY_next_ampls(){
	ampls_mode++;
	if(ampls_mode>=MAX_AMPLS){
		ampls_mode=0;
	}
	AY_reset(sound_mode);
}

uint8_t AY_get_ampl(){
	return ampls_mode;
}

uint8_t* ampls0=ampls[0];

void __not_in_flash_func(get_AY_Out)(uint8_t g_chip,uint8_t delta){

	uint8_t* ampls0=ampls[ampls_mode];
	
	ay_regs_t cchip = chips[g_chip];
	//AY_print_state_debug();
	
	/*
		cchip.main_ay_count_env=0;
		//отдельные счётчики
		cchip.chA_count=0;
		cchip.chB_count=0;
		cchip.chC_count=0;
		cchip.noise_ay_count=0;
	*/
	
	/*
		N регистра	Назначение или содержание	Значение	
		0, 2, 4 	Нижние 8 бит частоты голосов А, В, С 	0 - 255
		1, 3, 5 	Верхние 4 бита частоты голосов А, В, С 	0 - 15
		6 			Управление частотой генератора шума 	0 - 31
		7 			Управление смесителем и вводом/выводом 	0 - 255
		8, 9, 10 	Управление амплитудой каналов А, В, С 	0 - 15
		11 			Нижние 8 бит управления периодом пакета 	0 - 255
		12 			Верхние 8 бит управления периодом пакета 	0 - 255
		13 			Выбор формы волнового пакета 	0 - 15
		14, 15 		Регистры портов ввода/вывода 	0 - 255
		
		R7
		
		7 	  6 	  5 	  4 	  3 	  2 	  1 	  0
		порт В	порт А	шум С	шум В	шум А	тон С	тон В	тон А
		управление
		вводом/выводом	выбор канала для шума	выбор канала для тона
	*/
	
	//копирование прошлого значения каналов для более быстрой работы
	
	register bool chA_bitOut=(cchip.bools[0]);
	register bool chB_bitOut=(cchip.bools[1]);
	register bool chC_bitOut=(cchip.bools[2]);
	
	//#define nR7 (~chips[g_chip].ay_R7)
	
	uint8_t nR7=(~cchip.ay_R7);
	
	/*
		Если установлен "ТОН" в канале X, то 
		{
		увеличиваем chX_count на 5(delta);
		Если ( (chX_count >= значений в регистрах R1_R0(делитель частоты)) И (R1_R0(делитель частоты >=5(delta))
		{
		
		}
		}
	*/
	
	//nR7 - инвертированый R7 для прямой логики - 1 Вкл, 0 - Выкл
	
	if (nR7&0x1) {
		cchip.chA_count+=delta;
		if ((cchip.chA_count>=cchip.ay_R1_R0) && (cchip.ay_R1_R0>=delta)){
			(cchip.bools[0])^=1;
			cchip.chA_count=cchip.chA_count-cchip.ay_R1_R0;
		}
	} else {
		chA_bitOut=1;
		cchip.chA_count=0;
	}; /*Тон A*/
	if (nR7&0x2) {
		cchip.chB_count+=delta;
		if ((cchip.chB_count>=cchip.ay_R3_R2) && (cchip.ay_R3_R2>=delta) ) {
			(cchip.bools[1])^=1;
			cchip.chB_count=cchip.chB_count-cchip.ay_R3_R2;
		}
	} else {
		chB_bitOut=1;
		cchip.chB_count=0;
	}; /*Тон B*/
	if (nR7&0x4) {
		cchip.chC_count+=delta;
		if (cchip.chC_count>=cchip.ay_R5_R4 && (cchip.ay_R5_R4>=delta) ) {
			(cchip.bools[2])^=1;
			cchip.chC_count=cchip.chC_count-cchip.ay_R5_R4;
		}
	} else {
		chC_bitOut=1;
		cchip.chC_count=0;
	}; /*Тон C*/
	
	//проверка запрещения тона в каналах
	if (cchip.ay_R7&0x1) chA_bitOut=1; 
	if (cchip.ay_R7&0x2) chB_bitOut=1;
	if (cchip.ay_R7&0x4) chC_bitOut=1;
	
	//добавление шума, если разрешён шумовой канал
	if (nR7&0x38){//есть шум хоть в одном канале
		cchip.noise_ay_count+=delta;
		if (cchip.noise_ay_count>=(cchip.ay_R6<<1)) {(cchip.bools[3])=get_random();cchip.noise_ay_count=0;}//отдельный счётчик для шумового // R6 - частота шума
		if(!(cchip.bools[3])){//если бит шума ==1, то он не меняет состояние каналов
			if ((chA_bitOut)&&(nR7&0x08)) chA_bitOut=0;//шум в канале A
			if ((chB_bitOut)&&(nR7&0x10)) chB_bitOut=0;//шум в канале B
			if ((chC_bitOut)&&(nR7&0x20)) chC_bitOut=0;//шум в канале C
		};
	}
	
	//вычисление амплитуды огибающей
	if ((cchip.ay_R8&0x10)|(cchip.ay_R9&0x10)|(cchip.ay_R10&0x10)){
		
		cchip.main_ay_count_env+=delta;
		
		#define env_count_16 (cchip.envelope_ay_count&0x0f)
		
		if (cchip.is_envelope_begin) {cchip.envelope_ay_count=0;cchip.main_ay_count_env=0;cchip.is_envelope_begin=false;cchip.is_env_inv_enum=true;};
		
		if (((cchip.main_ay_count_env)>=(cchip.ay_R12_R11<<1))){//без операции деления
			
			cchip.envelope_ay_count++;
			cchip.main_ay_count_env-=cchip.ay_R12_R11<<1;
			if (env_count_16==0) cchip.is_env_inv_enum=!cchip.is_env_inv_enum;
			switch (cchip.ay_R13){
				case (0b0000):
				case (0b0001):
				case (0b0010):
				case (0b0011):
				case (0b1001):
				if (cchip.envelope_ay_count<16) cchip.ampl_ENV=ampls0[15-env_count_16]; else {cchip.ampl_ENV=ampls0[0];};
				break;
				case (0b0100):
				case (0b0101):
				case (0b0110):
				case (0b0111):
				case (0b1111):
				if (cchip.envelope_ay_count<16) cchip.ampl_ENV=ampls0[env_count_16]; else {cchip.ampl_ENV=ampls0[0];}
				break;
				case (0b1000):
				cchip.ampl_ENV=ampls0[15-env_count_16]; 
				break;
				case (0b1100):
				cchip.ampl_ENV=ampls0[env_count_16]; 
				break;
				case (0b1010):
				if (cchip.is_env_inv_enum) cchip.ampl_ENV=ampls0[15-env_count_16]; else cchip.ampl_ENV=ampls0[env_count_16];
				break;
				case (0b1110):
				if (!cchip.is_env_inv_enum) cchip.ampl_ENV=ampls0[15-env_count_16]; else cchip.ampl_ENV=ampls0[env_count_16];
				break;
				case (0b1011):
				if (cchip.envelope_ay_count<16) cchip.ampl_ENV=ampls0[15-env_count_16]; else {cchip.ampl_ENV=ampls0[15];}
				break;
				case (0b1101):
				if (cchip.envelope_ay_count<16) cchip.ampl_ENV=ampls0[env_count_16]; else {cchip.ampl_ENV=ampls0[15];}
				break;
				default:
				break;
			}
		}
	}
	if(g_chip==0){
		outs[0]=chA_bitOut?((cchip.ay_R8&0xf0)?cchip.ampl_ENV:ampls0[cchip.ay_R8]):0; //outA
		outs[1]=chB_bitOut?((cchip.ay_R9&0xf0)?cchip.ampl_ENV:ampls0[cchip.ay_R9]):0; //outB
		outs[2]=chC_bitOut?((cchip.ay_R10&0xf0)?cchip.ampl_ENV:ampls0[cchip.ay_R10]):0; //outC
	}
	if(g_chip==1){
		outs[3]=chA_bitOut?((cchip.ay_R8&0xf0)?cchip.ampl_ENV:ampls0[cchip.ay_R8]):0; //outA
		outs[4]=chB_bitOut?((cchip.ay_R9&0xf0)?cchip.ampl_ENV:ampls0[cchip.ay_R9]):0; //outB
		outs[5]=chC_bitOut?((cchip.ay_R10&0xf0)?cchip.ampl_ENV:ampls0[cchip.ay_R10]):0; //outC
	}
	
	
	//chips[g_chip]=cchip;
	
	
	chips[g_chip].chA_count=cchip.chA_count;
	chips[g_chip].chB_count=cchip.chB_count;
	chips[g_chip].chC_count=cchip.chC_count;
	chips[g_chip].noise_ay_count=cchip.noise_ay_count;
	chips[g_chip].main_ay_count_env=cchip.main_ay_count_env;
	chips[g_chip].is_envelope_begin=cchip.is_envelope_begin;
	chips[g_chip].envelope_ay_count=cchip.envelope_ay_count;
	chips[g_chip].is_env_inv_enum=cchip.is_env_inv_enum;
	chips[g_chip].bools[0]=cchip.bools[0];
	chips[g_chip].bools[1]=cchip.bools[1];
	chips[g_chip].bools[2]=cchip.bools[2];
	chips[g_chip].bools[3]=cchip.bools[3];
	chips[g_chip].ampl_ENV=cchip.ampl_ENV;
	
	//return outs;	
};