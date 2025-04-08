#include "util_pok.h"

#include <stdlib.h>
#include <string.h>

#include "screen_util.h"
#include "ps2.h"
#include "iface.h"
#include "util_sd.h"
#include "zx_emu/z80.h"
#include "zx_emu/zx_machine.h"
#include "kb_u_codes.h"
#include "../lib/joysticks/Joystics.h"
#include "small_logo.h"

#define MAX_BTN (2)

extern uint8_t RAM[ZX_RAM_PAGE_SIZE*ZX_RAM_PAGES];//Реальная память куском 128Кб
extern z80 cpu;
extern uint8_t zx_RAM_bank_active;
extern uint8_t* zx_cpu_ram[4];//Адреса 4х областей памяти CPU при использовании страниц
extern uint8_t* zx_ram_bank[8];
extern uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))
extern uint8_t* zx_video_ram;

extern bool zx_state_48k_MODE_BLOCK;
extern uint8_t zx_Border_color;

extern void zx_machine_set_7ffd_out(uint8_t val);
extern uint8_t zx_machine_get_7ffd_lastOut();

extern short int last_error;
#ifndef DEBUG_DISABLE_LOADERS
extern uint8_t temp_buffer_x[TEMP_BUFF_SIZE_X];
extern uint8_t temp_buffer_y[TEMP_BUFF_SIZE_Y];
#endif

extern char temp_msg[60];
extern uint8_t menu_mode[5];
extern uint8_t menu_ptr;
extern uint8_t data_joy;
//extern bool stateFlash;

extern void process_input(void);
extern void clear_input(void);

POKE_LINE* pokes_buff;
POKE_DATA prev_value;

uint8_t read_zx_mem(uint16_t addr){
	if (addr<16384) return zx_cpu_ram[0][addr];
	if (addr<32768) return zx_cpu_ram[1][addr-16384];
	if (addr<49152) return zx_cpu_ram[2][addr-32768];
	return zx_cpu_ram[3][addr-49152];
}

void write_zx_mem(uint16_t addr, uint8_t val){
	if (addr<16384) return;//запрещаем писать в ПЗУ
	if (addr<32768) {zx_cpu_ram[1][addr-16384]=val;return;};
	if (addr<49152) {zx_cpu_ram[2][addr-32768]=val;return;};
	zx_cpu_ram[3][addr-49152]=val;
}

const char *iface_btn_bottom[2][2] ={
	{"[Select All]\0","[Clear All]\0",},
	{"[Apply]\0","[Cancel]\0",},
};

