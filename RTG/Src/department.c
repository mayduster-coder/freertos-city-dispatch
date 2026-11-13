//department.c - Implements the 4 departments: Police, Fire, EMS, and Maintenance.
//dispatcher ──► [inbox] ──► dept_task ──► [work_q] ──► worker 1 / 2 / 3
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "config.h"
#include "log.h"
#include "city_event.h"
#include "city_stats.h"
#include "dispatcher.h"
#include "department.h"

typedef struct
{
  QueueHandle_t     inbox;
  QueueHandle_t     work_q;
  volatile uint32_t busy;
} DeptCtx_t;

typedef struct
{
  uint8_t dept;
  uint8_t num;
} WorkerArg_t;

static DeptCtx_t dept[DEPT_COUNT];
static WorkerArg_t      worker_arg[DEPT_COUNT][DEPT_POOL_SIZE];   /* must outlive xTaskCreate */


void Department_InitAll(void)// Create Queues
{
  for (uint8_t d = 0; d < DEPT_COUNT; d++)
  {
    dept[d].inbox  = xQueueCreate(DEPT_INBOX_LEN,      sizeof(DispatchJob_t));
    dept[d].work_q = xQueueCreate(DEPT_WORK_QUEUE_LEN, sizeof(DispatchJob_t));
    configASSERT(dept[d].inbox != NULL && dept[d].work_q != NULL);
    dept[d].busy = 0;
  }
}

int Department_HasRoom(uint8_t d)
{
  return (d < DEPT_COUNT) && (uxQueueSpacesAvailable(dept[d].inbox) > 0U); // only dept that has space will get an event
}

int Department_Send(uint8_t d, const DispatchJob_t *job)
{
  if (d >= DEPT_COUNT) return 0;
  return xQueueSend(dept[d].inbox, job, 0) == pdPASS; // send job to dept
}

uint32_t Department_Waiting(uint8_t d)
{
  if (d >= DEPT_COUNT) return 0;
  return (uint32_t)(uxQueueMessagesWaiting(dept[d].inbox) + uxQueueMessagesWaiting(dept[d].work_q));// several events waiting for dept
}

uint32_t Department_Busy(uint8_t d)
{
  return (d < DEPT_COUNT) ? dept[d].busy : 0U; // several tasks busy
}

static void dept_task(void *pv)
{
  uint8_t d = (uint8_t)(uintptr_t)pv;
  DispatchJob_t job;

  for (;;)
  {
    xQueueReceive(dept[d].inbox, &job, portMAX_DELAY);       // sleeps until an event comes
    Dispatcher_Wake();                                       //inbox has room again
    xQueueSend(dept[d].work_q, &job, portMAX_DELAY);         // sleeps if all workers busy
  }
}

static void busy_wait_ms(uint32_t ms)
{
  // real busy wait (CPU stays busy), measured in WALL time: if 3 workers share the
  //   CPU they all still finish after ~300 ms. Workers are the lowest priority, so
  //   every other task still runs, the loop ends when 300ms pass
  TickType_t start = xTaskGetTickCount();
  while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(ms))
  {
    __asm volatile ("nop");
  }
}

static void worker_task(void *pv)
{
  const WorkerArg_t *a = (const WorkerArg_t *)pv;
  DeptCtx_t  *me = &dept[a->dept];
  DispatchJob_t job;
  char addr[CITY_ADDR_LEN + 1U];

  for (;;)
  {
    xQueueReceive(me->work_q, &job, portMAX_DELAY);          // Receive from queue

    taskENTER_CRITICAL();

    me->busy++;  // add 1 to the dept "me" is pointing at
    taskEXIT_CRITICAL();

    uint32_t waited = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) - job.rx_ms;
    CityEvent_AddressStr(&job.ev, addr); // copy addr
    App_Log("[%-11s w%u] #%lu %-8s \"%s\" (waited %lu ms)\r\n", // print the event that is handled!!!!
            CityEvent_DeptName(a->dept), a->num, (unsigned long)job.id,
            CityEvent_SevName(job.ev.severity), addr, (unsigned long)waited);

    busy_wait_ms(DEPT_WORK_TIME_MS); //time to wait

    Stats_IncHandled(a->dept);
    taskENTER_CRITICAL(); me->busy--; taskEXIT_CRITICAL();
  }
}

void Department_StartAll(void) // creating 16 tasks
{
  static const char *const tname[DEPT_COUNT] = { "police", "fire", "ambul", "maint" };
  char name[configMAX_TASK_NAME_LEN];

  for (uint8_t d = 0; d < DEPT_COUNT; d++) // for every dept
  {
    BaseType_t ok = xTaskCreate(dept_task, tname[d], STACK_DEPT_MANAGER, // Create dept Task
                                (void *)(uintptr_t)d, PRIO_DEPT_MANAGER, NULL);
    configASSERT(ok == pdPASS);

    for (uint8_t w = 0; w < DEPT_POOL_SIZE; w++) // for every worker
    {
      worker_arg[d][w].dept = d;
      worker_arg[d][w].num  = (uint8_t)(w + 1U);

      uint32_t i = 0;
      while (tname[d][i] != '\0' && i < configMAX_TASK_NAME_LEN - 2U) { name[i] = tname[d][i]; i++; }
      name[i++] = (char)('1' + w);
      name[i]   = '\0';

      ok = xTaskCreate(worker_task, name, STACK_WORKER, &worker_arg[d][w], PRIO_WORKER, NULL); // Create worker Task
      configASSERT(ok == pdPASS);
    }
  }
}
