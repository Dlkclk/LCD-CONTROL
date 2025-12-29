/*
 * ILI9488.c
 * Optimized driver for ILI9488 TFT LCD with LVGL support
 * Combined and optimized version
 */

// Proje yapınıza göre bu path'i düzenleyin
#include "DISPLAY_INC/ili9488.h"
// Font header - eğer farklı yerdeyse path'i düzenle
#include "DISPLAY_INC/font.h"

/* ======================== LVGL INCLUDES ======================== */
#ifdef USE_LVGL
#include "../../../LVGL_Library/lvgl/lvgl.h"
#endif

/* ======================== GLOBAL VARIABLES ======================== */
static uint8_t v_buffer[V_BUFFER_SIZE];
static uint16_t buffer_counter = 0;
static uint16_t lcd_width = WIDTH;
static uint16_t lcd_height = HEIGHT;
static uint8_t rotationNum = 1;

/* Terminal variables */
static int16_t term_x = 0;
static int16_t term_y = 0;
#define FONT_W   6
#define FONT_H   8
#define TXT_SIZE 2
#define FG_COLOR COLOR_WHITE
#define BG_COLOR COLOR_BLACK

/* ======================== LOW-LEVEL SPI FUNCTIONS ======================== */

void spi_write(uint8_t data)
{
    HAL_SPI_Transmit(&hspi4, &data, 1, HAL_MAX_DELAY);
}

void write_buffer_blocking(uint8_t *buffer, uint16_t size)
{
    HAL_SPI_Transmit(&hspi4, buffer, size, HAL_MAX_DELAY);
}

/* ======================== LCD COMMAND/DATA FUNCTIONS ======================== */

void lcd_write_command(uint8_t cmd)
{
    DC_COMMAND();
    CS_A();
    spi_write(cmd);
    CS_D();
    // Small delay to ensure command is processed
    // osDelay(1); // Uncomment if needed
}

void lcd_write_data(uint8_t data)
{
    DC_DATA();
    CS_A();
    spi_write(data);
    CS_D();
}

void lcd_write_data_multi(uint8_t *buff, size_t buff_size)
{
    DC_DATA();
    CS_A();

    while (buff_size > 0)
    {
        uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;
        HAL_SPI_Transmit(&hspi4, buff, chunk_size, HAL_MAX_DELAY);
        buff += chunk_size;
        buff_size -= chunk_size;
    }

    CS_D();
}

/* ======================== COLOR CONVERSION FUNCTIONS ======================== */

void rgb565_to_rgb888(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = ((color >> 11) & 0x1F) * 255 / 31;
    *g = ((color >> 5)  & 0x3F) * 255 / 63;
    *b = (color & 0x1F) * 255 / 31;
}

uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void write16BitColor(uint16_t color)
{
    uint8_t r, g, b;
    rgb565_to_rgb888(color, &r, &g, &b);

    uint8_t px[3] = { r, g, b };
    lcd_write_data_multi(px, 3);
}

/* ======================== LCD INITIALIZATION ======================== */