uint8_t PokeValueDialogBox(char *header,char *message,char *value,uint8_t colorFG,uint8_t colorBG,uint8_t di_id){
	uint8_t max_len = strlen(header)>strlen(message)?strlen(header):strlen(message);
	if(max_len<10){max_len=10;}
	uint8_t left_x =(SCREEN_W/2)-((max_len/2)*FONT_W);
	uint8_t left_x_field =(SCREEN_W/2)-(((max_len-2)/2)*FONT_W);
	uint8_t left_x_val=(SCREEN_W/2)-(strlen(value)*FONT_W);
	uint8_t left_y = strlen(message)==0 ? (SCREEN_H/2)-(FONT_H/2):(SCREEN_H/2)-FONT_H;
	uint8_t height = FONT_H*4; //uint8_t height = strlen(message)>0 ? FONT_H*2+5:FONT_H+5;

	uint8_t val_len = strlen(value);

	bool need_redraw=false;
	bool old_stateFlash=false;

	draw_rect(left_x-1,left_y,(max_len*FONT_W)+2,height+1,COLOR_MAIN_RAMK,true);//Основная рамка
	draw_rect(left_x,left_y+FONT_H,(max_len*FONT_W),FONT_H*3,colorBG,true);//Фон главного окна
	draw_stripes(left_x+((max_len-5)*FONT_W),left_y);

	if (strlen(message)>0){
		draw_text(left_x,left_y,header,colorFG,CL_EMPTY);
		draw_text(left_x,left_y+FONT_H,message,colorFG,CL_EMPTY);
		draw_text(left_x_field,left_y+(FONT_H*2),value,colorFG,CL_GRAY);
	} else {
		draw_text(left_x,left_y+FONT_H,header,colorFG,CL_EMPTY);
		draw_text(left_x_field,left_y+(FONT_H*2),value,colorFG,CL_GRAY);
	}
	////printf("X:%d,Y:%d\n",left_x,left_y);
	if(menu_mode[menu_ptr]==EMULATION){
		zx_machine_enable_vbuf(false);
		busy_wait_ms(20);
	}
	/*
#define DLG_RES_NONE	0x00
#define DLG_RES_CANCEL	0xFF
#define DLG_RES_OK		0x01
#define DLG_RES_RET		0x02
#define DLG_RES_ABR		0x03	
	*/
	//btn_pos = settings_index-(poke_count-1);
	memset(value,0,val_len);
	short int btn_pos=1;
	short int cursor_pos=0;
	short int edit_pos=0;
	uint8_t dia_res=0;
	need_redraw=true;
	while(true){
		process_input();
		if(((KBD_BACK_SPACE)||(data_joy==D_JOY_B))&&(edit_pos>0)){
			edit_pos--;
			value[edit_pos]=0x00;
			cursor_pos--;
			need_redraw=true;
			busy_wait_ms(75);
			clear_input();
		}
		if(((KBD_LEFT)||(data_joy==D_JOY_LEFT))&&(cursor_pos>0)){
			cursor_pos--;
			need_redraw=true;
			busy_wait_ms(75);
			clear_input();
		}
		if((KBD_RIGHT)||(data_joy==D_JOY_RIGHT)&&(cursor_pos<(val_len+MAX_BTN))){
			cursor_pos++;
			need_redraw=true;
			busy_wait_ms(75);
			clear_input();
		}

		if((data_joy==D_JOY_UP)&&(cursor_pos<(val_len))){
			if((value[cursor_pos]>=0x20)&&(value[cursor_pos]<=0x5D)){
				value[cursor_pos]+=1;
				edit_pos=cursor_pos;
			} 
			if(value[cursor_pos]<0x20){value[cursor_pos]=0x5D;};
			if(value[cursor_pos]>0x5D){value[cursor_pos]=0x20;};
			need_redraw=true;
			busy_wait_ms(75);
			clear_input();
		}
		if((data_joy==D_JOY_DOWN)&&(cursor_pos<(val_len))){
			if((value[cursor_pos]>=0x20)&&(value[cursor_pos]<=0x5D)){
				value[cursor_pos]-=1;
				edit_pos=cursor_pos;
			}
			if(value[cursor_pos]<0x20){value[cursor_pos]=0x5D;};
			if(value[cursor_pos]>0x5D){value[cursor_pos]=0x20;};
			need_redraw=true;
			busy_wait_ms(75);
			clear_input();			
		}
		if((KBD_UP)&&(cursor_pos>0)){
			cursor_pos-=5;
			if(cursor_pos<0){cursor_pos==0;}
			need_redraw=true;
			busy_wait_ms(75);
			clear_input();
		}		
		if((KBD_DOWN)&&(cursor_pos<(val_len+MAX_BTN))){
			cursor_pos+=5;
			if(cursor_pos>(val_len+MAX_BTN)){cursor_pos==val_len+MAX_BTN;}
			need_redraw=true;
			busy_wait_ms(75);
			clear_input();
		}		

		if(((KBD_ENTER)||(data_joy==D_JOY_A))&&(cursor_pos>(val_len))){
			dia_res = iface_res[di_id][btn_pos-1];
			busy_wait_ms(75);
			clear_input();
			break;
		}
		if(((KBD_ESC)||(data_joy==D_JOY_START))&&(cursor_pos>(val_len))){
			memset(value,0,val_len);
			dia_res=DLG_RES_NONE;
			busy_wait_ms(75);
			clear_input();
			break;
		}
		if (KBD_PRESS){
			if(strlen(value)<=val_len){
				//strcat(value,convert_kb_u_to_char(value,kb_st_ps2));
				uint8_t chr = convert_kb_u_to_char(kb_st_ps2,true);
				if(chr>0){
					value[edit_pos]=chr;
					edit_pos++;
					cursor_pos++;
				}
				busy_wait_ms(75);
				clear_input();
				need_redraw=true;
			}
		}
		btn_pos = cursor_pos-val_len;
		if(need_redraw){
			printf("edit_pos:%d   cursor_pos:%d   val_len:%d\n",edit_pos,cursor_pos,val_len);
			short int pos = left_x+(max_len*FONT_W);
			uint8_t cnt_btn=MAX_BTN;
			do{
				if(strlen(iface_btn[di_id][cnt_btn-1])>1){
					pos-=strlen(iface_btn[di_id][cnt_btn-1])*FONT_W;
					memset(temp_msg,0,sizeof(temp_msg));
					strcpy(temp_msg,iface_btn[di_id][cnt_btn-1]);
					if((btn_pos>0)&&(btn_pos==(cnt_btn))){
						temp_msg[0]=0x10;
						temp_msg[strlen(iface_btn[di_id][cnt_btn-1])-1]=0x11;
					}					
					draw_text(pos,left_y+(FONT_H*3),temp_msg,COLOR_TEXT,btn_pos==cnt_btn?COLOR_CURRENT_BG:COLOR_BACKGOUND);
				}
				cnt_btn--;
			} while(cnt_btn>0);
			memset(temp_msg,0,sizeof(temp_msg));
			memset(temp_msg,0x20,(max_len-2));
			draw_text_len(left_x_field,left_y+(FONT_H*2),temp_msg,colorFG,CL_GRAY,(max_len-2));
			draw_text_len(left_x_val,left_y+(FONT_H*2),value,colorFG,CL_GRAY,strlen(value));
			if(btn_pos<1){
				if((cursor_pos<=edit_pos)){
					draw_text_len(left_x_val+(cursor_pos*FONT_W),left_y+(FONT_H*2),&value[cursor_pos],colorBG,colorFG,1);
				} else {
					memset(temp_msg,0,sizeof(temp_msg));
					//temp_msg[0]=0xDB;
					temp_msg[0]=0xFF;
					draw_text_len(left_x_val+(cursor_pos*FONT_W),left_y+(FONT_H*2),temp_msg,colorBG,colorFG,1);
				}
			}
			need_redraw=false;
		}
	}
	if(menu_mode[menu_ptr]==EMULATION) zx_machine_enable_vbuf(true);
	clear_input();
	return dia_res;
}

