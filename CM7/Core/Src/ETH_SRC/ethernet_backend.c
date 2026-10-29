/**
  ******************************************************************************
  * @file    ethernet_backend.c
  * @brief   Ethernet Backend Module Implementation
  * @author  Embedded Firmware Team
  * @date    2025
  ******************************************************************************
  * @attention
  *
  * This module implements all Ethernet/HTTP backend functionality:
  * - LwIP HTTP server management  
  * - CGI handlers for REST API endpoints
  * - SSI handlers for dynamic content
  * - JSON response generation
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include "../../Inc/freertos_custom.h"


//VERY IMPORTANT - FIRMWARE FILENAMING HAVE TO THIS
/*GNSS_NEW_VERSION_FIRMWARE_V1234.bin*/









/**
 * SSI (Server Side Include) tags for dynamic content injection in HTML pages
 * These tags are replaced with real-time data when serving web pages
 *
 * Tag mappings:
 * - "terminal"      : Real-time NMEA sentence stream for terminal display
 * - "gnrmc"        : Latest GPS position data (lat/lon/speed/time)
 * - "graph"        : Satellite signal strength data for SNR visualization
 * - "jam"          : Jamming detection status for all frequencies
 * - "alert"        : Alert notifications for jamming/spoofing events
 * - "csv"          : CSV export progress and data chunks
 * - "firmware"     : Firmware update status and progress
 * - "config-output": UART configuration settings response
 * - "info"         : Device information and firmware version
 * - "login"        : Authentication status response
 *
 * @note Tags are processed by EthernetBackend_SSIHandler() function
 * @note Each tag corresponds to an index in the handler (0-9)
 */
static const char *ssi_tags[] = {
    "terminal",
    "gnrmc",
	"graph",
	"jam",
	"alert",
	"csv",
	"firmware",
	"config-output",
    "info",
	"login"

};










/////////////////////////////////////////////////////////////////////////// GLOBAL-STATIC MACRO AND DEFINE DEFINITIONS BEGIN  /////////////////////////////////////////////////////////////




// External State Variables
extern volatile uint8_t StateGraphExt;




// NMEA Processing Macros
#define MAX_GBGSV_ENTRIES 50                           // Maximum satellite entries for GSV processing - Used in: EthernetBackend_AddAll_NMAE_SentenceDashboard()




// Terminal and NMEA Response Buffers
static char terminal_logs_response[8192];              // Terminal logs JSON response buffer - Used in: cgi_get_terminal_logs_handler(), EthernetBackend_SSIHandler()
static char gnrmc_logs_response[4600];                 // GNRMC data JSON response buffer - Used in: cgi_get_gnrmc_handler(), EthernetBackend_SSIHandler()
static char graph_nmea_response[4600];                 // Graph data JSON response buffer - Used in: cgi_get_graph_nmea_handler(), EthernetBackend_SSIHandler()




// UART NMEA Data Collection Buffers
static char uart_nmea_buffer[16384] = {0};             // Raw NMEA sentences buffer from UART3 - Used in: EthernetBackend_AddNMEASentence(), cgi_get_terminal_logs_handler()
static char uart_gnrmc_buffer[4096];                   // GNRMC sentences accumulation buffer - Used in: EthernetBackend_AddGNRMCSentence(), cgi_get_gnrmc_handler()
static uint8_t uart_all_nmea_buffer[8192];             // All NMEA data buffer for graph processing - Used in: EthernetBackend_AddAll_NMAE_SentenceDashboard(), cgi_get_graph_nmea_handler(), EthernetBackend_SSIHandler()




// NMEA Processing Counters
static uint16_t uart_nmea_count = 0;                   // NMEA sentence counter - Used in: EthernetBackend_AddNMEASentence(), cgi_get_terminal_logs_handler()
static uint16_t uart_gnrmc_count = 0;                  // GNRMC sentence counter - Used in: EthernetBackend_AddGNRMCSentence()
size_t uart_gbgsv_count = 0;                           // GSV sentence counter - Used in: EthernetBackend_AddAll_NMAE_SentenceDashboard(), EthernetBackend_SSIHandler()




// State Management Variables
uint8_t stateGraphHandler = 0;                         // Graph handler state flag - Used in: cgi_get_graph_nmea_handler(), EthernetBackend_AddAll_NMAE_SentenceDashboard(), EthernetBackend_SSIHandler()




// Jamming/Spoofing Detection Buffers
static char jamming_spoofing_response[512];            // Jamming/spoofing JSON response buffer - Used in: cgi_get_jamming_spoofing_handler(), EthernetBackend_SSIHandler()
static char alert_response[512];                       // Alert JSON response buffer - Used in: cgi_get_alert_handler(), EthernetBackend_SSIHandler()
uint8_t jam_buffer_global[7];                          // Global jamming status array (7 frequencies) - Used in: EthernetBackend_Add_JammingSpoofingSentence(), cgi_get_alert_handler(), cgi_get_jamming_spoofing_handler()
uint8_t spoof_buffer_global[7];                        // Global spoofing status array (7 frequencies) - Used in: EthernetBackend_Add_JammingSpoofingSentence(), cgi_get_alert_handler(), cgi_get_jamming_spoofing_handler()