void lcd_init_command_list(void)
{
    // Positive Gamma Control
    lcd_write_command(0xE0);
    lcd_write_data(0x00);
    lcd_write_data(0x03);
    lcd_write_data(0x09);
    lcd_write_data(0x08);
    lcd_write_data(0x16);
    lcd_write_data(0x0A);
    lcd_write_data(0x3F);
    lcd_write_data(0x78);
    lcd_write_data(0x4C);
    lcd_write_data(0x09);
    lcd_write_data(0x0A);
    lcd_write_data(0x08);
    lcd_write_data(0x16);
    lcd_write_data(0x1A);
    lcd_write_data(0x0F);

    // Negative Gamma Control
    lcd_write_command(0xE1);
    lcd_write_data(0x00);
    lcd_write_data(0x16);
    lcd_write_data(0x19);
    lcd_write_data(0x03);
    lcd_write_data(0x0F);
    lcd_write_data(0x05);
    lcd_write_data(0x32);
    lcd_write_data(0x45);
    lcd_write_data(0x46);
    lcd_write_data(0x04);
    lcd_write_data(0x0E);
    lcd_write_data(0x0D);
    lcd_write_data(0x35);
    lcd_write_data(0x37);
    lcd_write_data(0x0F);

    // Power Control 1
    lcd_write_command(0xC0);
    lcd_write_data(0x17);
    lcd_write_data(0x15);

    // Power Control 2
    lcd_write_command(0xC1);
    lcd_write_data(0x41);

    // VCOM Control
    lcd_write_command(0xC5);
    lcd_write_data(0x00);
    lcd_write_data(0x12);
    lcd_write_data(0x80);

    // Memory Access Control
    lcd_write_command(0x36);
#if LANDSCAPE
    lcd_write_data(MADCTL_MV | MADCTL_RGB);  // Landscape + RGB mode
#else
    lcd_write_data(MADCTL_MX | MADCTL_RGB);  // Portrait + RGB mode
#endif

    // Pixel Format Set (18-bit/pixel)
    lcd_write_command(0x3A);
    lcd_write_data(0x66);

    // Interface Mode Control
    lcd_write_command(0xB0);
    lcd_write_data(0x80);

    // Frame Rate Control
    lcd_write_command(0xB1);
    lcd_write_data(0xA0);

    // Display Inversion Control
    lcd_write_command(0xB4);
    lcd_write_data(0x02);

    // Display Function Control
    lcd_write_command(0xB6);
    lcd_write_data(0x02);
    lcd_write_data(0x02);

    // Set Image Function
    lcd_write_command(0xE9);
    lcd_write_data(0x00);

    // Adjust Control 3
    lcd_write_command(0xF7);
    lcd_write_data(0xA9);
    lcd_write_data(0x51);
    lcd_write_data(0x2C);
    lcd_write_data(0x82);

    // Exit Sleep
    lcd_write_command(ILI9488_SLPOUT);
    osDelay(120);

    // Invert Display (optional - some panels need this)
    lcd_write_command(0x21);
    osDelay(120);

    // Display ON
    lcd_write_command(ILI9488_DISPON);
}

void lcd_init(void)
{
    // Hardware Reset
    RST_A();
    osDelay(20);
    RST_D();
    osDelay(120);

    CS_D();
    osDelay(5);

    lcd_width  = ILI9488_TFTWIDTH;
    lcd_height = ILI9488_TFTHEIGHT;

    lcd_init_command_list();
}

/* ======================== ROTATION AND DISPLAY CONTROL ======================== */

void setRotation(uint8_t r)
{
    lcd_write_command(ILI9488_MADCTL);
    rotationNum = r % 4;

    switch (rotationNum) {
    case 0:
        lcd_write_data(MADCTL_MX | MADCTL_RGB);
        lcd_width = ILI9488_TFTWIDTH;
        lcd_height = ILI9488_TFTHEIGHT;
        break;
    case 1:
        lcd_write_data(MADCTL_MV | MADCTL_RGB);
        lcd_width = ILI9488_TFTHEIGHT;
        lcd_height = ILI9488_TFTWIDTH;
        break;
    case 2:
        lcd_write_data(MADCTL_MY | MADCTL_RGB);
        lcd_width = ILI9488_TFTWIDTH;
        lcd_height = ILI9488_TFTHEIGHT;
        break;
    case 3:
        lcd_write_data(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_RGB);
        lcd_width = ILI9488_TFTHEIGHT;
        lcd_height = ILI9488_TFTWIDTH;
        break;
    }
}

void invertDisplay(uint8_t i)
{
    lcd_write_command(i ? ILI9488_INVON : ILI9488_INVOFF);
}

/* ======================== WINDOW AND SCROLLING ======================== */

void set_draw_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    lcd_write_command(ILI9488_CASET);
    lcd_write_data(x1 >> 8);
    lcd_write_data(x1);
    lcd_write_data(x2 >> 8);
    lcd_write_data(x2);

    lcd_write_command(ILI9488_PASET);
    lcd_write_data(y1 >> 8);
    lcd_write_data(y1);
    lcd_write_data(y2 >> 8);
    lcd_write_data(y2);

    lcd_write_command(ILI9488_RAMWR);
}

