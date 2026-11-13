//heartbeat.c  -  highest app priority: if LD1 stops blinking, something is stuck
// The pin is configured HERE, so it works even if PB0 is not set in CubeMX.

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "config.h"
#include "heartbeat.h"

static void heartbeat_pin_init(void)
{
  GPIO_InitTypeDef g = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();// LD1 = PB0
  g.Pin   = HEARTBEAT_PIN;
  g.Mode  = GPIO_MODE_OUTPUT_PP;
  g.Pull  = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(HEARTBEAT_PORT, &g);
}

static void heartbeat_task(void *pv) //heart beat task
{
  (void)pv;
  for (;;)
  {
    HAL_GPIO_TogglePin(HEARTBEAT_PORT, HEARTBEAT_PIN);
    vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS));
  }
}

void Heartbeat_Start(void)
{
  heartbeat_pin_init();
  BaseType_t ok = xTaskCreate(heartbeat_task, "heartbeat", STACK_HEARTBEAT, NULL,
                              PRIO_HEARTBEAT, NULL);
  configASSERT(ok == pdPASS);
}
