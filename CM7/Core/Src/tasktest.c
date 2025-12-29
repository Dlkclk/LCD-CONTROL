/* tasktest.c - All Test Tasks and Functions */

#include "tasktest.h"
#include "main.h"
#include "cmsis_os.h"
#include "queue.h"
#include <string.h>
#include <stdio.h>

/* LVGL Includes */
#include "../../LVGL_Library/lvgl/lvgl.h"
#include "../../LVGL_Library/ui/ui.h"
#include "ui_custom.h"

/* Display and Touch includes */
#include "DISPLAY_INC/ili9488.h"
#include "DISPLAY_INC/ilitek_touch.h"

/* Flash and EEPROM includes */
#include "ExtFLASH_INC/sst26vf032b.h"
#include "EEPROM_INC/eeprom_at24mac402.h"

/* External handles - imported from main */
extern SPI_HandleTypeDef hspi4;
extern I2C_HandleTypeDef hi2c4;
extern UART_HandleTypeDef huart5;
extern uint16_t width;
extern uint16_t height;
extern QSPI_HandleTypeDef hqspi;
extern I2C_HandleTypeDef hi2c3;
extern osMutexId_t lcdMutexHandle;
extern osMutexId_t touchMutexHandle;
extern volatile uint8_t ETH_IRQn_Flag;

/* Task Handles - Declared in freertos_custom.c */
extern TaskHandle_t TaskLOGHandle;
extern TaskHandle_t TouchTaskHandle;
extern TaskHandle_t DisplayTaskHandle;
extern TaskHandle_t EEPROMTaskHandle;

/* UART -> Screen7 queue handle */
QueueHandle_t Screen7QueueHandle = NULL;

/* UART receive buffer for reading multiple bytes at once */
#define UART_RX_BUFFER_SIZE 256
static uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];

/* Forward declarations */
static void Screen7_ProcessInboundData(void);

/* ============== LOG TASK ============== */
void LogTaskHandler(void *params)
{
    osDelay(2000);

    while (1)
    {
        printf("[LOG] System Alive\r\n");

        if(ETH_IRQn_Flag == 1)
        {
            printf("[LOG] ETH IRQ: %d \r\n", ETH_IRQn_Flag);
            ETH_IRQn_Flag = 0;
            printf("[LOG] ETH IRQ: %d \r\n", ETH_IRQn_Flag);
        }

        osDelay(1500);
    }
}

/* ============== TOUCH TASK ============== */
void Touch_TaskFunction(void *params)
{
    osDelay(5000);
    vTaskSuspend(NULL); // Suspended - LVGL handles touch internally
}

/* ============== DISPLAY INIT TASK ============== */
void Display_TaskFunction(void *params)
{
    osDelay(200);

    // Hardware Reset
    HAL_GPIO_WritePin(CTP_RST_GPIO_Port, CTP_RST_Pin, GPIO_PIN_RESET);
    osDelay(50);
    HAL_GPIO_WritePin(CTP_RST_GPIO_Port, CTP_RST_Pin, GPIO_PIN_SET);
    osDelay(200);

    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
    osDelay(20);
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
    osDelay(120);

    // LVGL Init - MUST BE FIRST
    lv_init();
    osDelay(100);

    // Display Init - MUST BE SECOND (registers display with LVGL)
    Display_init(0);
    osDelay(100);

    // Touch Init - MUST BE THIRD (needs display to be registered)
    int touch_result = Touch_LVGL_Init();
    if (touch_result != 0) {
        // Touch init failed - silent continue
    }
    osDelay(100);

    // UI Init - LAST
    ui_init();
    
    // Custom UI initialization (Screen7 & Screen8)
    ui_custom_Screen7_init();
    ui_custom_Screen8_init();

    // LVGL Main Loop
    for(;;)
    {
        osMutexAcquire(lcdMutexHandle, osWaitForever);
        lv_timer_handler();
        Screen7_ProcessInboundData();
        osMutexRelease(lcdMutexHandle);

        osDelay(1);  // Çok hızlı timer handler (1ms) - maksimum responsive touch için
    }
}


