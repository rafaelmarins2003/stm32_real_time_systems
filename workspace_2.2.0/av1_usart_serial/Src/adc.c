#include "adc.h"

/* RCC_AHB1ENR / RCC_APB2ENR */
#define GPIOAEN         (1U << 0)
#define ADC1EN          (1U << 8)

#define ADC_CH1         (1U)      /* canal 1 do ADC1 = pino PA1 */
#define ADC_SEQ_LEN_1   (0x00U)   /* L[3:0] = 0 -> sequencia com 1 conversao */

/* ADC1_CR2 */
#define CR2_ADON        (1U << 0)
#define CR2_SWSTART     (1U << 30)

/* ADC1_SR */
#define SR_EOC          (1U << 1)

void adc1_ch1_init(void)
{
    /* Habilita o clock para acessar o GPIOA */
    RCC->AHB1ENR |= GPIOAEN;

    /* Configura PA1 (canal 1) em modo analogico (11) */
    GPIOA->MODER |= (1U << 2);
    GPIOA->MODER |= (1U << 3);

    /* Habilita o clock para acessar o ADC1 */
    RCC->APB2ENR |= ADC1EN;

    /* Seleciona o canal 1 como (unica) conversao da sequencia regular */
    ADC1->SQR3 = ADC_CH1;

    /* Configura o tamanho da sequencia de conversao (1 conversao) */
    ADC1->SQR1 = ADC_SEQ_LEN_1;

    /* Liga o modulo ADC (modo de conversao unica/simples aquisicao) */
    ADC1->CR2 |= CR2_ADON;
}

uint16_t adc1_ch1_read(void)
{
    /* Dispara uma nova conversao */
    ADC1->CR2 |= CR2_SWSTART;

    /* Espera a conversao terminar (flag EOC) */
    while (!(ADC1->SR & SR_EOC)) {}

    /* Le o resultado da conversao (12 bits, 0-4095, alinhado a direita) */
    return (uint16_t)(ADC1->DR);
}