short int load_pok_captions(char *file_name){
#ifndef DEBUG_DISABLE_LOADERS
	memset(temp_buffer_x, 0, TEMP_BUFF_SIZE_X);
	memset(temp_buffer_y, 0, TEMP_BUFF_SIZE_Y);
	size_t bytesRead=0;
	size_t filePos=0;
	size_t buffpos=0;
	size_t bufflen=TEMP_BUFF_SIZE_Y;
	POKE_LINE* pokes = (POKE_LINE*)&temp_buffer_x[0];
	pokes_buff = (POKE_LINE*)&temp_buffer_x[0];
	uint8_t* chr = &temp_buffer_y[0];
	uint8_t* begin=NULL;
	uint8_t* end=NULL;
	int poke_count=0;

	sd_res = sd_open_file(&sd_file,file_name,FA_READ);
	if (sd_res!=FR_OK){sd_close_file(&sd_file); return false;};
	while (filePos<sd_file_size(&sd_file)){
		sd_res = sd_read_file(&sd_file,chr,bufflen,&bytesRead);
		if (sd_res!=FR_OK){sd_close_file(&sd_file);break;}
		while (chr<(&temp_buffer_y[0]+TEMP_BUFF_SIZE_Y)) {
			if((*chr==0x4E)&&(begin==NULL)){
				begin = chr;
			}
			if((*chr==0x0A)&&(begin!=NULL)&&(end==NULL)){
				end = chr;
			}
			if((*chr==0x59)&&((*(chr-1))==0x0A)){
				begin=(void*)0xFFFF;
				end=(void*)0xFFFF;
				break;
			};
			if((begin!=NULL)&&(end!=NULL)){
				uint16_t size = end-begin;
				if(size>POKE_TEXT_LEN) size=POKE_TEXT_LEN-1;
				if((size>0)&&(size<=sizeof(pokes->text))){
					memcpy(&pokes->text[0],begin+1,size-1);
					printf(">>>%s\n",pokes->text);
					if(pokes<(pokes+TEMP_BUFF_SIZE_X)){
						pokes++;
						poke_count++;
					} else {
						begin=(void*)0xFFFF;
						end=(void*)0xFFFF;
						break;
					}
					begin=NULL;
					end=NULL;
				};
			}
			chr++;
		}
		if((end==NULL)){
			memset(&temp_buffer_y[0], 0, TEMP_BUFF_SIZE_Y);
			if(begin>0){
				bufflen=(begin-temp_buffer_y);
			} else {
				bufflen=(chr-temp_buffer_y);
			}									
			chr = &temp_buffer_y[0];									
			filePos+=bufflen;
			sd_res=sd_seek_file(&sd_file,filePos);
			if (sd_res!=FR_OK){sd_close_file(&sd_file);break;}
			begin=NULL;
			end=NULL;									
			continue;
		}
		if((begin==0xFFFF)&&(end==0xFFFF)){
			break;
		} //EOF
	}
	return poke_count;
#endif
}

