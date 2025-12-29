/*
 * ILI9488.h
 * Optimized driver for ILI9488 TFT LCD with LVGL support
 * Combined version with all features
 */

#ifndef ILI9488_H
#define ILI9488_H

#include "stm32h7xx_hal.h"
#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "cmsis_os.h"

/* ======================== LVGL CONFIGURATION ======================== */
#ifndef USE_LVGL
#define USE_LVGL
#endif

#ifdef USE_LVGL
#include "../../../LVGL_Library/lvgl/lvgl.h"
#else
typedef struct _lv_disp_drv_t lv_disp_drv_t;
typedef struct _lv_area_t lv_area_t;
typedef union _lv_color_t lv_color_t;
#endif

/* ======================== EXTERNAL DEPENDENCIES ======================== */
extern SPI_HandleTypeDef hspi4;
extern osMutexId_t lcdMutexHandle;

/* ======================== LCD PIN CONTROL MACROS ======================== */
#define RST_A()      HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define RST_D()      HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)
#define CS_A()       HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define CS_D()       HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
#define DC_COMMAND() HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define DC_DATA()    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)

/* ======================== DISPLAY DIMENSIONS ======================== */
#define ILI9488_TFTWIDTH   320
#define ILI9488_TFTHEIGHT  480
#define ILI9488_PIXEL_COUNT (ILI9488_TFTWIDTH * ILI9488_TFTHEIGHT)

/* ======================== ORIENTATION SETTINGS ======================== */
#define LANDSCAPE 1  // 0 = Portrait, 1 = Landscape

#if LANDSCAPE
    #define WIDTH  ILI9488_TFTHEIGHT
    #define HEIGHT ILI9488_TFTWIDTH
#else
    #define WIDTH  ILI9488_TFTWIDTH
    #define HEIGHT ILI9488_TFTHEIGHT
#endif

/* ======================== ILI9488 COMMAND DEFINITIONS ======================== */
#define ILI9488_NOP        0x00
#define ILI9488_SWRESET    0x01
#define ILI9488_RDDID      0x04
#define ILI9488_RDDST      0x09

#define ILI9488_SLPIN      0x10
#define ILI9488_SLPOUT     0x11
#define ILI9488_PTLON      0x12
#define ILI9488_NORON      0x13

#define ILI9488_RDMODE     0x0A
#define ILI9488_RDMADCTL   0x0B
#define ILI9488_RDPIXFMT   0x0C
#define ILI9488_RDIMGFMT   0x0D
#define ILI9488_RDSELFDIAG 0x0F

#define ILI9488_INVOFF     0x20
#define ILI9488_INVON      0x21
#define ILI9488_GAMMASET   0x26
#define ILI9488_DISPOFF    0x28
#define ILI9488_DISPON     0x29

#define ILI9488_CASET      0x2A
#define ILI9488_PASET      0x2B
#define ILI9488_RAMWR      0x2C
#define ILI9488_RAMRD      0x2E

#define ILI9488_PTLAR      0x30
#define ILI9488_MADCTL     0x36
#define ILI9488_PIXFMT     0x3A

#define ILI9488_FRMCTR1    0xB1
#define ILI9488_FRMCTR2    0xB2
#define ILI9488_FRMCTR3    0xB3
#define ILI9488_INVCTR     0xB4
#define ILI9488_DFUNCTR    0xB6

#define ILI9488_PWCTR1     0xC0
#define ILI9488_PWCTR2     0xC1
#define ILI9488_PWCTR3     0xC2
#define ILI9488_PWCTR4     0xC3
#define ILI9488_PWCTR5     0xC4
#define ILI9488_VMCTR1     0xC5
#define ILI9488_VMCTR2     0xC7

#define ILI9488_RDID1      0xDA
#define ILI9488_RDID2      0xDB
#define ILI9488_RDID3      0xDC
#define ILI9488_RDID4      0xDD

#define ILI9488_GMCTRP1    0xE0
#define ILI9488_GMCTRN1    0xE1

/* ======================== MADCTL REGISTER BITS ======================== */
#define MADCTL_MY   0x80
#define MADCTL_MX   0x40
#define MADCTL_MV   0x20
#define MADCTL_ML   0x10
#define MADCTL_RGB  0x00
#define MADCTL_BGR  0x08
#define MADCTL_MH   0x04

