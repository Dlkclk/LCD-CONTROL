/**
 * @file ilitek_touch.c
 * @brief ILITEK Touch Controller Driver with LVGL Integration + DEBUG
 * @details Implements I2C communication protocol for ILITEK ILI2XXX series
 *          capacitive touch controllers with LVGL support
 * @author Dilek ÇELİK
 * @date 2025
 * @version 2.1 - Full Debug Version
 */

#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmsis_os.h"
#include "freertos_custom.h"
#include "DISPLAY_INC/ili9488.h"
#include "../../../LVGL_Library/lvgl/lvgl.h"

/* ======================== EXTERNAL DEPENDENCIES ======================== */

extern I2C_HandleTypeDef hi2c4;
extern volatile uint8_t tp_irq_flag;
extern TaskHandle_t TouchTask;
extern osMutexId_t touchMutexHandle;

/* ======================== PRIVATE CONSTANTS ======================== */

/**
 * @brief ILITEK Protocol Command Definitions
 */
#define ILITEK_CMD_GET_FIRMWARE_VER    0x40
#define ILITEK_CMD_GET_PROTOCOL_VER    0x42
#define ILITEK_CMD_GET_SCREEN_RES      0x21
#define ILITEK_CMD_GET_KEY_INFO        0x22
#define ILITEK_CMD_SLEEP_MODE          0x30
#define ILITEK_CMD_WAKE_MODE           0x31

/**
 * @brief Protocol packet structure definitions
 */
#define ILITEK_PACKET_SIZE             5
#define ILITEK_REPORT_BUFFER_SIZE      64
#define ILITEK_REPORT_COUNT_OFFSET     61

/**
 * @brief Visual feedback configuration
 */
#define INDICATOR_SIZE                 20
#define INDICATOR_DURATION_MS          20
#define INDICATOR_COLOR_TOUCH          ILI9488_RED
#define INDICATOR_COLOR_BACKGROUND     ILI9488_BLUE
#define MAX_INDICATORS                 12

// Visual feedback açık/kapalı
#define TOUCH_VISUAL_FEEDBACK          1

// Debug mode (set to 1 only when verbose touch logging is required)
#define TOUCH_DEBUG                    0

#if TOUCH_DEBUG
#define TOUCH_LOG(...) printf(__VA_ARGS__)
#else
#define TOUCH_LOG(...)
#endif

// Touch coordinate mapping options - test edip doğru olanı seçin
#define TOUCH_MAP_SWAP_XY              0   // 1 = X ve Y swap et
#define TOUCH_MAP_INVERT_X             0   // 1 = X axis'i invert et
#define TOUCH_MAP_INVERT_Y             0   // 1 = Y axis'i invert et

/* ======================== PRIVATE TYPES ======================== */

/**
 * @brief Touch indicator structure for visual feedback
 */
typedef struct {
    int16_t x;
    int16_t y;
    uint32_t expire_time;
    bool active;
} TouchIndicator_t;

/* ======================== PRIVATE VARIABLES ======================== */

static TouchController_t controller_info = {0};
static TouchIndicator_t indicators[MAX_INDICATORS] = {0};
static bool touch_initialized = false;
static uint16_t background_color = INDICATOR_COLOR_BACKGROUND;

// LVGL input device driver
static lv_indev_drv_t indev_drv;
static lv_indev_t *touch_indev = NULL;

// Son dokunma noktası (RELEASED durumunda pozisyonu korumak için)
static TouchPoint_t last_point = {0};

/* ======================== PRIVATE FUNCTION PROTOTYPES ======================== */

static int ilitek_send_command(uint8_t cmd, uint8_t *response, uint16_t response_len, uint32_t timeout_ms);
static int ilitek_read_resolution(void);
static void ilitek_map_coordinates(uint16_t raw_x, uint16_t raw_y, uint16_t *mapped_x, uint16_t *mapped_y);

