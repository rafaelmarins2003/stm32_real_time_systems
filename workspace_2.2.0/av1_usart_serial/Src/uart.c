#include <stdio.h>
#include "uart.h"

/* RCC_AHB1ENR / RCC_APB2ENR */
#define GPIOAEN         (1U << 0)
#define GPIOCEN         (1U << 2)
#define USART1EN        (1U << 4)

/* USART1_CR1 */
#define CR1_RE          (1U << 2)
#define CR1_TE          (1U << 3)
#define CR1_RXNEIE      (1U << 5)
#define CR1_UE          (1U << 13)

/* USART1_SR */
#define SR_RXNE         (1U << 5)
#define SR_TXE          (1U << 7)

/* LED de status em GPIOC10, com toggle a cada byte recebido */
#define LED_PIN         (1U << 10)

#define SYS_FREQ        16000000U   /* HSI interno, sem PLL */
#define APB2_CLK        SYS_FREQ
#define UART_BAUDRATE   9600U

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t PeriphClk, uint32_t BaudRate);
static uint16_t compute_uart_bd(uint32_t PeriphClk, uint32_t BaudRate);
static void uart_callback(void);

void uart1_init(void)
{
    /**** Pinos GPIO: PA9 = TX, PA10 = RX (AF7) ****/
    RCC->AHB1ENR |= GPIOAEN;

    /* PA9 e PA10 em modo alternate function (10) */
    GPIOA->MODER &= ~(1U << 18);
    GPIOA->MODER |=  (1U << 19);
    GPIOA->MODER &= ~(1U << 20);
    GPIOA->MODER |=  (1U << 21);

    /* PA9/PA10 sao pinos 8..15 -> usam o registrador AFRH (AFR[1]),
     * diferente de PA2/PA3 (USART2) que usam o AFRL (AFR[0]) */
    GPIOA->AFR[1] |=  (1U << 4);
    GPIOA->AFR[1] |=  (1U << 5);
    GPIOA->AFR[1] |=  (1U << 6);
    GPIOA->AFR[1] &= ~(1U << 7);   /* PA9  -> AF7 (0111) */

    GPIOA->AFR[1] |=  (1U << 8);
    GPIOA->AFR[1] |=  (1U << 9);
    GPIOA->AFR[1] |=  (1U << 10);
    GPIOA->AFR[1] &= ~(1U << 11);  /* PA10 -> AF7 (0111) */

    /**** LED de status em PC10 (saida) ****/
    RCC->AHB1ENR |= GPIOCEN;
    GPIOC->MODER &= ~(1U << 21);
    GPIOC->MODER |=  (1U << 20);   /* PC10 -> 01: saida de proposito geral */

    /**** Configura o canal serial USART1 ****/
    RCC->APB2ENR |= USART1EN;

    uart_set_baudrate(USART1, APB2_CLK, UART_BAUDRATE);

    /* Habilita transmissao e recepcao */
    USART1->CR1 = (CR1_TE | CR1_RE);

    /* Habilita a interrupcao de recepcao (RXNE) e a IRQ correspondente no NVIC */
    USART1->CR1 |= CR1_RXNEIE;
    NVIC_EnableIRQ(USART1_IRQn);

    /* Habilita a USART1 */
    USART1->CR1 |= CR1_UE;
}

void uart1_write(int ch)
{
    /* Espera o registrador de transmissao ficar vazio (TXE) */
    while (!(USART1->SR & SR_TXE)) {}

    /* Escreve o dado no registrador de dados (data register) */
    USART1->DR = (ch & 0xFF);
}

char uart1_read(void)
{
    /* Espera chegar um dado (buffer de recepcao nao vazio) */
    while (!(USART1->SR & SR_RXNE)) {}

    return USART1->DR;
}

/* Callback de recepcao: chamado pela IRQ a cada byte recebido */
static void uart_callback(void)
{
    char key = USART1->DR;   /* a leitura do DR tambem limpa a flag RXNE */

     printf("tecla recebida: %c\r\n", key);
    (void)key;

    GPIOC->ODR ^= LED_PIN;
}

void USART1_IRQHandler(void)
{
    if (USART1->SR & SR_RXNE)
    {
        uart_callback();
    }
}

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t PeriphClk, uint32_t BaudRate)
{
    USARTx->BRR = compute_uart_bd(PeriphClk, BaudRate);
}

static uint16_t compute_uart_bd(uint32_t PeriphClk, uint32_t BaudRate)
{
    return ((PeriphClk + (BaudRate / 2U)) / BaudRate);
}
