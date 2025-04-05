#pragma once
#include "inttypes.h"
#include "../globals.h"


//количество бит на пиксел 4 или 8
#define ZX_BPP (4)
//количество графических буферов 1-я, 2-я, 3-я буферизация(2-я уменьшает FPS)
#define ZX_NUM_GBUF (1)

//размеры экрана для отрисовки
#define ZX_SCREENW (320)
#define ZX_SCREENH (240)

typedef struct ZX_Input_t{
    uint8_t kb_data[8];
    uint8_t kempston;
    uint8_t kempston_mouse_x;
    uint8_t kempston_mouse_y;
    uint8_t kempston_mouse_btn;
    uint8_t kempston_mouse_whl;
} ZX_Input_t;

extern uint8_t* zx_cpu_ram[4];//Адреса 4х областей памяти CPU при использовании страниц
extern uint8_t* zx_ram_bank[8];//Хранит адреса 8ми банков памяти
extern uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))
extern uint8_t RAM[ZX_RAM_PAGE_SIZE*ZX_RAM_PAGES];

extern bool stateFlash;
////цвета спектрума в формате 6 бит
extern uint8_t zx_color[];
extern bool zx_screen_refresh;
//extern bool int_en;

extern bool ack_input;
extern ZX_Input_t* zx_read_buffer;
extern ZX_Input_t* zx_write_buffer;


//функции, которые надо определить аппаратно
bool hw_zx_get_bit_LOAD();
void hw_zx_set_beep_out(uint8_t val);

//void hw_zx_set_snd_out(bool val);
//void hw_zx_set_save_out(bool val);

//работа со звуком - функции реального времени
void Soundrive(uint8_t port,uint8_t val);

//буфер для отрисовки
uint8_t* zx_machine_screen_get(uint8_t* current_screen);

//ввод сперктрума
void zx_machine_input_set();

// функции управления zx машиной
void zx_machine_reset(bool trdos);
void zx_machine_NMI();
void zx_machine_init();
void zx_machine_main_loop_start();//функция содержит бесконечный цикл
void zx_machine_flashATTR(void);

//void zx_machine_set_vbuf(uint8_t* vbuf);
void zx_machine_enable_vbuf(bool en_vbuf);
bool zx_machine_get_vbuf_en();
void zx_machine_slow_FR(bool fr_slow);

void zx_machine_set_7ffd_out(uint8_t val);
uint8_t zx_machine_get_7ffd_lastOut();

void zx_machine_set_pc(uint16_t pc);

//uint8_t zx_machine_tape_active();
//void zx_machine_set_tape_pos(uint16_t pos);





