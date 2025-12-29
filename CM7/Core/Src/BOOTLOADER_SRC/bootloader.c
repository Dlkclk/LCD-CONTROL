
#include "freertos_custom.h"

//extern CRC_HandleTypeDef hcrc;

// FLASH_SR_ALL_ERRORS tanımı
#define FLASH_SR_ALL_ERRORS (FLASH_SR_WRPERR | FLASH_SR_PGSERR | FLASH_SR_STRBERR | \
                            FLASH_SR_INCERR | FLASH_SR_OPERR | FLASH_SR_RDPERR | \
                            FLASH_SR_RDSERR | FLASH_SR_SNECCERR | FLASH_SR_DBECCERR)

HAL_StatusTypeDef Bootloader_Init(void)
{
    BootloaderMetadata_t *metadata = (BootloaderMetadata_t*)METADATA_START_ADDR;

    // Check metadata validity
    if(metadata->magic_start != 0xDEADBEEF || metadata->magic_end != 0xFEEDFACE) {
        printf("[BOOTLOADER] Invalid metadata, initializing defaults\n");

        HAL_FLASH_Unlock();
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);

        BootloaderMetadata_t default_meta = {0};
        default_meta.magic_start = 0xDEADBEEF;
        default_meta.magic_end = 0xFEEDFACE;
        default_meta.update_flag = 0;
        default_meta.boot_counter = 0;
        default_meta.app_valid = 0;
        default_meta.fw_size = 0;
        default_meta.fw_crc32 = 0;
        default_meta.fw_version = 0;

        // Bank2 alanları sıfırla
        default_meta.bank2_valid = 0;
        default_meta.bank2_size = 0;
        default_meta.bank2_crc32 = 0;
        default_meta.bank2_version = 0;
        default_meta.update_source = UPDATE_SOURCE_NONE;

        uint32_t metadata_words[32];  // Boyut artırıldı
        memset(metadata_words, 0xFF, sizeof(metadata_words));
        memcpy(metadata_words, &default_meta, sizeof(default_meta));

        for(uint32_t i = 0; i < sizeof(metadata_words); i += 32) {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                            METADATA_START_ADDR + i,
                            (uint32_t)&metadata_words[i/4]);
        }
        HAL_FLASH_Lock();
    }

    return HAL_OK;
}

HAL_StatusTypeDef Bootloader_CheckForUpdate(void)
{
    BootloaderMetadata_t *metadata = (BootloaderMetadata_t*)METADATA_START_ADDR;

    // Önce Bank2 kontrolü
    if(metadata->bank2_valid == 0x5A5A5A5A &&
       metadata->update_source == UPDATE_SOURCE_BANK2) {
        printf("[BOOTLOADER] Bank2 update detected\n");
        return HAL_OK;
    }

    // Sonra eski yöntem (Bank1 staging)
    if(metadata->update_flag == 0x12345678) {
        printf("[BOOTLOADER] Bank1 staging update detected\n");
        return HAL_OK;
    }

    return HAL_ERROR;
}

uint32_t Bootloader_CalculateCRC(uint32_t addr, uint32_t size)
{
    //__HAL_CRC_DR_RESET(&hcrc);

    uint32_t remaining = size;
    uint32_t current_addr = addr;
    uint32_t crc_value = 0;



    return crc_value;
}

