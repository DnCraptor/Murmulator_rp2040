#pragma once
#include <pico/stdlib.h>
#include "util_cfg.h"

#define CONFIG_MENU_ITEMS 27

const char __in_flash() *config_menu[CONFIG_MENU_ITEMS]={
	"Boot go to  :[            ]",	//0
	"Head Up Display:     [    ]",	//1
	"Tap load:[                ]",	//2
	"Ext tape load pin: [      ]",	//3
	"Joy 1 map :  [            ]",	//4
	"Joy 2 map :  [            ]",	//5
	"Keyboard map:[            ]",	//6
	"Reboot ZX before load:[   ]",	//7
	"Snd mode:[                ]",	//8
	"Sound out mode:       [   ]",	//9
	"Soft Sound Vol:[          ]",	//10
	"Ext TS Clock Pin:  [      ]",	//11
	"Ext TS Clk Speed: [       ]",	//12
	"HW TS Main Chip: [        ]",	//13
	"Video output:  [          ]",	//14
	"Video framerate:   [      ]",	//15
	"Mobile Murmulator:    [   ]",	//16
	"LCD BrightLev: [          ]",  //17
	"LCD Rotate:    [          ]",  //18
	"LCD inversion: [          ]",  //19
	"LCD Pixel format:   [     ]",  //20
	"           [LOAD]          ",  //21
	"          [DEFAULT]        ",  //22
	"           [SAVE]          ",  //23
	"     [UPDATE FIRMWARE]     ",  //24
	"       [!!!REBOOT!!!]      ",  //25
	"\0",	//26
};

const char __in_flash() *boot_scr_config[3]={
	"    LOGO    ",
	"FILE MANAGER",
	" EMULATION  ",
};

const char __in_flash() *HUD_config[3]={
	"OFF ",
	"ON  ",
	"TIME",
};

const char __in_flash() *tap_load_config[3]={
	" NORMAL LOADING ",
	"AUTOSTOP LOADING",
	"  FAST LOADING  "
};

const char __in_flash() *ext_tape_load_config[3]={
	"GPIO22",
	"GPIO29",
	"*OFF* ",
};

const char __in_flash() *joy_config[6]={
	" Joy>Kempst ",
	" Joy>Cursor ",
	" Joy>Sincl 1",
	" Joy>Sincl 2",
	" Joy>QAOPM  ",
	" Joy>Custom ",
};

const char __in_flash() *kbd_config[6]={
	" KBD>Cursor ",
	" KBD>Kempst ",
	" KBD>Sincl 1",
	" KBD>Sincl 2",
	" KBD>QAOPM  ",
	" KBD>Custom ",
};

const char __in_flash() *yes_no[2]={
	"No ",
	"Yes"
};

const char __in_flash() *sound_config[5]={
	"-=[Sound OFF]=-",
	"  Only Beeper  ",
	" Beeper+SoftAY ",
	" Beeper+SoftTS ",
	" HW TurboSound "
};

const char __in_flash() *sound_out_config[2]={
	"PWM",
	"I2S",
};


const char __in_flash() *sound_clock_config[3]={
	"*OFF* ",
	"GPIO21",
	"GPIO29",	
};

const char __in_flash() *sound_speed_config[3]={
	"1.75Mhz",
	"2.00Mhz",
	"*CUSTOM",
};

const char __in_flash() *ts_chip_config[2]={
	" Chip 1 ",
	" Chip 2 "
};

#ifdef VGA_HDMI
const char __in_flash() *video_out_config[8]={
	"   AUTO   ",
	"   VGA    ",
	"   HDMI   ",
	"  ST7789  ",
	"  ST7789V ",
	"  ILI9341 ",
	" ILI9341V ",
	"  GC9A01  "
};
const char __in_flash() *video_out_rotate[4]={
	"Horizont R",
	"Vertical R",
	"Horizont L",
	"Vertical L",
};
const char __in_flash() *video_out_inversion[2]={
	"  NORMAL  ",
	" INVERTED ",
};
const char __in_flash() *video_out_pixels[2]={
	" BGR ",
	" RGB ",
};

#endif

#ifdef COMPOSITE_TV
const char __in_flash() *video_out_config[3]={
	"   AUTO   ",
	"   NTSC   ",
	"   PAL    ",
};
#endif

#ifdef SOFT_COMPOSITE_TV
const char __in_flash() *video_out_config[3]={
	"   AUTO   ",
	"   NTSC   ",
	"   PAL    ",
};
#endif

const char __in_flash() *video_freq_config[4]={
	" 60Hz ",
	" 72Hz ",
	" 75Hz ",
	" 85Hz ",
};



const char __in_flash() *gaudge[11]={
	"          ",
	"*         ",
	"**        ",
	"***       ",
	"****      ",
	"*****     ",
	"******    ",
	"*******   ",
	"********  ",
	"********* ",
	"**********",
};