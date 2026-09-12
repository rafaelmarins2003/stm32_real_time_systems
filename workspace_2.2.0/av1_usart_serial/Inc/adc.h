#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>
#include "stm32f4xx.h"

void adc1_ch1_init(void);
uint16_t adc1_ch1_read(void);

#endif /* ADC_H_ */
