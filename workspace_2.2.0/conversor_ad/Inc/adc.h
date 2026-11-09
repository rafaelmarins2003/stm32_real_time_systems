#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

/* Configura PA1 como entrada analógica e o ADC1 (canal 1), sem interrupção. */
void pa1_adc_init(void);

/* Igual a pa1_adc_init(), mas com interrupção de fim de conversão (EOC) habilitada. */
void pa1_adc_interrupt_init(void);

/* Inicia UMA conversão (modo de simples conversão / single mode). */
void start_convertion(void);

/* Inicia conversões em sequência, sem parar (modo de conversão contínua). */
void start_continuous_convertion(void);

/* Espera o fim da conversão (EOC) e devolve o valor de 12 bits (0 a 4095). */
uint32_t adc_read(void);

#endif /* ADC_H_ */
