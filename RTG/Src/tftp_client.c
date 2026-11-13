/*
 * tftp_client.c - send request through lwIP
 *
 *   EVB (client)              -         PC (Tftpd64)
 *   RRQ "event007.bin" ----------->   port 69 to pc
 *                      <-----------   DATA blk 1 (100 B)   from a NEW port = server TID
 *   ACK 1              ----------->   TID
 *   (100 < 512 -> that was the last block, done)
 *
 * Every download opens a new UDP netconn on a random local port (a new TID, as the RFC says).
 */
#include <string.h>
#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"
#include "config.h"
#include "log.h"
#include "tftp_proto.h"
#include "tftp_client.h"

#if !LWIP_SO_RCVTIMEO
#error "CubeMX -> LWIP -> Key Options -> LWIP_SO_RCVTIMEO must be Enabled"
#endif

#if TFTP_VERBOSE
#define TLOG(...)  App_Log(__VA_ARGS__)
#else
#define TLOG(...)  do { } while (0)
#endif

//static: only the downloader task uses the client -> keeps its stack small
static uint8_t rx_pkt[TFTP_HDR_SIZE + TFTP_BLOCK_SIZE];
static uint8_t tx_pkt[128];

static err_t send_to(struct netconn *conn, const uint8_t *data, uint16_t len, // send packet in lwIP
                     const ip_addr_t *ip, uint16_t port)
{
  struct netbuf *nb = netbuf_new();
  if (nb == NULL) return ERR_MEM;

  void *p = netbuf_alloc(nb, len);
  if (p == NULL) { netbuf_delete(nb); return ERR_MEM; }
  memcpy(p, data, len);

  err_t e = netconn_sendto(conn, nb, ip, port);
  netbuf_delete(nb);
  return e;
}

TftpStatus_t Tftp_Get(const char *server_ip, const char *filename,
                      uint8_t *buf, uint32_t buf_size, uint32_t *out_len)
{
  if (!server_ip || !filename || !buf || !out_len) return TFTP_ERR_ARG;
  *out_len = 0;

  ip_addr_t srv;
  if (!ipaddr_aton(server_ip, &srv)) return TFTP_ERR_ARG;

  uint16_t tx_len = TftpProto_BuildRrq(tx_pkt, sizeof tx_pkt, filename);
  if (tx_len == 0U) return TFTP_ERR_ARG;

  struct netconn *conn = netconn_new(NETCONN_UDP); // set socket UPD
  if (conn == NULL) return TFTP_ERR_NETCONN;
  if (netconn_bind(conn, IP_ADDR_ANY, 0) != ERR_OK) { netconn_delete(conn); return TFTP_ERR_NETCONN; }
  netconn_set_recvtimeout(conn, TFTP_TIMEOUT_MS);

  TftpStatus_t st       = TFTP_OK;
  uint16_t     dst_port = TFTP_SERVER_PORT;   // RRQ goes to 69, the rest to the TID
  uint16_t     tid      = 0;
  uint16_t     expected = 1;
  uint32_t     total    = 0;
  uint32_t     retries  = 0;

  TLOG("[TFTP] RRQ %s -> %s\r\n", filename, server_ip);
  if (send_to(conn, tx_pkt, tx_len, &srv, dst_port) != ERR_OK) { st = TFTP_ERR_SEND; goto out; } // send rrq

  for (;;)
  {
    struct netbuf *nb = NULL;
    err_t e = netconn_recv(conn, &nb); // wait to receive connection

    if (e == ERR_TIMEOUT)
    {
      if (++retries > TFTP_MAX_RETRIES) { st = TFTP_ERR_TIMEOUT; break; }
      TLOG("[TFTP] timeout, resend %lu\r\n", (unsigned long)retries);
      (void)send_to(conn, tx_pkt, tx_len, &srv, dst_port);   // RRQ again, or the last ACK
      continue;
    }
    if (e != ERR_OK) { st = TFTP_ERR_NETCONN; break; }

    uint16_t from_port = netbuf_fromport(nb); // copy
    uint16_t len       = netbuf_copy(nb, rx_pkt, sizeof rx_pkt);// data has received
    netbuf_delete(nb);

    if (tid == 0U) { tid = from_port; dst_port = tid; }     // first answer = server TID
    else if (from_port != tid) continue;                    // stray packet

    TftpPkt_t pkt;
    if (!TftpProto_Parse(rx_pkt, len, &pkt)) { st = TFTP_ERR_PROTOCOL; break; }

    if (pkt.op == TFTP_OP_ERROR)
    {
      st = (pkt.block == TFTP_ERRCODE_NOT_FOUND) ? TFTP_ERR_NOT_FOUND : TFTP_ERR_SERVER;
      TLOG("[TFTP] server error %u\r\n", (unsigned)pkt.block);
      break;
    }
    if (pkt.op != TFTP_OP_DATA) { st = TFTP_ERR_PROTOCOL; break; }

    if (pkt.block == expected)
    {
      if (total + pkt.data_len > buf_size) { st = TFTP_ERR_TOO_BIG; break; }
      memcpy(&buf[total], pkt.data, pkt.data_len);// data move to buffer
      total += pkt.data_len;

      tx_len = TftpProto_BuildAck(tx_pkt, sizeof tx_pkt, pkt.block);
      if (send_to(conn, tx_pkt, tx_len, &srv, dst_port) != ERR_OK) { st = TFTP_ERR_SEND; break; }

      expected++;
      retries = 0;
      if (pkt.data_len < TFTP_BLOCK_SIZE) break;            // short block = last one
    }
    else if (pkt.block == (uint16_t)(expected - 1U))
    {
      (void)send_to(conn, tx_pkt, tx_len, &srv, dst_port);  // our ACK was lost: ACK again
    }
  }

out:
  netconn_delete(conn);
  if (st == TFTP_OK) *out_len = total;
  TLOG("[TFTP] %s: %s, %lu bytes\r\n", filename, Tftp_StatusStr(st), (unsigned long)total);
  return st;
}

const char *Tftp_StatusStr(TftpStatus_t st) // translate error code to words
{
  switch (st)
  {
    case TFTP_OK:            return "OK";
    case TFTP_ERR_ARG:       return "bad argument";
    case TFTP_ERR_NETCONN:   return "netconn error";
    case TFTP_ERR_SEND:      return "send failed";
    case TFTP_ERR_TIMEOUT:   return "timeout (server / cable / firewall?)";
    case TFTP_ERR_NOT_FOUND: return "file not found";
    case TFTP_ERR_SERVER:    return "server error";
    case TFTP_ERR_TOO_BIG:   return "file too big";
    case TFTP_ERR_PROTOCOL:  return "protocol error";
    default:                 return "?";
  }
}