/* ============== SPI TEST ============== */
void Test_SPI_Communication(void)
{


    HAL_SPI_StateTypeDef spi_state = HAL_SPI_GetState(&hspi4);
    printf("1. SPI State: %d (0=RESET, 1=READY, 2=BUSY_TX)\r\n", spi_state);

    printf("2. Testing SPI Transmit...\r\n");

    uint8_t test_data[] = {0x55, 0xAA, 0xFF, 0x00};

    CS_A();
    osDelay(1);

    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi4, test_data, sizeof(test_data), 1000);

    CS_D();

    if (status == HAL_OK) {
        printf("   [OK] SPI Transmit successful!\r\n");
    } else {
        printf("   [ERROR] SPI Transmit failed! Status: %d\r\n", status);
    }

    printf("3. Reading LCD ID (0x04 command)...\r\n");

    CS_A();
    DC_COMMAND();
    uint8_t cmd = 0x04;
    status = HAL_SPI_Transmit(&hspi4, &cmd, 1, 100);

    if (status == HAL_OK) {
        printf("   Command sent OK\r\n");

        DC_DATA();
        uint8_t id[3] = {0, 0, 0};
        status = HAL_SPI_Receive(&hspi4, id, 3, 100);

        if (status == HAL_OK) {
            printf("   LCD ID: 0x%02X 0x%02X 0x%02X\r\n", id[0], id[1], id[2]);
        } else {
            printf("   [ERROR] SPI Receive failed! Status: %d\r\n", status);
        }
    } else {
        printf("   [ERROR] Command send failed! Status: %d\r\n", status);
    }

    CS_D();

    printf("4. Checking GPIO states...\r\n");
    printf("   LCD_CS: %d\r\n", HAL_GPIO_ReadPin(LCD_CS_GPIO_Port, LCD_CS_Pin));
    printf("   LCD_DC: %d\r\n", HAL_GPIO_ReadPin(LCD_DC_GPIO_Port, LCD_DC_Pin));
    printf("   LCD_RST: %d\r\n", HAL_GPIO_ReadPin(LCD_RST_GPIO_Port, LCD_RST_Pin));

    printf("========== SPI TEST END ==========\r\n\r\n");
}

/* ============== QSPI FLASH TEST ============== */
void QSPI_Flash_Test(void)
{
    printf("\r\n========== QSPI FLASH TEST ==========\r\n");

    // 1. SST26 Init test
    printf("\n[TEST 1/3] SST26 Init...\r\n");
    if (SST26_Init(&hqspi) != HAL_OK)
    {
        printf("[FAILED] Flash Init FAILED!\r\n");
        return;
    }
    printf("[PASSED] Flash Init Successful\r\n");

    // 2. ID read test
    printf("\n[TEST 2/3] Read JEDEC ID...\r\n");
    uint8_t id[3];
    if (SST26_ReadID(&hqspi, id) == HAL_OK)
    {
        printf("JEDEC ID: 0x%02X 0x%02X 0x%02X\r\n", id[0], id[1], id[2]);
        if (id[0] == 0xBF && id[1] == 0x26 && id[2] == 0x42)
            printf("[PASSED] Correct chip detected (SST26VF032B)\r\n");
        else
            printf("[FAILED] Wrong chip ID!\r\n");
    }
    else
    {
        printf("[FAILED] Could not read JEDEC ID!\r\n");
    }

    // 3. Write/Read test
    printf("\n[TEST 3/3] Write/Read Test...\r\n");
    uint8_t write_data[16] = "QSPI_TEST_OK!";
    uint8_t read_data[16] = {0};

    printf("  -> Erasing sector...\r\n");
    if (SST26_EraseSector(&hqspi, 0x000000) != HAL_OK)
    {
        printf("[FAILED] Sector erase failed!\r\n");
        return;
    }
    osDelay(100);

    printf("  -> Writing test data...\r\n");
    if (SST26_Write(&hqspi, write_data, 0x000000, sizeof(write_data)) != HAL_OK)
    {
        printf("[FAILED] Write failed!\r\n");
        return;
    }

    printf("  -> Reading back...\r\n");
    if (SST26_Read(&hqspi, read_data, 0x000000, sizeof(write_data)) != HAL_OK)
    {
        printf("[FAILED] Read failed!\r\n");
        return;
    }

    printf("  -> Written: %s\r\n", write_data);
    printf("  -> Read:    %s\r\n", read_data);

    if (memcmp(write_data, read_data, sizeof(write_data)) == 0)
    {
        printf("[PASSED] Write/Read test successful!\r\n");
    }
    else
    {
        printf("[FAILED] Data mismatch!\r\n");
    }

    printf("\n========== ALL TESTS COMPLETED ==========\r\n\r\n");
}

