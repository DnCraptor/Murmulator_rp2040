#pragma once
#include <inttypes.h>
#include <pico.h>

#ifndef D_JOY1_DATA_PIN
#ifdef NES_GPIO_DATA1
#define D_JOY1_DATA_PIN (NES_GPIO_DATA1)
#elif defined(NES_GPIO_DATA)
#define D_JOY1_DATA_PIN (NES_GPIO_DATA)
#else
#define D_JOY1_DATA_PIN (16)
#endif
#endif

#ifndef D_JOY2_DATA_PIN
#ifdef NES_GPIO_DATA2
#define D_JOY2_DATA_PIN (NES_GPIO_DATA2)
#elif defined(NES_GPIO_DATA)
#define D_JOY2_DATA_PIN (NES_GPIO_DATA)
#else
#define D_JOY2_DATA_PIN (17)
#endif
#endif

#ifndef D_JOY_CLK_PIN
#ifdef NES_GPIO_CLK
#define D_JOY_CLK_PIN (NES_GPIO_CLK)
#else
#define D_JOY_CLK_PIN (14)
#endif
#endif

#ifndef D_JOY_LATCH_PIN
#ifdef NES_GPIO_LAT
#define D_JOY_LATCH_PIN (NES_GPIO_LAT)
#else
#define D_JOY_LATCH_PIN (15)
#endif
#endif

#define QNT_IMP_NES (13)                // количество импульсов чтения джойстика NES

uint32_t d_joy_get_data();
bool decode_joy();
void d_joy_init();