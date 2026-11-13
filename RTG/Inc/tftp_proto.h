/*
 * tftp_proto.h  -  build / parse TFTP packets (RFC 1350). Pure C, unit-tested on the PC.
 *
 *   RRQ   : [00 01] filename 00 "octet" 00
 *   DATA  : [00 03] [block hi lo] data (0..512)
 *   ACK   : [00 04] [block hi lo]
 *   ERROR : [00 05] [code hi lo] message 00
 */
#ifndef TFTP_PROTO_H
#define TFTP_PROTO_H

#include <stdint.h>

#define TFTP_SERVER_PORT   69U
#define TFTP_BLOCK_SIZE    512U
#define TFTP_HDR_SIZE      4U

#define TFTP_OP_RRQ        1U
#define TFTP_OP_WRQ        2U
#define TFTP_OP_DATA       3U
#define TFTP_OP_ACK        4U
#define TFTP_OP_ERROR      5U

#define TFTP_ERRCODE_NOT_FOUND  1U

typedef struct
{
  uint16_t       op;
  uint16_t       block;      /* DATA / ACK: block number. ERROR: error code */
  const uint8_t *data;       /* DATA: payload. ERROR: message (may be unterminated) */
  uint16_t       data_len;
} TftpPkt_t;

/* return the packet length, 0 if it does not fit */
uint16_t TftpProto_BuildRrq(uint8_t *buf, uint16_t size, const char *filename);
uint16_t TftpProto_BuildAck(uint8_t *buf, uint16_t size, uint16_t block);

/* 1 = valid header, fields in *pkt */
int      TftpProto_Parse(const uint8_t *buf, uint16_t len, TftpPkt_t *pkt);

#endif /* TFTP_PROTO_H */