/* ======================== LVGL CALLBACK IMPLEMENTATION ======================== */

/**
 * @brief LVGL touch input read callback
 * @param indev_drv Pointer to input device driver
 * @param data Pointer to input data structure to be filled
 */
void touch_read_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    TouchPoint_t points[1];
    int count;
    static bool last_state = false;
    bool mutex_acquired = false;

    // Initialize data structure with last known state (fallback)
    data->state = last_state ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = last_point.x;
    data->point.y = last_point.y;

    // Mutex koruma (eğer varsa)
    if (touchMutexHandle != NULL) {
        // Try to acquire mutex with timeout (don't block forever in callback)
        // Timeout 10ms - mutex alınamazsa bile last_point kullanılacak
        if (osMutexAcquire(touchMutexHandle, 10) == osOK) {
            mutex_acquired = true;
        } else {
            // Mutex alınamadı - last known state kullan (tutarlılık için)
            // Bu durumda LVGL'e en son bilinen durumu gönder
            return; // Last known state zaten set edildi
        }
    } else {
        mutex_acquired = true; // Mutex yoksa direkt oku
    }

    // Dokunma verisini oku
    count = Touch_ILITEK_Read(points, 1);

    if (count > 0 && points[0].pressed) {
        // Dokunma algılandı
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = points[0].x;
        data->point.y = points[0].y;

        // Son noktayı kaydet
        last_point = points[0];
        last_state = true;

        #if TOUCH_VISUAL_FEEDBACK
        // Görsel geri bildirim (LVGL ile çalışırken genelde kapalı olmalı)
        // Touch_DrawIndicator(points[0].x, points[0].y, INDICATOR_COLOR_TOUCH);
        #endif

    } else {
        // Dokunma yok
        data->state = LV_INDEV_STATE_RELEASED;
        // Keep last position for smooth release
        data->point.x = last_point.x;
        data->point.y = last_point.y;
        last_state = false;

        #if TOUCH_VISUAL_FEEDBACK
        // Tüm göstergeleri temizle
        Touch_ClearAllIndicators();
        #endif
    }

    // Mutex'i serbest bırak
    if (mutex_acquired && touchMutexHandle != NULL) {
        osMutexRelease(touchMutexHandle);
    }
}

/* ======================== PUBLIC FUNCTION IMPLEMENTATIONS ======================== */

/**
 * @brief Initialize LVGL touch input device
 * @return 0 on success, negative error code on failure
 */
int Touch_LVGL_Init(void)
{
    int result;

    // Önce hardware'i başlat
    result = Touch_ILITEK_Init();
    if (result != 0) {
        return result;
    }

    // LVGL input device driver'ı yapılandır
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    indev_drv.disp = lv_disp_get_default();  // Associate with default display
    
    // Input device ayarlarını optimize et (daha hızlı button response için)
    // Bu ayarlar register etmeden ÖNCE yapılmalı
    indev_drv.long_press_time = 300;          // Long press time (400ms -> 300ms) - daha hızlı click algılama
    indev_drv.long_press_repeat_time = 50;    // Long press repeat time (100ms -> 50ms)
    indev_drv.gesture_limit = 80;            // Gesture threshold (50px -> 80px) - yanlış gesture algılamayı azalt
    indev_drv.gesture_min_velocity = 5;      // Gesture min velocity (3px -> 5px) - daha net gesture algılama

    // Input device'ı LVGL'ye kaydet
    touch_indev = lv_indev_drv_register(&indev_drv);
    
    if (touch_indev == NULL) {
        return -1;
    }

    return 0;
}

/**
 * @brief Initialize the ILITEK touch controller (hardware only)
 */