// Firmware Update Variables
static char firmware_response[2048];                   // JSON response buffer for firmware upload status - Used in: cgi_get_firmware_update_handler(), EthernetBackend_SSIHandler()
uint8_t firmware_upload_active = 0;                    // Upload in progress flag - Used in: cgi_get_firmware_update_handler()
uint32_t firmware_write_offset = 0;                    // Current write position in flash memory - Used in: cgi_get_firmware_update_handler()
volatile uint8_t firmwareUpdateStatus = 0;             // Blocks other CGI handlers during update - Used in: ALL cgi_get_*_handler() functions
volatile uint8_t firmwareUpdateReadStatus = 0;         // Signals firmware ready for bootloader - Used in: cgi_get_firmware_update_handler()




/**
 * Base64 character-to-value lookup table for decoding
 * Maps ASCII characters to their Base64 values (0-63)
 * 255 = invalid character, 0-63 = valid Base64 values
 * Used in: base64_decode() function for firmware chunk decoding
 * @note Array index corresponds to ASCII character value
 */
static const uint8_t base64_decode_table[128] = {
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
     52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
    255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
     15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
    255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
     41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255
};




// CSV Export Variables
static char csv_response[8000];                        // JSON response buffer for CSV chunks - Used in: cgi_get_csv_file_handler(), EthernetBackend_SSIHandler()
static char csv_escaped_buffer[4200];                  // JSON-escaped data buffer - Used in: cgi_get_csv_file_handler()
static uint8_t csv_read_buffer[2048];                  // Raw data buffer from flash - Used in: cgi_get_csv_file_handler()
static uint32_t csv_read_offset = 0;                   // Current read position in flash - Used in: cgi_get_csv_file_handler()
static uint32_t csv_total_chunks = 0;                  // Total chunks to export - Used in: cgi_get_csv_file_handler()
static uint32_t csv_current_chunk = 0;                 // Current chunk being sent - Used in: cgi_get_csv_file_handler()
static uint32_t frozen_write_index = 0;                // Snapshot of write index during export - Used in: cgi_get_csv_file_handler()
volatile uint8_t flash_busy_flag = 0;                  // Flash access mutex flag - Used in: cgi_get_csv_file_handler()
volatile uint8_t CSV_ReadStatus = 0;                   // CSV read operation status - Used in: cgi_get_csv_file_handler()
uint8_t csv_export_active = 0;                         // Export in progress flag - Used in: cgi_get_csv_file_handler()




// UART Configuration Variables
static char config_response[256];                      // JSON response buffer for UART configuration - Used in: cgi_get_output_configuration_handler(), EthernetBackend_SSIHandler()




// Authentication Variables
static char login_response[256];                       // JSON response buffer for authentication - Used in: cgi_get_login_handler(), EthernetBackend_SSIHandler()
static const char* VALID_USERNAME = "sealink";         // Default username for web interface authentication - Used in: cgi_get_login_handler()
static const char* VALID_PASSWORD = "sealink";         // Default password for web interface authentication - Used in: cgi_get_login_handler()




// Device Information Variables
static char device_info_response[512];                                             // JSON response buffer for device information - Used in: cgi_get_device_info_handler(), EthernetBackend_SSIHandler()
static const char* DEVICE_PRODUCT_NAME    = "STM32-LCD-CONTROLLER";                        // Product name constant - Used in: cgi_get_device_info_handler(), EthernetBackend_SSIHandler()
static const char* DEVICE_SERIAL_NUMBER   = "GNSS-2025-001234";                   // Device serial number constant - Used in: cgi_get_device_info_handler(), EthernetBackend_SSIHandler()
static const char* DEVICE_MANUFACTURER    = "Sealink Technology B.V.";            // Manufacturer name constant - Used in: cgi_get_device_info_handler(), EthernetBackend_SSIHandler()
static const char* DEVICE_ADDRESS         = "Dalweg 3B 3233KK Ostvoorne, The Netherlands"; // Manufacturer address constant - Used in: cgi_get_device_info_handler(), EthernetBackend_SSIHandler()
static const char* DEVICE_WEBSITE         = "www.sealink.co";                     // Company website constant - Used in: cgi_get_device_info_handler(), EthernetBackend_SSIHandler()
static const char* DEVICE_FIRMWARE_VERSION= "v1.0.0";                             // Firmware version string constant - Used in: cgi_get_device_info_handler(), EthernetBackend_SSIHandler()

/////////////////////////////////////////////////////////////////////////// GLOBAL-STATIC MACRO AND DEFINE DEFINITIONS END   ///////////////////////////////////////////////////////////////























////////////////////////////////////////////////////////////////////////////   STATIC FUNCTION DEFINITIONS BEGIN   ////////////////////////////////////////////////////////////////////////////