void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    lcd_write_command(ILI9488_CASET);
    uint8_t data1[] = {(x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF};
    lcd_write_data_multi(data1, sizeof(data1));

    lcd_write_command(ILI9488_PASET);
    uint8_t data2[] = {(y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF};
    lcd_write_data_multi(data2, sizeof(data2));

    lcd_write_command(ILI9488_RAMWR);
}

void setScrollArea(uint16_t topFixedArea, uint16_t bottomFixedArea)
{
    lcd_write_command(0x33);
    lcd_write_data(topFixedArea >> 8);
    lcd_write_data(topFixedArea);
    lcd_write_data((lcd_height - topFixedArea - bottomFixedArea) >> 8);
    lcd_write_data(lcd_height - topFixedArea - bottomFixedArea);
    lcd_write_data(bottomFixedArea >> 8);
    lcd_write_data(bottomFixedArea);
}

void scroll(uint16_t pixels)
{
    lcd_write_command(0x37);
    lcd_write_data(pixels >> 8);
    lcd_write_data(pixels);
}

/* ======================== BASIC DRAWING FUNCTIONS ======================== */

void draw_pixel(uint16_t x, uint16_t y, uint16_t colour)
{
    uint8_t r, g, b;
    rgb565_to_rgb888(colour, &r, &g, &b);

    set_draw_window(x, y, x, y);

    DC_DATA();
    CS_A();

    uint8_t pixel[3] = {r, g, b};
    HAL_SPI_Transmit(&hspi4, pixel, 3, HAL_MAX_DELAY);

    CS_D();
}

void drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if ((x < 0) || (x >= lcd_width) || (y < 0) || (y >= lcd_height))
        return;

    draw_pixel(x, y, color);
}

void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    if ((x >= lcd_width) || (y >= lcd_height) || h == 0)
        return;

    if ((y + h - 1) >= lcd_height)
        h = lcd_height - y;

    uint8_t r, g, b;
    rgb565_to_rgb888(color, &r, &g, &b);

    setAddrWindow(x, y, x, y + h - 1);

    DC_DATA();
    CS_A();

    for (int16_t i = 0; i < h; i++) {
        uint8_t px[3] = {r, g, b};
        HAL_SPI_Transmit(&hspi4, px, 3, HAL_MAX_DELAY);
    }

    CS_D();
}

void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if ((x >= lcd_width) || (y >= lcd_height) || w == 0)
        return;

    if ((x + w - 1) >= lcd_width)
        w = lcd_width - x;

    uint8_t r, g, b;
    rgb565_to_rgb888(color, &r, &g, &b);

    setAddrWindow(x, y, x + w - 1, y);

    DC_DATA();
    CS_A();

    for (int16_t i = 0; i < w; i++) {
        uint8_t px[3] = {r, g, b};
        HAL_SPI_Transmit(&hspi4, px, 3, HAL_MAX_DELAY);
    }

    CS_D();
}

