/**
 * @file lv_conf.h
 * Custom configuration for LVGL v8.3.x optimized for STM32H7 + ILI9488 SPI + FreeRTOS
 */

#if 1 /*Enable LVGL configuration content*/

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#define LV_COLOR_SCREEN_TRANSP 0
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)

/*=========================
   MEMORY SETTINGS
 *=========================*/
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM == 1
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif

#define LV_MEM_BUF_MAX_NUM 16
#define LV_LAYER_SIMPLE_BUF_SIZE          (32 * 1024)
#define LV_LAYER_SIMPLE_FALLBACK_BUF_SIZE (4 * 1024)

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DISP_DEF_REFR_PERIOD 30
#define LV_INDEV_DEF_READ_PERIOD 5  // Çok hızlı touch okuma (5ms) - maksimum responsive için
#define LV_TICK_CUSTOM 0
#define LV_DPI_DEF 130

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/
#define LV_DRAW_COMPLEX 1
#define LV_USE_GPU_STM32_DMA2D 0
#define LV_USE_LOG 0
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_INFO
    #define LV_LOG_PRINTF 1
#endif

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_OBJ           0

#define LV_USE_USER_DATA 1
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

/*==================
 *   FONT SETTINGS
 *==================*/
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*=================
 *  TEXT SETTINGS
 *=================*/
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " ,.;:-_"
#define LV_USE_BIDI 0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*==================
 *  WIDGET USAGE
 *==================*/
#define LV_USE_BTN        1
#define LV_USE_LABEL      1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_BAR        1
#define LV_USE_IMG        1
#define LV_USE_CHECKBOX   1
#define LV_USE_TABLE      1
#define LV_USE_TEXTAREA   1
#define LV_USE_MSGBOX     1
#define LV_USE_TABVIEW    1
#define LV_USE_METER      1
#define LV_USE_LED        1

/*==================
 *  EXTRA COMPONENTS
 *==================*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK 0
    #define LV_THEME_DEFAULT_GROW 1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif

#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*==================
 *  FILE SYSTEM / IMAGE DECODERS
 *==================*/
#define LV_USE_PNG 1
#define LV_USE_BMP 1
#define LV_USE_GIF 0
#define LV_USE_SJPG 0

/*==================
 *  DEMOS / EXAMPLES
 *==================*/
#define LV_BUILD_EXAMPLES 0
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_MUSIC 0

#endif /*LV_CONF_H*/
#endif /*End of content enable*/
