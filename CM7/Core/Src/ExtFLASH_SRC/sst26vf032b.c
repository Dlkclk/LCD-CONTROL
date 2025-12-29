#include "freertos_custom.h"

/* DMA buffer for QuadSPI operations */
#if defined(__GNUC__)
__attribute__((section(".QspiDmaSection")))
__attribute__((aligned(32)))
static uint8_t qspi_dma_buffer[4096];
#endif


HAL_StatusTypeDef SST26_Init(QSPI_HandleTypeDef *hqspi)
{
    // ❌ ASLA MANUEL GPIO KONTROLÜ YAPMAYIN!
    // QSPI hardware otomatik CS kontrolü yapar
    // Bu satırlar SİLİNDİ:
    // HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_RESET);
    // HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_SET);

    printf("SST26_Init: Starting...\r\n");

    // Power-on delay ekleyelim
    osDelay(50);

    // Reset işlemi önce gelsin
    if (SST26_Reset(hqspi) != HAL_OK) {
        printf("SST26_Init: Reset FAILED\r\n");
        return HAL_ERROR;
    }
    printf("SST26_Init: Reset OK\r\n");
    osDelay(50);  // Reset recovery

    /* Verify chip ID */
    uint8_t id[3];
    if (SST26_ReadID(hqspi, id) != HAL_OK) {
        printf("SST26_Init: ReadID FAILED\r\n");
        return HAL_ERROR;
    }

    printf("SST26_Init: Chip ID: 0x%02X 0x%02X 0x%02X\r\n", id[0], id[1], id[2]);

    if (id[0] != 0xBF || id[1] != 0x26 || id[2] != 0x42) {
        printf("SST26_Init: Wrong chip ID!\r\n");
        return HAL_ERROR;
    }

    /* Enable write operations */
    if (SST26_WriteEnable(hqspi) != HAL_OK) {
        printf("SST26_Init: WriteEnable FAILED\r\n");
        return HAL_ERROR;
    }
    printf("SST26_Init: WriteEnable OK\r\n");

    /* Global block protection unlock - ÖNEMLİ! */
    if (SST26_GlobalUnlock(hqspi) != HAL_OK) {
        printf("SST26_Init: GlobalUnlock FAILED\r\n");
        return HAL_ERROR;
    }
    printf("SST26_Init: GlobalUnlock OK\r\n");

    printf("SST26_Init: SUCCESS\r\n");
    return HAL_OK;
}