void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);

    if (steep) {
        int16_t temp;
        temp = x0; x0 = y0; y0 = temp;
        temp = x1; x1 = y1; y1 = temp;
    }

    if (x0 > x1) {
        int16_t temp;
        temp = x0; x0 = x1; x1 = temp;
        temp = y0; y0 = y1; y1 = temp;
    }

    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++) {
        if (steep) {
            drawPixel(y0, x0, color);
        } else {
            drawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    if (x0 == x1) {
        if (y0 > y1) {
            int16_t temp = y0; y0 = y1; y1 = temp;
        }
        drawFastVLine(x0, y0, y1 - y0 + 1, color);
    } else if (y0 == y1) {
        if (x0 > x1) {
            int16_t temp = x0; x0 = x1; x1 = temp;
        }
        drawFastHLine(x0, y0, x1 - x0 + 1, color);
    } else {
        writeLine(x0, y0, x1, y1, color);
    }
}

/* ======================== SHAPE DRAWING FUNCTIONS ======================== */

void fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t colour)
{
    uint8_t r, g, b;
    rgb565_to_rgb888(colour, &r, &g, &b);

    set_draw_window(x1, y1, x2 - 1, y2 - 1);

    DC_DATA();
    CS_A();

    uint32_t total_pixels = (x2 - x1) * (y2 - y1);
    uint32_t chunk_pixels = V_BUFFER_SIZE / 3;

    while (total_pixels)
    {
        uint32_t send_now = (total_pixels > chunk_pixels) ? chunk_pixels : total_pixels;
        uint16_t idx = 0;

        for (uint32_t i = 0; i < send_now; i++)
        {
            v_buffer[idx++] = r;
            v_buffer[idx++] = g;
            v_buffer[idx++] = b;
        }

        HAL_SPI_Transmit(&hspi4, v_buffer, idx, HAL_MAX_DELAY);
        total_pixels -= send_now;
    }

    CS_D();
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    // MUTEX LOCK
    if (lcdMutexHandle != NULL) {
        osMutexAcquire(lcdMutexHandle, osWaitForever);
    }

    if ((x >= lcd_width) || (y >= lcd_height)) {
        if (lcdMutexHandle != NULL) osMutexRelease(lcdMutexHandle);
        return;
    }

    if ((x + w - 1) >= lcd_width)
        w = lcd_width - x;
    if ((y + h - 1) >= lcd_height)
        h = lcd_height - y;

    uint8_t r, g, b;
    rgb565_to_rgb888(color, &r, &g, &b);

    #define CHUNK_PIXELS 256
    #define CHUNK_SIZE (CHUNK_PIXELS * 3)
    static uint8_t frm_buf[CHUNK_SIZE];

    for (uint32_t i = 0; i < CHUNK_PIXELS; i++)
    {
        frm_buf[i*3]   = r;
        frm_buf[i*3+1] = g;
        frm_buf[i*3+2] = b;
    }

    setAddrWindow(x, y, x + w - 1, y + h - 1);

    uint32_t total_pixels = (uint32_t)w * h;
    uint32_t remaining_pixels = total_pixels;

    DC_DATA();
    CS_A();

    while (remaining_pixels > 0)
    {
        uint32_t pixels_to_send = (remaining_pixels > CHUNK_PIXELS) ? CHUNK_PIXELS : remaining_pixels;
        uint32_t bytes_to_send = pixels_to_send * 3;

        HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi4, frm_buf, bytes_to_send, 1000);

        if (status != HAL_OK) {
            break;
        }

        remaining_pixels -= pixels_to_send;
    }

    CS_D();

    // MUTEX UNLOCK
    if (lcdMutexHandle != NULL) osMutexRelease(lcdMutexHandle);
}

void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    if (lcdMutexHandle) osMutexAcquire(lcdMutexHandle, osWaitForever);

    drawFastVLine(x0, y0 - r, 2 * r + 1, color);

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawFastVLine(x0 + x, y0 - y, 2 * y + 1, color);
        drawFastVLine(x0 + y, y0 - x, 2 * x + 1, color);
        drawFastVLine(x0 - x, y0 - y, 2 * y + 1, color);
        drawFastVLine(x0 - y, y0 - x, 2 * x + 1, color);
    }

    if (lcdMutexHandle) osMutexRelease(lcdMutexHandle);
}

void fillScreen(uint16_t color)
{
    fillRect(0, 0, lcd_width, lcd_height, color);
}

void clear_screen(uint16_t color)
{
    fillScreen(color);
}

/* ======================== TEXT DRAWING FUNCTIONS ======================== */

void draw_fast_char(uint16_t x, uint16_t y, char c, uint16_t colour, uint16_t bg)
{
    uint8_t width = 8;
    uint8_t height = 13;
    uint8_t line;
    uint16_t this_px;
    uint16_t idx = 0;

    // Pre-calculate colors
    uint8_t fg_r, fg_g, fg_b;
    uint8_t bg_r, bg_g, bg_b;
    rgb565_to_rgb888(colour, &fg_r, &fg_g, &fg_b);
    rgb565_to_rgb888(bg, &bg_r, &bg_g, &bg_b);

    set_draw_window(x, y, x + width - 1, y + height - 1);

    DC_DATA();
    CS_A();

    for (uint8_t i = 0; i < height; i++)
    {
        line = FontLarge[c - 32][12 - i];

        for (int8_t j = 7; j >= 0; j--)
        {
            if (line & (1 << j)) {
                v_buffer[idx++] = fg_r;
                v_buffer[idx++] = fg_g;
                v_buffer[idx++] = fg_b;
            } else {
                v_buffer[idx++] = bg_r;
                v_buffer[idx++] = bg_g;
                v_buffer[idx++] = bg_b;
            }

            if (idx >= V_BUFFER_SIZE - 3)
            {
                HAL_SPI_Transmit(&hspi4, v_buffer, idx, HAL_MAX_DELAY);
                idx = 0;
            }
        }
    }

    if (idx)
        HAL_SPI_Transmit(&hspi4, v_buffer, idx, HAL_MAX_DELAY);

    CS_D();
}

