#include "uart.h"

#define GPIOAEN         (1U<<0)     /* RCC_AHB1ENR bit 0: clock do GPIOA (pinos PA2/PA3 da UART2) */
#define UART2EN         (1U<<17)    /* RCC_APB1ENR bit 17: clock da USART2 */

#define CR1_TE          (1U<<3)     /* USART_CR1 bit 3: habilita o transmissor */
#define CR1_RE          (1U<<2)     /* USART_CR1 bit 2: habilita o receptor */

#define CR1_UE          (1U<<13)    /* USART_CR1 bit 13: habilita o módulo USART */
#define SR_TXE          (1U<<7)     /* USART_SR bit 7: registrador de transmissão vazio */
#define SR_RXNE         (1U<<5)     /* USART_SR bit 5: dado recebido pronto para leitura */

#define SYS_FREQ        16000000    /* clock do sistema: HSI de 16 MHz (padrão após o reset) */
#define APB1_CLK        SYS_FREQ    /* sem prescaler, APB1 roda na mesma frequência do sistema */

#define UART_BAUDRATE   115200      /* taxa de transmissão (bits por segundo) */

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t PeriphClk, uint32_t BaudRate);
static uint16_t compute_uart_bd(uint32_t PeriphClk, uint32_t BaudRate);

void uart2_write(int ch);

/* Chamada pela biblioteca C (printf) para cada caractere a enviar.
 * Redireciona a saída do printf para a UART2. */
int __io_putchar(int ch)
{
	uart2_write(ch);
	return ch;
}

/* Configura PA2 (TX) e PA3 (RX) como UART2 e inicializa o módulo a 115200 bps. */
void uart2_rxtx_init(void)
{
	/***** Configura o pino GPIO da UART *****/
	/* Habilita o clock para acessar gpioa */
	RCC->AHB1ENR |= GPIOAEN;

	/* Configura PA2 para modo alternate function (MODER bits 5:4 = 10) */
	GPIOA->MODER &= ~(1U<<4);
	GPIOA->MODER |= (1U<<5);

	/* Seleciona a função alternativa de PA2: AFR[0] bits 11:8 = 0111 (AF7 = USART2_TX) */
	GPIOA->AFR[0] |= (1U<<8);
	GPIOA->AFR[0] |= (1U<<9);
	GPIOA->AFR[0] |= (1U<<10);
	GPIOA->AFR[0] &= ~(1U<<11);

	/* Configura PA3 para modo alternate function (MODER bits 7:6 = 10) */
	GPIOA->MODER &= ~(1U<<6);
	GPIOA->MODER |= (1U<<7);

	/* Seleciona a função alternativa de PA3: AFR[0] bits 15:12 = 0111 (AF7 = USART2_RX) */
	GPIOA->AFR[0] |= (1U<<12);
	GPIOA->AFR[0] |= (1U<<13);
	GPIOA->AFR[0] |= (1U<<14);
	GPIOA->AFR[0] &= ~(1U<<15);

	/***** Configura o módulo UART *****/
	/* Habilita o clock para acessar a UART2 */
	RCC->APB1ENR |= UART2EN;

	/* Configura o baudrate (valor gravado em BRR) */
	uart_set_baudrate(USART2, APB1_CLK, UART_BAUDRATE);

	/* Habilita a transmissão e recepção serial (a atribuição '=' também zera os demais bits do CR1) */
	USART2->CR1 = (CR1_TE | CR1_RE);

	/* Habilita o módulo UART (só depois de configurar baudrate e TE/RE) */
	USART2->CR1 |= CR1_UE;
}

/* Função que lê um byte pela serial (bloqueante: espera chegar um dado) */
char uart2_read(void)
{
	/* Testa se o registrador de recepção de dados não está vazio */
	while(!(USART2->SR & SR_RXNE)){}

	/* Lê o dado recebido (a leitura do DR limpa o flag RXNE) */
	return USART2->DR;
}

/* Função que transmite um byte pela serial (bloqueante: espera o TX ficar livre) */
void uart2_write(int ch)
{
	/* Testa se o registrador de transmissão de dados está vazio */
	while(!(USART2->SR & SR_TXE)){}

	/* Escreve no registrador de dados; isso dispara a transmissão do byte */
	USART2->DR = (ch & 0xFF);
}

/* Grava no BRR o divisor que gera o baudrate desejado a partir do clock do periférico */
static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t PeriphClk, uint32_t BaudRate)
{
	USARTx->BRR = compute_uart_bd(PeriphClk, BaudRate);
}

/* Divisor = clock / baudrate (oversampling de 16: BRR é o próprio quociente).
 * Somar BaudRate/2 arredonda para o inteiro mais próximo.
 * Ex.: (16000000 + 57600) / 115200 = 139 -> 16 MHz / 139 = ~115108 bps. */
static uint16_t compute_uart_bd(uint32_t PeriphClk, uint32_t BaudRate)
{
	return ((PeriphClk + (BaudRate/2U)) / BaudRate);
}
