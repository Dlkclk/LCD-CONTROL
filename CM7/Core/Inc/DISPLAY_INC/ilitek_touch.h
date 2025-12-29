/**
 * @file ilitek_touch.h
 * @brief ILITEK Touch Controller Driver Header File with LVGL Integration
 * @details This driver supports ILITEK ILI2XXX series capacitive touch controllers
 *          communicating over I2C interface. Compatible with ILI2130/2131/2132/2316/2322/2323/2326/2520/2521
 *          Includes LVGL input device support for seamless GUI integration
 * @author Dilek Celik
 * @date 2025
 * @version 2.0 - LVGL Integration
 */

#ifndef ILITEK_TOUCH_H
#define ILITEK_TOUCH_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../LVGL_Library/lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================== CONSTANTS & MACROS ======================== */

/**
 * @brief Touch screen resolution definitions
 * @note Modify these values according to your display resolution
 * @note Must match display resolution: DISP_HOR_RES x DISP_VER_RES (320x480)
 */
#define TOUCH_SCREEN_WIDTH   320   // Display width (portrait mode)
#define TOUCH_SCREEN_HEIGHT  480   // Display height (portrait mode)

/**
 * @brief ILITEK I2C Communication Parameters
 */
#define ILITEK_I2C_ADDR_7BIT    0x41   /**< 7-bit I2C address */
#define ILITEK_I2C_ADDR_8BIT    0x82   /**< 8-bit I2C address for HAL */
#define ILITEK_REPORT_ID        0x48   /**< Touch report identifier */
#define ILITEK_MAX_TOUCH_POINTS 10     /**< Maximum supported touch points */

/**
 * @brief Touch indicator colors (RGB565 format)
 */
#define INDICATOR_COLOR_TOUCH          ILI9488_RED
#define INDICATOR_COLOR_BACKGROUND     ILI9488_BLUE

/* ======================== TYPE DEFINITIONS ======================== */

/**
 * @brief Touch point coordinate structure
 * @details Represents a single touch point with its coordinates and status
 */
typedef struct {
    uint16_t x;         /**< X coordinate (0 to TOUCH_SCREEN_WIDTH-1) */
    uint16_t y;         /**< Y coordinate (0 to TOUCH_SCREEN_HEIGHT-1) */
    uint8_t  id;        /**< Touch point ID (0-9 for multi-touch) */
    bool     pressed;   /**< Touch status: true = pressed, false = released */
} TouchPoint_t;

/**
 * @brief Touch controller information structure
 * @details Contains touch controller configuration and status
 */
typedef struct {
    uint16_t min_x;     /**< Minimum X coordinate from controller */
    uint16_t max_x;     /**< Maximum X coordinate from controller */
    uint16_t min_y;     /**< Minimum Y coordinate from controller */
    uint16_t max_y;     /**< Maximum Y coordinate from controller */
    uint8_t  max_points;/**< Maximum number of simultaneous touch points */
    bool     initialized;/**< Initialization status flag */
} TouchController_t;

/* ======================== LVGL INTEGRATION FUNCTIONS ======================== */

/**
 * @brief Initialize LVGL touch input device (PRIMARY INIT FUNCTION)
 * @details Initializes ILITEK hardware AND registers touch input with LVGL
 *          This is the main initialization function to use
 * @return 0 on success, negative error code on failure
 * @note Call this AFTER lv_init() and Display_init()
 *
 * @example
 * lv_init();
 * Display_init(LV_DISP_ROT_NONE);
 * Touch_LVGL_Init();  // <-- Use this
 */
int Touch_LVGL_Init(void);

/**
 * @brief LVGL touch input read callback
 * @param indev_drv Pointer to LVGL input device driver
 * @param data Pointer to LVGL input data structure (will be filled)
 * @note This function is called automatically by LVGL, don't call manually
 */
void touch_read_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

/**
 * @brief Get LVGL input device handle
 * @return Pointer to LVGL input device, NULL if not initialized
 * @note Use this to configure LVGL input device settings (cursor, etc.)
 */
