/* freertos_custom.c - Task Creation with xTaskCreate */

#include "freertos_custom.h"
#include "tasktest.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

/* External handles */
extern UART_HandleTypeDef huart5;
extern SPI_HandleTypeDef hspi4;
extern uint16_t width;
extern uint16_t height;
extern QSPI_HandleTypeDef hqspi;
extern I2C_HandleTypeDef hi2c3;

/* Mutex Handles */
osMutexId_t lcdMutexHandle;
osMutexId_t touchMutexHandle;

/* Task Handles */
TaskHandle_t TaskLOGHandle = NULL;
TaskHandle_t TouchTaskHandle = NULL;
TaskHandle_t DisplayTaskHandle = NULL;
TaskHandle_t EEPROMTaskHandle = NULL;

/* Queue Handle - Defined externally in tasktest.c */
extern QueueHandle_t Screen7QueueHandle;

/* ============================================== */
/*  MAIN FREERTOS INITIALIZATION                 */
/* ============================================== */
void MainProcessFreertosInit(void)
{
    BaseType_t result;

    /* Create LCD Mutex for thread-safe display access */
    lcdMutexHandle = osMutexNew(NULL);
    if (lcdMutexHandle == NULL) {
        Error_Handler();
    }

    /* Create Touch Mutex */
    touchMutexHandle = osMutexNew(NULL);
    if (touchMutexHandle == NULL) {
        Error_Handler();
    }

    /* Create UART -> Screen7 queue */
    Screen7QueueHandle = xQueueCreate(512, sizeof(uint8_t));
    if (Screen7QueueHandle == NULL) {
        Error_Handler();
    }

    /* Display Init Task - High Priority (Priority 4) */
    result = xTaskCreate(Display_TaskFunction, "DisplayInit", 4096, NULL, 4, &DisplayTaskHandle);
    if (result != pdPASS) {
        Error_Handler();
    }

    /* Touch Task - Above Normal Priority (Priority 3) */
    result = xTaskCreate(Touch_TaskFunction, "TouchTask", 2048, NULL, 3, &TouchTaskHandle);
    if (result != pdPASS) {
        Error_Handler();
    }

    /* UART Receive Task for NMEA/Serial data - Priority 2 */
    result = xTaskCreate(UART_Receive_TaskFunction, "UARTReceive", 2048, NULL, 2, NULL);
    if (result != pdPASS) {
        Error_Handler();
    }

    /* EEPROM Test Task - Normal Priority (Priority 2) */
    result = xTaskCreate(EEPROM_TestTask, "EEPROMTest", 2048, NULL, 2, &EEPROMTaskHandle);
    if (result != pdPASS) {
        Error_Handler();
    }

    /* Log Task - Normal Priority (Priority 2) */
    result = xTaskCreate(LogTaskHandler, "LogTask", 1024, NULL, 2, &TaskLOGHandle);
    if (result != pdPASS) {
        Error_Handler();
    }
}

/* ============================================== */
/*  ETHERNET BACKEND INITIALIZATION              */
/* ============================================== */
void Ethernet_BackendInfrastructure_Init(void)
{
    printf("EthernetBackend_RegisterCGIHandlers init start\n");
    EthernetBackend_RegisterCGIHandlers();
    printf("EthernetBackend_RegisterCGIHandlers init end\n");
}
