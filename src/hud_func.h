#pragma once

#include "inttypes.h"
#include "stdbool.h"
#include <string.h>
#include "icons.h"
#include "globals.h"
#include "util_cfg.h"
#include "kbd_img.h"

#define  HM_OFF 		 0x0000
#define  HM_ON 		 	 0x0001
#define  HM_TIME	 	 0x0002
#define  HM_TAPE	 	 0x0004

#define  HM_MAIN_HUD	 0x0008
#define  HM_TAPE_HUD  	 0x0010
#define  HM_KBD_HUD  	 0x0020
#define  HM_SHOW_VOLUME  0x0040
#define  HM_SHOW_BRIGHT  0x0080
#define  HM_SHOW_BATTERY 0x0800
#define  HM_SHOW_KEYLOCK 0x8000

#define HUD_TEXT_LINE_LEN 51

#define HUD_PIXEL_POS (30)

#define KBD_MAX_COL (9)
#define KBD_MAX_ROW (4)
#define KBD_SCREEN_PERIOD 50 //ms

extern uint8_t tape_disp;
extern uint8_t old_tape_disp;
extern uint32_t hud_timer;
extern void* hud_ptr;
extern uint16_t old_hud_mode;
extern uint16_t current_hud_mode;
extern char hud_text[104];

extern uint8_t battery_status;
extern uint8_t battery_status_ico;

extern uint8_t drives_status[4];
extern short int kbd_col;
extern short int kbd_row;
extern bool kbd_draw;
extern bool kbd_cshift;
extern bool kbd_sshift;
extern bool kbd_emode;
extern short int kbd_first_line;
extern uint8_t vol_strength;
extern uint8_t vol_bank;

bool hud_battery(short int line);
bool hud_prepare(short int line);
bool hud_prepare_scale(short int line);
bool hud_prepare_tape(short int line);
bool hud_kb_lock(short int line);
bool hud_prepare_kbd(short int line);