lv_indev_t* Touch_GetLVGLDevice(void);

/**
 * @brief Test touch read manually (for debugging)
 * @details Call this function to test touch reading outside of LVGL callback
 * @note Useful for debugging touch hardware issues
 */
void Touch_TestManualRead(void);

/* ======================== HARDWARE FUNCTIONS ======================== */

/**
 * @brief Initialize the ILITEK touch controller hardware only
 * @details Performs hardware reset, reads controller capabilities,
 *          and configures touch resolution parameters
 * @return 0 on success, negative error code on failure
 * @note Usually called internally by Touch_LVGL_Init(), use that instead
 */
int Touch_ILITEK_Init(void);

/**
 * @brief Read current touch status from the controller
 * @param points Array to store touch point data
 * @param max_points Maximum number of points to read
 * @return Number of active touch points detected (0 if no touch)
 * @note Called internally by touch_read_cb(), but can be used standalone
 */
int Touch_ILITEK_Read(TouchPoint_t *points, uint8_t max_points);

/**
 * @brief Get raw touch data without coordinate mapping
 * @param buffer Buffer to store raw I2C data (minimum 64 bytes)
 * @return 0 on success, negative error code on failure
 * @note Useful for debugging and advanced touch processing
 */
int Touch_ILITEK_GetRawData(uint8_t *buffer);

/**
 * @brief Reset the touch controller hardware
 * @details Performs a hardware reset sequence using the RST pin
 * @param reset_time_ms Reset pulse duration in milliseconds (typical: 50ms)
 */
void Touch_ILITEK_Reset(uint32_t reset_time_ms);

/**
 * @brief Check if touch controller is responding
 * @return true if controller responds, false otherwise
 * @note Useful for diagnostic purposes
 */
bool Touch_ILITEK_IsAlive(void);

/**
 * @brief Get touch controller information
 * @return Pointer to TouchController_t structure with controller info
 * @note Returns NULL if controller is not initialized
 */
const TouchController_t* Touch_ILITEK_GetInfo(void);

/* ======================== VISUAL FEEDBACK FUNCTIONS ======================== */

/**
 * @brief Draw a visual indicator at touch position
 * @param x X coordinate of touch point
 * @param y Y coordinate of touch point
 * @param color Color of the indicator (RGB565 format)
 * @note This is an optional visual feedback function (can be disabled)
 */
void Touch_DrawIndicator(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief Update and clean expired touch indicators
 * @details Should be called periodically to remove old touch indicators
 * @note Call this from main loop or timer interrupt (10-50Hz recommended)
 */
void Touch_UpdateIndicators(void);

/**
 * @brief Force clear all touch indicators immediately
 * @details Removes all active touch indicators without waiting for expiration
 * @note Use this when touch is released to immediately clear all visual feedback
 */
void Touch_ClearAllIndicators(void);

/**
 * @brief Set background color for indicator clearing
 * @param color Background color in RGB565 format
 * @note Update this if your screen background color changes
 */
void Touch_SetBackgroundColor(uint16_t color);

/* ======================== TASK FUNCTION ======================== */

/**
 * @brief FreeRTOS task function for touch processing
 * @param params Task parameters (unused)
 * @note Create a FreeRTOS task with this function if using RTOS
 * @warning When using LVGL, this may not be needed as LVGL handles touch in lv_timer_handler()
 */
void Touch_TaskFunction(void *params);

/* ======================== DEPRECATED FUNCTIONS ======================== */

/**
 * @deprecated Use Touch_LVGL_Init() and LVGL widgets instead
 * @brief Legacy function for reading and drawing touch points
 */
void Touch_ReadAndDraw_Dot(void) __attribute__((deprecated("Use Touch_LVGL_Init() and LVGL widgets instead")));

/**
 * @deprecated Use Touch_UpdateIndicators() instead
 * @brief Legacy function for updating visual indicators
 */
void Blips_Tick(void) __attribute__((deprecated("Use Touch_UpdateIndicators() instead")));

#ifdef __cplusplus
}
#endif

#endif /* ILITEK_TOUCH_H */
