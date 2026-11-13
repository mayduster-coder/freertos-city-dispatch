/* PC simulation: real event_db / dispatcher / department / stats / city_event code
   on the FreeRTOS POSIX port. TFTP is replaced by reading the same event files from disk. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "usart.h"
#include "config.h"
#include "log.h"
#include "city_event.h"
#include "event_db.h"
#include "city_stats.h"
#include "department.h"
#include "dispatcher.h"
#include "tests.h"

UART_HandleTypeDef huart3; GPIO_TypeDef GPIOB_;
void HAL_GPIO_TogglePin(GPIO_TypeDef *p, uint16_t n){ (void)p; (void)n; }
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *h, const uint8_t *d, uint16_t n, uint32_t t)
{ (void)h; (void)t; fwrite(d,1,n,stdout); fflush(stdout); return HAL_OK; }
void vAssertCalled(const char *f, int l){ printf("ASSERT %s:%d\n", f, l); fflush(stdout); exit(2); }
void vApplicationMallocFailedHook(void){ printf("MALLOC FAILED\n"); exit(3); }

static const char *dir;
static volatile uint32_t period = 100;
static volatile int paused;
static uint32_t next_id = 1;

static int load(unsigned idx, uint8_t *raw)
{
  char p[512]; snprintf(p, sizeof p, "%s/event%03u.bin", dir, idx);
  FILE *f = fopen(p, "rb"); if (!f) return -1;
  int n = (int)fread(raw, 1, 200, f); fclose(f); return n;
}

static void producer(void *pv)      /* = downloader logic, file read instead of TFTP */
{
  (void)pv; unsigned idx = 0; uint8_t raw[200];
  for (;;) {
    if (!paused && EventDb_Count() < EventDb_Capacity()) {
      int n = load(idx, raw);
      if (n < 0) { idx = 0; continue; }
      DispatchJob_t j;
      if (CityEvent_Parse(raw, (uint32_t)n, &j.ev) == EV_OK) {
        j.id = next_id++; j.rx_ms = xTaskGetTickCount(); j.file_index = (uint16_t)idx;
        EventDb_Push(&j); Dispatcher_Wake(); Stats_IncDownloaded();
      } else Stats_IncRejected();
      idx++;
    } else if (!paused) Stats_IncDbFull();
    vTaskDelay(pdMS_TO_TICKS(period));
  }
}

static void dump_stats(const char *tag)
{
  AppStats_t s; Stats_Get(&s);
  App_Log("@STATS %s dl=%lu db=%lu", tag, (unsigned long)s.downloaded, (unsigned long)EventDb_Count());
  for (int d = 0; d < DEPT_COUNT; d++) App_Log(" %s=%lu/%lu", CityEvent_DeptName(d), (unsigned long)s.sent[d], (unsigned long)s.handled[d]);
  App_Log(" sev=%lu/%lu/%lu dbfull=%lu\n", (unsigned long)s.sent_by_sev[0], (unsigned long)s.sent_by_sev[1], (unsigned long)s.sent_by_sev[2], (unsigned long)s.db_full);
}

static void wait_idle(void)
{
  for (;;) {
    int busy = EventDb_Count() > 0;
    for (int d = 0; d < DEPT_COUNT; d++) busy |= Department_Waiting(d) || Department_Busy(d);
    if (!busy) return;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

static void scenario(void *pv)
{
  (void)pv;
  App_Log("@PHASE A normal 100 ms, 6 s\n");
  vTaskDelay(pdMS_TO_TICKS(6000));
  paused = 1; wait_idle(); dump_stats("A");

  App_Log("@PHASE B burst\n");
  static const uint8_t sev[9] = {2,2,1,2,0,1,0,2,1};
  static DispatchJob_t jobs[9];
  for (int i = 0; i < 9; i++) { jobs[i].id = 900000u + i; jobs[i].rx_ms = xTaskGetTickCount(); jobs[i].file_index = 0xFFFF;
    CityEvent_Make(&jobs[i].ev, DEPT_MAINTENANCE, sev[i], "BURST"); }
  EventDb_PushBatch(jobs, 9); Dispatcher_Wake();
  vTaskDelay(pdMS_TO_TICKS(100)); wait_idle(); dump_stats("B");

  App_Log("@PHASE C overload 10 ms, 4 s\n");
  Stats_Reset(); period = 10; paused = 0;
  vTaskDelay(pdMS_TO_TICKS(4000));
  paused = 1; wait_idle(); dump_stats("C");
  App_Log("@END\n");
  exit(0);
}

int main(int argc, char **argv)
{
  dir = argc > 1 ? argv[1] : "events";
  App_Log_Init(); Stats_Reset(); EventDb_Init(); Department_InitAll();
  if (Tests_RunCore(App_Log) != 0) return 1;
  Department_StartAll(); Dispatcher_Start();
  xTaskCreate(producer, "producer", 512, NULL, PRIO_DOWNLOADER, NULL);
  xTaskCreate(scenario, "scenario", 512, NULL, tskIDLE_PRIORITY + 5, NULL);
  vTaskStartScheduler();
  return 0;
}
