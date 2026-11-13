/*
 * city_main.c  -  creates everything(queue, mutex, tasks). Only the native FreeRTOS API is used.
 *
 *   PC Tftpd64 --UDP--> downloaderTask --> [job-queue DB] --> dispatcherTask
 *                                       (severity first)        |
 *          +--------------------+---------------------+---------+---------+
 *          v                    v                     v                   v
 *      policeTask           fireTask            ambulTask             maintTask    (department tasks)
 *     [msg queue]          [msg queue]          [msg queue]          [msg queue]
 */
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "usart.h"
#include "config.h"
#include "log.h"
#include "event_db.h"
#include "city_stats.h"
#include "department.h"
#include "dispatcher.h"
#include "downloader.h"
#include "console.h"
#include "heartbeat.h"
#include "tests.h"
#include "city_main.h"

void App_Start(void) // main function
{
  // objects (mutexes, queues) BEFORE any task that uses them
  App_Log_Init();
  Stats_Reset();
  EventDb_Init();
  Department_InitAll();
  Console_Init();

  App_Log("\r\n\r\n######## CITY DISPATCH - FreeRTOS + lwIP TFTP ########\r\n");

  // tasks - consumers first, producer last

  Heartbeat_Start();
  Department_StartAll();
  Dispatcher_Start();
  Console_Start();
  Downloader_Start();

  App_Log("[MAIN] 20 tasks created, free heap %u bytes\r\n", (unsigned)xPortGetFreeHeapSize());
}

//FreeRTOS hooks
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask;
  static const char msg[] = "\r\n!!! STACK OVERFLOW in task: ";
  HAL_UART_Transmit(&LOG_UART, (uint8_t *)msg, sizeof msg - 1U, HAL_MAX_DELAY);
  HAL_UART_Transmit(&LOG_UART, (uint8_t *)pcTaskName, (uint16_t)strlen(pcTaskName), HAL_MAX_DELAY);
  taskDISABLE_INTERRUPTS();
  for (;;) { }
}

void vApplicationMallocFailedHook(void)
{
  static const char msg[] = "\r\n!!! MALLOC FAILED - increase TOTAL_HEAP_SIZE\r\n";
  HAL_UART_Transmit(&LOG_UART, (uint8_t *)msg, sizeof msg - 1U, HAL_MAX_DELAY);
  taskDISABLE_INTERRUPTS();
  for (;;) { }
}
