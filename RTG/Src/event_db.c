
//event_db  -  the "dispatcher memory"
// with freertos
#include "FreeRTOS.h"
#include "semphr.h"
#include "config.h"
#include "event_db_core.h"
#include "event_db.h"

static EvDbCore_t        db;//lives in RAM, not in the FreeRTOS heap
static SemaphoreHandle_t db_mtx;

void EventDb_Init(void)
{
  EvDbCore_Init(&db);// set db
  db_mtx = xSemaphoreCreateMutex();// create mutex
  configASSERT(db_mtx != NULL);
}

int EventDb_Push(const DispatchJob_t *job)// add event to data base
{
  xSemaphoreTake(db_mtx, portMAX_DELAY);// use mutex
  int ok = EvDbCore_Push(&db, job);
  xSemaphoreGive(db_mtx);
  return ok;
}

uint32_t EventDb_PushBatch(const DispatchJob_t *jobs, uint32_t n)//Entering several events at once
{
  uint32_t done = 0;
  xSemaphoreTake(db_mtx, portMAX_DELAY);
  while (done < n && EvDbCore_Push(&db, &jobs[done])) done++;
  xSemaphoreGive(db_mtx);
  return done;
}

int EventDb_PopBest(uint8_t dept_mask, DispatchJob_t *out)//The most urgent expenditure
{
  xSemaphoreTake(db_mtx, portMAX_DELAY);
  int got = EvDbCore_PopBest(&db, dept_mask, out);
  xSemaphoreGive(db_mtx);
  return got;
}

uint32_t EventDb_Count(void)// counts event
{
  xSemaphoreTake(db_mtx, portMAX_DELAY);
  uint32_t n = EvDbCore_Count(&db);
  xSemaphoreGive(db_mtx);
  return n;
}

uint32_t EventDb_CountSeverity(uint8_t sev) // Count Severity
{
  xSemaphoreTake(db_mtx, portMAX_DELAY);
  uint32_t n = EvDbCore_CountSeverity(&db, sev);
  xSemaphoreGive(db_mtx);
  return n;
}

uint32_t EventDb_Capacity(void) // data base Capacity
{
  return EVENT_DB_CAPACITY;
}