void draw_string(unsigned int x, unsigned int y, unsigned int colour, char size, char *str)
{
    unsigned int offset = size * 6;

    for (uint16_t i = 0; i < 40 && str[i] != '\0'; i++)
    {
        draw_fast_char(x + (offset * i), y, str[i], colour, COLOR_BLACK);
    }
}

void draw_fast_string(unsigned int x, unsigned int y, unsigned int colour, unsigned int bg_colour, char *str)
{
    unsigned int offset = 6;

    for (uint16_t i = 0; i < 40 && str[i] != '\0'; i++)
    {
        draw_fast_char(x + (offset * i), y, str[i], colour, bg_colour);
    }
}

void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size)
{
    if (rotationNum == 1 || rotationNum == 3) {
        if ((x >= ILI9488_TFTHEIGHT) || (y >= ILI9488_TFTWIDTH) ||
            ((x + 6 * size - 1) < 0) || ((y + 8 * size - 1) < 0))
            return;
    } else {
        if ((y >= ILI9488_TFTHEIGHT) || (x >= ILI9488_TFTWIDTH) ||
            ((y + 6 * size - 1) < 0) || ((x + 8 * size - 1) < 0))
            return;
    }

    // Use simple font from your code
    const unsigned char font1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, // Space
        0x00, 0x00, 0x5F, 0x00, 0x00, // !
        // ... (include full font array or reference external font)
    };

    for (int8_t i = 0; i < 6; i++) {
        uint8_t line;
        if (i == 5)
            line = 0x0;
        else
            line = FontLarge[c - 32][i];

        for (int8_t j = 0; j < 8; j++) {
            if (line & 0x1) {
                if (size == 1)
                    drawPixel(x + i, y + j, color);
                else
                    fillRect(x + (i * size), y + (j * size), size, size, color);
            } else if (bg != color) {
                if (size == 1)
                    drawPixel(x + i, y + j, bg);
                else
                    fillRect(x + i * size, y + j * size, size, size, bg);
            }
            line >>= 1;
        }
    }
}

void ILI9488_printText(char text[], int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t size)
{
    int16_t offset = size * 6;

    for (uint16_t i = 0; i < 40 && text[i] != '\0'; i++)
    {
        drawChar(x + (offset * i), y, text[i], color, bg, size);
    }
}

void ILI9488_printText_fast(char *text, int16_t x, int16_t y, uint16_t fg, uint16_t bg, uint8_t size)
{
    uint16_t len = 0;
    while (text[len] && len < 40) len++;

    if (len) fillRect(x, y, len * 6 * size, 8 * size, bg);

    int16_t dx = size * 6;
    for (uint16_t i = 0; i < len; i++) {
        draw_fast_char(x + i * dx, y, (unsigned char)text[i], fg, bg);
    }
}

/* ======================== TERMINAL FUNCTIONS ======================== */

void lcd_clear(void)
{
    fillScreen(BG_COLOR);
    term_x = 0;
    term_y = 0;
}

static inline void lcd_newline(void)
{
    term_x = 0;
    term_y += (FONT_H * TXT_SIZE);
    if (term_y + (FONT_H * TXT_SIZE) > lcd_height) {
        lcd_clear();
    }
}

void lcd_putc(char c)
{
    if (c == '\r') return;
    if (c == '\n') {
        lcd_newline();
        return;
    }

    char buf[2] = { c, 0 };
    ILI9488_printText_fast(buf, term_x, term_y, FG_COLOR, BG_COLOR, TXT_SIZE);

    term_x += (FONT_W * TXT_SIZE);

    if (term_x + (FONT_W * TXT_SIZE) > lcd_width) {
        lcd_newline();
    }
}

/* ======================== IMAGE FUNCTIONS ======================== */