HAL_StatusTypeDef Bootloader_VerifyApplication(uint32_t addr, uint32_t size, uint32_t expected_crc)
{
    printf("[BOOTLOADER] Verifying app at 0x%08lX, size=%lu, crc=0x%08lX\n",
           addr, size, expected_crc);

    uint32_t stack_ptr = *(__IO uint32_t*)addr;
    printf("[BOOTLOADER] Stack pointer: 0x%08lX\n", stack_ptr);

    uint8_t valid_stack = 0;

    if(stack_ptr >= 0x20000000 && stack_ptr <= 0x20020000) {
        printf("[BOOTLOADER] Stack in DTCM RAM\n");
        valid_stack = 1;
    }
    else if(stack_ptr >= 0x24000000 && stack_ptr <= 0x24080100) {
        printf("[BOOTLOADER] Stack in AXI SRAM\n");
        valid_stack = 1;
    }
    else if(stack_ptr >= 0x30000000 && stack_ptr <= 0x30048000) {
        printf("[BOOTLOADER] Stack in SRAM1-3\n");
        valid_stack = 1;
    }
    else if(stack_ptr >= 0x38000000 && stack_ptr <= 0x38010000) {
        printf("[BOOTLOADER] Stack in SRAM4\n");
        valid_stack = 1;
    }

    if(!valid_stack) {
        printf("[BOOTLOADER] Invalid stack pointer: 0x%08lX\n", stack_ptr);
        return HAL_ERROR;
    }

    uint32_t reset_vector = *(__IO uint32_t*)(addr + 4);
    printf("[BOOTLOADER] Reset vector: 0x%08lX\n", reset_vector);

    if(reset_vector < 0x08000000 || reset_vector > 0x081FFFFF) {
        printf("[BOOTLOADER] Invalid reset vector\n");
        return HAL_ERROR;
    }

    if(expected_crc != 0 && size > 0) {
        printf("[BOOTLOADER] Calculating CRC...\n");
        uint32_t calculated_crc = Bootloader_CalculateCRC(addr, size);
        printf("[BOOTLOADER] Calculated CRC: 0x%08lX, Expected: 0x%08lX\n",
               calculated_crc, expected_crc);

        if(calculated_crc != expected_crc) {
            printf("[BOOTLOADER] CRC mismatch!\n");
            return HAL_ERROR;
        }
    }

    printf("[BOOTLOADER] Application verified successfully\n");
    return HAL_OK;
}









boot_bank_t Bootloader_GetActiveBank(void)
{
    // STM32H747 için doğru register kontrolü
    uint32_t bank_swap_opt = FLASH->OPTSR_CUR & FLASH_OPTSR_SWAP_BANK_OPT;

    // SWAP_BANK_OPT biti set ise Bank2'den boot ediliyor
    if(bank_swap_opt) {
        printf("[BOOTLOADER] Currently running from Bank2 (SWAP_BANK_OPT=1)\n");
        return BOOT_BANK2;
    } else {
        printf("[BOOTLOADER] Currently running from Bank1 (SWAP_BANK_OPT=0)\n");
        return BOOT_BANK1;
    }
}
HAL_StatusTypeDef Bootloader_SwapBanks(void)
{
    FLASH_OBProgramInitTypeDef OBInit = {0};
    HAL_StatusTypeDef status;
    uint32_t current_bank_opt;

    printf("[BOOTLOADER] ===== BANK SWAP OPERATION =====\n");

    // Flash unlock
    status = HAL_FLASH_Unlock();
    if(status != HAL_OK) {
        printf("[BOOTLOADER] ERROR: Failed to unlock flash\n");
        return status;
    }

    // Option Bytes unlock
    status = HAL_FLASH_OB_Unlock();
    if(status != HAL_OK) {
        printf("[BOOTLOADER] ERROR: Failed to unlock option bytes\n");
        HAL_FLASH_Lock();
        return status;
    }

    // Clear any pending errors
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

    // Read current configuration
    HAL_FLASHEx_OBGetConfig(&OBInit);

    // Check current bank
    current_bank_opt = FLASH->OPTSR_CUR & FLASH_OPTSR_SWAP_BANK_OPT;
    printf("[BOOTLOADER] Current SWAP_BANK_OPT: %s (0x%08lX)\n",
           current_bank_opt ? "SET (Bank2)" : "CLEAR (Bank1)",
           FLASH->OPTSR_CUR);

    // Configure for STM32H747
    OBInit.OptionType = OPTIONBYTE_USER;
    OBInit.USERType = OB_USER_SWAP_BANK;
    OBInit.Banks = FLASH_BANK_1;

    // Read current USER configuration
    uint32_t current_user = OBInit.USERConfig;
    printf("[BOOTLOADER] Current USERConfig: 0x%08lX\n", current_user);

    // Toggle SWAP_BANK bit
    if(current_bank_opt) {
        // Clear bit to boot from Bank1
        OBInit.USERConfig = current_user & ~FLASH_OPTSR_SWAP_BANK_OPT;
        printf("[BOOTLOADER] New USERConfig: 0x%08lX (Bank1)\n", OBInit.USERConfig);
    } else {
        // Set bit to boot from Bank2
        OBInit.USERConfig = current_user | FLASH_OPTSR_SWAP_BANK_OPT;
        printf("[BOOTLOADER] New USERConfig: 0x%08lX (Bank2)\n", OBInit.USERConfig);
    }

    // Wait for any ongoing operations
    FLASH_WaitForLastOperation(1000, FLASH_BANK_1);
    FLASH_WaitForLastOperation(1000, FLASH_BANK_2);

    // Program option bytes
    printf("[BOOTLOADER] Programming option bytes...\n");
    status = HAL_FLASHEx_OBProgram(&OBInit);

    if(status != HAL_OK) {
        uint32_t error = HAL_FLASH_GetError();
        printf("[BOOTLOADER] ERROR: OB Program failed!\n");
        printf("  HAL Status: 0x%02X\n", status);
        printf("  Error Code: 0x%08lX\n", error);

        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
        return status;
    }

    // Verify programming
    uint32_t new_opt = FLASH->OPTSR_PRG & FLASH_OPTSR_SWAP_BANK_OPT;
    printf("[BOOTLOADER] SWAP_BANK_OPT programmed: %s\n",
           new_opt ? "SET" : "CLEAR");

    // Lock option bytes ÖNCE
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();

    printf("[BOOTLOADER] Launching option bytes (system will reset)...\n");
    printf("[BOOTLOADER] =====================================\n");

    // Cache ve interrupt temizliği
    __disable_irq();
    SCB_CleanDCache();
    SCB_DisableDCache();
    SCB_DisableICache();

    // Kısa bekleme
    HAL_Delay(100);

    // Option bytes launch - HAL fonksiyonu yerine direct
    // Bu sistem reset'i tetikleyecek
    FLASH->OPTCR |= FLASH_OPTCR_OPTSTART;

    // Eğer reset olmazsa manuel reset
    HAL_Delay(100);

    printf("[BOOTLOADER] Manual reset fallback...\n");
    NVIC_SystemReset();

    // Buraya ulaşmamalı
    while(1) {
        __NOP();
    }
}