void apply_poke(POKE_DATA value){
	char dialog_value[40];
	
	uint8_t* ptr;
	if((value.new_val>0xFF)&&(prev_value.id!=value.id)){
		clear_input();
		busy_wait_ms(100);
		memset(dialog_value,0,sizeof(dialog_value));
		memset(dialog_value,0x20,3);
		if(PokeValueDialogBox("Please enter value",value.id->text,&dialog_value[0],CL_LT_BLUE,COLOR_BACKGOUND,DIALOG_OK_CAN)==DLG_RES_OK){
			printf("%s \n",dialog_value);
			value.new_val=atoi(dialog_value);
			if(value.new_val>0xFF){
				value.new_val=0xFF;
			}
		}
	}
	if((value.page&0x08)==0){
		ptr = zx_ram_bank[value.page&0x07]; //&zx_ram_bank[value.page&0x07];
	} else {
		write_zx_mem(value.addr,value.new_val&0xFF);
		//ptr = &zx_ram_bank[0];
	}
	//ptr+=value.addr;
	//*ptr=(value.new_val&0xFF);
	//prev_value=value;
}

void set_pok_values(POKE_LINE* pokes,short int poke_count,char *file_name){
	memset(temp_buffer_y, 0, TEMP_BUFF_SIZE_Y);
	POKE_LINE* poke;
	uint8_t* chr = &temp_buffer_y[0];
	size_t bytesRead=0;
	size_t filePos=0;
	size_t buffpos=0;
	size_t bufflen=TEMP_BUFF_SIZE_Y;
	bool poke_found=false;
	uint8_t* begin=NULL;
	uint8_t* end=NULL;
	uint8_t* begin_value=NULL;
	uint8_t* end_value=NULL;
	char* pval;
	POKE_DATA vals;

	sd_res = sd_open_file(&sd_file,file_name,FA_READ);
	if (sd_res!=FR_OK){sd_close_file(&sd_file); return;};
	while (filePos<sd_file_size(&sd_file)){
		sd_res = sd_read_file(&sd_file,chr,bufflen,&bytesRead);
		if (sd_res!=FR_OK){sd_close_file(&sd_file);break;}
		while (chr<(&temp_buffer_y[0]+TEMP_BUFF_SIZE_Y)) {
			if(!poke_found){
				if((*chr==0x4E)&&(begin==NULL)){
					begin = chr;
				}
				if((*chr==0x0A)&&(begin!=NULL)&&(end==NULL)){
					end = chr;
				}
				if((*chr==0x59)&&((*(chr-1))==0x0A)){
					begin=(void*)0xFFFF;
					end=(void*)0xFFFF;
					break;
				};
			}
			if(poke_found){
				if((*chr==0x4E)&&((begin_value==NULL)&&(end_value==NULL))){
					//printf("Resume search\n");
					*chr--;
					poke_found = false;
					continue;
				}
				if(((*chr==0x4D)||(*chr==0x5A))&&(begin_value==NULL)){
					begin_value = chr;
				}
				if((*chr==0x0A)&&(begin_value!=NULL)&&(end_value==NULL)){
					end_value = chr;
				}
				if((*chr==0x59)&&((*(chr-1))==0x0A)){
					begin=(void*)0xFFFF;
					end=(void*)0xFFFF;
					break;
				};
			}
			if((begin!=NULL)&&(end!=NULL)){
				uint16_t size = end-begin;
				if(size>POKE_TEXT_LEN) size=POKE_TEXT_LEN-1;
				memset(temp_msg,0,sizeof(temp_msg));
				memcpy(temp_msg,begin+1,size-1);
				printf("file:%s\n",temp_msg);
				if(size>0){
					for(short int i=0;i<poke_count;i++){
						poke = (POKE_LINE*)&pokes[i];
						if((poke->checked)&&(strncmp(poke->text,temp_msg,POKE_TEXT_LEN)==0)){
							printf("mem:%s\n",poke->text);
							poke_found = true;
							break;
						}
					}
				}
				begin=NULL;
				end=NULL;
			}
			if((begin_value!=NULL)&&(end_value!=NULL)){
				uint16_t size = end_value-begin_value;
				memset(temp_msg,0,sizeof(temp_msg));
				memcpy(temp_msg,begin_value+1,size-1);
				vals.id = poke;
				pval = strtok(temp_msg," ");
				vals.page = atoi(pval);
				pval = strtok(NULL," ");
				vals.addr = atoi(pval);
				pval = strtok(NULL," ");
				vals.new_val = atoi(pval);
				pval = strtok(NULL," ");
				vals.old_val = atoi(pval);
				printf("page>:%d   addr:[%04X]   new_val:%d  old_val:%d\n",vals.page,vals.addr,vals.new_val,vals.old_val);
				apply_poke(vals);
				begin_value=NULL;
				end_value=NULL;
			}			
			chr++;
		}
		if((end==NULL)||(end_value==NULL)){
			memset(&temp_buffer_y[0], 0, TEMP_BUFF_SIZE_Y);
			if(begin>0){
				bufflen=(begin-temp_buffer_y);
			} else {
				bufflen=(chr-temp_buffer_y);
			}									
			chr = &temp_buffer_y[0];									
			filePos+=bufflen;
			sd_res=sd_seek_file(&sd_file,filePos);
			if (sd_res!=FR_OK){sd_close_file(&sd_file);break;}
			begin=NULL;
			end=NULL;									
			continue;
		}
		if((begin==0xFFFF)&&(end==0xFFFF)){
			break;
		} //EOF
	}
}

