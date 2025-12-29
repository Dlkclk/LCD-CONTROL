#ifndef INC_FREERTOS_CUSTOM_H_
#define INC_FREERTOS_CUSTOM_H_

#include "main.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>

#include "lwip/apps/httpd.h"
#include "lwipopts.h"
#include "lwip/tcpip.h"
#include "lan8742.h"
#include "stm32h7xx_hal_conf.h"
#include "lwip.h"

/* *** ETHERNET INCLUDES - EKLENEN *** */
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/ip_addr.h"
#include "ethernetif.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "timers.h"
#include "semphr.h"
#include "task.h"

/* ================= Project Includes ================= */
#include "../../../CM7/Core/Inc/GLOBAL_LIB_INC/cJSON.h"
#include "../../../CM7/Core/Inc/ETH_INC/ethernet_backend.h"
#include "../../../CM7/Core/Inc/BOOTLOADER_INC/bootloader.h"
#include "../../../CM7/Core/Inc/ExtFLASH_INC/sst26vf032b.h"
#include "DISPLAY_INC/ili9488.h"
#include "DISPLAY_INC/ilitek_touch.h"
#include "EEPROM_INC/eeprom_at24mac402.h"

/* ================= Global LCD Variables ================= */
extern uint16_t width;
extern uint16_t height;

/* ================= FreeRTOS Handles ================= */
extern osMutexId_t lcdMutexHandle;
extern osMutexId_t touchMutexHandle;

/* Task Handles - Using Native FreeRTOS API (xTaskCreate) */
extern TaskHandle_t TaskLOGHandle;
extern TaskHandle_t TouchTaskHandle;
extern TaskHandle_t DisplayTaskHandle;
extern TaskHandle_t EEPROMTaskHandle;

/* ================= Function Prototypes ================= */

/**
* @brief Initialize Ethernet Backend Infrastructure
* @called_from main() during system startup
* @registers CGI handlers for HTTP server
*/
void Ethernet_BackendInfrastructure_Init(void);

/**
* @brief Initialize FreeRTOS Tasks and Resources
* @called_from main() after hardware initialization
* @creates
* - Display_InitTask (4KB stack) - Priority 3 (High)
* - LogTaskHandler (1KB stack) - Priority 1 (Normal)
* - Touch_TaskFunction (4KB stack) - Priority 2 (Above Normal)
* - EEPROM_TestTask (2KB stack) - Priority 1 (Normal)
* - lcdMutexHandle (mutex for LCD access)
* @note Uses xTaskCreate (Native FreeRTOS API)
*/
void MainProcessFreertosInit(void);

/* *** ETHERNET FUNCTION PROTOTYPES - EKLENEN *** */

/**
* @brief Print current network information
* @note Shows IP, Netmask, Gateway, MAC address
*/
void Print_Network_Info(void);

/**
* @brief Initialize Ethernet interface
* @note Configures network interface with DHCP
*/
void Ethernet_Init(void);

/**
* @brief Ethernet link monitoring task
* @param params Task parameters (unused)
* @note Monitors cable connection and DHCP status
*/
void Ethernet_Link_TaskFunction(void *params);

#endif /* INC_FREERTOS_CUSTOM_H_ */