/**
 * Splits GPS/GNSS NMEA data by comma delimiter
 * Used to parse GNRMC message fields like UTC, position, speed
 * Example input: "GN,12:31:17,4047.59811,02927.81033,0.053"
 * @return Number of fields found
 * @note Used in cgi_get_gnrmc_handler() to parse GPS data for JSON response
 */
static int split_csv(char *line, char *tok[], int max_tok)
{
    int n = 0;
    for (char *p = line; *p && n < max_tok; ++p)
    {
        if (n == 0) tok[n++] = p;
        if (*p == ',') { *p = '\0'; tok[n++] = p + 1; }
    }
    return n;
}




/**
 * Decodes Base64 encoded string to binary data
 * Converts firmware chunks from Base64 format to raw binary for flash writing
 * @param input Base64 encoded string from HTTP request
 * @param output Buffer to store decoded binary data
 * @param max_output_len Maximum size of output buffer
 * @return Number of decoded bytes, or 0 if decode failed
 * @note Used by cgi_get_firmware_update_handler() to decode firmware chunks
 */
static int base64_decode(const char* input, uint8_t* output, int max_output_len)
{
    int input_len = strlen(input);
    int output_len = 0;
    uint32_t accumulator = 0;
    int bits = 0;

    for(int i = 0; i < input_len && output_len < max_output_len; i++)
    {
        char c = input[i];
        if(c == '=') break;
        if(c & 0x80 || base64_decode_table[(int)c] == 255) continue;

        accumulator = (accumulator << 6) | base64_decode_table[(int)c];
        bits += 6;

        if(bits >= 8) {
            bits -= 8;
            output[output_len++] = (accumulator >> bits) & 0xFF;
        }
    }

    return output_len;
}

////////////////////////////////////////////////////////////////////////////   STATIC FUNCTION DEFINITIONS END  ////////////////////////////////////////////////////////////////////////////
























////////////////////////////////////////////////////////////////////////////   UART DATA COLLECTION FUNCTIONS BEGIN /////////////////////////////////////////////////////////////////////////

/**
 * Collects and formats NMEA sentences from UART for terminal display
 * Filters for GNRMC, GNGGA, GNVTG, and GSV messages only
 * Maintains rolling buffer of last 50 sentences in JSON array format
 * @param nmea_sentence Raw NMEA sentence from UART3
 * @note Used by cgi_get_terminal_logs_handler() for terminal-logs.shtml
 */
void EthernetBackend_AddNMEASentence(const char* nmea_sentence)
{
    if (uart_nmea_count < 50)
    {
    	// Keep last 50 sentences
        char cleaned_sentence[256];
        // 🛠️ STEP 1: Clean raw newlines and carriage returns from input
        int clean_idx = 0;
        for (int i = 0; nmea_sentence[i] != '\0' && clean_idx < sizeof(cleaned_sentence) - 1; i++)
        {
            if (nmea_sentence[i] != '\n' && nmea_sentence[i] != '\r')
            {
                cleaned_sentence[clean_idx++] = nmea_sentence[i];
            }
        }
        cleaned_sentence[clean_idx] = '\0';

        // Filter to only keep sentences the frontend currently displays
        if (!(strncmp(cleaned_sentence, "$GNRMC", 6) == 0 ||
              strncmp(cleaned_sentence, "$GNGGA", 6) == 0 ||
              strncmp(cleaned_sentence, "$GNVTG", 6) == 0 ||
              strncmp(cleaned_sentence, "$GPGSV", 6) == 0 ||
              strncmp(cleaned_sentence, "$GLGSV", 6) == 0 ||
              strncmp(cleaned_sentence, "$GAGSV", 6) == 0 ||
              strncmp(cleaned_sentence, "$GNGST", 6) == 0 ||
              strncmp(cleaned_sentence, "$GNGBS", 6) == 0 ||
              strncmp(cleaned_sentence, "$GNZDA", 6) == 0 ||
              strncmp(cleaned_sentence, "$GBGSV", 6) == 0))
        {
            return; // skip other types to ensure text area shows lines
        }

        // Add to buffer with JSON escaping (no extra newlines needed)
        if (uart_nmea_count == 0)
        {
            strncat(uart_nmea_buffer, "\"", sizeof(uart_nmea_buffer) - strlen(uart_nmea_buffer) - 1);
        }
        else
        {
            strncat(uart_nmea_buffer, ",\"", sizeof(uart_nmea_buffer) - strlen(uart_nmea_buffer) - 1);
        }
        strncat(uart_nmea_buffer, cleaned_sentence, sizeof(uart_nmea_buffer) - strlen(uart_nmea_buffer) - 1);
        strncat(uart_nmea_buffer, "\"", sizeof(uart_nmea_buffer) - strlen(uart_nmea_buffer) - 1);

        uart_nmea_count++;
        //printf("UART: Added NMEA sentence #%d: %.50s...\r\n", uart_nmea_count, cleaned_sentence);
    }
    else
    {
        // Buffer full, reset and start fresh
        memset(uart_nmea_buffer, 0, sizeof(uart_nmea_buffer));
        uart_nmea_count = 0;
        //printf("UART: Buffer reset, starting fresh collection\r\n");
    }
}






