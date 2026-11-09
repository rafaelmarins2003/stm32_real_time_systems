#include "keypad.h"

/* A0 do shield cai em A0 da Nucleo = PA0 = ADC1_IN0. Se a medicao indicar PA1, troque para 1. */
#define KEYPAD_CH   0U

/* Limites no meio entre as leituras medidas na placa (12 bits):
 * RIGHT ~1, UP ~570, DOWN ~1471, LEFT ~2332, SELECT ~3679, sem tecla 4095. */
#define TH_RIGHT    300U
#define TH_UP       1020U
#define TH_DOWN     1900U
#define TH_LEFT     3000U
#define TH_SELECT   3890U

void keypad_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;

    /* MODER: 11 = analogico no pino do canal escolhido. */
    GPIOA->MODER |= (3U << (2U * KEYPAD_CH));

    /* Tempo de amostragem de 480 ciclos: a rede resistiva do shield tem impedancia alta. */
    ADC1->SMPR2 |= (7U << (3U * KEYPAD_CH));

    ADC1->SQR1 = 0U;           /* Sequencia de 1 conversao. */
    ADC1->SQR3 = KEYPAD_CH;    /* A primeira (e unica) conversao e o canal do teclado. */
    ADC1->CR2 |= ADC_CR2_ADON; /* Liga o ADC. */
}

uint16_t keypad_read_raw(void)
{
    ADC1->CR2 |= ADC_CR2_SWSTART;            /* Inicia uma conversao. */
    while ((ADC1->SR & ADC_SR_EOC) == 0U) {} /* Espera o fim da conversao. */
    return (uint16_t)ADC1->DR;               /* A leitura limpa o EOC. */
}

uint8_t keypad_read_key(void)
{
    uint16_t v = keypad_read_raw();

    if (v < TH_RIGHT) return KEY_RIGHT;
    if (v < TH_UP)    return KEY_UP;
    if (v < TH_DOWN)  return KEY_DOWN;
    if (v < TH_LEFT)  return KEY_LEFT;
    if (v < TH_SELECT) return KEY_SELECT;
    return KEY_NONE;
}
