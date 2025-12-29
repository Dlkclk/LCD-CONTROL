/**
 * @file ui_custom.c
 * @brief Custom UI extensions for Screen7 and Screen8
 */

#include "ui_custom.h"
#include "../../LVGL_Library/ui/ui.h"
#include "../../LVGL_Library/ui/ui_events.h"
#include "../../LVGL_Library/ui/screens/ui_Screen7.h"
#include "../../LVGL_Library/ui/screens/ui_Screen8.h"
#include "main.h"
#include "FreeRTOS.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* External UART handle */
extern UART_HandleTypeDef huart5;

/* UART transmit timer handle */
static TimerHandle_t uart_transmit_timer = NULL;

/* UART transmit state */
static volatile uint8_t uart_transmit_active = 0;

/**
 * @brief Custom initialization for Screen7 (UART receive screen)
 * @note Called after ui_init() to setup additional UI elements
 */
void ui_custom_Screen7_init(void)
{
    // Check if ui_tareceive exists
    if (ui_tareceive == NULL) {
        printf("[WARNING] ui_tareceive not found in Screen7\r\n");
        return;
    }

    // Configure text area for UART data display
    lv_textarea_set_text(ui_tareceive, "");  // Clear initial text
    lv_textarea_set_cursor_click_pos(ui_tareceive, true);
    
    printf("[INFO] Screen7 custom init complete\r\n");
}

/**
 * @brief Custom initialization for Screen8 (UART transmit/settings screen)
 * @note Called after ui_init() to setup additional UI elements
 */
void ui_custom_Screen8_init(void)
{
    // Check if ui_tanmea and ui_tainterval exist
    if (ui_tanmea == NULL || ui_tainterval == NULL) {
        printf("[WARNING] Screen8 text areas not found\r\n");
        return;
    }

    // Configure text areas with default values
    lv_textarea_set_placeholder_text(ui_tanmea, "NMEA sentence");
    lv_textarea_set_placeholder_text(ui_tainterval, "1");
    
    printf("[INFO] Screen8 custom init complete\r\n");
}

/**
 * @brief FreeRTOS timer callback for periodic UART transmission
 * @param xTimer Timer handle
 */
static void uart_transmit_timer_callback(TimerHandle_t xTimer)
{
    if (!uart_transmit_active || ui_tanmea == NULL) {
        return;
    }

    // Get NMEA sentence from text area
    const char *nmea_text = lv_textarea_get_text(ui_tanmea);
    if (nmea_text == NULL || strlen(nmea_text) == 0) {
        return;
    }

    // Send NMEA sentence via UART
    uint16_t len = strlen(nmea_text);
    HAL_UART_Transmit(&huart5, (uint8_t *)nmea_text, len, 100);
    
    // Add newline if not present
    if (nmea_text[len - 1] != '\n' && nmea_text[len - 1] != '\r') {
        HAL_UART_Transmit(&huart5, (uint8_t *)"\r\n", 2, 100);
    }
}

/**
 * @brief Start UART transmission event handler
 * @param e LVGL event pointer
 * @note Reads NMEA sentence and interval from Screen8 text areas
 */
void start_transmit_event(lv_event_t * e)
{
    if (ui_tanmea == NULL || ui_tainterval == NULL) {
        printf("[ERROR] Screen8 text areas not available\r\n");
        return;
    }

    // Stop any existing transmission
    if (uart_transmit_timer != NULL) {
        stop_transmit_event(e);
    }

    // Get interval from text area (in seconds)
    const char *interval_text = lv_textarea_get_text(ui_tainterval);
    if (interval_text == NULL || strlen(interval_text) == 0) {
        printf("[ERROR] Interval not set\r\n");
        return;
    }

    uint32_t interval_sec = (uint32_t)atoi(interval_text);
    uint32_t interval_ms;
    if (interval_sec > 0 && interval_sec <= 3600) { // Max 1 hour
        interval_ms = interval_sec * 1000; // Convert seconds to milliseconds
    } else {
        interval_ms = 1000; // Default to 1 second
    }

    // Get NMEA sentence
    const char *nmea_text = lv_textarea_get_text(ui_tanmea);
    if (nmea_text == NULL || strlen(nmea_text) == 0) {
        printf("[ERROR] NMEA sentence is empty\r\n");
        return;
    }

    // Create FreeRTOS timer for periodic transmission
    uart_transmit_timer = xTimerCreate(
        "UART_TX_Timer",
        pdMS_TO_TICKS(interval_ms),
        pdTRUE,  // Auto-reload
        NULL,
        uart_transmit_timer_callback
    );

    if (uart_transmit_timer == NULL) {
        printf("[ERROR] Failed to create UART transmit timer\r\n");
        return;
    }

    // Start transmission
    uart_transmit_active = 1;
    if (xTimerStart(uart_transmit_timer, 0) != pdPASS) {
        printf("[ERROR] Failed to start UART transmit timer\r\n");
        uart_transmit_active = 0;
        xTimerDelete(uart_transmit_timer, 0);
        uart_transmit_timer = NULL;
        return;
    }

    printf("[INFO] UART transmission started (interval: %lu ms)\r\n", interval_ms);
}

/**
 * @brief Stop UART transmission event handler
 * @param e LVGL event pointer
 */
void stop_transmit_event(lv_event_t * e)
{
    (void)e; // Unused parameter

    if (uart_transmit_timer == NULL) {
        return;
    }

    // Stop timer
    uart_transmit_active = 0;
    xTimerStop(uart_transmit_timer, portMAX_DELAY);
    xTimerDelete(uart_transmit_timer, portMAX_DELAY);
    uart_transmit_timer = NULL;

    printf("[INFO] UART transmission stopped\r\n");
}