/**
 * Stores GNRMC sentences for GPS position and speed tracking
 * Maintains buffer of last 50 GNRMC messages in JSON string format
 * Extracts UTC, date, latitude, longitude, speed from NMEA data
 * @param nmea_sentence Raw GNRMC sentence from GPS receiver
 * @note Used by cgi_get_gnrmc_handler() for gnrmc-logs.shtml
 */
void EthernetBackend_AddGNRMCSentence(const char* nmea_sentence)
{
    if (!nmea_sentence) return;

    // Kapasite dolduysa sıfırla (50 satır tut)
    if (uart_gnrmc_count >= 50) {
        uart_gnrmc_buffer[0] = '\0';
        uart_gnrmc_count = 0;
    }

    // 1) CR/LF temizle
    char cleaned[256];
    int ci = 0;
    for (int i = 0; nmea_sentence[i] != '\0' && ci < (int)sizeof(cleaned) - 1; ++i) {
        char ch = nmea_sentence[i];
        if (ch != '\r' && ch != '\n') cleaned[ci++] = ch;
    }
    cleaned[ci] = '\0';
    if (ci == 0) return;



    // JSON string listesine güvenli ekleme:  ,"<satır>"
    // mevcut kullanım ve kalan kapasiteyi hesapla
    size_t used = strlen((const char*)uart_gnrmc_buffer);
    size_t cap  = sizeof(uart_gnrmc_buffer);

    // eklenecek parça: ilk öğede "\"<satır>\"" (3 + len), sonrakilerde ",\"<satır>\"" (4 + len)
    size_t need = (uart_gnrmc_count == 0 ? 2 : 3) + strlen(cleaned) + 1; // açılış + satır + kapanış + NUL
    if (used + need >= cap) {
        // yer yoksa resetle ve en baştan ekle
        uart_gnrmc_buffer[0] = '\0';
        uart_gnrmc_count = 0;
        used = 0;
    }

    snprintf((char*)uart_gnrmc_buffer + used, cap - used, "%s%s\"", (uart_gnrmc_count == 0) ? "\"" : ",\"", cleaned);
    uart_gnrmc_count++;
}






/**
 * Updates jamming and spoofing detection status for 5 GPS frequencies
 * Stores detection flags for L1/L5/L2/E6/L1C frequencies
 * @param jam_buffer 7-byte array with jamming status per frequency
 * @param spoof_buffer 7-byte array with spoofing status per frequency
 * @note Used by cgi_get_jamming_spoofing_handler() and cgi_get_alert_handler()
 */
void EthernetBackend_Add_JammingSpoofingSentence(const char* jam_buffer, const char* spoof_buffer)
{
    // Gelen bufferları global değişkenlere kopyala
    memcpy(jam_buffer_global, jam_buffer, 7);
    memcpy(spoof_buffer_global, spoof_buffer, 7);
}






/**
 * Collects satellite signal data for SNR graph visualization
 * Parses GSV sentences to extract PRN and signal strength values
 * Maintains buffer for up to 50 satellite entries (G/R/E/C constellations)
 * @param gbgsv_sentence Raw GSV sentence containing satellite data
 * @note Used by cgi_get_graph_nmea_handler() for graph-nmea.shtml
 */
void EthernetBackend_AddAll_NMAE_SentenceDashboard(const char* gbgsv_sentence)
{
    if (!gbgsv_sentence) return;

    

    size_t current_size = strlen((const char*)uart_all_nmea_buffer);
    if (current_size > 7000) {  // %85 doluluk
        uart_all_nmea_buffer[0] = '\0';
        uart_gbgsv_count = 0;
        //printf("[WARNING] Buffer reset at 85%% capacity\n");
        return;
    }


    if (uart_gbgsv_count >= MAX_GBGSV_ENTRIES * 6)
    {
    	// 300 instead of 100 (allow full constellation cycles)
    	//printf("[BUFFER] Emergency overflow reset at %u entries\n", (unsigned int)uart_gbgsv_count);
    	uart_all_nmea_buffer[0] = '\0';
        uart_gbgsv_count = 0;
    }

    // CR/LF temizle
    char cleaned[256];
    int ci = 0;
    for (int i = 0; gbgsv_sentence[i] != '\0' && ci < (int)sizeof(cleaned) - 1; ++i) {
        char ch = gbgsv_sentence[i];
        if (ch != '\r' && ch != '\n') cleaned[ci++] = ch;
    }
    cleaned[ci] = '\0';
    if (ci == 0) return;

    size_t used = strlen((const char*)uart_all_nmea_buffer);
    size_t cap  = sizeof(uart_all_nmea_buffer);

    // eklenecek parça: ilk öğede "\"<satır>\"" , sonrakilerde ",\"<satır>\""
    size_t need = (uart_gbgsv_count == 0 ? 2 : 3) + strlen(cleaned) + 1;
    if (used + need >= cap)
    {
    	uart_all_nmea_buffer[0] = '\0';
        uart_gbgsv_count = 0;
        used = 0;
    }


    if((StateGraphExt == 1))
    {
    	if((stateGraphHandler == 0))
    	{
    snprintf((char*)uart_all_nmea_buffer + used, cap - used,
             "%s%s\"",
             (uart_gbgsv_count == 0) ? "\"" : ",\"",
             cleaned);
    uart_gbgsv_count++;
    	}
    	else
    	{
    		memset(uart_all_nmea_buffer, 0, sizeof(uart_all_nmea_buffer));
    		stateGraphHandler = 0;
    	}


    }

}

