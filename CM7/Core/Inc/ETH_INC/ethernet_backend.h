/**
  ******************************************************************************
  * @file    ethernet_backend.h
  * @brief   Ethernet Backend Module - HTTP Server, CGI & SSI Handlers
  * @author  Embedded Firmware Team
  * @date    2025
  ******************************************************************************
  * @attention
  *
  * This module handles all Ethernet/HTTP backend functionality:
  * - LwIP HTTP server management  
  * - CGI handlers for REST API endpoints
  * - SSI handlers for dynamic content
  * - JSON response generation
  *
  ******************************************************************************
  */

#ifndef __ETHERNET_BACKEND_H
#define __ETHERNET_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"
#include "lwip/def.h"
#include "lwip/tcpip.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* Exported constants --------------------------------------------------------*/
#define LATEST_SENTENCES_COUNT      5
#define MAX_SENTENCE_LEN            128





/*****************************************  UART DATA COLLECTION FUNCTIONS ******************************************/
const char* cgi_get_jamming_spoofing_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* cgi_get_terminal_logs_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* cgi_get_graph_nmea_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* cgi_get_gnrmc_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* cgi_get_alert_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* cgi_get_csv_file_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* cgi_get_firmware_update_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* cgi_get_output_configuration_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* cgi_get_login_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* cgi_get_device_info_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);


/**
 * @brief Initialize Ethernet Backend Module
 * @retval None
 */
void EthernetBackend_Init(void);





/**
 * @brief Add NMEA sentence from UBLOX_ProcessTaskHandler
 * @param nmea_sentence: NMEA sentence string
 * @retval None
 */
void EthernetBackend_AddNMEASentence(const char* nmea_sentence);
void EthernetBackend_AddGNRMCSentence(const char* nmea_sentence);
void EthernetBackend_AddAll_NMAE_SentenceDashboard(const char* gbgsv_sentence);
void EthernetBackend_Add_JammingSpoofingSentence(const char* jam_buffer, const char* spoof_buffer);

void EthernetBackend_RegisterCGIHandlers(void);
void EthernetBackend_RegisterSSIHandler(void);


/* SSI Handler Functions */
u16_t EthernetBackend_SSIHandler(int iIndex, char *pcInsert, int iInsertLen
#if LWIP_HTTPD_SSI_MULTIPART
  , u16_t current_tag_part, u16_t *next_tag_part
#endif
#if defined(LWIP_HTTPD_FILE_STATE) && LWIP_HTTPD_FILE_STATE
  , void *connection_state
#endif
);




#ifdef __cplusplus
}
#endif

#endif /* __ETHERNET_BACKEND_H */ 
