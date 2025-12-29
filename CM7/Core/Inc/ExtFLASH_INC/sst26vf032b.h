#ifndef SST26VF032B_H
#define SST26VF032B_H

#include "main.h"
#include "freertos_custom.h"

/* SST26VF032B Commands */
#define SST26_CMD_READ              0x03
#define SST26_CMD_FAST_READ         0x0B
#define SST26_CMD_QUAD_READ         0x6B
#define SST26_CMD_PAGE_PROGRAM      0x02
#define SST26_CMD_QUAD_PAGE_PROGRAM 0x32
#define SST26_CMD_SECTOR_ERASE      0x20
#define SST26_CMD_CHIP_ERASE        0xC7
#define SST26_CMD_WRITE_ENABLE      0x06
#define SST26_CMD_WRITE_DISABLE     0x04
#define SST26_CMD_READ_STATUS       0x05
#define SST26_CMD_WRITE_STATUS      0x01
#define SST26_CMD_READ_CONFIG       0x35
#define SST26_CMD_RESET_ENABLE      0x66
#define SST26_CMD_RESET             0x99
#define SST26_CMD_READ_ID           0x9F
#define SST26_CMD_GLOBAL_UNLOCK     0x98
#define SST26_CMD_QUAD_ENABLE       0x38

/* Memory parameters */
#define SST26_FLASH_SIZE            0x400000  /* 4MB */
#define SST26_SECTOR_SIZE           0x1000    /* 4KB */
#define SST26_PAGE_SIZE             256

/* Status Register bits */
#define SST26_SR_BUSY               0x01
#define SST26_SR_WEL                0x02

/* Function prototypes */
HAL_StatusTypeDef SST26_Init(QSPI_HandleTypeDef *hqspi);
HAL_StatusTypeDef SST26_Reset(QSPI_HandleTypeDef *hqspi);
HAL_StatusTypeDef SST26_WriteEnable(QSPI_HandleTypeDef *hqspi);
HAL_StatusTypeDef SST26_GlobalUnlock(QSPI_HandleTypeDef *hqspi);
HAL_StatusTypeDef SST26_QuadEnable(QSPI_HandleTypeDef *hqspi);
HAL_StatusTypeDef SST26_ReadID(QSPI_HandleTypeDef *hqspi, uint8_t *id);
HAL_StatusTypeDef SST26_Read(QSPI_HandleTypeDef *hqspi, uint8_t *pData, uint32_t address, uint32_t size);
HAL_StatusTypeDef SST26_Write(QSPI_HandleTypeDef *hqspi, uint8_t *pData, uint32_t address, uint32_t size);
HAL_StatusTypeDef SST26_EraseSector(QSPI_HandleTypeDef *hqspi, uint32_t address);
HAL_StatusTypeDef SST26_EraseChip(QSPI_HandleTypeDef *hqspi);
HAL_StatusTypeDef SST26_WaitForReady(QSPI_HandleTypeDef *hqspi, uint32_t timeout);
HAL_StatusTypeDef SST26_EnableMemoryMappedMode(QSPI_HandleTypeDef *hqspi);

#endif