// Bootloader_PerformUpdateFromBank2_Safe() içinde metadata güncelleme kısmı:

HAL_StatusTypeDef Bootloader_WriteMetadataToBothBanks(BootloaderMetadata_t *metadata)
{
    HAL_StatusTypeDef status;

    printf("[BOOTLOADER] Writing metadata to both banks...\n");

    HAL_FLASH_Unlock();

    // Buffer hazırla
    uint32_t metadata_words[32];
    memset(metadata_words, 0xFF, sizeof(metadata_words));
    memcpy(metadata_words, metadata, sizeof(BootloaderMetadata_t));

    // Bank1 metadata yazma
    printf("Writing to Bank1 metadata (0x%08X)...\n", METADATA_BANK1_ADDR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);

    for(uint32_t i = 0; i < sizeof(metadata_words); i += 32) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                                 METADATA_BANK1_ADDR + i,
                                 (uint32_t)&metadata_words[i/4]);
        if(status != HAL_OK) {
            printf("ERROR: Bank1 metadata write failed\n");
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }
    }

    // Bank2 metadata yazma
    printf("Writing to Bank2 metadata (0x%08X)...\n", METADATA_BANK2_ADDR);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

    for(uint32_t i = 0; i < sizeof(metadata_words); i += 32) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                                 METADATA_BANK2_ADDR + i,
                                 (uint32_t)&metadata_words[i/4]);
        if(status != HAL_OK) {
            printf("ERROR: Bank2 metadata write failed\n");
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }
    }

    HAL_FLASH_Lock();

    printf("Metadata written to both banks successfully\n");
    return HAL_OK;
}