void drawImage(const uint8_t* img, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if ((x >= lcd_width) || (y >= lcd_height))
        return;
    if ((x + w - 1) >= lcd_width)
        w = lcd_width - x;
    if ((y + h - 1) >= lcd_height)
        h = lcd_height - y;

    setAddrWindow(x, y, x + w - 1, y + h - 1);

    DC_DATA();
    CS_A();

    uint8_t linebuff[w * 3 + 1];
    uint32_t count = 0;

    for (uint16_t i = 0; i < h; i++) {
        uint16_t pixcount = 0;
        for (uint16_t o = 0; o < w; o++) {
            uint8_t b1 = img[count++];
            uint8_t b2 = img[count++];
            uint16_t color = b1 << 8 | b2;

            uint8_t r, g, b;
            rgb565_to_rgb888(color, &r, &g, &b);

            linebuff[pixcount++] = r;
            linebuff[pixcount++] = g;
            linebuff[pixcount++] = b;
        }
        HAL_SPI_Transmit(&hspi4, linebuff, w * 3, 100);
    }

    CS_D();
}

/* ======================== TEST FUNCTIONS ======================== */

void testLines(uint8_t color)
{
    int x1, y1, x2, y2, w = lcd_width, h = lcd_height;
    fillScreen(ILI9488_BLACK);

    x1 = y1 = 0;
    y2 = h - 1;

    for (x2 = 0; x2 < w; x2 += 6)
        drawLine(x1, y1, x2, y2, color);
    x2 = w - 1;
    for (y2 = 0; y2 < h; y2 += 6)
        drawLine(x1, y1, x2, y2, color);

    osDelay(500);

    fillScreen(ILI9488_BLACK);

    x1 = w - 1;
    y1 = 0;
    y2 = h - 1;

    for (x2 = 0; x2 < w; x2 += 6)
        drawLine(x1, y1, x2, y2, color);
    x2 = 0;
    for (y2 = 0; y2 < h; y2 += 6)
        drawLine(x1, y1, x2, y2, color);

    osDelay(500);
}

static void demo_fill_colors(void)
{
    uint16_t colors[] = {
        ILI9488_BLACK, ILI9488_BLUE, ILI9488_GREEN, ILI9488_RED,
        ILI9488_DARKGREY, ILI9488_GREY, ILI9488_ORANGE, ILI9488_YELLOW,
        ILI9488_PINK, ILI9488_PURPLE, ILI9488_LIGHTGREEN
    };

    for (uint32_t i = 0; i < sizeof(colors) / 2; i++) {
        fillScreen(colors[i]);
        osDelay(250);
    }
}

static void demo_lines(void)
{
    fillScreen(ILI9488_BLACK);

    for (int x = 0; x < lcd_width; x += 6)
        drawFastVLine(x, 0, lcd_height, ILI9488_RED);
    for (int y = 0; y < lcd_height; y += 6)
        drawFastHLine(0, y, lcd_width, ILI9488_GREEN);

    osDelay(500);
}

static void demo_rects(void)
{
    fillScreen(ILI9488_BLACK);
    int cx = lcd_width / 2, cy = lcd_height / 2;
    int max_r = (lcd_width < lcd_height ? lcd_width : lcd_height) / 2;

    for (int r = 10; r < max_r; r += 12) {
        int x = cx - r, y = cy - r, w = r * 2, h = r * 2;
        uint16_t c = (r & 0x20) ? ILI9488_ORANGE : ILI9488_PINK;
        fillRect(x, y, w, h, c);
        osDelay(20);
    }
    osDelay(400);
}

static void demo_texts(void)
{
    fillScreen(ILI9488_DARKBLUE);

    ILI9488_printText("HELLO AVATEC!", 10, 10, ILI9488_BLACK, ILI9488_DARKBLUE, 2);
    ILI9488_printText("Standard Text", 10, 40, ILI9488_GREEN, ILI9488_DARKBLUE, 1);
    ILI9488_printText_fast("Fast line 1", 10, 70, ILI9488_YELLOW, ILI9488_DARKBLUE, 1);
    ILI9488_printText_fast("Fast BIG", 10, 95, ILI9488_RED, ILI9488_DARKBLUE, 2);
    ILI9488_printText_fast("Fast HUGE", 10, 135, ILI9488_BLACK, ILI9488_DARKBLUE, 3);

    osDelay(900);
}

void ILI9488_Test_All_Auto(void)
{
    for (;;) {
        demo_fill_colors();
        demo_lines();
        demo_rects();
        demo_texts();
    }
}

/* ======================== LVGL INTEGRATION ======================== */

#ifdef USE_LVGL