/* ============== QSPI CUSTOM DATA TEST ============== */
void QSPI_Custom_Data_Test(void)
{


    char custom_message[] = "STM32H747 Dual Core System - QSPI Flash Working Perfectly!";
    uint32_t flash_address = 0x001000;
    uint32_t message_size = strlen(custom_message) + 1;
    char read_buffer[128] = {0};

    // ============= 1. DATA TO BE WRITTEN =============
    printf("[1] DATA TO BE WRITTEN:\r\n");
    printf("    %s\r\n\r\n", custom_message);

    // Write to flash
    SST26_EraseSector(&hqspi, flash_address);
    osDelay(100);
    SST26_Write(&hqspi, (uint8_t*)custom_message, flash_address, message_size);
    printf("    --> Written to flash address: 0x%06lX\r\n\r\n", flash_address);

    // ============= 2. DATA READ FROM FLASH =============
    printf("[2] DATA READ FROM FLASH:\r\n");
    SST26_Read(&hqspi, (uint8_t*)read_buffer, flash_address, message_size);
    printf("    %s\r\n", read_buffer);

    if (strcmp(custom_message, read_buffer) == 0) {
        printf("    --> Verification: SUCCESS\r\n\r\n");
    } else {
        printf("    --> Verification: ERROR!\r\n\r\n");
    }

    // ============= 3. ERASING DATA =============
    printf("[3] ERASING DATA...\r\n");
    SST26_EraseSector(&hqspi, flash_address);
    osDelay(100);

    memset(read_buffer, 0, sizeof(read_buffer));
    SST26_Read(&hqspi, (uint8_t*)read_buffer, flash_address, message_size);

    // Check if erased
    int is_erased = 1;
    for (uint32_t i = 0; i < message_size; i++) {
        if ((uint8_t)read_buffer[i] != 0xFF) {
            is_erased = 0;
            break;
        }
    }

    if (is_erased) {
        printf("    After erase: [All data 0xFF - Clean]\r\n");
        printf("    --> Erase operation: SUCCESS\r\n");
    } else {
        printf("    After erase: %s\r\n", read_buffer);
        printf("    --> Warning: Some data may remain\r\n");
    }

}

/* ============== MAC ADDRESS GENERATION ============== */
void Generate_MAC_From_UID(uint8_t *mac)
{
    uint32_t uid0 = HAL_GetUIDw0(); // First 32 bits of 96-bit UID
    uint32_t uid1 = HAL_GetUIDw1(); // Middle 32 bits

    // Create MAC address
    mac[0] = 0x02; // Locally administered, unicast
    mac[1] = 0x00; // Fixed value
    mac[2] = (uid0 >> 24) & 0xFF;
    mac[3] = (uid0 >> 16) & 0xFF;
    mac[4] = (uid0 >> 8) & 0xFF;
    mac[5] = (uid1 >> 24) & 0xFF; // Mix from uid1

    printf("\r\n========================================\r\n");
    printf("GENERATED MAC ADDRESS:\r\n");
    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("(Generated from STM32 Unique ID)\r\n");
    printf("========================================\r\n\r\n");
}

