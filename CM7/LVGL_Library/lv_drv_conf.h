/**
 * @file lv_drv_conf.h
 * Custom driver configuration for STM32H747 + ILI9488 (SPI4) + ILITEK (I2C4)
 */

#if 1 /*Set to "1" to enable content*/

#ifndef LV_DRV_CONF_H
#define LV_DRV_CONF_H

#include "lv_conf.h"

/*********************
 * DELAY INTERFACE
 *********************/
#define LV_DRV_DELAY_INCLUDE "main.h"
#define LV_DRV_DELAY_US(us)  /* if needed, use custom delay_us */
#define LV_DRV_DELAY_MS(ms)  HAL_Delay(ms)

/*********************
 * DISPLAY INTERFACE
 *********************/

/* Common control pins (to be defined in ili9488.c or your main) */
#define LV_DRV_DISP_CMD_DATA(val)  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, (val ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define LV_DRV_DISP_RST(val)       HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, (val ? GPIO_PIN_SET : GPIO_PIN_RESET))

/* SPI interface for display */
#define LV_DRV_DISP_SPI_CS(val)          HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, (val ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define LV_DRV_DISP_SPI_WR_BYTE(data)    HAL_SPI_Transmit(&hspi4, &(data), 1, HAL_MAX_DELAY)
#define LV_DRV_DISP_SPI_WR_ARRAY(adr, n) HAL_SPI_Transmit(&hspi4, (uint8_t*)(adr), (n), HAL_MAX_DELAY)

/*********************
 * INPUT DEVICE INTERFACE
 *********************/
#define LV_DRV_INDEV_INCLUDE "main.h"
#define LV_DRV_INDEV_RST(val)    HAL_GPIO_WritePin(TOUCH_RST_GPIO_Port, TOUCH_RST_Pin, (val ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define LV_DRV_INDEV_IRQ_READ    HAL_GPIO_ReadPin(TOUCH_INT_GPIO_Port, TOUCH_INT_Pin)

/* I2C interface for ILITEK touch controller (I2C4) */
#define LV_DRV_INDEV_I2C_START              /* optional if HAL handles it */
#define LV_DRV_INDEV_I2C_STOP               /* optional if HAL handles it */
#define LV_DRV_INDEV_I2C_WR(data)           HAL_I2C_Master_Transmit(&hi2c4, ILI_ADDR, (uint8_t *)&(data), 1, HAL_MAX_DELAY)
#define LV_DRV_INDEV_I2C_READ(last_read)    ({ uint8_t d; HAL_I2C_Master_Receive(&hi2c4, ILI_ADDR, &d, 1, HAL_MAX_DELAY); d; })

/*********************
 *  DISPLAY DRIVERS
 *********************/

/* Disable all PC and simulation drivers */
#define USE_MONITOR     0
#define USE_WINDOWS     0
#define USE_WIN32DRV    0
#define USE_GTK         0
#define USE_WAYLAND     0
#define USE_FBDEV       0
#define USE_BSD_FBDEV   0
#define USE_DRM         0

/* Custom ILI9488 SPI display */
#ifndef USE_ILI9488
#  define USE_ILI9488   1
#endif

#if USE_ILI9488
#  define ILI9488_HOR_RES  480
#  define ILI9488_VER_RES  320
#  define ILI9488_COLOR_DEPTH 16
#  define ILI9488_ORIENTATION 1 /* 1 = landscape, 0 = portrait */
#endif

/*********************
 *  INPUT DEVICES
 *********************/

/* ILITEK I2C TOUCH (custom) */
#ifndef USE_ILITEK_TOUCH
#  define USE_ILITEK_TOUCH  1
#endif

#if USE_ILITEK_TOUCH
#  define ILITEK_TOUCH_I2C_ADDRESS  (0x41 << 1)  /* change if needed */
#  define ILITEK_TOUCH_MAX_X        480
#  define ILITEK_TOUCH_MAX_Y        320
#  define ILITEK_TOUCH_SWAP_AXES    0
#  define ILITEK_TOUCH_INVERT_X     0
#  define ILITEK_TOUCH_INVERT_Y     0
#endif

/* Disable unused touch controllers */
#define USE_XPT2046    0
#define USE_FT5406EE8  0
#define USE_AD_TOUCH   0
#define USE_LIBINPUT   0
#define USE_EVDEV      0

#endif /*LV_DRV_CONF_H*/
#endif /*End of content enable*/
