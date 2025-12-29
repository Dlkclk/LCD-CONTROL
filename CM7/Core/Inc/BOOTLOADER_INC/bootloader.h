#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "stm32h7xx_hal.h"
#include <stdio.h>
#include "freertos_custom.h"

/* Memory map definitions - STM32H747 Flash Bank 1 */
#define BOOTLOADER_START_ADDR    0x08000000   // Bank1 Sector 0
#define BOOTLOADER_SIZE          0x20000      // 128KB
#define APP_START_ADDR           0x08020000   // Bank1 Sector 1
#define APP_MAX_SIZE             0x80000      // 384KB (Sectors 1-3)

/* Bank1 eski staging alanı - artık kullanılmayacak */
#define OLD_STAGING_START_ADDR   0x08080000   // Bank1 Sector 4
#define OLD_STAGING_MAX_SIZE     0x80000      // 512KB (Sectors 4-7)

/* Bank2 - Yeni staging alanı tanımları */
#define BANK2_STAGING_START      0x08100000   // Bank2 başlangıcı
#define BANK2_STAGING_SIZE       0x80000      // 512KB (Bank2 Sectors 0-3)
#define BANK2_BACKUP_START       0x08180000   // Bank2 Sector 4
#define BANK2_BACKUP_SIZE        0x60000      // 384KB backup area

/* Metadata konumları - HER İKİ BANK İÇİN */
#define METADATA_BANK1_ADDR      0x080FFE00   // Bank1 Sector 7 sonu
#define METADATA_BANK2_ADDR      0x081FFE00   // Bank2 Sector 7 sonu
#define METADATA_SIZE            512          // 512 byte metadata

/* Eski tanım - geriye uyumluluk için */
#define METADATA_START_ADDR      METADATA_BANK1_ADDR

/* Flash sector definitions for H747 */
#define BOOTLOADER_FLASH_SECTOR_SIZE  0x20000  // 128KB per sector
#define FLASH_BANK1_SECTORS            8
#define FLASH_BANK2_SECTORS            8

/* External Flash firmware storage */
#define EXT_FLASH_FW_START       0x00200000   // 2MB offset in external flash
#define EXT_FLASH_FW_MAX_SIZE    0x200000     // 2MB max firmware size

// Hangi bank'tan boot edildiğini kontrol
typedef enum {
    BOOT_BANK1 = 0,
    BOOT_BANK2 = 1
} boot_bank_t;

/* Firmware metadata structure - Genişletildi */
typedef struct {
    uint32_t magic_start;      // 0xDEADBEEF
    uint32_t fw_size;          // Firmware boyutu
    uint32_t fw_crc32;         // Firmware CRC32
    uint32_t fw_version;       // Version numarası
    uint32_t update_flag;      // 0x12345678 = update pending
    uint32_t boot_counter;     // Boot sayacı
    uint32_t app_valid;        // 0xAAAA5555 = app valid

    /* Bank2 staging bilgileri - YENİ */
    uint32_t bank2_valid;      // 0x5A5A5A5A = Bank2'de geçerli fw var
    uint32_t bank2_size;       // Bank2'deki fw boyutu
    uint32_t bank2_crc32;      // Bank2'deki fw CRC'si
    uint32_t bank2_version;    // Bank2'deki fw versiyonu

    /* Update source bilgisi - YENİ */
    uint32_t update_source;    // 0=None, 1=Bank1, 2=Bank2, 3=ExtFlash
    uint32_t reserved[3];      // İleride kullanım için

    uint32_t magic_end;        // 0xFEEDFACE
} BootloaderMetadata_t;

/* Update source definitions - YENİ */
#define UPDATE_SOURCE_NONE       0
#define UPDATE_SOURCE_BANK1      1
#define UPDATE_SOURCE_BANK2      2
#define UPDATE_SOURCE_EXTFLASH   3

/* Function prototypes */
HAL_StatusTypeDef Bootloader_Init(void);
HAL_StatusTypeDef Bootloader_CheckForUpdate(void);
HAL_StatusTypeDef Bootloader_PerformUpdate(void);
HAL_StatusTypeDef Bootloader_VerifyApplication(uint32_t addr, uint32_t size, uint32_t crc);
void Bootloader_JumpToApplication(uint32_t addr);
uint32_t Bootloader_CalculateCRC(uint32_t addr, uint32_t size);

/* Yeni fonksiyon prototipleri - Bank2 desteği için */
HAL_StatusTypeDef Bootloader_CopyToBank2(uint32_t src_addr, uint32_t size);
HAL_StatusTypeDef Bootloader_ValidateBank2(void);
HAL_StatusTypeDef Bootloader_CopyBank2ToBank1(void);
HAL_StatusTypeDef Bootloader_EraseBank2(uint32_t sectors);
HAL_StatusTypeDef Bootloader_PerformUpdateFromBank2(void);
HAL_StatusTypeDef Bootloader_PerformUpdateFromBank2_Safe(void);

/* Bank swap fonksiyonları */
HAL_StatusTypeDef Bootloader_SwapBanks(void);
boot_bank_t Bootloader_GetActiveBank(void);
HAL_StatusTypeDef Bootloader_SwapBanks_Direct(void);
HAL_StatusTypeDef Bootloader_SwapBanks_HAL(void);

/* Metadata yönetimi - YENİ */
HAL_StatusTypeDef Bootloader_WriteMetadataToBothBanks(BootloaderMetadata_t *metadata);

#endif
