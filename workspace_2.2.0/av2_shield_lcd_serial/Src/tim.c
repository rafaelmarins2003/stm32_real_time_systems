#include "tim.h"

void tim2_init(void)
{
    /* Habilita o acesso ao timer interno da placa STM32. */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;

    //CR1 = Control Register 1 do timer. É o registrador principal de controle do TIM2.
    TIM2->CR1 = 0U;   /* Deixa o contador parado durante a configuracao. */

    // DIER (DMA/Interrupt Enable Register): habilita as interrupções e as requisições de DMA do timer.
    TIM2->DIER = 0U;  /* Sem interrupcoes. */

    // PSC (Prescaler): divide o clock de entrada por PSC + 1. Com PSC = 15, 16 MHz ÷ 16 = 1 MHz, ou seja, 1 contagem por µs.
    TIM2->PSC = 16U - 1U;      /* Divide por 16: um passo a cada 1 microssegundo. */

    // ARR (Auto-Reload Register): valor máximo da contagem. Ao chegar nele, o contador volta a 0.
    // Com 0xFFFFFFFF (32 bits), só dá a volta depois de ~71 min.
    TIM2->ARR = 0xFFFFFFFFU;   /* TIM2 tem 32 bits: so da a volta apos ~71 minutos. */

    // EGR (Event Generation Register): permite gerar eventos por software.
    TIM2->EGR = TIM_EGR_UG;    /* Aplica a configuracao e reinicia a contagem. */

    // SR (Status Register): guarda os flags do timer, no caso, limpamos ele.
    TIM2->SR = 0U;
    TIM2->CR1 |= TIM_CR1_CEN;  /* Contador livre: nunca mais para, CNT sobe 1 por us. */
}


void delay_us(uint32_t us)
{
    uint32_t start = TIM2->CNT;

    /* A subtracao sem sinal continua correta mesmo se CNT der a volta. */
    while ((TIM2->CNT - start) < us) {}
}


void delay_ms(uint32_t ms)
{
    while (ms > 0U)
    {
        delay_us(1000U);
        ms--;
    }
}