/* ======================== COLOR DEFINITIONS (RGB565) ======================== */
#define COLOR_BLACK          0x0000
#define COLOR_WHITE          0xFFFF
#define COLOR_BLUE           0x001F
#define COLOR_GREEN          0x07E0
#define COLOR_RED            0xF800
#define COLOR_NAVY           0x000F
#define COLOR_DARKBLUE       0x0011
#define COLOR_DARKGREEN      0x03E0
#define COLOR_DARKCYAN       0x03EF
#define COLOR_CYAN           0x07FF
#define COLOR_TURQUOISE      0x471A
#define COLOR_INDIGO         0x4810
#define COLOR_DARKRED        0x8000
#define COLOR_OLIVE          0x7BE0
#define COLOR_GRAY           0x8410
#define COLOR_GREY           0x8410
#define COLOR_SKYBLUE        0x867D
#define COLOR_BLUEVIOLET     0x895C
#define COLOR_LIGHTGREEN     0x9772
#define COLOR_DARKVIOLET     0x901A
#define COLOR_YELLOWGREEN    0x9E66
#define COLOR_BROWN          0xA145
#define COLOR_DARKGRAY       0x7BEF
#define COLOR_DARKGREY       0x7BEF
#define COLOR_SIENNA         0xA285
#define COLOR_LIGHTBLUE      0xAEDC
#define COLOR_GREENYELLOW    0xAFE5
#define COLOR_SILVER         0xC618
#define COLOR_LIGHTGRAY      0xC618
#define COLOR_LIGHTGREY      0xC618
#define COLOR_LIGHTCYAN      0xE7FF
#define COLOR_VIOLET         0xEC1D
#define COLOR_AZUR           0xF7FF
#define COLOR_BEIGE          0xF7BB
#define COLOR_MAGENTA        0xF81F
#define COLOR_TOMATO         0xFB08
#define COLOR_GOLD           0xFEA0
#define COLOR_ORANGE         0xFD20
#define COLOR_SNOW           0xFFDF
#define COLOR_YELLOW         0xFFE0

/* Alternative color definitions mapped to accurate RGB565 values */
#define ILI9488_WHITE        COLOR_WHITE
#define ILI9488_BLACK        COLOR_BLACK
#define ILI9488_BLUE         COLOR_BLUE        /* Deep sea blue */
#define ILI9488_RED          COLOR_RED
#define ILI9488_GREEN        COLOR_GREEN
#define ILI9488_CYAN         COLOR_CYAN
#define ILI9488_MAGENTA      COLOR_MAGENTA
#define ILI9488_YELLOW       COLOR_YELLOW
#define ILI9488_ORANGE       0xFD20
#define ILI9488_GREY         COLOR_GRAY
#define ILI9488_DARKGREY     COLOR_DARKGRAY
#define ILI9488_LIGHTGREEN   COLOR_LIGHTGREEN
#define ILI9488_PURPLE       COLOR_BLUEVIOLET
#define ILI9488_PINK         COLOR_MAGENTA
#define ILI9488_DARKBLUE     COLOR_NAVY
#define ILI9488_DARKPURPLE   0x780F

/* ======================== BUFFER CONFIGURATION ======================== */
#define V_BUFFER_SIZE  1024

/* ======================== FUNCTION PROTOTYPES ======================== */

/* === Low-Level SPI Functions === */
void spi_write(uint8_t data);
void write_buffer_blocking(uint8_t *buffer, uint16_t size);

/* === LCD Command/Data Functions === */
void lcd_write_data(uint8_t data);
void lcd_write_command(uint8_t cmd);
void lcd_write_data_multi(uint8_t *buff, size_t buff_size);

/* === Initialization Functions === */
void lcd_init(void);
void lcd_init_command_list(void);
void setRotation(uint8_t r);
void invertDisplay(uint8_t i);

/* === Window & Address Functions === */
void set_draw_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/* === Scrolling Functions === */
void setScrollArea(uint16_t topFixedArea, uint16_t bottomFixedArea);
void scroll(uint16_t pixels);

/* === Basic Drawing Functions === */
void draw_pixel(uint16_t x, uint16_t y, uint16_t colour);
void drawPixel(int16_t x, int16_t y, uint16_t color);
void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

/* === Shape Drawing Functions === */
void fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t colour);
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void fillScreen(uint16_t color);
void clear_screen(uint16_t color);

/* === Text Drawing Functions === */
void draw_fast_char(uint16_t x, uint16_t y, char c, uint16_t colour, uint16_t bg);
void draw_char(unsigned int x, unsigned int y, char c, unsigned int colour, char size);
void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
void draw_string(unsigned int x, unsigned int y, unsigned int colour, char size, char *str);
void draw_fast_string(unsigned int x, unsigned int y, unsigned int colour, unsigned int bg_colour, char *str);
void ILI9488_printText(char text[], int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t size);
void ILI9488_printText_fast(char *text, int16_t x, int16_t y, uint16_t fg, uint16_t bg, uint8_t size);

/* === Terminal Functions === */
void lcd_putc(char c);
void lcd_clear(void);

/* === Color Functions === */
uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
void pushColor(uint16_t color);
void pushColors(uint16_t *data, uint8_t len, uint8_t first);
void write16BitColor(uint16_t color);

/* === Image Functions === */
void drawImage(const uint8_t* img, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void draw_bitmap(unsigned int x, unsigned int y, int scale, const unsigned int *bmp);

/* === Test Functions === */
void testLines(uint8_t color);
void ILI9488_Test_All_Auto(void);

/* === LVGL Integration === */
#ifdef USE_LVGL
void Display_init(int rotation);
#endif

/* === LVGL Flush Callback (declare always but implement only if USE_LVGL) === */
#ifdef USE_LVGL
void tft_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
#endif

/* === Utility Functions === */
void rgb565_to_rgb888(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b);

#endif /* ILI9488_H */