////////////////////////////////////////////////////////////////////////////   UART DATA COLLECTION FUNCTIONS END ///////////////////////////////////////////////////////////////////////
























////////////////////////////////////////////////////////////////////////////   CGI HANDLER FUNCTIONS BEGIN //////////////////////////////////////////////////////////////////////////////

/**
 * Extracts latest GPS position data from GNRMC sentence buffer
 * Parses UTC time, date, latitude, longitude, speed and course over ground
 * Returns JSON response with navigation data for real-time position tracking
 * @return JSON response for gnrmc-logs.shtml AJAX requests
 * @note Updates every second with most recent GNRMC sentence
 */
const char* cgi_get_gnrmc_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{


    return "/gnrmc-logs.shtml";
}







/**
 * Processes satellite signal strength data for web graph display
 * Parses GSV buffer to extract PRN and SNR values for visualization
 * Generates JSON with up to 64 satellite signals from GPS/GLONASS/Galileo/BeiDou
 * @return JSON response for graph-nmea.shtml AJAX request
 * @note Maximum 64 signals to prevent buffer overflow
 */
const char* cgi_get_graph_nmea_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    return "/graph-nmea.shtml";
}







/**
 * Handles jamming/spoofing alert status requests for 5 GPS frequencies
 * Returns detection status for L1/L5/L2/E6/L1C bands (1575/1176/1227/1268/1278 MHz)
 * @return JSON response for alert.shtml AJAX polling
 * @note Updates every second with real-time detection status
 */
const char* cgi_get_alert_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    return "/alert.shtml";
}







/**
 * Provides detailed jamming/spoofing status for frequency monitoring page
 * Returns JSON with detection flags for each GPS frequency band
 * @return JSON response for jamming-spoofing.shtml dashboard
 * @note Identical to alert handler but for different web page
 */
const char* cgi_get_jamming_spoofing_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    return "/jamming-spoofing.shtml";
}







/**
 * Streams real-time NMEA sentences for terminal display
 * Returns buffered GNRMC/GNGGA/GNVTG/GSV messages in JSON array format
 * @return JSON response for terminal-logs.shtml live monitoring
 * @note Auto-clears buffer after 30 messages to prevent overflow
 */
const char* cgi_get_terminal_logs_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    return "/terminal-logs.shtml";
}







/**
 * Manages CSV data export from flash memory in chunks
 * Reads 2KB chunks from SST26 flash and escapes for JSON transmission
 * Supports start/stop actions with progress tracking
 * @return JSON response with chunk data for csv-download.shtml
 * @note Freezes write index during export to ensure data consistency
 */
const char* cgi_get_csv_file_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{


   return "/csv-download.shtml";
}







/**
 * Handles firmware update via chunked Base64 upload
 * Manages flash erase, Base64 decode, and progressive write to SST26
 * Supports start/chunk/stop actions with 512-byte chunks
 * @return JSON response with upload progress for firmware-update.shtml
 * @note Writes to flash address 0x200000 with 2MB maximum size
 */
const char* cgi_get_firmware_update_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    return "/firmware-update.shtml";
}







/**
 * Configures UART3 output baudrate for NMEA data transmission
 * Supports standard rates: 9600/19200/38400/57600/115200/230400
 * Reinitializes UART with new settings and FIFO configuration
 * @return JSON response for output-configuration.shtml settings page
 * @note Maintains 8N1 format with no flow control
 */
const char* cgi_get_output_configuration_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    return "/output-configuration.shtml";
}







/**
 * Authenticates user credentials for web interface access
 * Validates username/password against hardcoded credentials
 * @return JSON response with authentication status for login.shtml
 * @note Default credentials: Configured by VALID_USERNAME and VALID_PASSWORD above
 */
const char* cgi_get_login_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	if(firmwareUpdateStatus == 1) firmwareUpdateStatus = 0;
	//printf("firmwareUpdateStatus: %d  -------  CSV_ReadStatus : %d\n", firmwareUpdateStatus, CSV_ReadStatus);


    char *username = NULL;
    char *password = NULL;

    for(int i = 0; i < iNumParams; i++)	// Parametreleri parse et
    {
        if(strcmp(pcParam[i], "username") == 0)
        {
            username = pcValue[i];
        }
        else if(strcmp(pcParam[i], "passw	ord") == 0)
        {
            password = pcValue[i];
        }
    }


    if(username && password)	// Kullanıcı adı ve şifre kontrolü
    {
        if(strcmp(username, VALID_USERNAME) == 0 && strcmp(password, VALID_PASSWORD) == 0)
        {
            snprintf(login_response, sizeof(login_response), "{\"status\":\"success\",\"authenticated\":1,\"message\":\"Login successful\"}");	// Login başarılı
        }
        else
        {
            snprintf(login_response, sizeof(login_response), "{\"status\":\"error\",\"authenticated\":0,\"message\":\"Invalid username or password\"}");	// Login başarısız
        }
    }
    else
    {
        snprintf(login_response, sizeof(login_response), "{\"status\":\"error\",\"authenticated\":0,\"message\":\"Missing credentials\"}");	// Parametreler eksik
    }

    return "/login.shtml";
}