int Touch_ILITEK_Init(void)
{
    int result;

    // Reset touch controller
    Touch_ILITEK_Reset(50);

    // Read and configure resolution
    result = ilitek_read_resolution();
    if (result != 0) {
        // Based on log analysis: Raw X range is approximately 0-450, Raw Y range is approximately 0-300
        // These values will be calibrated automatically during first touches
        controller_info.min_x = 0;
        controller_info.max_x = 450;  // Empirical max raw X (will be auto-calibrated)
        controller_info.min_y = 0;
        controller_info.max_y = 300;  // Empirical max raw Y (will be auto-calibrated)
    }

    controller_info.max_points = ILITEK_MAX_TOUCH_POINTS;
    controller_info.initialized = true;
    touch_initialized = true;

    return 0;
}

/**
 * @brief Read current touch status from the controller
 */
int Touch_ILITEK_Read(TouchPoint_t *points, uint8_t max_points)
{
    uint8_t buffer[ILITEK_REPORT_BUFFER_SIZE];
    HAL_StatusTypeDef status;
    int touch_count = 0;

    if (!touch_initialized || points == NULL) {
        return -1;
    }

    // Read touch data via I2C
    // Timeout 30ms - güvenilir okuma için (20ms çok kısa olabilir)
    // I2C read genelde 5-15ms sürer, 30ms güvenli bir değer
    status = HAL_I2C_Master_Receive(&hi2c4, ILITEK_I2C_ADDR_8BIT,
                                     buffer, ILITEK_REPORT_BUFFER_SIZE, 30);

    if (status != HAL_OK) {
        // Return 0 (no touch) instead of error for better compatibility with LVGL
        return 0;
    }

    // Verify report ID
    if (buffer[0] != ILITEK_REPORT_ID) {
        // Report ID yanlış - touch verisi yok, hemen çık
        return 0;  // No valid touch data - EARLY EXIT
    }

    // Report ID doğru, şimdi touch point'leri parse et
    uint8_t reported_points = buffer[ILITEK_REPORT_COUNT_OFFSET];

    if (reported_points > max_points) {
        reported_points = max_points;
    }

    for (uint8_t i = 0; i < reported_points && i < max_points; i++) {
        uint8_t offset = 1 + (i * ILITEK_PACKET_SIZE);

        // Check if point is active (bit 6 of status byte)
        if (buffer[offset] & 0x40) {
            points[touch_count].id = buffer[offset] & 0x3F;

            // Extract coordinates (little-endian 16-bit values)
            uint16_t raw_x = buffer[offset + 1] | (buffer[offset + 2] << 8);
            uint16_t raw_y = buffer[offset + 3] | (buffer[offset + 4] << 8);

            // Map to screen coordinates
            ilitek_map_coordinates(raw_x, raw_y,
                                  &points[touch_count].x,
                                  &points[touch_count].y);

            points[touch_count].pressed = true;
            touch_count++;
        }
    }

    return touch_count;
}

/**
 * @brief Reset the touch controller hardware
 */