void draw_pokes_list(POKE_LINE* pokes, short int poke_count,short int startLine, short int selected, bool check){
	
	if(check){
		draw_text_len(17+FONT_W*FILE_NAME_LEN,16,"Select patches to apply",COLOR_TEXT,COLOR_BACKGOUND,23);
	} else {
		draw_text_len(18+FONT_W*FILE_NAME_LEN,16,"File contents:",COLOR_TEXT,COLOR_BACKGOUND,14);
	}
	
	for(uint8_t y=0;y<((PREVIEW_HEIGHT/FONT_5x7_H));y++){
		memset(temp_msg,0,sizeof(temp_msg));
		POKE_LINE* disp_poke = (POKE_LINE*)&pokes[y+startLine];
		if(poke_count>(short int)(startLine+y)){
			if(check){
				char chk = 0x20;
				if(disp_poke->checked){chk=0xB1;} //2A 2B F9
				if(strlen(disp_poke->text)>0){
					sprintf(temp_msg,"[%c] %-30s",chk,disp_poke->text);
					if(y==(selected-startLine)){
						draw_text5x7_len(PREVIEW_POS_X,25+(FONT_5x7_H)*y,temp_msg,COLOR_TEXT,COLOR_CURRENT_BG,(PREVIEW_WIDTH/FONT_5x7_W)-1);
					} else {
						draw_text5x7_len(PREVIEW_POS_X,25+(FONT_5x7_H)*y,temp_msg,COLOR_TEXT,COLOR_BACKGOUND,(PREVIEW_WIDTH/FONT_5x7_W)-1);
					}
				} else {
					sprintf(temp_msg,"%38s"," ");
					draw_text5x7_len(PREVIEW_POS_X,25+(FONT_5x7_H)*y,temp_msg,COLOR_TEXT,COLOR_BACKGOUND,(PREVIEW_WIDTH/FONT_5x7_W)-1);
				}
			} else {
				sprintf(temp_msg,"%-30s",disp_poke->text);
				draw_text5x7_len(PREVIEW_POS_X,25+(FONT_5x7_H)*y,temp_msg,COLOR_TEXT,COLOR_BACKGOUND,(PREVIEW_WIDTH/FONT_5x7_W)-1);
			}
			/*
			if(disp_pokes<(pokes+(sizeof(POKE_LINE)*poke_count))){
				*disp_pokes++;
			}
			*/
		} else {
			sprintf(temp_msg,"%38s"," ");
			draw_text5x7_len(PREVIEW_POS_X,25+(FONT_5x7_H)*y,temp_msg,COLOR_TEXT,COLOR_BACKGOUND,(PREVIEW_WIDTH/FONT_5x7_W)-1);
		};
	}
}