/**
 * Provides device information and firmware details
 * Returns product name, serial number, manufacturer, and version info
 * @return JSON response for info.shtml device information page
 * @note Static device info for system identification
 */
const char* cgi_get_device_info_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    return "/info.shtml";

}

////////////////////////////////////////////////////////////////////////////   CGI HANDLER FUNCTIONS END /////////////////////////////////////////////////////////////////////////////////
























/**
 * @brief Register CGI Handlers for HTTP Server REST API Endpoints
 *
 * Registers all CGI (Common Gateway Interface) handlers with the LwIP HTTP server.
 * Maps URL endpoints to their corresponding handler functions for dynamic content generation.
 * These handlers process HTTP GET/POST requests and return appropriate responses.
 *
 * Registered API endpoints:
 * - /api/system/logs       : Terminal NMEA sentence streaming
 * - /api/nmea/gnrmc       : GPS position and navigation data
 * - /api/nmea/graph       : Satellite signal strength data for graphs
 * - /api/nmea/jam-spoof   : Jamming/spoofing detection status
 * - /api/nmea/alert       : Real-time alert notifications
 * - /api/nmea/csv         : CSV data export from flash memory
 * - /api/firmware-update  : Firmware update via Base64 chunks
 * - /api/output-config    : UART3 baudrate configuration
 * - /api/device-info      : Device information and version
 * - /api/get-login-logs   : Authentication handler
 * - /service/get-terminal-logs : Legacy terminal endpoint
 *
 * @note Called once during HTTP server initialization
 * @note All handlers must follow LwIP CGI callback signature
 * @note Maximum URL length limited by LWIP_HTTPD_MAX_REQUEST_URI_LEN
 */
void EthernetBackend_RegisterCGIHandlers(void)
{
    // Ajax-Centric RESTful API Endpoints
	static const tCGI cgi_handlers[] = {
	    {"/api/system/logs", 			cgi_get_terminal_logs_handler},
	    {"/api/nmea/gnrmc",  			cgi_get_gnrmc_handler},
	    {"/api/nmea/graph",  			cgi_get_graph_nmea_handler},

	    {"/api/nmea/jam-spoof",    		cgi_get_jamming_spoofing_handler},
	    {"/api/nmea/alert",    	   		cgi_get_alert_handler},
	    {"/api/nmea/csv",    	   		cgi_get_csv_file_handler},
	    {"/api/firmware-update",   		cgi_get_firmware_update_handler},
	    {"/api/output-config",     		cgi_get_output_configuration_handler},
        {"/api/device-info", 			cgi_get_device_info_handler},  // Yeni handler
        {"/api/get-login-logs", 		cgi_get_login_handler},

	    {"/service/get-terminal-logs",  cgi_get_terminal_logs_handler},
	};

    http_set_cgi_handlers(cgi_handlers, sizeof(cgi_handlers)/sizeof(cgi_handlers[0]));
}















/**
 * @brief Register SSI Handler for Dynamic Content Injection
 *
 * Registers the Server Side Include (SSI) handler with the LwIP HTTP server.
 * SSI enables dynamic content injection into static HTML pages at runtime.
 * When the server encounters SSI tags in HTML files, it calls the registered
 * handler to replace those tags with dynamically generated content.
 *
 * SSI Processing Flow:
 * 1. HTTP server loads .shtml file containing SSI tags (<!--#tag_name-->)
 * 2. Server identifies SSI tags and calls EthernetBackend_SSIHandler
 * 3. Handler generates appropriate JSON/text content based on tag index
 * 4. Server replaces tag with generated content before sending to client
 *
 * Registered SSI Tags (10 total):
 * - Index 0: "terminal"      - NMEA sentence stream for terminal display
 * - Index 1: "gnrmc"         - GPS position data (lat/lon/speed/time)
 * - Index 2: "graph"         - Satellite SNR data for signal strength graph
 * - Index 3: "jam"           - Jamming detection status (5 frequencies)
 * - Index 4: "alert"         - Alert notifications for jamming/spoofing
 * - Index 5: "csv"           - CSV export progress and data chunks
 * - Index 6: "firmware"      - Firmware update status and progress
 * - Index 7: "config-output" - UART configuration response
 * - Index 8: "info"          - Device information and version
 * - Index 9: "login"         - Authentication status response
 *
 * @note Called once during HTTP server initialization
 * @note SSI tags must be defined in ssi_tags[] array
 * @note Handler function must match lwIP SSI callback signature
 * @note Maximum SSI response limited by LWIP_HTTPD_MAX_TAG_INSERT_LEN
 * @see EthernetBackend_SSIHandler() for tag processing implementation
 */