HAL_StatusTypeDef Bootloader_SwapBanks_HAL(void)
{
    FLASH_OBProgramInitTypeDef OBInit = {0};
    HAL_StatusTypeDef status;

    printf("[BOOTLOADER] Bank Swap using HAL\n");

    // Unlock
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();

    // Clear errors
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

    // Get current config
    HAL_FLASHEx_OBGetConfig(&OBInit);

    // Configure
    OBInit.OptionType = OPTIONBYTE_USER;
    OBInit.USERType = OB_USER_SWAP_BANK;
    OBInit.Banks = FLASH_BANK_BOTH;  // Her iki bank için

    // Toggle
    if(FLASH->OPTSR_CUR & FLASH_OPTSR_SWAP_BANK_OPT) {
        OBInit.USERConfig = (OBInit.USERConfig & ~FLASH_OPTSR_SWAP_BANK_OPT);
    } else {
        OBInit.USERConfig = (OBInit.USERConfig | FLASH_OPTSR_SWAP_BANK_OPT);
    }

    // Program with retry
    for(int retry = 0; retry < 3; retry++) {
        status = HAL_FLASHEx_OBProgram(&OBInit);
        if(status == HAL_OK) {
            break;
        }
        printf("Retry %d...\n", retry + 1);
        HAL_Delay(100);
    }

    if(status != HAL_OK) {
        printf("ERROR: Failed after 3 retries\n");
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
        return HAL_ERROR;
    }

    // Launch
    HAL_FLASH_OB_Launch();

    while(1);
}




HAL_StatusTypeDef Bootloader_SwapBanks_Direct(void)
{
    printf("[BOOTLOADER] Bank Swap using HAL Library...\n");

    // 1. Mevcut bank durumunu kontrol et
    boot_bank_t current_bank = Bootloader_GetActiveBank();
    printf("Current bank: %s\n", current_bank == BOOT_BANK1 ? "Bank1" : "Bank2");

    // 2. Flash ve Option Bytes unlock
    HAL_StatusTypeDef status = HAL_FLASH_Unlock();
    if(status != HAL_OK) {
        printf("ERROR: Flash unlock failed\n");
        return status;
    }

    status = HAL_FLASH_OB_Unlock();
    if(status != HAL_OK) {
        printf("ERROR: Option bytes unlock failed\n");
        HAL_FLASH_Lock();
        return status;
    }

    // 3. Tüm hataları temizle
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

    // 4. Option bytes yapılandırması
    FLASH_OBProgramInitTypeDef OBInit = {0};

    // Mevcut ayarları oku
    HAL_FLASHEx_OBGetConfig(&OBInit);

    // Sadece SWAP_BANK bitini değiştir
    OBInit.OptionType = OPTIONBYTE_USER;
    OBInit.USERType = OB_USER_SWAP_BANK;
    OBInit.Banks = FLASH_BANK_1;  // Bank1 option bytes

    // SWAP_BANK bitini toggle et
    if(current_bank == BOOT_BANK1) {
        // Bank2'ye geç
        OBInit.USERConfig = OBInit.USERConfig | FLASH_OPTSR_SWAP_BANK_OPT;
        printf("Setting SWAP_BANK bit (will boot from Bank2)\n");
    } else {
        // Bank1'e geç
        OBInit.USERConfig = OBInit.USERConfig & ~FLASH_OPTSR_SWAP_BANK_OPT;
        printf("Clearing SWAP_BANK bit (will boot from Bank1)\n");
    }

    // 5. Option bytes'ı programla
    printf("Programming option bytes...\n");
    status = HAL_FLASHEx_OBProgram(&OBInit);

    if(status != HAL_OK) {
        uint32_t error = HAL_FLASH_GetError();
        printf("ERROR: OB Programming failed!\n");
        printf("  Status: 0x%02X\n", status);
        printf("  Error code: 0x%08lX\n", error);

        // Hata detayları
        if(error & HAL_FLASH_ERROR_WRP) printf("  - Write Protection Error\n");
        if(error & HAL_FLASH_ERROR_PGS) printf("  - Programming Sequence Error\n");
        if(error & HAL_FLASH_ERROR_STRB) printf("  - Strobe Error\n");
        if(error & HAL_FLASH_ERROR_OPE) printf("  - Operation Error\n");

        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
        return status;
    }

    printf("Option bytes programmed successfully\n");

    // 6. Lock
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();

    // 7. Option bytes'ı etkinleştir ve reset at
    printf("Launching option bytes (system will reset)...\n");
    printf("=====================================\n");

    // Cache temizliği
    SCB_CleanDCache();
    SCB_DisableDCache();
    SCB_DisableICache();

    HAL_Delay(100);

    // Option bytes launch - bu otomatik reset atacak
    HAL_FLASH_OB_Launch();

    // Buraya ulaşmamalı
    while(1);
}



