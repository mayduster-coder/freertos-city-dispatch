/*
 * tftp_client.h  -  TFTP client (download only, octet mode) over lwIP netconn
 *
 * CubeMX -> LWIP -> Key Options (Show Advanced): LWIP_SO_RCVTIMEO = Enabled
 * Call ONLY from a FreeRTOS task, after MX_LWIP_Init().
 */
#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

#include <stdint.h>

typedef enum
{
  TFTP_OK = 0,
  TFTP_ERR_ARG,
  TFTP_ERR_NETCONN,
  TFTP_ERR_SEND,
  TFTP_ERR_TIMEOUT,      /* server / cable / firewall */
  TFTP_ERR_NOT_FOUND,    /* server: file not found (= no more events) */
  TFTP_ERR_SERVER,       /* any other ERROR packet */
  TFTP_ERR_TOO_BIG,
  TFTP_ERR_PROTOCOL
} TftpStatus_t;

TftpStatus_t Tftp_Get(const char *server_ip, const char *filename,
                      uint8_t *buf, uint32_t buf_size, uint32_t *out_len);

const char  *Tftp_StatusStr(TftpStatus_t st);

#endif /* TFTP_CLIENT_H */
