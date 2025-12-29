/**
 * @file ui_custom.h
 * @brief Custom UI extensions for Screen7 and Screen8
 */

#ifndef UI_CUSTOM_H
#define UI_CUSTOM_H

#include "../../LVGL_Library/lvgl/lvgl.h"
#include "FreeRTOS.h"
#include "queue.h"

/* Include Screen7 and Screen8 headers for UI objects */
#include "../../LVGL_Library/ui/screens/ui_Screen7.h"
#include "../../LVGL_Library/ui/screens/ui_Screen8.h"

/* Custom initialization functions */
void ui_custom_Screen7_init(void);
void ui_custom_Screen8_init(void);

/* Queue handle for UART -> Screen7 data */
extern QueueHandle_t Screen7QueueHandle;

#endif /* UI_CUSTOM_H */
