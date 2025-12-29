// eeprom_at24mac402.h
#ifndef EEPROM_AT24MAC402_H
#define EEPROM_AT24MAC402_H

#include "stm32h7xx_hal.h"

#define AT24MAC402_I2C_ADDR   (0x50 << 1)
#define AT24MAC402_PAGE_SIZE  16

HAL_StatusTypeDef AT24MAC402_Write(I2C_HandleTypeDef *hi2c, uint16_t memAddr, uint8_t *data, uint16_t len);
HAL_StatusTypeDef AT24MAC402_Read(I2C_HandleTypeDef *hi2c, uint16_t memAddr, uint8_t *data, uint16_t len);
HAL_StatusTypeDef AT24MAC402_ReadMAC(I2C_HandleTypeDef *hi2c, uint8_t *mac);

#endif