HAL_StatusTypeDef Bootloader_PerformUpdateFromBank2_Safe(void)
{
    BootloaderMetadata_t *metadata = (BootloaderMetadata_t*)METADATA_START_ADDR;
    boot_bank_t current_bank = Bootloader_GetActiveBank();
    HAL_StatusTypeDef status;

    printf("\n=== SAFE BANK2 UPDATE STRATEGY ===\n");
    printf("Current boot bank: %s\n", current_bank == BOOT_BANK1 ? "Bank1" : "Bank2");
    printf("Bank2 valid flag: 0x%08lX\n", metadata->bank2_valid);
    printf("Update source: %lu\n", metadata->update_source);

    // DURUM 1: Bank1'den çalışıyoruz, Bank2'de firmware var
    if(current_bank == BOOT_BANK1 &&
       metadata->bank2_valid == 0x5A5A5A5A) {

        printf("\n[STEP 1] Running from Bank1, firmware ready in Bank2\n");
        printf("[STEP 2] Updating metadata for Bank2 boot...\n");

        // Metadata'yı güncelle ve flash'a yaz
        HAL_FLASH_Unlock();
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);

        // Metadata sector'ü sil (Sector 7)
        FLASH_EraseInitTypeDef EraseInitStruct = {0};
        uint32_t SectorError = 0;

        EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
        EraseInitStruct.Banks = FLASH_BANK_1;
        EraseInitStruct.Sector = 7;
        EraseInitStruct.NbSectors = 1;
        EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

        status = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
        if(status != HAL_OK) {
            printf("ERROR: Failed to erase metadata sector\n");
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }

        // Yeni metadata hazırla
        BootloaderMetadata_t new_meta = *metadata;
        new_meta.update_source = UPDATE_SOURCE_BANK2;  // Bank2'den güncelleme

        // Metadata'yı flash'a yaz
        uint32_t metadata_words[32];
        memset(metadata_words, 0xFF, sizeof(metadata_words));
        memcpy(metadata_words, &new_meta, sizeof(new_meta));

        for(uint32_t i = 0; i < sizeof(metadata_words); i += 32) {
            status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                                     METADATA_START_ADDR + i,
                                     (uint32_t)&metadata_words[i/4]);
            if(status != HAL_OK) {
                printf("ERROR: Metadata write failed\n");
                HAL_FLASH_Lock();
                return HAL_ERROR;
            }
        }

        HAL_FLASH_Lock();
        printf("[STEP 3] Metadata updated, performing bank swap...\n");

        // Bank swap yap - reset atacak
        return Bootloader_SwapBanks();
    }

    // DURUM 2: Bank2'den çalışıyoruz, Bank1'i güncelleyebiliriz
    else if(current_bank == BOOT_BANK2 &&
            metadata->update_source == UPDATE_SOURCE_BANK2) {

        printf("\n[STEP 4] Now running from Bank2!\n");
        printf("[STEP 5] Safe to update Bank1...\n");

        // Bank1'i güvenle silebiliriz
        HAL_FLASH_Unlock();
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);

        // Bank1 Sectors 1-3'ü sil (Application area)
        FLASH_EraseInitTypeDef EraseInitStruct = {0};
        uint32_t SectorError = 0;

        EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
        EraseInitStruct.Banks = FLASH_BANK_1;
        EraseInitStruct.Sector = 1;  // App başlangıcı
        EraseInitStruct.NbSectors = 3;  // 3 sector (384KB)
        EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

        printf("Erasing Bank1 sectors 1-3 (0x%08X - 0x%08X)...\n",
               APP_START_ADDR, APP_START_ADDR + APP_MAX_SIZE - 1);

        status = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
        if(status != HAL_OK) {
            printf("ERROR: Failed to erase Bank1 (sector %lu)\n", SectorError);
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }

        printf("Bank1 erased successfully\n");

        // Bank2'den Bank1'e firmware kopyala
        printf("[STEP 6] Copying firmware from Bank2 to Bank1...\n");

        uint32_t src_addr = BANK2_STAGING_START;
        uint32_t dst_addr = APP_START_ADDR;
        uint32_t remaining = metadata->bank2_size;
        uint32_t copied = 0;
        uint8_t buffer[256];

        while(remaining > 0) {
            uint32_t chunk = (remaining > 256) ? 256 : remaining;

            // Bank2'den oku
            memcpy(buffer, (void*)src_addr, chunk);

            // Padding ekle
            if(chunk < 256) {
                memset(&buffer[chunk], 0xFF, 256 - chunk);
            }

            // Bank1'e 32-byte aligned yaz
            uint32_t aligned = ((chunk + 31) / 32) * 32;
            for(uint32_t i = 0; i < aligned; i += 32) {
                status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                                         dst_addr + i,
                                         (uint32_t)&buffer[i]);
                if(status != HAL_OK) {
                    printf("ERROR: Write failed at 0x%08lX\n", dst_addr + i);
                    HAL_FLASH_Lock();
                    return HAL_ERROR;
                }
            }

            src_addr += chunk;
            dst_addr += chunk;
            remaining -= chunk;
            copied += chunk;

            // Progress
            if(copied % 16384 == 0 || remaining == 0) {
                printf("  Progress: %lu/%lu bytes (%lu%%)\n",
                       copied, metadata->bank2_size,
                       (copied * 100) / metadata->bank2_size);
            }
        }

        printf("Copy completed: %lu bytes\n", copied);

        // Bank1'deki firmware'i doğrula
        printf("[STEP 7] Verifying Bank1 firmware...\n");
        status = Bootloader_VerifyApplication(APP_START_ADDR,
                                             metadata->bank2_size, 0);
        if(status != HAL_OK) {
            printf("ERROR: Bank1 verification failed\n");
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }

        // Metadata'yı temizle
        printf("[STEP 8] Clearing update flags...\n");

        // Metadata sector'ü sil
        EraseInitStruct.Sector = 7;
        EraseInitStruct.NbSectors = 1;

        status = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
        if(status != HAL_OK) {
            printf("ERROR: Failed to erase metadata\n");
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }

        // Temiz metadata yaz
        BootloaderMetadata_t clean_meta = {0};
        clean_meta.magic_start = 0xDEADBEEF;
        clean_meta.magic_end = 0xFEEDFACE;
        clean_meta.fw_size = metadata->bank2_size;
        clean_meta.fw_crc32 = metadata->bank2_crc32;
        clean_meta.fw_version = metadata->bank2_version;
        clean_meta.app_valid = 0xAAAA5555;
        clean_meta.bank2_valid = 0;  // Bank2 flag'ini temizle
        clean_meta.update_source = UPDATE_SOURCE_NONE;
        clean_meta.update_flag = 0;
        clean_meta.boot_counter = 0;

        uint32_t metadata_words[32];
        memset(metadata_words, 0xFF, sizeof(metadata_words));
        memcpy(metadata_words, &clean_meta, sizeof(clean_meta));

        for(uint32_t i = 0; i < sizeof(metadata_words); i += 32) {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                            METADATA_START_ADDR + i,
                            (uint32_t)&metadata_words[i/4]);
        }

        HAL_FLASH_Lock();

        printf("[STEP 9] Update complete, swapping back to Bank1...\n");
        printf("=== UPDATE SUCCESSFUL ===\n\n");

        // Bank1'e geri dön
        return Bootloader_SwapBanks();
    }

    // DURUM 3: Beklenmeyen durum
    else {
        printf("\nERROR: Unexpected state!\n");
        printf("  Current bank: %s\n",
               current_bank == BOOT_BANK1 ? "Bank1" : "Bank2");
        printf("  Bank2 valid: 0x%08lX\n", metadata->bank2_valid);
        printf("  Update source: %lu\n", metadata->update_source);

        // Güvenlik için Bank1'e dön
        if(current_bank == BOOT_BANK2) {
            printf("Safety measure: Returning to Bank1...\n");
            return Bootloader_SwapBanks();
        }
    }

    return HAL_ERROR;
}




