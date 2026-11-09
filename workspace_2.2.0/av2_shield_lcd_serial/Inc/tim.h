#ifndef TIM_H
#define TIM_H

#include <stdint.h>
#include "stm32f4xx.h"

void tim2_init(void);
void delay_us(uint32_t);
void delay_ms(uint32_t);

#endif
