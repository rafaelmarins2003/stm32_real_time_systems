#include "stm32f4xx.h"
#include "adc.h"

/*
 * Máscaras de bits usadas nos registradores.
 * (1U<<n) = número inteiro sem sinal com apenas o bit n em 1.
 */
#define GPIOAEN         (1U<<0)     /* RCC_AHB1ENR bit 0: liga o clock da porta GPIOA */
#define ADC1EN          (1U<<8)     /* RCC_APB2ENR bit 8: liga o clock do ADC1 */
#define ADC_CH1         (1U<<0)     /* SQ1 = 1 -> primeira conversão da sequência é o canal 1 (PA1) */
#define ADC_SEQ_LEN_1   0x00        /* L[3:0] = 0 -> sequência com 1 conversão (L = tamanho - 1) */
#define CR2_AD0N        (1U<<0)     /* ADC_CR2 bit 0 (ADON): liga o conversor */
#define CR2_SWSTART     (1U<<30)    /* ADC_CR2 bit 30: inicia a conversão por software */
#define CR2_CONT        (1U<<1)     /* ADC_CR2 bit 1: modo de conversão contínua */
#define CR1_EOCIE       (1U<<5)     /* ADC_CR1 bit 5: habilita a interrupção de fim de conversão */
#define SR_EOC          (1U<<1)     /* ADC_SR bit 1: fim de conversão (End Of Conversion) */

/*
 * Configura o pino PA1 como entrada analógica e prepara o ADC1 para
 * converter o canal 1 (PA1 = ADC1_IN1). A conversão NÃO é iniciada aqui.
 */
void pa1_adc_init(void)
{
	/***** Configura o pino GPIO do ADC *****/

	/* Habilita o clock para acessar o GPIOA
	 * Foi definido inicialmente o GPIOAEN na posição de bit 0, ai nesse caso, estamos atribuindo 1 a posição de bit 0 do
	 * registrador AHB1ENR para o block acessar o GPIOA
	 * */
	RCC->AHB1ENR |= GPIOAEN;

	/* Configura o modo de PA1 para analógico.
	 * MODER usa 2 bits por pino; PA1 ocupa os bits 3:2 que representa (pegando como exemplo os ultimos 8 bit):
	 * 7 6 5 4 3 2 1 0
	 * 0 0 0 0 1 1 0 0.
	 * Modos do MODER:
	 * | Valor dos dois bits | Modo |
		|---|---|
		| `00` | Entrada digital |
		| `01` | Saída digital |
		| `10` | Função alternativa |
		| `11` | Analógico |
			 *
	 * 11 = modo analógico (liga o pino direto ao ADC, sem buffer digital). */
	GPIOA->MODER |= (1U<<2);
	GPIOA->MODER |= (1U<<3);

	/***** Configura o módulo ADC *****/

	/* Habilita o clock para acessar o ADC (ADC1 fica no barramento APB2)
	 * Mesma exata ideia do RCC->AHB1ENR != GPIOAEN, mas agora para acessa o ADC agora, que fica em outro registrador,
	 * por isso ADC1EN*/
	RCC->APB2ENR |= ADC1EN;

	/* Inicia a sequência de conversão.
	 * SQR3 guarda os canais da sequência; SQ1[4:0] (bits 4:0) é o primeiro.
	 * Escrevemos 1 para converter o canal 1 (PA1).
	 * Basicamente Ele reescreve o SQR3 só com o bit 1 na posição 0 do registrador, indicando que no canal 1 ele vai
	 * primeiro e somente acessar o PA1*/
	ADC1->SQR3 = ADC_CH1;

	/* Configura o tamanho da sequência de conversão.
	 * SQR1 bits 23:20 (L[3:0]) = número de conversões - 1.
	 * 0 -> apenas uma conversão (um único canal).
	 * Efetivamente, ele  representa o número de conversoem que serão feitas por sequencia do SQR3
	 * nesse caso como o ADC_SEQ_LEN_1 é 0 em decimal e ele está reescrevendo o registrador inteiro, então que só
	 * teremos uma conversão por sequencia.*/
	ADC1->SQR1 = ADC_SEQ_LEN_1;

	/* Habilita o módulo ADC (bit ADON). Na primeira vez, ele acorda do
	 * modo de economia de energia.
	 * Ou seja, estamos efetivamente ligado o ADC o CR2 é de control register 2 que um dos bits dele é
	 *  justamente controlar o adc
	 *  ADON = 1*/
	ADC1->CR2 |= CR2_AD0N;
}

/*
 * Mesma configuração de pa1_adc_init(), mas com a interrupção de fim de
 * conversão habilitada. Quando o ADC terminar, a CPU executa ADC_IRQHandler().
 */
void pa1_adc_interrupt_init(void)
{
	pa1_adc_init();

	/* Habilita o fim da conversão por interrupção (EOCIE em ADC_CR1) */
	ADC1->CR1 |= CR1_EOCIE;

	/* Habilita a interrupção do ADC no NVIC (controlador de interrupções do núcleo).
	 * Sem isso, o ADC levanta a requisição mas a CPU a ignora. */
	NVIC_EnableIRQ(ADC_IRQn);
}

/*
 * Modo de SIMPLES CONVERSÃO: realiza uma única conversão e o ADC para.
 */
void start_convertion(void)
{
	/* Garante que o modo contínuo está desligado (CONT = 0) */
	ADC1->CR2 &= ~CR2_CONT;

	/* Inicia a conversão AD (SWSTART = 1) */
	ADC1->CR2 |= CR2_SWSTART;
}

/*
 * Modo de CONVERSÃO CONTÍNUA: após cada conversão o ADC começa outra
 * automaticamente, sem intervenção do software.
 */
void start_continuous_convertion(void)
{
	/* Habilita a conversão contínua (CONT = 1) */
	ADC1->CR2 |= CR2_CONT;

	/* Inicia a conversão AD (SWSTART = 1) */
	ADC1->CR2 |= CR2_SWSTART;
}

/*
 * Espera a conversão terminar e devolve o resultado.
 */
uint32_t adc_read(void)
{
	/* Espera a conversão ter terminado (EOC = 1 em ADC_SR) */
	while(!(ADC1->SR & SR_EOC)){}

	/* Lê o resultado da conversão (12 bits, alinhado à direita) no registrador
	 * de dados DR. A leitura de DR limpa o flag EOC automaticamente. */
	return (ADC1->DR);
}
