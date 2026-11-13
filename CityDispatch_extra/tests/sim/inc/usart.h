#pragma once
#include "main.h"
typedef enum{HAL_OK=0}HAL_StatusTypeDef;
#define HAL_UART_STATE_READY 0x20U
#define HAL_MAX_DELAY 0xFFFFFFFFU
typedef struct{ volatile uint32_t RxState; } UART_HandleTypeDef; extern UART_HandleTypeDef huart3;
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*,const uint8_t*,uint16_t,uint32_t);
