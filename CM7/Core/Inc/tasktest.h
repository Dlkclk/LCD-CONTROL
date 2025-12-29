/* tasktest.h - Task and Test Function Prototypes */

#ifndef TASKTEST_H
#define TASKTEST_H

#include "cmsis_os.h"
#include "stm32h7xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

/* Task Handles - Defined in freertos_custom.c */
extern TaskHandle_t TaskLOGHandle;
extern TaskHandle_t TouchTaskHandle;
extern TaskHandle_t DisplayTaskHandle;
extern TaskHandle_t EEPROMTaskHandle;

/* Task Functions - Implemented in tasktest.c */
void LogTaskHandler(void *params);
void Touch_TaskFunction(void *params);
void Display_TaskFunction(void *params);
void EEPROM_TestTask(void *params);
void UART_Receive_TaskFunction(void *params);

/* Test Functions - Implemented in tasktest.c */
void Test_SPI_Communication(void);
void QSPI_Flash_Test(void);
void QSPI_Custom_Data_Test(void);

/* Helper Functions - Implemented in tasktest.c */
void Generate_MAC_From_UID(uint8_t *mac);

#endif /* TASKTEST_H */
