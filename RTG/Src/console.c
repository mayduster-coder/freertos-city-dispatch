//this file is responsible to get chars from the terminal
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "usart.h"
#include "config.h"
#include "log.h"
#include "city_event.h"
#include "event_db.h"
#include "department.h"
#include "dispatcher.h"
#include "downloader.h"
#include "city_stats.h"
#include "tests.h"
#include "console.h"

static QueueHandle_t rx_q; // create handle queue
static uint8_t       rx_ch;

void Console_Init(void)
{
  rx_q = xQueueCreate(16, sizeof(uint8_t)); //create queue
  configASSERT(rx_q != NULL);
}

static void arm_rx(void) // receive chars
{
  HAL_UART_Receive_IT(&LOG_UART, &rx_ch, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) // UART Callback for the console
{
  if (huart != &LOG_UART) return;
  BaseType_t woken = pdFALSE;
  xQueueSendFromISR(rx_q, &rx_ch, &woken); // send to queue
  arm_rx();
  portYIELD_FROM_ISR(woken);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart == &LOG_UART) arm_rx();
}

#if CONSOLE_OWN_UART_IRQ

void USART3_IRQHandler(void) // for interrupt
{
  HAL_UART_IRQHandler(&LOG_UART);
}

static void uart_irq_enable(void)
{
  HAL_NVIC_SetPriority(USART3_IRQn, CONSOLE_UART_IRQ_PRIO, 0);
  HAL_NVIC_EnableIRQ(USART3_IRQn);
}
#else
static void uart_irq_enable(void) { }
#endif

//commands
static void print_help(void)
{
  App_Log("\r\n---- City Dispatch keys ----\r\n"
          " s stats | + faster | - slower | p pause | b burst test\r\n"
          " t self test | v verbose | r reset stats | h help\r\n");
}

static void print_stats(void)
{
  AppStats_t s;
  Stats_Get(&s);
  uint32_t up = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000U);

  App_Log("\r\n======== CITY DISPATCH STATS (uptime %lu s) ========\r\n", (unsigned long)up);
  App_Log("download : every %lu ms %s | ok %lu  rejected %lu  tftp err %lu  db-full %lu\r\n",
          (unsigned long)Downloader_GetPeriod(), Downloader_IsPaused() ? "(PAUSED)" : "",
          (unsigned long)s.downloaded, (unsigned long)s.rejected,
          (unsigned long)s.tftp_errors, (unsigned long)s.db_full);
  App_Log("job DB   : %lu/%lu  (Critical %lu, Warning %lu, Notice %lu)\r\n",
          (unsigned long)EventDb_Count(), (unsigned long)EventDb_Capacity(),
          (unsigned long)EventDb_CountSeverity(SEV_CRITICAL),
          (unsigned long)EventDb_CountSeverity(SEV_WARNING),
          (unsigned long)EventDb_CountSeverity(SEV_NOTICE));
  App_Log("department    sent  handled  waiting  busy\r\n");

  uint32_t ts = 0, th = 0;
  for (uint8_t d = 0; d < DEPT_COUNT; d++)
  {
    App_Log("%-11s %6lu  %7lu  %7lu  %4lu\r\n", CityEvent_DeptName(d),
            (unsigned long)s.sent[d], (unsigned long)s.handled[d],
            (unsigned long)Department_Waiting(d), (unsigned long)Department_Busy(d));
    ts += s.sent[d];
    th += s.handled[d];
  }
  App_Log("TOTAL       %6lu  %7lu\r\n", (unsigned long)ts, (unsigned long)th);
  App_Log("sent by severity: Critical %lu, Warning %lu, Notice %lu\r\n",
          (unsigned long)s.sent_by_sev[SEV_CRITICAL], (unsigned long)s.sent_by_sev[SEV_WARNING],
          (unsigned long)s.sent_by_sev[SEV_NOTICE]);
  App_Log("free heap %u bytes (min ever %u)\r\n",
          (unsigned)xPortGetFreeHeapSize(), (unsigned)xPortGetMinimumEverFreeHeapSize());
}


static void burst_test(void) // commend to to check 9 events in different range
{
  static const uint8_t sev[9] = { SEV_NOTICE, SEV_NOTICE, SEV_WARNING, SEV_NOTICE, SEV_CRITICAL, // priority check
                                  SEV_WARNING, SEV_CRITICAL, SEV_NOTICE, SEV_WARNING };
  static DispatchJob_t jobs[9];               /* static: 9 x 110 bytes, not on the stack */
  static uint32_t      burst_id = 900000;
  char addr[40];

  uint32_t now = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
  for (uint32_t i = 0; i < 9U; i++)
  {
    snprintf(addr, sizeof addr, "BURST TEST %lu (pushed #%lu)", (unsigned long)burst_id, (unsigned long)i);
    jobs[i].id         = burst_id++;
    jobs[i].rx_ms      = now;
    jobs[i].file_index = 0xFFFFU;
    CityEvent_Make(&jobs[i].ev, DEPT_MAINTENANCE, sev[i], addr); // make events
  }
  uint32_t n = EventDb_PushBatch(jobs, 9); // send all jobs
  App_Log("[TEST] burst: %lu events pushed to Maintenance in the order N N W N C W C N W\r\n"
          "[TEST] expected handling order: C C W W W N N N N\r\n", (unsigned long)n);
  Dispatcher_Wake();
}

static void handle_key(uint8_t c) // handle all keys
{
  switch (c)
  {
    case 's': case 'S': print_stats(); break;
    case '+':           App_Log("[KEY] faster: every %lu ms\r\n", (unsigned long)Downloader_Faster()); break;
    case '-':           App_Log("[KEY] slower: every %lu ms\r\n", (unsigned long)Downloader_Slower()); break;
    case 'p': case 'P': App_Log("[KEY] download %s\r\n", Downloader_TogglePause() ? "PAUSED" : "RESUMED"); break;
    case 'b': case 'B': burst_test(); break;
    case 't': case 'T': (void)Tests_RunCore(App_Log); break;
    case 'v': case 'V':
    {
      int on = !Downloader_GetVerbose();
      Downloader_SetVerbose(on);
      Dispatcher_SetVerbose(on);
      App_Log("[KEY] verbose %s (department prints always stay)\r\n", on ? "ON" : "OFF");
      break;
    }
    case 'r': case 'R': Stats_Reset(); App_Log("[KEY] statistics reset\r\n"); break;
    case 'h': case 'H': case '?': print_help(); break;
    case '\r': case '\n': break;
    default: App_Log("[KEY] '%c'? press h for help\r\n", (c >= 32U && c < 127U) ? c : '.'); break;
  }
}

static void console_task(void *pv) // the main task
{
  (void)pv;
  uint8_t c;

  //self test at boot - here and not in defaultTask, because this stack is bigger
  uint32_t fails = Tests_RunCore(App_Log);
  if (fails != 0U) App_Log("!!! %lu self-test checks FAILED\r\n", (unsigned long)fails);

  print_help();
  uart_irq_enable();
  arm_rx();

  for (;;)
  {
    if (xQueueReceive(rx_q, &c, pdMS_TO_TICKS(100)) == pdTRUE) // receive from queue
      handle_key(c);

    // if the re-arm in the ISR failed (UART busy sending), arm again from here
    if (LOG_UART.RxState == HAL_UART_STATE_READY)
      arm_rx(); // safety net
  }
}

void Console_Start(void) // create task
{
  BaseType_t ok = xTaskCreate(console_task, "console", STACK_CONSOLE, NULL, PRIO_CONSOLE, NULL);
  configASSERT(ok == pdPASS);
}
