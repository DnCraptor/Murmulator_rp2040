#include "hud_func.h"


uint8_t tape_disp=0;
uint8_t old_tape_disp=0;

uint32_t hud_timer;
void* hud_ptr;
uint16_t old_hud_mode=0;
uint16_t current_hud_mode=0;
char hud_text[104];


uint8_t battery_status=0;
uint8_t battery_status_ico=0;

uint8_t drives_status[4]={0,0,0,0};

//uint32_t kbd_scr_update = 0;
short int kbd_col=0;
short int kbd_row=0;
bool kbd_draw=false;
bool kbd_cshift=false;
bool kbd_sshift=false;
bool kbd_emode=false;
short int kbd_first_line=0;

uint8_t vol_strength	= 0;
uint8_t vol_bank 		= 0;

/*
#define CL_BLACK	 0x00
#define CL_BLUE	  0x01
#define CL_RED	   0x02
#define CL_PINK	  0x03
#define CL_GREEN	 0x04
#define CL_CYAN	  0x05
#define CL_YELLOW	0x06
#define CL_GRAY	  0x07
#define CL_LT_BLACK  0x08
#define CL_LT_BLUE   0x09
#define CL_LT_RED	0x0a
#define CL_LT_PINK   0x0b
#define CL_LT_GREEN  0x0c
#define CL_LT_CYAN   0x0d
#define CL_LT_YELLOW 0x0e
#define CL_WHITE	 0x0f
*/ 

/*-------Graphics--------*/
/*typedef struct HUD_struct {
	bool active;
	bool changed;
	int16_t  first_line;
	int16_t  last_line;
	int16_t  left_pos;
	int16_t	 img_width;
	uint8_t* img_ptr;
	int16_t	 ptr_inc;
	int16_t	 ptr_addr;
} __attribute__((packed)) HUD_data;

HUD_data hud[10];
*/

short int img_up=0;
short int img_down=0;
short int img_left=0;
short int img_right=0;
short int img_first_line=0;
short int img_last_line=0;

/*
#pragma GCC push_options
#pragma GCC optimize("-Ofast")
bool FAST_FUNC(hud_prepare)(int line){
	if(!show_hud) return false;
	if(line<1){
		return false;
	}
	bool linedraw=false;
	memset(hud_line,0x11,SCREEN_W);	//fill buffer EMPTY color

	for(uint8_t idx=0;idx<10;idx++){
		if(hud[idx].active){
			if((line=hud[idx].first_line)&&(line<hud[idx].last_line)){
				if(hud[idx].img_ptr!=NULL){
					memcpy(&hud_line[hud[idx].left_pos],&hud[idx].img_ptr[hud[idx].ptr_addr],hud[idx].img_width);
					hud[idx].ptr_addr+=hud[idx].ptr_inc;
					linedraw=true;
				}
			}
		}
		if(line==hud[idx].last_line){
			hud[idx].ptr_addr=0;
		}
	}
	return linedraw;
}
#pragma GCC pop_options
*/

/*

*/