HAL_StatusTypeDef SST26_Reset(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef cmd;

    /* Reset Enable */
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = SST26_CMD_RESET_ENABLE;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_NONE;
    cmd.DummyCycles       = 0;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Reset */
    cmd.Instruction = SST26_CMD_RESET;

    if (HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    /* SST26VF032B datasheet: tRST = 10-50us reset recovery time */
    /* FreeRTOS içinde olduğumuz için osDelay kullan */
    osDelay(1);  /* 1ms bekleme, yeterli */

    return HAL_OK;
}

HAL_StatusTypeDef SST26_WriteEnable(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef cmd;

    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = SST26_CMD_WRITE_ENABLE;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_NONE;
    cmd.DummyCycles       = 0;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    return HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

HAL_StatusTypeDef SST26_GlobalUnlock(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef cmd;

    /* Write Enable first */
    if (SST26_WriteEnable(hqspi) != HAL_OK) {
        return HAL_ERROR;
    }

    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = SST26_CMD_GLOBAL_UNLOCK;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_NONE;
    cmd.DummyCycles       = 0;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    // Global Unlock sonrası mutlaka WaitForReady ekle!
    return SST26_WaitForReady(hqspi, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

HAL_StatusTypeDef SST26_QuadEnable(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef cmd;
    uint8_t reg;

    /* Read Configuration Register */
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = SST26_CMD_READ_CONFIG;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_1_LINE;
    cmd.DummyCycles       = 0;
    cmd.NbData            = 1;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    if (HAL_QSPI_Receive(hqspi, &reg, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Check if Quad mode already enabled (bit 1) */
    if (reg & 0x02) {
        return HAL_OK;
    }

    /* Enable Quad mode */
    if (SST26_WriteEnable(hqspi) != HAL_OK) {
        return HAL_ERROR;
    }

    cmd.Instruction = SST26_CMD_QUAD_ENABLE;
    cmd.DataMode    = QSPI_DATA_NONE;

    return HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

HAL_StatusTypeDef SST26_ReadID(QSPI_HandleTypeDef *hqspi, uint8_t *id)
{
    QSPI_CommandTypeDef cmd;

    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = SST26_CMD_READ_ID;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_1_LINE;
    cmd.DummyCycles       = 0;
    cmd.NbData            = 3;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_QSPI_Receive(hqspi, id, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

HAL_StatusTypeDef SST26_WaitForReady(QSPI_HandleTypeDef *hqspi, uint32_t timeout)
{
    QSPI_CommandTypeDef cmd;
    QSPI_AutoPollingTypeDef config;

    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = SST26_CMD_READ_STATUS;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_1_LINE;
    cmd.DummyCycles       = 0;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    config.Match           = 0x00;
    config.Mask            = SST26_SR_BUSY;
    config.MatchMode       = QSPI_MATCH_MODE_AND;
    config.StatusBytesSize = 1;
    config.Interval        = 0x10;
    config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    return HAL_QSPI_AutoPolling(hqspi, &cmd, &config, timeout);
}

HAL_StatusTypeDef SST26_Write(QSPI_HandleTypeDef *hqspi, uint8_t *pData, uint32_t address, uint32_t size)
{
    QSPI_CommandTypeDef cmd;
    uint32_t end_addr, current_size, current_addr;

    current_addr = address;

    while (current_addr < (address + size)) {
        if (SST26_WriteEnable(hqspi) != HAL_OK) {
            return HAL_ERROR;
        }

        /* Calculate page boundary */
        end_addr = (current_addr + SST26_PAGE_SIZE) & (~(SST26_PAGE_SIZE - 1));
        current_size = ((current_addr + size) > end_addr) ?
                       (end_addr - current_addr) :
                       ((address + size) - current_addr);

        cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
        cmd.Instruction       = SST26_CMD_PAGE_PROGRAM;  // 0x02 - Normal page program
        cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
        cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
        cmd.Address           = current_addr;
        cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
        cmd.DataMode          = QSPI_DATA_1_LINE;  // 1-line data mode
        cmd.DummyCycles       = 0;
        cmd.NbData            = current_size;
        cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
        cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
        cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

        if (HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
            return HAL_ERROR;
        }

        if (HAL_QSPI_Transmit(hqspi, pData, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
            return HAL_ERROR;
        }

        if (SST26_WaitForReady(hqspi, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
            return HAL_ERROR;
        }

        current_addr += current_size;
        pData += current_size;
    }

    return HAL_OK;
}

HAL_StatusTypeDef SST26_Read(QSPI_HandleTypeDef *hqspi, uint8_t *pData, uint32_t address, uint32_t size)
{
    QSPI_CommandTypeDef cmd;

    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = SST26_CMD_READ;  // 0x03 - Normal read
    cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
    cmd.Address           = address;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_1_LINE;  // 1-line data mode
    cmd.DummyCycles       = 0;  // Normal read'de dummy cycle yok
    cmd.NbData            = size;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_QSPI_Receive(hqspi, pData, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

HAL_StatusTypeDef SST26_EraseSector(QSPI_HandleTypeDef *hqspi, uint32_t address)
{
    QSPI_CommandTypeDef cmd;

    if (SST26_WriteEnable(hqspi) != HAL_OK) {
        return HAL_ERROR;
    }

    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = SST26_CMD_SECTOR_ERASE;
    cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
    cmd.Address           = address;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_NONE;
    cmd.DummyCycles       = 0;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    return SST26_WaitForReady(hqspi, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}
