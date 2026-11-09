#include "uart.h"

#define SYS_FREQ        16000000U   /* HSI de 16 MHz (padrao apos o reset); APB1 sem prescaler */
#define UART_BAUDRATE   115200U

/* Configura PA2 (TX) e PA3 (RX) como USART2 (AF7) a 115200 bps, 8N1. */
void uart2_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;

    /* PA2 e PA3 em modo funcao alternativa (MODER = 10). */
    GPIOA->MODER = (GPIOA->MODER & ~((3U << 4U) | (3U << 6U)))
                 | (2U << 4U) | (2U << 6U);

    /* AF7 = USART2 em AFR[0] nos bits 11:8 (PA2) e 15:12 (PA3). */
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFFU << 8U)) | (0x77U << 8U);

    /* BRR = clock / baudrate (oversampling de 16), arredondado: 16 MHz / 115200 = 139. */
    USART2->BRR = (SYS_FREQ + (UART_BAUDRATE / 2U)) / UART_BAUDRATE;

    USART2->CR1 = USART_CR1_TE | USART_CR1_RE;
    USART2->CR1 |= USART_CR1_UE;
}

/* Retorna 1 se ha um byte recebido esperando leitura (nao bloqueia). */
uint8_t uart2_rx_ready(void)
{
    return (USART2->SR & USART_SR_RXNE) != 0U;
}

/* Le o byte recebido; a leitura do DR limpa o RXNE. Use depois de uart2_rx_ready(). */
char uart2_read(void)
{
    return (char)USART2->DR;
}

/* Envia um caractere: espera o TXE (registrador de transmissao vazio) e escreve no DR. */
void uart2_write_char(char c)
{
    while ((USART2->SR & USART_SR_TXE) == 0U) {}
    USART2->DR = (uint8_t)c;
}

void uart2_write_string(const char *str)
{
    while (*str)
        uart2_write_char(*str++);
}

/* Envia um inteiro em decimal, sem printf. */
void uart2_write_int(int32_t value)
{
    char digits[11];
    uint8_t n = 0U;
    uint32_t v;

    if (value < 0)
    {
        uart2_write_char('-');
        v = (uint32_t)(-(value + 1)) + 1U;   /* Evita overflow em INT32_MIN. */
    }
    else
    {
        v = (uint32_t)value;
    }

    /* Os digitos saem do menos para o mais significativo, por isso ficam guardados e sao enviados ao contrario. */
    do
    {
        digits[n++] = (char)('0' + (v % 10U));
        v /= 10U;
    } while (v > 0U);

    while (n > 0U)
        uart2_write_char(digits[--n]);
}