void draw_pokes_bottom_btn(uint8_t xPos,uint8_t yPos,uint8_t dia_pos){
	uint8_t max_btn = 1;
	for (uint8_t y=0;y<2;y++){
		short int pos = 0;
		for (uint8_t x=0;x<2;x++){
			if(strlen(iface_btn_bottom[y][x])>1){
				memset(temp_msg,0,sizeof(temp_msg));
				strcpy(temp_msg,iface_btn_bottom[y][x]);
				if(dia_pos==max_btn){
					temp_msg[0]=0x10;
					temp_msg[strlen(iface_btn_bottom[y][x])-1]=0x11;
				}
				if((x%2)==0){
					pos = xPos+(PREVIEW_WIDTH/2);
					pos-=strlen(iface_btn_bottom[y][x])*FONT_W;
				} else {
					pos = xPos+(PREVIEW_WIDTH/2)+1;
					//pos+=strlen(iface_btn_bottom[y][x])*FONT_W;
				}
				draw_text(pos,yPos+(y*FONT_H),temp_msg,COLOR_TEXT,(dia_pos==max_btn)?COLOR_CURRENT_BG:COLOR_BACKGOUND);
				max_btn++;
			}
		}
		
	};
}

void draw_poke_menu(uint8_t xPos,uint8_t yPos,bool drawbg,char* text_src,uint8_t lines,uint8_t active){
	////printf("xPos:%d  yPos:%d  width:%d  height:%d\n",xPos,yPos,width,height);
	//draw_rect(0,0,SCREEN_W,SCREEN_H,COLOR_FULLSCREEN,true);//Заливаем экран 
	/*uint8_t lines = 0;
	for(uint8_t i=0;i<FAST_MENU_LINES;i++){
		if(*fast_menu[i]==0){
			lines=i;
			break;
		}
	}*/
	uint8_t width =(16*FONT_W)+2;
	uint8_t height =(lines*FONT_H)+FONT_H+1;
	////printf("xPos:%d  yPos:%d  width:%d  height:%d lines:%d\n",xPos,yPos,width,height,lines);
	if(drawbg){
		draw_rect(xPos-1,yPos,width,height,COLOR_MAIN_RAMK,true);//Основная рамка
		draw_rect(xPos,yPos+FONT_H,width-2,height-9,COLOR_BACKGOUND,true);//Фон главного окна
		draw_logo_header(xPos+2,yPos);
	}

	for(uint8_t y=0;y<lines;y++){
		//memset(temp_msg,0,sizeof(temp_msg));
		////printf("%s\n",help_text[y]);
		/*if((startLine+y)>HELP_LINES){
			draw_text_len(8,20+(y*8),"									 ",CL_BLACK,CL_WHITE,37);
			continue;
		}*/
		/*if (convert_utf8_to_windows1251(help_text[startLine+y], temp_msg, strlen(help_text[startLine+y]))>0){
			
		};
		if((menu==0)&&(!init_fs)&&(y<5)){
			draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),fast_menu[menu][y],COLOR_DTEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,16);	
		} else{
			draw_text_len(xPos,(yPos+FONT_H)+(y*FONT_H),fast_menu[menu][y],COLOR_TEXT,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,16);
		}
		if(menu==1){
			draw_text_len(xPos+FONT_W,(yPos+FONT_H)+(y*FONT_H),"*",save_slots[y]==1?CL_RED:CL_GREEN,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,1);
		}
		if(menu==2){
			draw_text_len(xPos+FONT_W,(yPos+FONT_H)+(y*FONT_H),"*",save_slots[y]==1?CL_GREEN:CL_RED,y==active?COLOR_CURRENT_BG:COLOR_BACKGOUND,1);
		}
		*/
	}
	//memset(temp_msg, 0, sizeof(temp_msg));
}

