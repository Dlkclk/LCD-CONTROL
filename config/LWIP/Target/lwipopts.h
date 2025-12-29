/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : Target/lwipopts.h
  * Description        : This file overrides LwIP stack default configuration
  *                      done in opt.h file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion --------------------------------------*/
#ifndef __LWIPOPTS__H__
#define __LWIPOPTS__H__

#include "main.h"

/*-----------------------------------------------------------------------------*/
/* Current version of LwIP supported by CubeMx: 2.1.2 -*/
/*-----------------------------------------------------------------------------*/

/* Within 'USER CODE' section, code will be kept by default at each generation */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

#ifdef __cplusplus
 extern "C" {
#endif

/* STM32CubeMX Specific Parameters (not defined in opt.h) ---------------------*/
/* Parameters set in STM32CubeMX LwIP Configuration GUI -*/
/*----- WITH_RTOS enabled (Since FREERTOS is set) -----*/
#define WITH_RTOS 1
/* Temporary workaround to avoid conflict on errno defined in STM32CubeIDE and lwip sys_arch.c errno */
#undef LWIP_PROVIDE_ERRNO
/*----- CHECKSUM_BY_HARDWARE enabled -----*/
#define CHECKSUM_BY_HARDWARE 1
/*-----------------------------------------------------------------------------*/

/* LwIP Stack Parameters (modified compared to initialization value in opt.h) -*/
/* Parameters set in STM32CubeMX LwIP Configuration GUI -*/
/*----- Default value in ETH configuration GUI in CubeMx: 1524 -----*/
#define ETH_RX_BUFFER_SIZE 1536
/*----- Value in opt.h for MEM_ALIGNMENT: 1 -----*/
#define MEM_ALIGNMENT 4
/*----- Default Value for MEM_SIZE: 1600 ---*/
#define MEM_SIZE 131048
/*----- Default Value for H7 devices: 0x30004000 -----*/
#define LWIP_RAM_HEAP_POINTER 0x30020000
/*----- Value supported for H7 devices: 1 -----*/
#define LWIP_SUPPORT_CUSTOM_PBUF 1
/*----- Value in opt.h for LWIP_ETHERNET: LWIP_ARP || PPPOE_SUPPORT -*/
#define LWIP_ETHERNET 1
/*----- Value in opt.h for LWIP_DNS_SECURE: (LWIP_DNS_SECURE_RAND_XID | LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING | LWIP_DNS_SECURE_RAND_SRC_PORT) -*/
#define LWIP_DNS_SECURE 7
/*----- Default Value for TCP_MSS: 536 ---*/
#define TCP_MSS 1460
/*----- Default Value for TCP_SND_BUF: 2920 ---*/
#define TCP_SND_BUF 5840
/*----- Default Value for TCP_SND_QUEUELEN: 17 ---*/
#define TCP_SND_QUEUELEN 16
/*----- Value in opt.h for LWIP_NETIF_LINK_CALLBACK: 0 -----*/
#define LWIP_NETIF_LINK_CALLBACK 1
/*----- Value in opt.h for TCPIP_THREAD_STACKSIZE: 0 -----*/
#define TCPIP_THREAD_STACKSIZE 2048
/*----- Value in opt.h for TCPIP_THREAD_PRIO: 1 -----*/
#define TCPIP_THREAD_PRIO 24
/*----- Value in opt.h for TCPIP_MBOX_SIZE: 0 -----*/
#define TCPIP_MBOX_SIZE 6
/*----- Value in opt.h for SLIPIF_THREAD_STACKSIZE: 0 -----*/
#define SLIPIF_THREAD_STACKSIZE 1024
/*----- Value in opt.h for SLIPIF_THREAD_PRIO: 1 -----*/
#define SLIPIF_THREAD_PRIO 3
/*----- Value in opt.h for DEFAULT_THREAD_STACKSIZE: 0 -----*/
#define DEFAULT_THREAD_STACKSIZE 2048
/*----- Value in opt.h for DEFAULT_THREAD_PRIO: 1 -----*/
#define DEFAULT_THREAD_PRIO 3
/*----- Value in opt.h for DEFAULT_UDP_RECVMBOX_SIZE: 0 -----*/
#define DEFAULT_UDP_RECVMBOX_SIZE 6
/*----- Value in opt.h for DEFAULT_TCP_RECVMBOX_SIZE: 0 -----*/
#define DEFAULT_TCP_RECVMBOX_SIZE 6
/*----- Value in opt.h for DEFAULT_ACCEPTMBOX_SIZE: 0 -----*/
#define DEFAULT_ACCEPTMBOX_SIZE 6
/*----- Value in opt.h for RECV_BUFSIZE_DEFAULT: INT_MAX -----*/
#define RECV_BUFSIZE_DEFAULT 2000000000
/*----- Default Value for LWIP_HTTPD: 0 ---*/
#define LWIP_HTTPD 1
/*----- Value in opt.h for LWIP_STATS: 1 -----*/
#define LWIP_STATS 0
/*----- Value in opt.h for CHECKSUM_GEN_IP: 1 -----*/
#define CHECKSUM_GEN_IP 0
/*----- Value in opt.h for CHECKSUM_GEN_UDP: 1 -----*/
#define CHECKSUM_GEN_UDP 0
/*----- Value in opt.h for CHECKSUM_GEN_TCP: 1 -----*/
#define CHECKSUM_GEN_TCP 0
/*----- Value in opt.h for CHECKSUM_GEN_ICMP6: 1 -----*/
#define CHECKSUM_GEN_ICMP6 0
/*----- Value in opt.h for CHECKSUM_CHECK_IP: 1 -----*/
#define CHECKSUM_CHECK_IP 0
/*----- Value in opt.h for CHECKSUM_CHECK_UDP: 1 -----*/
#define CHECKSUM_CHECK_UDP 0
/*----- Value in opt.h for CHECKSUM_CHECK_TCP: 1 -----*/
#define CHECKSUM_CHECK_TCP 0
/*----- Value in opt.h for CHECKSUM_CHECK_ICMP6: 1 -----*/
#define CHECKSUM_CHECK_ICMP6 0
/*-----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

 #undef LWIP_PROVIDE_ERRNO
 #define LWIP_ERRNO_STDINCLUDE
 #define LWIPERF_CHECK_RX_DATA 1

 #define LOCK_TCPIP_CORE sys_lock_tcpip_core
 #define UNLOCK_TCPIP_CORE sys_unlock_tcpip_core

 #define LWIP_ASSERT_CORE_LOCKED sys_check_core_locking
 #define LWIP_MARK_TCPIP_THREAD sys_mark_tcpip_thread

 void sys_lock_tcpip_core(void);
 void sys_unlock_tcpip_core(void);

 void sys_check_core_locking(void);
 void sys_mark_tcpip_thread(void);

#define LWIP_HTTPD_CGI 1
#define LWIP_HTTPD_SSI 1
#define LWIP_HTTPD_DYNAMIC_HEADERS 1
#define LWIP_HTTPD_MAX_TAG_INSERT_LEN 8192

#define LWIP_HTTPD_SSI_BY_FILE_EXTENSION   1
#define LWIP_HTTPD_SSI_FILE_EXTENSION      ".shtml"
#define LWIP_HTTPD_MAX_TAG_NAME_LEN        16
#define LWIP_HTTPD_SSI_MULTIPART           1

/* ================================================================ */
/* AJAX-CENTRIC HTTP SERVER OPTIMIZATION PARAMETERS               */
/* ================================================================ */

/* 🔗 CONNECTION POOLING & LIMITS */
#define MEMP_NUM_TCP_PCB 20              /* Max TCP connections (default: 5) */
#define MEMP_NUM_TCP_PCB_LISTEN 8        /* Max listening TCP connections */
#define MEMP_NUM_TCP_SEG 80              /* TCP segments pool size */
#define MEMP_NUM_ALTCP_PCB 20            /* Alternative TCP PCB pool */

/* 📦 BUFFER MANAGEMENT */
#define PBUF_POOL_SIZE 32                /* Packet buffer pool (default: 16) */
#define PBUF_POOL_BUFSIZE 1536           /* Buffer size per pool entry */
#define TCP_WND 4096                     /* TCP receive window (default: 2920) */

/* ⚡ HTTP SERVER PERFORMANCE */
#define HTTPD_MAX_RETRIES 3              /* Connection retry limit (default: 4) */
#define HTTPD_POLL_INTERVAL 2            /* Poll interval in 500ms units (default: 4) */
#define HTTPD_TCP_PRIO TCP_PRIO_MIN+1    /* TCP priority (higher than default) */
#define HTTPD_USE_MEM_POOL 1             /* Use memory pools for HTTP states */

/* 🚀 CONCURRENT REQUEST HANDLING */
#define MEMP_NUM_PARALLEL_HTTPD_CONNS 8  /* Max parallel HTTP connections */
#define MEMP_NUM_PARALLEL_HTTPD_SSI_CONNS 4  /* Max parallel SSI connections */

/* 📊 AJAX OPTIMIZATION */
#define LWIP_HTTPD_SUPPORT_EXTHEADER 1   /* Support for custom headers (CORS) */
#define LWIP_HTTPD_SUPPORT_POST 0        /* Disable POST method support */
#define LWIP_HTTPD_POST_MAX_PAYLOAD_LEN 512  /* Max POST payload size */
#define LWIP_HTTPD_TIMING 1              /* Enable request timing */
#define LWIP_HTTPD_ABORT_ON_CLOSE_MEM_ERROR 0  /* Don't abort on memory errors */

/* 🛡️ RELIABILITY & ERROR RECOVERY */
#define LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED 1  /* Kill old connections when limit reached */
#define TCP_OVERSIZE TCP_MSS             /* TCP oversizing for efficiency */
#define TCP_QUEUE_OOSEQ 1                /* Enable out-of-sequence queueing */

/* 💾 MEMORY OPTIMIZATION */
#define MEM_LIBC_MALLOC 0                /* Use lwIP's memory management */
#define MEMP_OVERFLOW_CHECK 0            /* Disable overflow check for performance */
#define MEMP_SANITY_CHECK 0              /* Disable sanity check for performance */

/* 🔄 KEEPALIVE & TIMEOUT */
#define LWIP_TCP_KEEPALIVE 1

/* 🚨 DHCP TIMEOUT OPTIMIZATION */
#define DHCP_TIMEOUT_COUNT       3     /* 3x retry instead of default 60 */
#define DHCP_REQUEST_TIMEOUT_MS  5000  /* 5 second timeout */
#define TCP_KEEPIDLE_DEFAULT 7200000UL   /* Keepalive idle time (2 hours) */
#define TCP_KEEPINTVL_DEFAULT 75000UL    /* Keepalive interval (75 seconds) */
#define TCP_KEEPCNT_DEFAULT 9            /* Keepalive probe count */

#define HTTPD_THREAD_STACKSIZE 16384        // HTTP server için 16KB






 /* HTTP REQUEST SIZE LIMITS - ÖNEMLİ! */
 #define HTTPD_MAX_REQ_LENGTH 2048        /* URL + headers için maksimum boyut */
 #define HTTPD_MAX_CGI_PARAMETERS 10      /* Maksimum CGI parametre sayısı */
 #define HTTPD_MAX_REQUEST_URI_LEN 1024   /* Maksimum URI uzunluğu */

 /* CGI BUFFER LIMITS */
 #define LWIP_HTTPD_MAX_CGI_PARAM_LEN 700  /* Her CGI parametresi için max boyut */

 /* PBUF MEMORY POOLS - Artırın */
 #define MEMP_NUM_PBUF 64                  /* Mevcut: 32, önerilen: 64 */


/* USER CODE END 1 */

#ifdef __cplusplus
}
#endif
#endif /*__LWIPOPTS__H__ */