/* ============== EEPROM TEST TASK ============== */
void EEPROM_TestTask(void *params)
{
    printf("\r\n*** EEPROM TASK STARTED! ***\r\n");

    osDelay(2000);

    printf("\r\n========================================\r\n");
    printf("EEPROM DETAILED TEST STARTING\r\n");
    printf("========================================\r\n\r\n");

    // ============= TEST 1: Write/Read =============
    printf("[TEST 1/4] Write and Read Test\r\n");
    uint8_t writeData[] = "HELLO_EEPROM_TEST";
    uint8_t readData[20] = {0};

    printf("  -> Data to write: %s\r\n", writeData);

    if (AT24MAC402_Write(&hi2c3, 0x0000, writeData, strlen((char*)writeData)) == HAL_OK)
        printf("  -> Write: SUCCESS\r\n");
    else {
        printf("  -> Write: ERROR\r\n");
        printf("  -> Check I2C connection!\r\n\r\n");
        vTaskDelete(NULL);
        return;
    }

    osDelay(10); // EEPROM write cycle time

    if (AT24MAC402_Read(&hi2c3, 0x0000, readData, strlen((char*)writeData)) == HAL_OK)
        printf("  -> Read: SUCCESS\r\n");
    else {
        printf("  -> Read: ERROR\r\n\r\n");
        vTaskDelete(NULL);
        return;
    }

    printf("  -> Read data: %s\r\n", readData);

    if (strcmp((char*)writeData, (char*)readData) == 0)
        printf("  -> Verification: SUCCESS\r\n\r\n");
    else {
        printf("  -> Verification: ERROR! Data mismatch!\r\n");
        printf("     Expected: %s\r\n", writeData);
        printf("     Read:     %s\r\n\r\n", readData);
    }

    // ============= TEST 2: Write to Different Address =============
    printf("[TEST 2/4] Write to Different Address Test\r\n");
    uint8_t data2[] = "TEST_0x0100";
    uint8_t read2[20] = {0};

    if (AT24MAC402_Write(&hi2c3, 0x0100, data2, strlen((char*)data2)) == HAL_OK)
        printf("  -> Write to address 0x0100: SUCCESS\r\n");
    else
        printf("  -> Write to address 0x0100: ERROR\r\n");

    osDelay(10);

    if (AT24MAC402_Read(&hi2c3, 0x0100, read2, strlen((char*)data2)) == HAL_OK) {
        printf("  -> Read: %s\r\n", read2);
        if (strcmp((char*)data2, (char*)read2) == 0)
            printf("  -> Verification: SUCCESS\r\n\r\n");
        else
            printf("  -> Verification: ERROR\r\n\r\n");
    }

    // ============= TEST 3: Number Write/Read =============
    printf("[TEST 3/4] Number Data Test\r\n");
    uint8_t numbers[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0xAA, 0xBB, 0xCC};
    uint8_t readNumbers[8] = {0};

    AT24MAC402_Write(&hi2c3, 0x0200, numbers, 8);
    osDelay(10);
    AT24MAC402_Read(&hi2c3, 0x0200, readNumbers, 8);

    printf("  -> Written: ");
    for(int i = 0; i < 8; i++)
        printf("0x%02X ", numbers[i]);
    printf("\r\n");

    printf("  -> Read:    ");
    for(int i = 0; i < 8; i++)
        printf("0x%02X ", readNumbers[i]);
    printf("\r\n");

    if (memcmp(numbers, readNumbers, 8) == 0)
        printf("  -> Verification: SUCCESS\r\n\r\n");
    else
        printf("  -> Verification: ERROR\r\n\r\n");

    // ============= TEST 4: MAC Address Read =============
    printf("[TEST 4/4] Unique MAC Address Read\r\n");
    uint8_t mac[6] = {0};

    if (AT24MAC402_ReadMAC(&hi2c3, mac) == HAL_OK) {
        printf("  -> MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        // Valid MAC check (should not be all 0x00 or 0xFF)
        int allZero = 1, allFF = 1;
        for(int i = 0; i < 6; i++) {
            if(mac[i] != 0x00) allZero = 0;
            if(mac[i] != 0xFF) allFF = 0;
        }

        if(allZero || allFF)
            printf("  -> WARNING: MAC address appears invalid!\r\n\r\n");
        else
            printf("  -> MAC read: SUCCESS\r\n\r\n");
    } else {
        printf("  -> MAC read: ERROR\r\n");
        printf("  -> Note: This may be normal (different I2C address may be required)\r\n\r\n");
    }

    // ============= TEST 5: Check First Address Data Retention =============
    printf("[TEST 5/5] Check First Data Retention\r\n");
    memset(readData, 0, sizeof(readData));
    AT24MAC402_Read(&hi2c3, 0x0000, readData, strlen((char*)writeData));
    printf("  -> Data at address 0x0000: %s\r\n", readData);

    if (strcmp((char*)writeData, (char*)readData) == 0)
        printf("  -> Data retained: SUCCESS\r\n");
    else
        printf("  -> WARNING: Data changed!\r\n");

    printf("\r\n========================================\r\n");
    printf("ALL TESTS COMPLETED!\r\n");
    printf("========================================\r\n\r\n");

    // ============= BONUS: MAC GENERATION TEST =============
    printf("[BONUS TEST] MAC Generation from STM32 UID\r\n");
    uint8_t generated_mac[6];
    Generate_MAC_From_UID(generated_mac);
    printf("You can use this MAC address in LwIP!\r\n\r\n");

    vTaskDelete(NULL);
}

/* ============== SCREEN7 DATA PROCESSING ============== */
/**
 * @brief Process incoming UART data and display on Screen7 textarea
 * @note Called from Display task main loop
 */
static void Screen7_ProcessInboundData(void)
{
    if (Screen7QueueHandle == NULL || ui_tareceive == NULL) {
        return;
    }

    uint8_t incoming;
    char single_char[2] = {0, 0};
    uint32_t char_count = 0;

    // Read ALL available data from queue and add each character immediately
    while (xQueueReceive(Screen7QueueHandle, &incoming, 0) == pdPASS)
    {
        char_count++;
        
        // Convert byte to character
        if (incoming == '\r') {
            single_char[0] = '\n';
        } else {
            single_char[0] = (char)incoming;
        }

        // Add character to textarea immediately
        lv_textarea_add_text(ui_tareceive, single_char);
    }
    
    // Update cursor position if we added any characters
    if (char_count > 0) {
        const char *text = lv_textarea_get_text(ui_tareceive);
        if (text != NULL) {
            uint32_t len = strlen(text);
            lv_textarea_set_cursor_pos(ui_tareceive, len);
        }
    }
}

/* ============== UART RECEIVE TASK ============== */
/**
 * @brief Receive UART data and push to Screen7 queue
 * @note Reads one byte at a time continuously - ensures no data loss
 */
void UART_Receive_TaskFunction(void *params)
{
    uint8_t uart_rx_byte;
    HAL_StatusTypeDef status;

    // Wait for system to stabilize
    osDelay(1000);

    for(;;)
    {
        // Read one byte - blocking wait
        status = HAL_UART_Receive(&huart5, &uart_rx_byte, 1, HAL_MAX_DELAY);
        
        if (status == HAL_OK) {
            // Send byte to queue immediately
            if (Screen7QueueHandle != NULL) {
                xQueueSend(Screen7QueueHandle, &uart_rx_byte, portMAX_DELAY);
            }
        }
    }
}
