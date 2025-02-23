#pragma once
#include "inttypes.h"
#include <pico/stdlib.h>

#define FAST_MENU_MAIN 0
#define FAST_MENU_SAVE 1
#define FAST_MENU_LOAD 2
#define FAST_MENU_TAPE 3
#define FAST_MENU_MOUNT 4

#define FAST_SUBMENU_LINES 10
#define TRD_SUBMENU_LINES 8

#define FAST_MENU_COUNT 5
#define FAST_MENU_LINES 13


#define FAST_MAIN_MANAGER		(0)
#define FAST_MAIN_KEYBOARD		(1)
#define FAST_MAIN_SAVE			(2)
#define FAST_MAIN_LOAD			(3)
#define FAST_MAIN_TAPE			(4)
#define FAST_MAIN_POKE			(5)
#define FAST_MAIN_HELP			(6)
#define FAST_MAIN_SETTINGS		(7)
#define FAST_MAIN_SOFT_RESET	(8)
#define FAST_MAIN_HARD_RESET	(9)


const uint8_t __in_flash() *fast_menu_lines[FAST_MENU_COUNT]={(uint8_t*)10,(uint8_t*)11,(uint8_t*)11,(uint8_t*)6,(uint8_t*)9};
const char __in_flash() *fast_menu[FAST_MENU_COUNT][FAST_MENU_LINES]={
	{ 
		
		" [FILE BROWSER] ",
		"   [KEYBOARD]   ",
		"     [SAVE]     ",
		"     [LOAD]     ",
		"     [TAPE]     ",
		"     [POKE]     ",
		"     [HELP]     ",
		"   [SETTINGS]   ",
		"   Soft reset   ",
		"   Hard reset   ",
		"\0",
		/*
		"     [SAVE]     ",
		"     [LOAD]     ",
		" [FILE BROWSER] ",
		"   [KEYBOARD]   ",
		"     [TAPE]     ",
		"     [HELP]     ",
		"   [SETTINGS]   ",
		"  [Soft reset]  ",
		"  [Hard reset]  ",
		"\0",
		*/
	},
	{
		"[ ] Quick Save  ",
		"[ ]  SAVE 1     ",
		"[ ]  SAVE 2     ",
		"[ ]  SAVE 3     ",
		"[ ]  SAVE 4     ",
		"[ ]  SAVE 5     ",
		"[ ]  SAVE 6     ",
		"[ ]  SAVE 7     ",
		"[ ]  SAVE 8     ",
		"[ ]  SAVE 9     ",
		"[ ]  SAVE 10    ",
		"\0",        
	},
	{
		"[ ] Quick Load  ",
		"[ ]  LOAD 1     ",
		"[ ]  LOAD 2     ",
		"[ ]  LOAD 3     ",
		"[ ]  LOAD 4     ",
		"[ ]  LOAD 5     ",
		"[ ]  LOAD 6     ",
		"[ ]  LOAD 7     ",
		"[ ]  LOAD 8     ",
		"[ ]  LOAD 9     ",
		"[ ]  LOAD 10    ",
		"\0",
	},
	{
		"  [Tape START]  ",
		"  [Tape STOP]   ",
		" [Tape REWIND]  ",
		" [Tape BLOCK<<] ",        
		" [Tape BLOCK>>] ",
		"  [Tape EJECT]  ",
		"\0",
	},
	{
		"[>Mount A:]     ",
		" [>Mount B:]    ",
		"  [>Mount C:]   ",
		"   [>Mount D:]  ",
		"[^Eject A:]     ",
		" [^Eject B:]    ",
		"  [^Eject C:]   ",
		"   [^Eject D:]  ",
		"    [^Eject ALL]",
		"\0",
	}
};