// ESKİ: Bank1 Staging'den güncelleme (geriye uyumluluk için)
HAL_StatusTypeDef Bootloader_PerformUpdate(void)
{
    HAL_StatusTypeDef status;
    BootloaderMetadata_t *metadata = (BootloaderMetadata_t*)METADATA_START_ADDR;

    printf("[BOOTLOADER] Starting update process (Bank1 staging)...\n");
    printf("[BOOTLOADER] Staging addr: 0x%08X\n", OLD_STAGING_START_ADDR);
    printf("[BOOTLOADER] App addr: 0x%08X\n", APP_START_ADDR);
    printf("[BOOTLOADER] FW size: %lu bytes\n", metadata->fw_size);
    printf("[BOOTLOADER] FW CRC: 0x%08lX\n", metadata->fw_crc32);

    // Size kontrolü
    if(metadata->fw_size == 0 || metadata->fw_size > OLD_STAGING_MAX_SIZE) {
        printf("[BOOTLOADER] Invalid firmware size: %lu\n", metadata->fw_size);
        return HAL_ERROR;
    }

    // Staging area'yı doğrula
    status = Bootloader_VerifyApplication(OLD_STAGING_START_ADDR,
                                         metadata->fw_size,
                                         0);
    if(status != HAL_OK) {
        printf("[BOOTLOADER] Staging area verification failed\n");
        return HAL_ERROR;
    }

    // Application area'yı sil
    printf("[BOOTLOADER] Erasing application area...\n");

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);

    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    uint32_t SectorError = 0;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Sector = 1;  // Sector 1'den başla (0x08020000)
    EraseInitStruct.NbSectors = 3;  // 3 sector sil (1, 2, 3)
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    printf("[BOOTLOADER] Erasing sectors 1-3...\n");

    HAL_StatusTypeDef result = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);

    if(result == HAL_OK) {
        printf("[BOOTLOADER] All sectors erased successfully\n");
    } else {
        printf("[BOOTLOADER] Erase failed, error: 0x%08lX, sector: %lu\n",
               HAL_FLASH_GetError(), SectorError);
        HAL_FLASH_Lock();
        return HAL_ERROR;
    }

    // Staging'den App'e kopyala
    printf("[BOOTLOADER] Copying firmware to application area...\n");
    uint32_t remaining = metadata->fw_size;
    uint32_t offset = 0;
    uint32_t progress_counter = 0;

    while(remaining > 0) {
        uint32_t chunk_size = (remaining > 256) ? 256 : remaining;
        uint32_t aligned_chunk = ((chunk_size + 31) / 32) * 32;
        uint8_t buffer[256];

        memcpy(buffer, (void*)(OLD_STAGING_START_ADDR + offset), chunk_size);

        if(chunk_size < aligned_chunk) {
            memset(&buffer[chunk_size], 0xFF, aligned_chunk - chunk_size);
        }

        for(uint32_t i = 0; i < aligned_chunk; i += 32) {
            // BSY bekle
            uint32_t timeout = 0;
            while(FLASH->SR1 & FLASH_SR_BSY) {
                if(++timeout > 1000000) {
                    HAL_FLASH_Lock();
                    return HAL_ERROR;
                }
            }

            // Hataları temizle
            if(FLASH->SR1 & FLASH_SR_ALL_ERRORS) {
                FLASH->CCR1 = FLASH_SR_ALL_ERRORS;
            }

            // Programming'i etkinleştir
            FLASH->CR1 = FLASH_CR_PG | FLASH_CR_PSIZE_1;

            // 256 bit (32 byte) yaz
            volatile uint32_t *dest = (volatile uint32_t*)(APP_START_ADDR + offset + i);
            uint32_t *src = (uint32_t*)&buffer[i];

            for(int j = 0; j < 8; j++) {
                dest[j] = src[j];
            }
            __DSB();

            // Tamamlanmasını bekle
            timeout = 0;
            while(FLASH->SR1 & FLASH_SR_BSY) {
                if(++timeout > 1000000) {
                    FLASH->CR1 = 0;
                    HAL_FLASH_Lock();
                    return HAL_ERROR;
                }
            }

            // Programming'i devre dışı bırak
            FLASH->CR1 = 0;

            // Hata kontrolü
            if(FLASH->SR1 & FLASH_SR_ALL_ERRORS) {
                HAL_FLASH_Lock();
                return HAL_ERROR;
            }
        }

        offset += chunk_size;
        remaining -= chunk_size;
        progress_counter++;

        if(progress_counter % 128 == 0 || remaining == 0) {
            printf("[BOOTLOADER] Progress: %lu/%lu bytes (%lu%%)\n",
                   offset, metadata->fw_size, (offset * 100) / metadata->fw_size);
        }
    }

    HAL_FLASH_Lock();
    printf("[BOOTLOADER] Copy completed: %lu bytes\n", offset);

    // Kopyalanan uygulamayı doğrula
    printf("[BOOTLOADER] Verifying copied application...\n");
    status = Bootloader_VerifyApplication(APP_START_ADDR,
                                         metadata->fw_size,
                                         0);
    if(status != HAL_OK) {
        printf("[BOOTLOADER] App area verification failed\n");
        return HAL_ERROR;
    }

    // Metadata güncelle
    printf("[BOOTLOADER] Clearing update flag...\n");
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);

    BootloaderMetadata_t updated_meta;
    memcpy(&updated_meta, metadata, sizeof(BootloaderMetadata_t));
    updated_meta.update_flag = 0;
    updated_meta.app_valid = 0xAAAA5555;
    updated_meta.boot_counter = 0;

    uint32_t metadata_words[32];
    memset(metadata_words, 0xFF, sizeof(metadata_words));
    memcpy(metadata_words, &updated_meta, sizeof(updated_meta));

    for(uint32_t i = 0; i < sizeof(metadata_words); i += 32) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                         METADATA_START_ADDR + i,
                         (uint32_t)&metadata_words[i/4]);
    }
    HAL_FLASH_Lock();

    printf("[BOOTLOADER] Update completed successfully!\n");
    return HAL_OK;
}

