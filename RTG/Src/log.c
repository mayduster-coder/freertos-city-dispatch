// log- The file provides a single function for all tasks to print to the terminal.
#include <stdarg.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "usart.h"
#include "config.h"
#include "log.h"

static SemaphoreHandle_t log_mtx; // create mutex Handle
static char              line[LOG_LINE_MAX];// protected by log_mtx

void App_Log_Init(void)
{
  log_mtx = xSemaphoreCreateMutex();// create mutex
  configASSERT(log_mtx != NULL);
}

void App_Log(const char *fmt, ...)
{
  int locked = (log_mtx != NULL) && // lock if the 2 statements true
               (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
  if (locked) xSemaphoreTake(log_mtx, portMAX_DELAY);

  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(line, sizeof line, fmt, ap);
  va_end(ap);

  if (n > 0)
  {
    if (n >= (int)sizeof line) n = (int)sizeof line - 1;   //line was cut
    HAL_UART_Transmit(&LOG_UART, (uint8_t *)line, (uint16_t)n, 100); // print to terminal
  }

  if (locked) xSemaphoreGive(log_mtx);
}