void Touch_ILITEK_Reset(uint32_t reset_time_ms)
{
    HAL_GPIO_WritePin(CTP_RST_GPIO_Port, CTP_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(reset_time_ms);
    HAL_GPIO_WritePin(CTP_RST_GPIO_Port, CTP_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100);  // Wait for controller to initialize
}

/**
 * @brief Check if touch controller is responding
 */
bool Touch_ILITEK_IsAlive(void)
{
    uint8_t dummy[8];
    bool alive = (ilitek_send_command(ILITEK_CMD_GET_PROTOCOL_VER, dummy, 8, 50) == 0);
    return alive;
}

/**
 * @brief Get touch controller information
 */
const TouchController_t* Touch_ILITEK_GetInfo(void)
{
    return touch_initialized ? &controller_info : NULL;
}

/**
 * @brief Get LVGL input device handle
 */
lv_indev_t* Touch_GetLVGLDevice(void)
{
    return touch_indev;
}

#if TOUCH_DEBUG
/**
 * @brief Test touch read manually (for debugging)
 * @details Call this function to test touch reading outside of LVGL
 */
void Touch_TestManualRead(void)
{
    // Debug kapalıyken fonksiyon boş bırakılır
}
#else
void Touch_TestManualRead(void)
{
    // Debug kapalıyken fonksiyon boş bırakılır
}
#endif

/* ======================== PRIVATE FUNCTION IMPLEMENTATIONS ======================== */

/**
 * @brief Send command to touch controller and read response
 */
static int ilitek_send_command(uint8_t cmd, uint8_t *response, uint16_t response_len, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;

    // Send command
    status = HAL_I2C_Master_Transmit(&hi2c4, ILITEK_I2C_ADDR_8BIT, &cmd, 1, timeout_ms);
    if (status != HAL_OK) {
        return -1;
    }

    // Wait for processing
    HAL_Delay(5);

    // Read response if requested
    if (response && response_len > 0) {
        status = HAL_I2C_Master_Receive(&hi2c4, ILITEK_I2C_ADDR_8BIT,
                                        response, response_len, timeout_ms);
        if (status != HAL_OK) {
            return -2;
        }
    }

    return 0;
}

/**
 * @brief Read touch resolution from controller
 */
static int ilitek_read_resolution(void)
{
    uint8_t buffer[8];
    int result;

    result = ilitek_send_command(ILITEK_CMD_GET_SCREEN_RES, buffer, 8, 50);
    if (result == 0) {
        controller_info.min_x = buffer[0] | (buffer[1] << 8);
        controller_info.min_y = buffer[2] | (buffer[3] << 8);
        controller_info.max_x = buffer[4] | (buffer[5] << 8);
        controller_info.max_y = buffer[6] | (buffer[7] << 8);

        // Validate resolution
        if (controller_info.max_x == 0 || controller_info.max_y == 0 ||
            controller_info.max_x < controller_info.min_x ||
            controller_info.max_y < controller_info.min_y) {
            return -1;
        }
    }

    return result;
}

/**
 * @brief Map raw touch coordinates to screen coordinates
 * @details Maps raw touch controller coordinates to display screen coordinates
 *          Uses linear interpolation based on controller's actual coordinate ranges
 *          Supports axis inversion and swapping if touch panel orientation doesn't match display
 * 
 * @note CALIBRATION OPTIONS:
 *       If touch coordinates are wrong, try these fixes:
 *       1. Swap X/Y: swap raw_x and raw_y in the mapping
 *       2. Invert X: x = screen_x_range - 1 - x
 *       3. Invert Y: y = screen_y_range - 1 - y
 */
static void ilitek_map_coordinates(uint16_t raw_x, uint16_t raw_y,
                                   uint16_t *mapped_x, uint16_t *mapped_y)
{
    int32_t x, y;
    int32_t raw_x_val = (int32_t)raw_x;
    int32_t raw_y_val = (int32_t)raw_y;

    // Get touch controller coordinate ranges
    int32_t touch_min_x = (int32_t)controller_info.min_x;
    int32_t touch_max_x = (int32_t)controller_info.max_x;
    int32_t touch_min_y = (int32_t)controller_info.min_y;
    int32_t touch_max_y = (int32_t)controller_info.max_y;

    // Calculate touch controller ranges
    int32_t touch_x_range = touch_max_x - touch_min_x;
    int32_t touch_y_range = touch_max_y - touch_min_y;

    // Prevent division by zero
    if (touch_x_range <= 0) touch_x_range = 1;
    if (touch_y_range <= 0) touch_y_range = 1;

    // Screen coordinate ranges
    int32_t screen_x_range = TOUCH_SCREEN_WIDTH;
    int32_t screen_y_range = TOUCH_SCREEN_HEIGHT;

    // ============================================================
    // COORDINATE MAPPING - Based on actual touch data analysis
    // ============================================================
    // From logs: Raw X range ~47-418 (actual ~0-450), Raw Y range ~1-285 (actual ~0-300)
    // Screen: 320x480
    
    // X and Y SWAPPED + X AXIS INVERTED (based on user feedback)
    // Raw X (47-418) maps to screen Y, Raw Y (1-285) maps to screen X
    // X axis is inverted: right side acts like left side
    x = ((raw_y_val - touch_min_y) * screen_x_range) / touch_y_range;
    
    // Y ekseni düzeltmesi: Ekranın alt kısmında touch yukarı kayıyor
    // Sorun: Touch controller'ın Y range'i yanlış veya non-linear mapping gerekli
    // İlk iki mode (üstte) düzgün çalışıyor, aşağı gidildikçe kayma artıyor
    // Bu, touch X range'inin (screen Y'ye map ediliyor) gerçek değerden daha küçük olduğunu gösteriyor
    // Çözüm: Y mapping'inde touch X range'ini biraz genişlet
    // Touch X range'ini %5 genişlet - bu alt kısımdaki touch'ları daha aşağıya map edecek
    int32_t x_adjusted_range = touch_x_range + (touch_x_range * 5) / 100;  // %5 genişlet
    y = ((raw_x_val - touch_min_x) * screen_y_range) / x_adjusted_range;
    
    // Invert X axis (right becomes left, left becomes right)
    x = screen_x_range - 1 - x;

    // Clamp to valid screen range
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= TOUCH_SCREEN_WIDTH) x = TOUCH_SCREEN_WIDTH - 1;
    if (y >= TOUCH_SCREEN_HEIGHT) y = TOUCH_SCREEN_HEIGHT - 1;

    *mapped_x = (uint16_t)x;
    *mapped_y = (uint16_t)y;
}

