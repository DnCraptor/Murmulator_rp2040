#pragma once
#include "inttypes.h"
#include "stdbool.h"
#include "screen_util.h"

#define POKE_TEXT_LEN ((PREVIEW_WIDTH/FONT_5x7_W)-4)

#define BTN_POS_MAX (4)

extern const char *iface_btn_bottom[2][2];

typedef struct poke_line{
	bool checked;
	char text[POKE_TEXT_LEN];
}POKE_LINE;


typedef struct poke_data{
	POKE_LINE* id;
	uint8_t page;
	uint16_t addr;
	uint16_t new_val;
	uint8_t old_val;
}POKE_DATA;

extern POKE_LINE* pokes_buff;

short int load_pok_captions(char *file_name);
void draw_pokes_list(POKE_LINE* pokes, short int poke_count,short int startLine, short int selected, bool check);
void draw_pokes_bottom_btn(uint8_t xPos,uint8_t yPos,uint8_t dia_pos);
void set_pok_values(POKE_LINE* pokes,short int poke_count,char *file_name);
void draw_poke_menu(uint8_t xPos,uint8_t yPos,bool drawbg,char* text_src,uint8_t lines,uint8_t active);