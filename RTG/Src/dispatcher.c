//The dispatcher connects the repository to the departments and fulfills two requirements.:
//1.Each event is sent to the department based on its department field.
//2.The most urgent event will be sent first.
#include "FreeRTOS.h"
#include "task.h"
#include "config.h"
#include "log.h"
#include "city_event.h"
#include "event_db.h"
#include "event_db_core.h"
#include "department.h"
#include "city_stats.h"
#include "dispatcher.h"

static TaskHandle_t     disp_handle;
static volatile uint8_t verbose = 1;

void Dispatcher_SetVerbose(int on) { verbose = (uint8_t)(on != 0); }

void Dispatcher_Wake(void)// wake up
{
  if (disp_handle != NULL)
	  xTaskNotifyGive(disp_handle); // send task notification
}

static uint8_t free_departments(void)// check which dept are free
{
  uint8_t mask = 0;
  for (uint8_t d = 0; d < DEPT_COUNT; d++)
    if (Department_HasRoom(d)) mask |= (uint8_t)(1U << d); // bitwaise to see who is free
  return mask;
}

static void dispatcher_task(void *pv) // main loop
{
  (void)pv;
  App_Log("[DISP] started\r\n");

  for (;;)
  {
    DispatchJob_t job;
    uint8_t mask = free_departments();

    if (mask != 0U && EventDb_PopBest(mask, &job)) // take most urgent
    {
      uint8_t d = job.ev.department;
      if (Department_Send(d, &job)) // send queue
      {
        Stats_IncSent(d, job.ev.severity); // add dept
        if (verbose)
          App_Log("[DISP] #%lu %-8s -> %-11s (db left %lu)\r\n", (unsigned long)job.id,  // print event
                  CityEvent_SevName(job.ev.severity), CityEvent_DeptName(d),
                  (unsigned long)EventDb_Count());
      }
      else
      {
        (void)EventDb_Push(&job);
      }
      continue;
    }

    //DB empty, or every department with work is full -> sleep
    (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(DISPATCH_IDLE_POLL_MS));
  }
}

void Dispatcher_Start(void) // main task
{
  BaseType_t ok = xTaskCreate(dispatcher_task, "dispatch", STACK_DISPATCHER, NULL,
                              PRIO_DISPATCHER, &disp_handle);
  configASSERT(ok == pdPASS);
}
