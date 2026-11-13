#pragma once
#include <stdint.h>
typedef struct{int x;} GPIO_TypeDef; extern GPIO_TypeDef GPIOB_;
#define GPIOB (&GPIOB_)
#define GPIO_PIN_0 1U
void HAL_GPIO_TogglePin(GPIO_TypeDef*,uint16_t);
