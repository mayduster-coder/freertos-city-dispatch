// tftp_proto.c -translate TFTP protocol

#include <string.h>
#include "tftp_proto.h"

static void put16(uint8_t *p, uint16_t v) //Separate bytes
{
  p[0] = (uint8_t)(v >> 8);
  p[1] = (uint8_t)(v & 0xFFU);
}

static uint16_t get16(const uint8_t *p)
{
  return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

uint16_t TftpProto_BuildRrq(uint8_t *buf, uint16_t size, const char *filename) // Build RRQ
{
  static const char mode[] = "octet";//binary - the event is raw bytes
  if (buf == NULL || filename == NULL) return 0;

  size_t n    = strlen(filename);
  size_t need = 2U + n + 1U + sizeof mode;// sizeof mode already counts '\0'
  if (n == 0U || need > size) return 0;

  put16(buf, TFTP_OP_RRQ);
  memcpy(&buf[2], filename, n + 1U);
  memcpy(&buf[2U + n + 1U], mode, sizeof mode);
  return (uint16_t)need;
}

uint16_t TftpProto_BuildAck(uint8_t *buf, uint16_t size, uint16_t block) // ACK
{
  if (buf == NULL || size < TFTP_HDR_SIZE) return 0;
  put16(&buf[0], TFTP_OP_ACK);
  put16(&buf[2], block);
  return TFTP_HDR_SIZE;
}

int TftpProto_Parse(const uint8_t *buf, uint16_t len, TftpPkt_t *pkt) // Unpacking a package that arrived
{
  if (buf == NULL || pkt == NULL || len < TFTP_HDR_SIZE) return 0;

  pkt->op       = get16(&buf[0]);
  pkt->block    = get16(&buf[2]);
  pkt->data     = &buf[TFTP_HDR_SIZE];
  pkt->data_len = (uint16_t)(len - TFTP_HDR_SIZE);

  switch (pkt->op)
  {
    case TFTP_OP_DATA:  return pkt->data_len <= TFTP_BLOCK_SIZE;
    case TFTP_OP_ACK:   return pkt->data_len == 0U;
    case TFTP_OP_ERROR: return 1;
    default:            return 0;               // RRQ never come to a client
  }
}
