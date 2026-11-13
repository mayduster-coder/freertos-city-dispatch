//the downloader - implementing 3 requirements
// 1. downloading events from TFTP
// 2. event every 100ms
// 3. change rate
//event007.bin ──TFTP──► 100 bit ──CityEvent_Parse──► DispatchJob_t ──► stack
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "lwip/netif.h"
#include "config.h"
#include "log.h"
#include "city_event.h"
#include "event_db.h"
#include "tftp_client.h"
#include "tftp_proto.h"
#include "city_stats.h"
#include "dispatcher.h"
#include "downloader.h"

extern struct netif gnetif; //created by CubeMX in lwip.c

static volatile uint32_t period_ms = DOWNLOAD_PERIOD_DEFAULT_MS;   // 32-bit write is atomic
static volatile uint8_t  paused;
static volatile uint8_t  verbose = 1;

static uint8_t rx_buf[TFTP_BLOCK_SIZE];

uint32_t Downloader_GetPeriod(void) { return period_ms; } // return period

uint32_t Downloader_Faster(void)// Download Faster if user put "+"
{
  period_ms = Downloader_ClampPeriod((int32_t)period_ms - (int32_t)DOWNLOAD_PERIOD_STEP_MS);
  return period_ms;
}

uint32_t Downloader_Slower(void)// Download Slower if user put "-"
{
  period_ms = Downloader_ClampPeriod((int32_t)period_ms + (int32_t)DOWNLOAD_PERIOD_STEP_MS);
  return period_ms;
}
// Different state of downloading events
int  Downloader_TogglePause(void) { paused = (uint8_t)!paused; return paused; }
int  Downloader_IsPaused(void)    { return paused; }
void Downloader_SetVerbose(int on){ verbose = (uint8_t)(on != 0); }
int  Downloader_GetVerbose(void)  { return verbose; }

static uint32_t now_ms(void)
{
  return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);// time since the code running
}

static void downloader_task(void *pv)// downloader task
{
  (void)pv;

  while (!netif_is_link_up(&gnetif) || !netif_is_up(&gnetif))// wait- cable in + interface up
    vTaskDelay(pdMS_TO_TICKS(200));
  vTaskDelay(pdMS_TO_TICKS(500));
  App_Log("[DL] network up, board %s, server %s, period %lu ms\r\n", // good connection
          ip4addr_ntoa(netif_ip4_addr(&gnetif)), APP_SERVER_IP, (unsigned long)period_ms);

  uint32_t index       = EVENT_FILE_FIRST; // next file number
  uint32_t in_round    = 0;          // files downloaded since the first file
  uint32_t next_id     = 1;         // next number
  uint8_t  full_shown  = 0;
  char     name[32];

  for (;;)
  {
    TickType_t t0    = xTaskGetTickCount();
    TickType_t extra = 0;            // longer pause after "no files"

    if (!paused)
    {
      if (EventDb_Count() >= EventDb_Capacity()) // if stack is full
      {
        Stats_IncDbFull();
        if (!full_shown) App_Log("[DL] job DB full - waiting for the dispatcher\r\n");
        full_shown = 1;
      }
      else
      {
        full_shown = 0;
        snprintf(name, sizeof name, EVENT_FILE_FMT, (unsigned)index);// create name

        uint32_t     len = 0;
        TftpStatus_t st  = Tftp_Get(APP_SERVER_IP, name, rx_buf, sizeof rx_buf, &len); // TFTP connection with IP and copy data to rx_buf

        if (st == TFTP_OK)
        {
          DispatchJob_t job;
          EvStatus_t    es = CityEvent_Parse(rx_buf, len, &job.ev); // check and copy to job.ev
          if (es == EV_OK)
          {
            job.id         = next_id++;
            job.rx_ms      = now_ms();
            job.file_index = (uint16_t)index;
            (void)EventDb_Push(&job);//room was checked above, one writer
            Dispatcher_Wake();
            Stats_IncDownloaded();// add 1 to down load file
            if (verbose)
              App_Log("[DL] %s -> #%lu %-11s %-8s (db %lu)\r\n", name, (unsigned long)job.id,
                      CityEvent_DeptName(job.ev.department), CityEvent_SevName(job.ev.severity),
                      (unsigned long)EventDb_Count());
          }
          else//if not ok
          {
            Stats_IncRejected();
            App_Log("[DL] %s REJECTED: %s (%lu bytes)\r\n", name,
                    CityEvent_StatusStr(es), (unsigned long)len);
          }
          index++;
          in_round++;
        }
        else if (st == TFTP_ERR_NOT_FOUND || st == TFTP_ERR_SERVER)
        {
          if (in_round == 0U)
          {
            App_Log("[DL] %s not on the server - is the folder empty?\r\n", name);
            extra = pdMS_TO_TICKS(DOWNLOAD_EMPTY_RETRY_MS);
          }
          else
          {
            App_Log("[DL] end of the list: %lu files\r\n", (unsigned long)in_round);
#if EVENT_WRAP_AROUND
            index    = EVENT_FILE_FIRST;
            in_round = 0;
#else
            extra = pdMS_TO_TICKS(DOWNLOAD_EMPTY_RETRY_MS);// wait for new files
#endif
          }
        }
        else
        {
          Stats_IncTftpError();
          App_Log("[DL] %s: %s\r\n", name, Tftp_StatusStr(st));// same file next time
        }
      }
    }

    //keep the period from START to START (the download time is included)
    TickType_t period  = pdMS_TO_TICKS(period_ms) + extra;
    TickType_t elapsed = xTaskGetTickCount() - t0;
    vTaskDelay(elapsed < period ? period - elapsed : 1);
  }
}

void Downloader_Start(void) // Downloader task
{
  BaseType_t ok = xTaskCreate(downloader_task, "download", STACK_DOWNLOADER, NULL,
                              PRIO_DOWNLOADER, NULL);
  configASSERT(ok == pdPASS);
}