#define DISP_VER_RES 480
#define DISP_HOR_RES 320
#define MAX(a,b) ((a)>(b) ? (a):(b))

static lv_disp_drv_t disp_drv;
static lv_color_t disp_buf1[DISP_HOR_RES * DISP_VER_RES / 10];
static lv_color_t disp_buf2[DISP_HOR_RES * DISP_VER_RES / 10];

static volatile uint32_t t_saved = 0;

void monitor_cb(lv_disp_drv_t *d, uint32_t t, uint32_t p)
{
    t_saved = t;
}

void tft_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    static uint32_t flush_count = 0;
    flush_count++;
    
    // Debug: Print first few flushes to verify LVGL is working
    if (flush_count <= 5) {
        printf("[LVGL] tft_flush #%lu: Area (%d,%d) to (%d,%d)\r\n", 
               flush_count, area->x1, area->y1, area->x2, area->y2);
    }
    
    // Check bounds
    if (area->x2 < 0 || area->y2 < 0 ||
        area->x1 >= DISP_HOR_RES || area->y1 >= DISP_VER_RES)
    {
        if (flush_count <= 5) {
            printf("[LVGL] tft_flush: Out of bounds, skipping\r\n");
        }
        lv_disp_flush_ready(disp);
        return;
    }

    uint16_t x1 = (area->x1 < 0) ? 0 : area->x1;
    uint16_t y1 = (area->y1 < 0) ? 0 : area->y1;
    uint16_t x2 = (area->x2 >= DISP_HOR_RES) ? DISP_HOR_RES - 1 : area->x2;
    uint16_t y2 = (area->y2 >= DISP_VER_RES) ? DISP_VER_RES - 1 : area->y2;

    if (flush_count <= 5) {
        printf("[LVGL] tft_flush: Clipped to (%d,%d) to (%d,%d), pixels: %lu\r\n",
               x1, y1, x2, y2, (uint32_t)(x2-x1+1)*(y2-y1+1));
    }

    // Set window
    setAddrWindow(x1, y1, x2, y2);

    DC_DATA();
    CS_A();

    uint32_t width = x2 - x1 + 1;
    uint32_t height = y2 - y1 + 1;
    uint32_t total_pixels = width * height;

    #define BURST_PIXELS 256
    static uint8_t tx_buf[BURST_PIXELS * 3];

    while (total_pixels > 0)
    {
        uint32_t pixels_to_send = (total_pixels > BURST_PIXELS) ? BURST_PIXELS : total_pixels;
        uint32_t idx = 0;

        for (uint32_t i = 0; i < pixels_to_send; i++)
        {
            lv_color_t color = *color_p++;

            uint8_t r = (uint8_t)((LV_COLOR_GET_R(color) * 255 + 15) / 31);
            uint8_t g = (uint8_t)((LV_COLOR_GET_G(color) * 255 + 31) / 63);
            uint8_t b = (uint8_t)((LV_COLOR_GET_B(color) * 255 + 15) / 31);

            /* ILI9488 expects B, then G, then R */
            tx_buf[idx++] = b;
            tx_buf[idx++] = g;
            tx_buf[idx++] = r;
        }

        HAL_SPI_Transmit(&hspi4, tx_buf, pixels_to_send * 3, HAL_MAX_DELAY);
        total_pixels -= pixels_to_send;
    }

    CS_D();

    if (flush_count <= 5) {
        printf("[LVGL] tft_flush #%lu: Completed successfully\r\n", flush_count);
    }

    lv_disp_flush_ready(disp);
}

void Display_init(int rotation)
{
    static lv_disp_draw_buf_t buf;

    lv_disp_draw_buf_init(&buf, disp_buf1, disp_buf2, DISP_HOR_RES * DISP_VER_RES / 10);
    
    lv_disp_drv_init(&disp_drv);

    lcd_init();
    
    setRotation(rotation);

    disp_drv.draw_buf = &buf;
    disp_drv.flush_cb = tft_flush;
    disp_drv.monitor_cb = monitor_cb;
    disp_drv.hor_res = DISP_HOR_RES;
    disp_drv.ver_res = DISP_VER_RES;
    disp_drv.rotated = rotation;

    lv_disp_drv_register(&disp_drv);
}

#endif /* USE_LVGL */
