#include "globals.h"
#include <stdalign.h> //Выравнивание массивов в памяти

char temp_msg[60]; // Буфер для вывода строк
uint8_t save_slots[11];
bool show_slots = false;

uint8_t now_joy1_mode=0;
uint8_t now_joy2_mode=0;
uint8_t now_kbd_mode=0;

bool kbd_lock=false;

bool zx_screen_refresh;
//bool int_en=true;

#ifdef NUM_V_BUF
    uint8_t graph_buf[V_BUF_SZ*NUM_V_BUF];
    bool is_show_frame[NUM_V_BUF];
    int draw_vbuf_inx=0;
    int show_vbuf_inx=0;

#else
    alignas(1024) uint8_t graph_buf[V_BUF_SZ+128];
    uint8_t hud_line[SCREEN_W+10];
#endif


#ifndef DEBUG_DISABLE_LOADERS
    uint8_t __scratch_x("temp_data_x") temp_buffer_x[TEMP_BUFF_SIZE];
    uint8_t __scratch_y("temp_data_y") temp_buffer_y[TEMP_BUFF_SIZE];
#endif


int	null_printf(const char *str, ...){return 0;};

uint32_t my_millis(){
	return us_to_ms(time_us_32());
}