// YENİ: Bank2 helper fonksiyonlar
HAL_StatusTypeDef Bootloader_EraseBank2(uint32_t sectors)
{
    printf("[BOOTLOADER] Erasing Bank2 sectors (0-%lu)...\n", sectors-1);

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    uint32_t SectorError = 0;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Banks = FLASH_BANK_2;
    EraseInitStruct.Sector = 0;
    EraseInitStruct.NbSectors = sectors;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    HAL_StatusTypeDef result = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
    HAL_FLASH_Lock();

    if(result != HAL_OK) {
        printf("[BOOTLOADER] Bank2 erase failed at sector %lu\n", SectorError);
    }

    return result;
}

HAL_StatusTypeDef Bootloader_CopyToBank2(uint32_t src_addr, uint32_t size)
{
    printf("[BOOTLOADER] Copying %lu bytes to Bank2...\n", size);

    HAL_FLASH_Unlock();

    uint32_t offset = 0;
    uint8_t buffer[256];

    while(offset < size) {
        uint32_t chunk = (size - offset > 256) ? 256 : (size - offset);

        memcpy(buffer, (void*)(src_addr + offset), chunk);

        uint32_t aligned_chunk = ((chunk + 31) / 32) * 32;

        for(uint32_t i = 0; i < aligned_chunk; i += 32) {
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                               BANK2_STAGING_START + offset + i,
                               (uint32_t)&buffer[i]) != HAL_OK) {
                HAL_FLASH_Lock();
                return HAL_ERROR;
            }
        }

        offset += chunk;
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}

