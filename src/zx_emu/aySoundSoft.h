#pragma once
#include "inttypes.h"
#include "stdbool.h" 

#define NO_SOUND 0
#define ONLY_BEEPER 1
#define BEEPER_AND_SOFT_AY 2
#define BEEPER_AND_SOFT_TS 3
#define HARDWARE_TS 4

extern uint8_t outs[6];

void AY_select_reg(uint8_t N_reg);
uint8_t AY_get_reg();
void AY_set_reg(uint8_t val);
void AY_next_ampls();
uint8_t AY_get_ampl();

//uint8_t* get_AY_Out(uint8_t g_chip,uint8_t delta);
void get_AY_Out(uint8_t g_chip,uint8_t delta);
void  AY_reset(uint8_t s_mode);
void AY_print_state_debug();
void AY_to595Beep(bool Beep);

void saa1099_write(uint8_t addr, uint8_t byte);