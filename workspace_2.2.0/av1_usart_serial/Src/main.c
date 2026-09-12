/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : AV1 STR - UART1 + toggle LED (PC10) + leitura ADC1 CH1 em Volts
 ******************************************************************************
 */

#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "uart.h"
#include "adc.h"

#define VREF            3.3f
#define ADC_MAX_VALUE   4095.0f   /* ADC de 12 bits: 2^12 - 1 */

/* Redefinicao da funcao usada pelo printf para escrever um caractere na serial */
int __io_putchar(int ch)
{
    uart1_write(ch);
    return ch;
}

int main(void)
{
    uint16_t adc_raw;
    float tensao;

    uart1_init();
    adc1_ch1_init();

    printf("UART1 pronto. Envie qualquer caractere para dar toggle no LED (PC10).\r\n");

    while (1)
    {
        adc_raw = adc1_ch1_read();
        tensao = (adc_raw * VREF) / ADC_MAX_VALUE;

        printf("ADC1 CH1 (PA1): bruto = %u, tensao = %.2f V\r\n", adc_raw, tensao);

        /* pequeno atraso entre leituras, so para nao inundar o terminal */
        for (volatile int i = 0; i < 300000; i++) {}
    }
}