HAL_StatusTypeDef Bootloader_ValidateBank2(void)
{
    BootloaderMetadata_t *metadata = (BootloaderMetadata_t*)METADATA_START_ADDR;

    if(metadata->bank2_valid != 0x5A5A5A5A) {
        return HAL_ERROR;
    }

    return Bootloader_VerifyApplication(BANK2_STAGING_START,
                                       metadata->bank2_size,
                                       metadata->bank2_crc32);
}

HAL_StatusTypeDef Bootloader_CopyBank2ToBank1(void)
{
    BootloaderMetadata_t *metadata = (BootloaderMetadata_t*)METADATA_START_ADDR;

    if(metadata->bank2_valid != 0x5A5A5A5A) {
        printf("[BOOTLOADER] Bank2 not valid\n");
        return HAL_ERROR;
    }

    return Bootloader_PerformUpdateFromBank2();
}

void Bootloader_JumpToApplication(uint32_t app_addr)
{
    typedef void (*app_entry_point_t)(void);

    uint32_t stack_ptr = *(__IO uint32_t*)app_addr;
    app_entry_point_t app_entry = (app_entry_point_t)(*(__IO uint32_t*)(app_addr + 4));

    printf("[BOOTLOADER] Jumping to app at 0x%08lX\n", app_addr);
    printf("[BOOTLOADER] Stack: 0x%08lX, Entry: 0x%08lX\n",
           stack_ptr, (uint32_t)app_entry);

    __disable_irq();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    for(int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    SCB->VTOR = app_addr;
    __set_MSP(stack_ptr);
    __DSB();
    __ISB();

    app_entry();
    while(1);
}