void EthernetBackend_RegisterSSIHandler(void)
{
    http_set_ssi_handler(EthernetBackend_SSIHandler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
    
}




















/**
 * @brief SSI Handler Callback for Dynamic HTML Content Injection
 *
 * Processes Server Side Include (SSI) tags found in .shtml files and replaces them
 * with dynamically generated content. Called by LwIP HTTP server when SSI tags are
 * encountered during HTML page serving. Each tag is replaced with real-time data
 * formatted as JSON or text strings.
 *
 * @param iIndex Tag index (0-9) corresponding to ssi_tags[] array position
 * @param pcInsert Output buffer where dynamic content will be written
 * @param iInsertLen Maximum size of pcInsert buffer (typically 192 bytes)
 * @param current_tag_part Current part number for multipart SSI (if enabled)
 * @param next_tag_part Pointer to set next part number for multipart SSI
 * @param connection_state HTTP connection state information (if enabled)
 *
 * @return Number of bytes written to pcInsert buffer
 *
 * Tag Index Mapping:
 * - 0: "terminal"      - NMEA sentences array for terminal display (8192 bytes max)
 * - 1: "gnrmc"         - GPS position JSON (UTC/date/lat/lon/speed/COG)
 * - 2: "graph"         - Satellite signal strength JSON array (up to 64 signals)
 * - 3: "jam"           - Jamming detection status for 5 frequencies
 * - 4: "alert"         - Alert notifications JSON for jamming/spoofing events
 * - 5: "csv"           - CSV export progress JSON (chunk info and data)
 * - 6: "firmware"      - Firmware update status JSON (progress/errors)
 * - 7: "config-output" - UART configuration response JSON
 * - 8: "info"          - Device information JSON (generated on-demand if empty)
 * - 9: "login"         - Authentication status JSON response
 *
 * Processing Steps:
 * 1. Identify tag by index
 * 2. Check if response buffer has content (provide default if empty)
 * 3. Calculate content length and enforce buffer limits
 * 4. Clear destination buffer for safety
 * 5. Copy content to pcInsert buffer
 * 6. Add null terminator
 * 7. Clear source buffer after use (prevents duplicate data)
 * 8. Return bytes written
 *
 * @note Buffer overflow protection: Content truncated if exceeds iInsertLen
 * @note Source buffers cleared after use to prevent stale data
 * @note Graph tag also clears auxiliary buffers and resets state
 * @note Device info generated on-demand if buffer empty
 * @note All responses are JSON formatted for AJAX consumption
 *
 * @warning Maximum insert length limited by LWIP_HTTPD_MAX_TAG_INSERT_LEN
 * @warning Must complete quickly to avoid blocking HTTP server
 */
u16_t EthernetBackend_SSIHandler(int iIndex, char *pcInsert, int iInsertLen
#if LWIP_HTTPD_SSI_MULTIPART
  , u16_t current_tag_part, u16_t *next_tag_part
#endif
#if defined(LWIP_HTTPD_FILE_STATE) && LWIP_HTTPD_FILE_STATE
  , void *connection_state
#endif
)
{
    if (iIndex == 0)
    {

        memset(graph_nmea_response, 0, sizeof(graph_nmea_response));
        memset(uart_all_nmea_buffer, 0, sizeof(uart_all_nmea_buffer));


        int len = strlen(terminal_logs_response);

        if (len > iInsertLen - 1)
        {
            len = iInsertLen - 1;
        }

        memset(pcInsert, ' ', iInsertLen);
        memset(pcInsert, 0, iInsertLen);

        memcpy(pcInsert, terminal_logs_response, len);		// Copy our JSON content to COMPLETELY replace the tag

        pcInsert[len] = '\0';								// Add explicit null terminator


        if (len < iInsertLen - 1)							// SAFETY: Clear any possible trailing data
        {
            memset(pcInsert + len + 1, 0, iInsertLen - len - 1);
        }

        memset(terminal_logs_response, 0, sizeof(terminal_logs_response));

        return len;
    }
    else if (iIndex == 1)  // "gnrmc" tag
    {
        int len = (int)strlen(gnrmc_logs_response);
        if (len > iInsertLen - 1)
        {
            len = iInsertLen - 1;
        }

        memset(pcInsert, 0, iInsertLen);
        memcpy(pcInsert, gnrmc_logs_response, (size_t)len);

        pcInsert[len] = '\0';

        memset(gnrmc_logs_response, 0, sizeof(gnrmc_logs_response));

        return (u16_t)len;
    }
    else if (iIndex == 2)  // "graph" tag
    {

        if (!graph_nmea_response[0])
        {
            const char* default_response = "{\"signals\":[]}";
            int len = strlen(default_response);

            if (len > iInsertLen - 1) {
                len = iInsertLen - 1;
            }

            memset(pcInsert, 0, iInsertLen);
            memcpy(pcInsert, default_response, len);
            pcInsert[len] = '\0';

            return (u16_t)len;
        }

        int len = (int)strlen(graph_nmea_response);
        if (len > iInsertLen - 1) {
            len = iInsertLen - 1;
        }

        memset(pcInsert, 0, iInsertLen);
        memcpy(pcInsert, graph_nmea_response, (size_t)len);
        pcInsert[len] = '\0';

        memset(graph_nmea_response, 0, sizeof(graph_nmea_response));
        memset(uart_all_nmea_buffer, 0, sizeof(uart_all_nmea_buffer));

        uart_gbgsv_count = 0;

        stateGraphHandler = 1;

        return (u16_t)len;
    }
    else if (iIndex == 3)  // "jamming-spoofing" tag
    {
        if (!jamming_spoofing_response[0]) {
            const char* default_response = "{\"signals\":[]}";
            int len = strlen(default_response);

            if (len > iInsertLen - 1) {
                len = iInsertLen - 1;
            }

            memset(pcInsert, 0, iInsertLen);
            memcpy(pcInsert, default_response, len);
            pcInsert[len] = '\0';

            return (u16_t)len;
        }

        int len = (int)strlen(jamming_spoofing_response);
        if (len > iInsertLen - 1) {
            len = iInsertLen - 1;
        }

        memset(pcInsert, 0, iInsertLen);
        memcpy(pcInsert, jamming_spoofing_response, (size_t)len);
        pcInsert[len] = '\0';

        memset(jamming_spoofing_response, 0, sizeof(jamming_spoofing_response));

        return (u16_t)len;
    }
    else if (iIndex == 4)  // "jamming-spoofing" tag
    {
        if (!alert_response[0]) {
            const char* default_response = "{\"signals\":[]}";
            int len = strlen(default_response);

            if (len > iInsertLen - 1) {
                len = iInsertLen - 1;
            }

            memset(pcInsert, 0, iInsertLen);
            memcpy(pcInsert, default_response, len);
            pcInsert[len] = '\0';

            return (u16_t)len;
        }

        int len = (int)strlen(alert_response);
        if (len > iInsertLen - 1) {
            len = iInsertLen - 1;
        }

        memset(pcInsert, 0, iInsertLen);
        memcpy(pcInsert, alert_response, (size_t)len);
        pcInsert[len] = '\0';

        memset(alert_response, 0, sizeof(alert_response));

        return (u16_t)len;
    }
    else if (iIndex == 5)  // "csv" tag
    {

        int len = strlen(csv_response);
        if (len > iInsertLen - 1) {
            len = iInsertLen - 1;
        }

        memset(pcInsert, 0, iInsertLen);
        memcpy(pcInsert, csv_response, len);
        pcInsert[len] = '\0';

        return (u16_t)len;
    }
    else if (iIndex == 6)  // "firmware" tag
    {
        int len = strlen(firmware_response);
        if (len > iInsertLen - 1) {
            len = iInsertLen - 1;
        }

        memset(pcInsert, 0, iInsertLen);
        memcpy(pcInsert, firmware_response, len);
        pcInsert[len] = '\0';

        return (u16_t)len;
    }
    else if (iIndex == 7)  // "config-output" tag
    {

        int len = strlen(config_response);
        if (len > iInsertLen - 1) {
            len = iInsertLen - 1;
        }

        memset(pcInsert, 0, iInsertLen);
        memcpy(pcInsert, config_response, len);
        pcInsert[len] = '\0';

        return (u16_t)len;
    }
    else if (iIndex == 8)  // "device-info" tag
    {
        if (!device_info_response[0]) {
            snprintf(device_info_response, sizeof(device_info_response),
                "{"
                  "\"status\":\"success\","
                  "\"product_name\":\"%s\","
                  "\"serial_number\":\"%s\","
                  "\"manufacturer\":\"%s\","
                  "\"address\":\"%s\","
                  "\"website\":\"%s\","
                  "\"firmware_version\":\"%s\""
                "}",
                DEVICE_PRODUCT_NAME,
                DEVICE_SERIAL_NUMBER,
                DEVICE_MANUFACTURER,
                DEVICE_ADDRESS,
                DEVICE_WEBSITE,
                DEVICE_FIRMWARE_VERSION
            );
        }

        int len = strlen(device_info_response);
        if (len > iInsertLen - 1) len = iInsertLen - 1;

        memset(pcInsert, 0, iInsertLen);
        memcpy(pcInsert, device_info_response, len);
        pcInsert[len] = '\0';

        return (u16_t)len;
    }
    else if (iIndex == 9)  // "login" tag
    {

        int len = strlen(login_response);
        if (len > iInsertLen - 1) {
            len = iInsertLen - 1;
        }

        memset(pcInsert, 0, iInsertLen);
        memcpy(pcInsert, login_response, len);
        pcInsert[len] = '\0';


        return (u16_t)len;
    }

    return 0;
}




