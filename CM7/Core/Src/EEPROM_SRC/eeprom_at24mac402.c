// eeprom_at24mac402.c
#include "eeprom_at24mac402.h"
#include <string.h>
HAL_StatusTypeDef AT24MAC402_Write(I2C_HandleTypeDef *hi2c, uint16_t memAddr, uint8_t *data, uint16_t len)
{
    uint8_t buffer[AT24MAC402_PAGE_SIZE + 2];
    buffer[0] = (uint8_t)(memAddr >> 8);
    buffer[1] = (uint8_t)(memAddr & 0xFF);
    memcpy(&buffer[2], data, len);
    return HAL_I2C_Master_Transmit(hi2c, AT24MAC402_I2C_ADDR, buffer, len + 2, HAL_MAX_DELAY);
}

HAL_StatusTypeDef AT24MAC402_Read(I2C_HandleTypeDef *hi2c, uint16_t memAddr, uint8_t *data, uint16_t len)
{
    uint8_t addr[2];
    addr[0] = (uint8_t)(memAddr >> 8);
    addr[1] = (uint8_t)(memAddr & 0xFF);
    if (HAL_I2C_Master_Transmit(hi2c, AT24MAC402_I2C_ADDR, addr, 2, HAL_MAX_DELAY) != HAL_OK)
        return HAL_ERROR;
    return HAL_I2C_Master_Receive(hi2c, AT24MAC402_I2C_ADDR, data, len, HAL_MAX_DELAY);
}

/* ✅ Yeni MAC okuma fonksiyonunu buraya ekliyorsun 👇 */
HAL_StatusTypeDef AT24MAC402_ReadMAC(I2C_HandleTypeDef *hi2c, uint8_t *mac)
{
    uint8_t euiAddr = 0x9A;           // MAC (EUI-48) başlangıç adresi
    uint16_t devAddr = (0x60 << 1);   // Extended memory device address (senin çipte 0x60!)

    // Dummy write: adres işaretçisini MAC başlangıcına set et
    if (HAL_I2C_Master_Transmit(hi2c, devAddr, &euiAddr, 1, 1000) != HAL_OK)
        return HAL_ERROR;

    // 6 byte MAC oku
    if (HAL_I2C_Master_Receive(hi2c, devAddr, mac, 6, 1000) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}