/* ======================== VISUAL FEEDBACK IMPLEMENTATIONS ======================== */

/**
 * @brief Draw a visual indicator at touch position
 */
void Touch_DrawIndicator(uint16_t x, uint16_t y, uint16_t color)
{
    // Find free indicator slot
    for (int i = 0; i < MAX_INDICATORS; i++) {
        if (!indicators[i].active) {
            indicators[i].x = x;
            indicators[i].y = y;
            indicators[i].expire_time = HAL_GetTick() + INDICATOR_DURATION_MS;
            indicators[i].active = true;

            // Draw indicator
            fillCircle(x, y, INDICATOR_SIZE / 2, color);
            break;
        }
    }
}

/**
 * @brief Update and clean expired touch indicators
 */
void Touch_UpdateIndicators(void)
{
    uint32_t current_time = HAL_GetTick();

    for (int i = 0; i < MAX_INDICATORS; i++) {
        if (indicators[i].active) {
            if (current_time >= indicators[i].expire_time) {
                // Clear indicator
                fillCircle(indicators[i].x, indicators[i].y,
                          INDICATOR_SIZE / 2, background_color);
                indicators[i].active = false;
            }
        }
    }
}

/**
 * @brief Clear all active touch indicators immediately
 */
void Touch_ClearAllIndicators(void)
{
    for (int i = 0; i < MAX_INDICATORS; i++) {
        if (indicators[i].active) {
            fillCircle(indicators[i].x, indicators[i].y,
                      INDICATOR_SIZE / 2, background_color);
            indicators[i].active = false;
        }
    }
}

/**
 * @brief Set background color for indicator clearing
 */
void Touch_SetBackgroundColor(uint16_t color)
{
    background_color = color;
}

/* ======================== DEPRECATED FUNCTIONS ======================== */

/**
 * @deprecated Use Touch_LVGL_Init() instead
 */
void Touch_ReadAndDraw_Dot(void)
{
    TouchPoint_t points[1];
    int count = Touch_ILITEK_Read(points, 1);

    if (count > 0) {
        Touch_DrawIndicator(points[0].x, points[0].y, INDICATOR_COLOR_TOUCH);
    }
}

/**
 * @deprecated Use Touch_UpdateIndicators() instead
 */
void Blips_Tick(void)
{
    Touch_UpdateIndicators();
}