#pragma GCC push_options
#if COMPOSITE_TV||SOFT_COMPOSITE_TV
#pragma GCC optimize("-Ofast")
#else
#pragma GCC optimize("-Os")
#endif
bool FAST_FUNC(hud_battery)(short int line){
	if(line==(ICON_BAT_TOP-1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_BAT_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}
	//memset(hud_line,0x10,SCREEN_W);	//fill buffer EMPTY color
	uint16_t addr=0;


	if((line>=ICON_BAT_TOP)&&(line<=ICON_BAT_BOT)){
		if((battery_status&0xF0)>0){
			img_up			= (ico_yfpos[ico_battery_map[10+(battery_status>>4)]]);
			img_down		= (ico_ytpos[ico_battery_map[10+(battery_status>>4)]]);
			img_left		= (ico_xfpos[ico_battery_map[10+(battery_status>>4)]]);
			img_right		= (ico_xtpos[ico_battery_map[10+(battery_status>>4)]]);
			img_last_line	= (ICON_BAT_TOP+(img_down-img_up)+1);
			if(line<img_last_line){
				addr=(((line-ICON_BAT_TOP)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[ICON_CHG_LEFT],&icons[addr+img_left],(img_right-img_left));
			}
		};
		img_up			= (ico_yfpos[ico_battery_map[battery_status&0x0f]]);
		img_down		= (ico_ytpos[ico_battery_map[battery_status&0x0f]]);
		img_left		= (ico_xfpos[ico_battery_map[battery_status&0x0f]]);
		img_right		= (ico_xtpos[ico_battery_map[battery_status&0x0f]]);
		img_last_line	= (ICON_BAT_TOP+(img_down-img_up)+1);
		if(line<img_last_line){
			addr=(((line-ICON_BAT_TOP)+img_up)*ICONS_WIDTH);
			memcpy(&hud_line[ICON_BAT_LEFT],&icons[addr+img_left],(img_right-img_left));
			return true;
		}
	}
	return false;
}
#pragma GCC pop_options

#pragma GCC push_options
#if COMPOSITE_TV||SOFT_COMPOSITE_TV
#pragma GCC optimize("-Ofast")
#else
#pragma GCC optimize("-Os")
#endif
bool FAST_FUNC(hud_prepare)(short int line){
	//if(!show_hud) return false;
	if(line==(ICON_BAT_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_DISK_TOP-1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_DISK_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_TAPE_BOT+1)) {memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_JOY_BOT+1)) {memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_SLOT_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}

	//memset(hud_line,0x10,SCREEN_W);	//fill buffer EMPTY color
	uint16_t addr=0;
	if(current_hud_mode&HM_SHOW_BATTERY){
		if((line>=ICON_BAT_TOP)&&(line<=ICON_BAT_BOT)){
			if((battery_status&0xF0)>0){
				img_up			= (ico_yfpos[ico_battery_map[10+(battery_status>>4)]]);
				img_down		= (ico_ytpos[ico_battery_map[10+(battery_status>>4)]]);
				img_left		= (ico_xfpos[ico_battery_map[10+(battery_status>>4)]]);
				img_right		= (ico_xtpos[ico_battery_map[10+(battery_status>>4)]]);
				img_last_line	= (ICON_BAT_TOP+(img_down-img_up)+1);
				if(line<img_last_line){
					addr=(((line-ICON_BAT_TOP)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_CHG_LEFT],&icons[addr+img_left],(img_right-img_left));
				}
			};
			img_up			= (ico_yfpos[ico_battery_map[battery_status&0x0f]]);
			img_down		= (ico_ytpos[ico_battery_map[battery_status&0x0f]]);
			img_left		= (ico_xfpos[ico_battery_map[battery_status&0x0f]]);
			img_right		= (ico_xtpos[ico_battery_map[battery_status&0x0f]]);
			img_last_line	= (ICON_BAT_TOP+(img_down-img_up)+1);
			if(line<img_last_line){
				addr=(((line-ICON_BAT_TOP)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[ICON_BAT_LEFT],&icons[addr+img_left],(img_right-img_left));
				return true;
			}
		}
	}

	#ifdef TRDOS_COMPILE
	if((line>=ICON_DISK_TOP)&&(line<=ICON_DISK_BOT)){
		for (uint8_t dr=0;dr<4;dr++){
			img_up			= (ico_yfpos[drives_status[dr]]);
			img_down		= (ico_ytpos[drives_status[dr]]);
			img_left		= (ico_xfpos[drives_status[dr]]);
			img_right		= (ico_xtpos[drives_status[dr]]);
			img_first_line	= (icon_disk_top[dr]);
			img_last_line	= (icon_disk_top[dr]+(img_down-img_up)+1);
			if((line>=img_first_line)&&(line<img_last_line)){
				addr=(((line-img_first_line)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[ICON_COLUMN_LEFT],&icons[addr+img_left],(img_right-img_left));
				return true;
			}
		}
	}
	#endif

	if(line>=ICON_TAPE_TOP){
		img_up			= (ico_yfpos[icon_tape_map[tape_disp]]);
		img_down		= (ico_ytpos[icon_tape_map[tape_disp]]);
		img_left		= (ico_xfpos[icon_tape_map[tape_disp]]);
		img_right		= (ico_xtpos[icon_tape_map[tape_disp]]);
		img_last_line	= (ICON_TAPE_TOP+(img_down-img_up)+1);
		if(line<img_last_line){
			addr=(((line-ICON_TAPE_TOP)+img_up)*ICONS_WIDTH);
			memcpy(&hud_line[ICON_COLUMN_LEFT],&icons[addr+img_left],(img_right-img_left));
			//return true;
		}
	}
	if(tape_disp>0){
		if((line>=ICON_TAPE_TEXT_TOP)&&(line<(ICON_TAPE_TEXT_TOP+FONT_5x7_H))){
			draw_bufline_text5x7_len(&hud_line[32],(line-ICON_TAPE_TEXT_TOP),hud_text,COLOR_HUD,CL_EMPTY,HUD_TEXT_LINE_LEN);
			return true;
		}
		if((cfg_tap_load_mode==FAST_LOAD_TAP)&&(line>=(ICON_TAPE_TEXT_TOP+FONT_5x7_H+2))){
			memset(hud_line,0x10,SCREEN_W);
			return false;
		}
		if((line>=(ICON_TAPE_TEXT_TOP+FONT_5x7_H+2)&&(line<(ICON_TAPE_TEXT_TOP+(FONT_5x7_H*2)+2)))){
			draw_bufline_text5x7_len(&hud_line[32],(line-(ICON_TAPE_TEXT_TOP+FONT_5x7_H+2)),&hud_text[HUD_TEXT_LINE_LEN],COLOR_HUD,CL_EMPTY,HUD_TEXT_LINE_LEN);
			return true;
		}
	}


	if((line>=ICON_JOY_TOP)&&(line<=ICON_JOY_BOT)){		
		//[now_kbd_mode]	[now_joy1_mode]	[now_joy2_mode]
		for (uint8_t idx=0;idx<9;idx++){
			if(icon_map[idx]<0xFF){
				img_up			= (ico_yfpos[icon_map[idx]]);
				img_down		= (ico_ytpos[icon_map[idx]]);
				img_left		= (ico_xfpos[icon_map[idx]]);
				img_right		= (ico_xtpos[icon_map[idx]]);
				img_first_line	= (icon_map_top[idx]);
				img_last_line	= (icon_map_top[idx]+(img_down-img_up)+1);
			}
			if((icon_map[idx]==0xFF)&&(idx==2)){
				img_up			= (ico_yfpos[ico_kbd_map[now_kbd_mode]]);
				img_down		= (ico_ytpos[ico_kbd_map[now_kbd_mode]]);
				img_left		= (ico_xfpos[ico_kbd_map[now_kbd_mode]]);
				img_right		= (ico_xtpos[ico_kbd_map[now_kbd_mode]]);
				img_first_line	= (icon_map_top[idx]);
				img_last_line	= (icon_map_top[idx]+(img_down-img_up)+1);
			}
			
			if((icon_map[idx]==0xFF)&&(idx==6)){
				img_up			= (ico_yfpos[ico_state_map[now_joy1_mode]]);
				img_down		= (ico_ytpos[ico_state_map[now_joy1_mode]]);
				img_left		= (ico_xfpos[ico_state_map[now_joy1_mode]]);
				img_right		= (ico_xtpos[ico_state_map[now_joy1_mode]]);
				img_first_line	= (icon_map_top[idx]);
				img_last_line	= (icon_map_top[idx]+(img_down-img_up)+1);
			}
			if((icon_map[idx]==0xFF)&&(idx==8)){
				img_up			= (ico_yfpos[ico_state_map[now_joy2_mode]]);
				img_down		= (ico_ytpos[ico_state_map[now_joy2_mode]]);
				img_left		= (ico_xfpos[ico_state_map[now_joy2_mode]]);
				img_right		= (ico_xtpos[ico_state_map[now_joy2_mode]]);
				img_first_line	= (icon_map_top[idx]);
				img_last_line	= (icon_map_top[idx]+(img_down-img_up)+1);
			}
			if((line>=img_first_line)&&(line<img_last_line)){
				addr=(((line-img_first_line)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[icon_map_xcoord[idx]],&icons[addr+img_left],(img_right-img_left));
				if((idx==5)||(idx==7)) continue;
				return true;
			}
		}		

	}
	if(show_slots){
		if((line>=ICON_SLOT_TOP)&&(line<=ICON_SLOT_BOT)){
			for (uint8_t idx=1;idx<11;idx++){
				img_up			= (ico_yfpos[ico_slots_map[save_slots[idx]][idx-1]]);
				img_down		= (ico_ytpos[ico_slots_map[save_slots[idx]][idx-1]]);
				img_left		= (ico_xfpos[ico_slots_map[save_slots[idx]][idx-1]]);
				img_right		= (ico_xtpos[ico_slots_map[save_slots[idx]][idx-1]]);
				img_first_line	= (ICON_SLOT_TOP);
				img_last_line	= (ICON_SLOT_TOP+(img_down-img_up)+1);
				if((line>=img_first_line)&&(line<img_last_line)){
					addr=(((line-img_first_line)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_SLOT_LEFT+((idx-1)*(img_right-img_left))],&icons[addr+img_left],(img_right-img_left));
				}
			}
			return true;
		}
	}

	return false;
}
#pragma GCC pop_options

#pragma GCC push_options
#if COMPOSITE_TV||SOFT_COMPOSITE_TV
#pragma GCC optimize("-Ofast")
#else
#pragma GCC optimize("-Os")
#endif
bool FAST_FUNC(hud_prepare_scale)(short int line){
	//if(!show_hud) return false;
	if(line<1){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_BAT_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_VOL_TOP-1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_VOL_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}
	uint16_t addr=0;
	if(current_hud_mode&HM_SHOW_BATTERY){
		if((line>=ICON_BAT_TOP)&&(line<=ICON_BAT_BOT)){
			if((battery_status&0xF0)>0){
				img_up			= (ico_yfpos[ico_battery_map[10+(battery_status>>4)]]);
				img_down		= (ico_ytpos[ico_battery_map[10+(battery_status>>4)]]);
				img_left		= (ico_xfpos[ico_battery_map[10+(battery_status>>4)]]);
				img_right		= (ico_xtpos[ico_battery_map[10+(battery_status>>4)]]);
				img_last_line	= (ICON_BAT_TOP+(img_down-img_up)+1);
				if(line<img_last_line){
					addr=(((line-ICON_BAT_TOP)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_CHG_LEFT],&icons[addr+img_left],(img_right-img_left));
				}
			};
			img_up			= (ico_yfpos[ico_battery_map[battery_status&0x0f]]);
			img_down		= (ico_ytpos[ico_battery_map[battery_status&0x0f]]);
			img_left		= (ico_xfpos[ico_battery_map[battery_status&0x0f]]);
			img_right		= (ico_xtpos[ico_battery_map[battery_status&0x0f]]);
			img_last_line	= (ICON_BAT_TOP+(img_down-img_up)+1);
			if(line<img_last_line){
				addr=(((line-ICON_BAT_TOP)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[ICON_BAT_LEFT],&icons[addr+img_left],(img_right-img_left));
				return true;
			}
		}
	}
	if(current_hud_mode&HM_SHOW_VOLUME){
		//memset(hud_line,0x10,SCREEN_W);
		if((line>=ICON_VOL_TOP)&&(line<=ICON_VOL_BOT)){
			if (vol_bank>0){
				if(line>=ICON_VOL_BANK_TOP){
					img_up			= (ico_yfpos[ico_vol_pre_map[(vol_bank-1)]]);
					img_down		= (ico_ytpos[ico_vol_pre_map[(vol_bank-1)]]);
					img_left		= (ico_xfpos[ico_vol_pre_map[(vol_bank-1)]]);
					img_right		= (ico_xtpos[ico_vol_pre_map[(vol_bank-1)]]);
					img_last_line	= (ICON_VOL_BANK_TOP+(img_down-img_up));
					if(line<img_last_line){
						addr=(((line-ICON_VOL_BANK_TOP)+img_up)*ICONS_WIDTH);
						memcpy(&hud_line[ICON_VOL_BANK_LEFT],&icons[addr+img_left],(img_right-img_left));
					}	
				}
			}
			if(line>=ICON_VOL_STRENGTH_TOP){				
				img_up			= (ico_yfpos[ico_vol_map[vol_strength]]);
				img_down		= (ico_ytpos[ico_vol_map[vol_strength]]);
				img_left		= (ico_xfpos[ico_vol_map[vol_strength]]);
				img_right		= (ico_xtpos[ico_vol_map[vol_strength]]);
				img_last_line	= (ICON_VOL_STRENGTH_TOP+(img_down-img_up));
				if(line<img_last_line){
					addr=(((line-ICON_VOL_STRENGTH_TOP)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_VOL_STRENGTH_LEFT],&icons[addr+img_left],(img_right-img_left));
				}	
			}
			if(line>=ICON_VOL_NAME_TOP){
				img_up			= (ico_yfpos[ico_vol_scale_map[0]]);
				img_down		= (ico_ytpos[ico_vol_scale_map[0]]);
				img_left		= (ico_xfpos[ico_vol_scale_map[0]]);
				img_right		= (ico_xtpos[ico_vol_scale_map[0]]);
				img_last_line	= (ICON_VOL_NAME_TOP+(img_down-img_up));
				if(line<img_last_line){
					addr=(((line-ICON_VOL_NAME_TOP)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_VOL_NAME_LEFT],&icons[addr+img_left],(img_right-img_left));
					return true;
				}	
			}
			if(line>=ICON_VOL_SCALE_TOP){
				img_up			= (ico_yfpos[ico_vol_scale_map[1]]);
				img_down		= (ico_ytpos[ico_vol_scale_map[1]]);
				img_left		= (ico_xfpos[ico_vol_scale_map[1]]);
				img_right		= (ico_xtpos[ico_vol_scale_map[1]]);
				img_last_line	= (ICON_VOL_SCALE_TOP+(img_down-img_up));
				img_first_line	= (ico_yfpos[ico_vol_scale_map[2]]);

				if(line<=img_last_line){
					addr=(((line-ICON_VOL_SCALE_TOP)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_VOL_SCALE_LEFT],&icons[addr+img_left],(img_right-img_left));
					addr=(((line-ICON_VOL_SCALE_TOP)+img_first_line)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_VOL_SCALE_LEFT],&icons[addr+img_left],(5*(cfg_volume/8))+1);
					return true;
				}	
			}		
		}
	}
	if(current_hud_mode&HM_SHOW_BRIGHT){
		if((line>=ICON_BRI_TOP)&&(line<=ICON_BRI_BOT)){
			if(line>=ICON_BRI_NAME_TOP){
				img_up			= (ico_yfpos[ico_bri_scale_map[0]]);
				img_down		= (ico_ytpos[ico_bri_scale_map[0]]);
				img_left		= (ico_xfpos[ico_bri_scale_map[0]]);
				img_right		= (ico_xtpos[ico_bri_scale_map[0]]);
				img_last_line	= (ICON_BRI_NAME_TOP+(img_down-img_up));
				if(line<img_last_line){
					addr=(((line-ICON_BRI_NAME_TOP)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_BRI_NAME_LEFT],&icons[addr+img_left],(img_right-img_left));
					return true;
				}	
			}
			if(line>=ICON_BRI_SCALE_TOP){
				img_up			= (ico_yfpos[ico_bri_scale_map[1]]);
				img_down		= (ico_ytpos[ico_bri_scale_map[1]]);
				img_left		= (ico_xfpos[ico_bri_scale_map[1]]);
				img_right		= (ico_xtpos[ico_bri_scale_map[1]]);
				img_last_line	= (ICON_BRI_SCALE_TOP+(img_down-img_up));
				img_first_line	= (ico_yfpos[ico_bri_scale_map[2]]);
				if(line<=img_last_line){
					addr=(((line-ICON_BRI_SCALE_TOP)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_BRI_SCALE_LEFT],&icons[addr+img_left],(img_right-img_left));
					addr=(((line-ICON_BRI_SCALE_TOP)+img_first_line)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_BRI_SCALE_LEFT],&icons[addr+img_left],(16*(cfg_brightness))+1);
					return true;
				}	
			}		
		}		
		/*
		if (vol_bank>0){
			if(line>=ICON_VOL_BANK_TOP){
				img_up			= (ico_yfpos[ico_vol_pre_map[(vol_bank-1)]]);
				img_down		= (ico_ytpos[ico_vol_pre_map[(vol_bank-1)]]);
				img_left		= (ico_xfpos[ico_vol_pre_map[(vol_bank-1)]]);
				img_right		= (ico_xtpos[ico_vol_pre_map[(vol_bank-1)]]);
				img_first_line	= (ICON_VOL_BANK_TOP);
				img_last_line	= (ICON_VOL_BANK_TOP+(img_down-img_up));
				if((line>=img_first_line)&&(line<img_last_line)){
					addr=(((line-img_first_line)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_VOL_BANK_LEFT],&icons[addr+img_left],(img_right-img_left));
				}	
			}
		}

		if(line>=ICON_VOL_STRENGTH_TOP){
			img_up			= (ico_yfpos[ico_vol_map[vol_strength]]);
			img_down		= (ico_ytpos[ico_vol_map[vol_strength]]);
			img_left		= (ico_xfpos[ico_vol_map[vol_strength]]);
			img_right		= (ico_xtpos[ico_vol_map[vol_strength]]);
			img_first_line	= (ICON_VOL_STRENGTH_TOP);
			img_last_line	= (ICON_VOL_STRENGTH_TOP+(img_down-img_up));
			if((line>=img_first_line)&&(line<img_last_line)){
				addr=(((line-img_first_line)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[ICON_VOL_STRENGTH_LEFT],&icons[addr+img_left],(img_right-img_left));
			}	
		}

		if(line>=ICON_VOL_NAME_TOP){
			img_up			= (ico_yfpos[ico_vol_scale_map[0]]);
			img_down		= (ico_ytpos[ico_vol_scale_map[0]]);
			img_left		= (ico_xfpos[ico_vol_scale_map[0]]);
			img_right		= (ico_xtpos[ico_vol_scale_map[0]]);
			img_first_line	= (ICON_VOL_NAME_TOP);
			img_last_line	= (ICON_VOL_NAME_TOP+(img_down-img_up));
			if((line>=img_first_line)&&(line<img_last_line)){
				addr=(((line-img_first_line)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[ICON_VOL_NAME_LEFT],&icons[addr+img_left],(img_right-img_left));
				return true;
			}	
		}
		if(line>=ICON_VOL_SCALE_TOP){
			img_up			= (ico_yfpos[ico_vol_scale_map[1]]);
			img_down		= (ico_ytpos[ico_vol_scale_map[1]]);
			img_left		= (ico_xfpos[ico_vol_scale_map[1]]);
			img_right		= (ico_xtpos[ico_vol_scale_map[1]]);
			img_first_line	= (ICON_VOL_SCALE_TOP);
			img_last_line	= (ICON_VOL_SCALE_TOP+(img_down-img_up));
			if((line>=img_first_line)&&(line<img_last_line)){
				addr=(((line-img_first_line)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[ICON_VOL_SCALE_LEFT],&icons[addr+img_left],(img_right-img_left));
				memcpy(&hud_line[ICON_VOL_SCALE_LEFT],&icons[addr+img_left+(23*ICONS_WIDTH)],(5*(cfg_volume/8))+1);
				return true;
			}	
	
		}
		*/
	}

	return false;
}
#pragma GCC pop_options

//void draw_bufline_text5x7_len(uint8_t* ptr, int line,char* text,color_t colorText,color_t colorBg,int len);
#ifndef DEBUG_DISABLE_LOADERS

#pragma GCC push_options
#if COMPOSITE_TV||SOFT_COMPOSITE_TV
#pragma GCC optimize("-Ofast")
#else
#pragma GCC optimize("-Os")
#endif
bool FAST_FUNC(hud_prepare_tape)(short int line){
	//if(!show_hud) return false;
	if(line<1){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_BAT_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_TAPE_TOP-1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_TAPE_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_TAPE_TEXT_TOP-1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_TAPE_TEXT_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}

	uint16_t addr=0;
	if(current_hud_mode&HM_SHOW_BATTERY){
		if((line>=ICON_BAT_TOP)&&(line<=ICON_BAT_BOT)){
			if((battery_status&0xF0)>0){
				img_up			= (ico_yfpos[ico_battery_map[10+(battery_status>>4)]]);
				img_down		= (ico_ytpos[ico_battery_map[10+(battery_status>>4)]]);
				img_left		= (ico_xfpos[ico_battery_map[10+(battery_status>>4)]]);
				img_right		= (ico_xtpos[ico_battery_map[10+(battery_status>>4)]]);
				img_last_line	= (ICON_BAT_TOP+(img_down-img_up)+1);
				if(line<img_last_line){
					addr=(((line-ICON_BAT_TOP)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_CHG_LEFT],&icons[addr+img_left],(img_right-img_left));
				}
			};
			img_up			= (ico_yfpos[ico_battery_map[battery_status&0x0f]]);
			img_down		= (ico_ytpos[ico_battery_map[battery_status&0x0f]]);
			img_left		= (ico_xfpos[ico_battery_map[battery_status&0x0f]]);
			img_right		= (ico_xtpos[ico_battery_map[battery_status&0x0f]]);
			img_last_line	= (ICON_BAT_TOP+(img_down-img_up)+1);
			if(line<img_last_line){
				addr=(((line-ICON_BAT_TOP)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[ICON_BAT_LEFT],&icons[addr+img_left],(img_right-img_left));
				return true;
			}
		}
	}	
	if(line>=ICON_TAPE_TOP){
		img_up			= (ico_yfpos[icon_tape_map[tape_disp]]);
		img_down		= (ico_ytpos[icon_tape_map[tape_disp]]);
		img_left		= (ico_xfpos[icon_tape_map[tape_disp]]);
		img_right		= (ico_xtpos[icon_tape_map[tape_disp]]);
		img_last_line	= (ICON_TAPE_TOP+(img_down-img_up)+1);
		if(line<img_last_line){
				addr=(((line-ICON_TAPE_TOP)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[ICON_COLUMN_LEFT],&icons[addr+img_left],(img_right-img_left));
				return true;
			}
	}
	if((line>=ICON_TAPE_TEXT_TOP)&&(line<(ICON_TAPE_TEXT_TOP+FONT_5x7_H))){
		draw_bufline_text5x7_len(&hud_line[32],(line-ICON_TAPE_TEXT_TOP),hud_text,CL_LT_GREEN,CL_EMPTY,HUD_TEXT_LINE_LEN);
		return true;
	}
	if((cfg_tap_load_mode==FAST_LOAD_TAP)&&(line>=(ICON_TAPE_TEXT_TOP+FONT_5x7_H+2))){
		memset(hud_line,0x10,SCREEN_W);
		return false;
	}
	if((line>=(ICON_TAPE_TEXT_TOP+FONT_5x7_H+2)&&(line<(ICON_TAPE_TEXT_TOP+(FONT_5x7_H*2)+2)))){
		draw_bufline_text5x7_len(&hud_line[32],(line-(ICON_TAPE_TEXT_TOP+FONT_5x7_H+2)),&hud_text[HUD_TEXT_LINE_LEN],CL_LT_GREEN,CL_EMPTY,HUD_TEXT_LINE_LEN);
		return true;
	}
	
	return false;
}
#pragma GCC pop_options



#pragma GCC push_options
#if COMPOSITE_TV||SOFT_COMPOSITE_TV
#pragma GCC optimize("-Ofast")
#else
#pragma GCC optimize("-Os")
#endif
bool FAST_FUNC(hud_kb_lock)(short int line){
	if(line<1){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_BAT_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_LOCK_TOP-1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_LOCK_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}

	uint16_t addr=0;
	if(current_hud_mode&HM_SHOW_BATTERY){
		if((line>=ICON_BAT_TOP)&&(line<=ICON_BAT_BOT)){
			if((battery_status&0xF0)>0){
				img_up			= (ico_yfpos[ico_battery_map[10+(battery_status>>4)]]);
				img_down		= (ico_ytpos[ico_battery_map[10+(battery_status>>4)]]);
				img_left		= (ico_xfpos[ico_battery_map[10+(battery_status>>4)]]);
				img_right		= (ico_xtpos[ico_battery_map[10+(battery_status>>4)]]);
				img_last_line	= (ICON_BAT_TOP+(img_down-img_up)+1);
				if(line<img_last_line){
					addr=(((line-ICON_BAT_TOP)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_CHG_LEFT],&icons[addr+img_left],(img_right-img_left));
				}
			} else {
				memset(&hud_line[ICON_CHG_LEFT],0x10,10);
			}
			img_up			= (ico_yfpos[ico_battery_map[battery_status&0x0f]]);
			img_down		= (ico_ytpos[ico_battery_map[battery_status&0x0f]]);
			img_left		= (ico_xfpos[ico_battery_map[battery_status&0x0f]]);
			img_right		= (ico_xtpos[ico_battery_map[battery_status&0x0f]]);
			img_last_line	= (ICON_BAT_TOP+(img_down-img_up)+1);
			if(line<img_last_line){
				addr=(((line-ICON_BAT_TOP)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[ICON_BAT_LEFT],&icons[addr+img_left],(img_right-img_left));
				return true;
			}
		}
	}	
	if((current_hud_mode&HM_SHOW_KEYLOCK)){
		if(line>=ICON_LOCK_TOP){
			img_up			= (ico_yfpos[ico_lock_map[(uint8_t)kbd_lock]]);
			img_down		= (ico_ytpos[ico_lock_map[(uint8_t)kbd_lock]]);
			img_left		= (ico_xfpos[ico_lock_map[(uint8_t)kbd_lock]]);
			img_right		= (ico_xtpos[ico_lock_map[(uint8_t)kbd_lock]]);
			img_last_line	= (ICON_LOCK_TOP+(img_down-img_up)+1);
			if(line<img_last_line){
				addr=(((line-ICON_LOCK_TOP)+img_up)*ICONS_WIDTH);
				memcpy(&hud_line[ICON_LOCK_LEFT],&icons[addr+img_left],(img_right-img_left));
				return true;
			}
		}
	}
	return false;
}
#pragma GCC pop_options

#endif

/*
bool FAST_FUNC(hud_prepare)(short int line){
	if(line<24){
		if(line%2){
			memset(hud_line,0x0F,SCREEN_W);
			return true;	
		}
		for (uint8_t ptr;ptr<(SCREEN_W/2);ptr++){
			hud_line[ptr]=0x11;
		}
		return false;
	} else {
		memset(hud_line,0x66,SCREEN_W);
	}
	return false;
}
*/


#pragma GCC push_options
#if COMPOSITE_TV||SOFT_COMPOSITE_TV
#pragma GCC optimize("-Ofast")
#else
#pragma GCC optimize("-Os")
#endif
bool FAST_FUNC(hud_prepare_kbd)(short int line){
	if(line<1){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==(ICON_BAT_BOT+1)){memset(hud_line,0x10,SCREEN_W);return false;}
	if(line==200){memset(hud_line,0x10,SCREEN_W);return false;} 
	if(line>239){memset(hud_line,0x10,SCREEN_W);return false;} 
	if(line>=0){
		uint16_t addr=0;
		/*if(current_hud_mode&HM_SHOW_BATTERY){
			if((line>=ICON_BAT_TOP)&&(line<=ICON_BAT_BOT)){
				img_up			= (ico_yfpos[ico_battery_map[battery_status&0x0f]]);
				img_down		= (ico_ytpos[ico_battery_map[battery_status&0x0f]]);
				img_left		= (ico_xfpos[ico_battery_map[battery_status&0x0f]]);
				img_right		= (ico_xtpos[ico_battery_map[battery_status&0x0f]]);
				if(line<(ICON_BAT_TOP+(img_down-img_up)+1)){
					addr=(((line-ICON_BAT_TOP)+img_up)*ICONS_WIDTH);
					memcpy(&hud_line[ICON_BAT_LEFT],&icons[addr+img_left],(img_right-img_left));
					return true;
				}
			}
		}*/		

		//if(kbd_first_line<200){return false;}
		//if((img_up==0)||(img_down==0)||(img_left==0)||(img_right==0)||(img_first_line==0)||(img_last_line==0)||(kbd_first_line==0)) return false;
		//for(uint16_t pos=SCREEN_W;pos--;) hud_line[pos]=0xFF;		
		//memset(hud_line,0x11,SCREEN_W);	//fill buffer EMPTY color
		img_first_line=kbd_full_yfpos[kbd_row];
		img_last_line=kbd_full_ytpos[kbd_row];		
		kbd_first_line=kbd_row_full_start_line[kbd_row];	
		if((line>=kbd_first_line)&&(line<=(kbd_first_line+(img_last_line-img_first_line)))){
			addr=((line-kbd_first_line)+(img_first_line))*255;
			if((addr>=0)&&(addr<=40800)){
				img_up=kbd_btn_yfpos[kbd_row];
				img_down=kbd_btn_ytpos[kbd_row];
				img_left =kbd_xfpos[kbd_row][kbd_col]+1;
				img_right =kbd_xtpos[kbd_row][kbd_col]+1;
				//memcpy(&hud_line[0],&kbd_img[0],32);
				//memcpy(&hud_line[285],&kbd_img[0],35);
				//memset(hud_line,0x08,SCREEN_W);
				memcpy(&hud_line[HUD_PIXEL_POS],&kbd_img[addr],255);
				if((((line-kbd_first_line)+(img_first_line))<img_up)||(((line-kbd_first_line)+(img_first_line))>=img_down)){
					memset(&hud_line[HUD_PIXEL_POS],0x10,img_left+1);
					memset(&hud_line[HUD_PIXEL_POS+1+img_right],0x10,254-img_right);
				}
				if((kbd_cshift)&&(kbd_row==4)){
					for(uint16_t pos=kbd_xfpos[kbd_row][0]+1;pos<=kbd_xtpos[kbd_row][0]+1;pos++){
						if(hud_line[HUD_PIXEL_POS+pos]==0x07){hud_line[HUD_PIXEL_POS+pos]=0x0A;}
					}
				}
				if((kbd_sshift)&&(kbd_row==4)){
					for(uint16_t pos=kbd_xfpos[kbd_row][8]+1;pos<=kbd_xtpos[kbd_row][8]+1;pos++){
						if(hud_line[HUD_PIXEL_POS+pos]==0x07){hud_line[HUD_PIXEL_POS+pos]=0x0E;}
					}
				}
				for(uint16_t pos=img_left;pos<img_right;pos++){
					if(hud_line[HUD_PIXEL_POS+pos]==0x07){hud_line[HUD_PIXEL_POS+pos]=0x04;}
					if(hud_line[HUD_PIXEL_POS+pos]==0x0A){hud_line[HUD_PIXEL_POS+pos]=0x05;}
					if(hud_line[HUD_PIXEL_POS+pos]==0x0E){hud_line[HUD_PIXEL_POS+pos]=0x03;}
				}
				return true;
			}
		}
		return false;
	}
	return false;
}
#pragma GCC pop_options

