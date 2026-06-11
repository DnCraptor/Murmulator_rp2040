#pragma once
#if PICO_RP2350
#include "boards/pico2.h"
#else
#include "boards/pico.h"
#endif

// 16MB flash
#define PICO_FLASH_SIZE_BYTES 16777216
// SDCARD
#define SDCARD_PIN_SPI0_CS 5
#define SDCARD_PIN_SPI0_SCK 6
#define SDCARD_PIN_SPI0_MOSI 7
#define SDCARD_PIN_SPI0_MISO 4

// PS2KBD
#define PS2KBD_GPIO_FIRST 2

// NES Gamepad
#define NES_GPIO_CLK 20
#define NES_GPIO_DATA 26
#define NES_GPIO_DATA1 26
#define NES_GPIO_DATA2 27
#define NES_GPIO_LAT 21

// 74HC595 / AY
#define LATCH_595_PIN 9
#define CLK_595_PIN 10
#define DATA_595_PIN 11
//#define CLK_AY_PIN1 21
//#define CLK_AY_PIN2 29

// Misc
#define PIN_ZX_LOAD 22
#define ZX_AY_PWM_PIN0 10
#define ZX_AY_PWM_PIN1 11
#define ZX_BEEP_PIN 9
#define WORK_LED_PIN PICO_DEFAULT_LED_PIN

// VGA 8 pins starts from pin:
#define VGA_BASE_PIN 12

// HDMI 8 pins starts from pin:
#define HDMI_BASE_PIN 12

// TFT
#define TFT_CS_PIN 12
#define TFT_RST_PIN 14
#define TFT_LED_PIN 15
#define TFT_DC_PIN 16
#define TFT_DATA_PIN 18
#define TFT_CLK_PIN 19

#define SMS_SINGLE_FILE 1

// Sound
#if defined(AUDIO_PWM)
#define AUDIO_PWM_PIN 9
#define AUDIO_DATA_PIN 9
#define AUDIO_CLOCK_PIN 10
#else
#define AUDIO_DATA_PIN 9
#define AUDIO_CLOCK_PIN 10
#endif