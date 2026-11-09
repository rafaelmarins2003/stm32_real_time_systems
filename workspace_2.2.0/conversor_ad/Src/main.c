/*
 * Prática 4 - Conversor analógico digital (ADC1, canal 1 = PA1)
 *
 * O ADC do STM32 é de 12 bits por aproximação sucessiva: a tensão de entrada
 * (0 a 3,3 V) vira um número de 0 a 4095. O resultado é enviado pela serial
 * (UART2, 115200 bps) e pode ser visto no terminal Hercules.
 *
 * Escolha a parte do experimento em PRATICA_PARTE:
 *   1 - Parte 1:  simples aquisição (polling)
 *   2 - Parte 2:  conversão contínua (polling)
 *   3 - Parte 3:  conversão contínua com interrupção
 *   4 - Tarefa 1: conversão simples ao pressionar o botão (B1, PC13), fim por interrupção
 *   5 - Tarefa 2: igual à Tarefa 1, com o valor convertido em volts (float)
 */

#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "uart.h"
#include "adc.h"

/* Seleciona qual parte do experimento será compilada (1 a 5) */
#define PRATICA_PARTE   5

/* Tensão de referência do ADC (VDDA da placa Nucleo) e valor máximo de 12 bits (2^12 - 1) */
#define VREF            3.3f
#define ADC_MAX         4095.0f

#define GPIOCEN         (1U<<2)     /* RCC_AHB1ENR bit 2: clock do GPIOC (botão B1 está em PC13) */
#define BTN_PIN         (1U<<13)    /* PC13 - B1 do usuário, ativo em nível baixo (0 = pressionado) */
#define SR_EOC_FLAG     (1U<<1)     /* ADC_SR bit 1: fim de conversão (EOC) */

/*
 * Variáveis globais com o resultado da conversão.
 * São globais para poderem ser inseridas em Live Expressions no DEBUG.
 */
#if PRATICA_PARTE == 1 || PRATICA_PARTE == 2
int sensor_value;                   /* último valor digital lido (0 a 4095) */
#elif PRATICA_PARTE == 3
static void adc_callback(void);     /* protótipo: chamada pela ISR do ADC */
uint32_t sensor_value;              /* último valor digital lido (0 a 4095) */
#else
/* volatile: estas variáveis são alteradas na ISR e lidas no main (loop principal),
 * então o compilador não pode guardá-las em registrador nem otimizá-las. */
volatile uint32_t adc_raw;          /* valor digital entregue pela ISR */
volatile uint8_t  adc_done;         /* flag: 1 = há uma nova conversão pronta para imprimir */
#endif

#if PRATICA_PARTE >= 3
/*
 * Rotina de tratamento da interrupção do ADC.
 * O nome ADC_IRQHandler vem da tabela de vetores (startup_*.s), onde está
 * declarado como "weak"; ao definirmos aqui, ele substitui o handler padrão.
 */
void ADC_IRQHandler(void)
{
	/* Verifica o bit EOC - Fim da conversão */
	if ((ADC1->SR & SR_EOC_FLAG) != 0)
	{
#if PRATICA_PARTE == 3
		/* Limpa o EOC (escrever 0 em um bit rc_w0 o zera) */
		ADC1->SR &= ~SR_EOC_FLAG;
		adc_callback();
#else
		/* Lê o resultado; a leitura do DR já limpa o EOC */
		adc_raw = ADC1->DR;

		/* Avisa o loop principal que existe um novo valor.
		 * A ISR fica curta: o printf (lento) é feito no main. */
		adc_done = 1;
#endif
	}
}
#endif

#if PRATICA_PARTE == 3
/* Chamada pela ISR a cada conversão concluída: lê o valor e envia pela serial. */
static void adc_callback(void)
{
	sensor_value = ADC1->DR;
	printf("Sensor value : %d \n\r", (int)sensor_value);

	/* Atraso simples só para o terminal não ficar inundado de mensagens */
	for (int i = 0; i < 100000; i++) {}
}
#endif

#if PRATICA_PARTE >= 4
/* Configura PC13 (botão B1 da placa Nucleo) como entrada digital. */
static void button_init(void)
{
	/* Habilita o clock do GPIOC */
	RCC->AHB1ENR |= GPIOCEN;

	/* MODER bits 27:26 = 00 -> PC13 como entrada */
	GPIOC->MODER &= ~(3U<<26);
}

/* Retorna 1 se o botão está pressionado. O B1 liga o pino ao GND ao ser
 * apertado (nível 0), por isso o resultado de IDR é invertido. */
static int button_pressed(void)
{
	return !(GPIOC->IDR & BTN_PIN);
}
#endif

int main(void)
{
	/* Habilita a FPU (acesso aos coprocessadores CP10 e CP11 em CPACR).
	 * O projeto é compilado com float ABI "hard", então sem isso qualquer
	 * instrução de ponto flutuante causaria uma falha (HardFault). */
	SCB->CPACR |= (0xFU << 20);

	/* Inicializa a serial (printf passa a sair pela UART2) */
	uart2_rxtx_init();

#if PRATICA_PARTE == 1
	/* PARTE 1 - simples aquisição: inicia uma conversão, espera e lê, repetidamente */
	pa1_adc_init();

	while (1)
	{
		start_convertion();                 /* dispara UMA conversão */
		sensor_value = adc_read();          /* espera o EOC e lê o resultado */
		printf("valor digital lido = %d \n\r", sensor_value);

		/* Atraso simples entre as amostras */
		for (int i = 0; i < 100000; i++) {}
	}
#elif PRATICA_PARTE == 2
	/* PARTE 2 - conversão contínua: o ADC converte sem parar depois de iniciado */
	pa1_adc_init();
	start_continuous_convertion();          /* inicia uma única vez; o ADC repete sozinho */

	while (1)
	{
		sensor_value = adc_read();          /* pega o valor da conversão mais recente */
		printf("valor digital lido = %d \n\r", sensor_value);

		for (int i = 0; i < 100000; i++) {}
	}
#elif PRATICA_PARTE == 3
	/* PARTE 3 - interrupção: a cada fim de conversão o ADC chama ADC_IRQHandler */
	pa1_adc_interrupt_init();
	start_continuous_convertion();

	while (1)
	{
		/* nada: todo o trabalho acontece na interrupção */
	}
#else
	/* TAREFAS 1 e 2 - uma conversão simples a cada clique no botão B1,
	 * com o fim da conversão detectado por interrupção. */
	int prev = 0;                           /* estado anterior do botão, para detectar a borda */

	button_init();
	pa1_adc_interrupt_init();

	while (1)
	{
		int now = button_pressed();

		/* Borda de pressionamento (antes solto, agora pressionado):
		 * dispara UMA conversão. Evita disparar repetidamente enquanto o botão fica apertado. */
		if (now && !prev)
		{
			adc_done = 0;
			start_convertion();
		}
		prev = now;

		/* A ISR sinalizou que a conversão terminou: imprime o resultado */
		if (adc_done)
		{
			adc_done = 0;
#if PRATICA_PARTE == 4
			/* Tarefa 1: valor digital bruto (0 a 4095) */
			printf("valor digital lido = %d \n\r", (int)adc_raw);
#else
			/* Tarefa 2: converte para volts. V = valor * VREF / 4095.
			 * Ex.: 2048 -> 2048 * 3,3 / 4095 = ~1,65 V.
			 * %.3f só funciona com a opção "-u _printf_float" do linker. */
			float tensao = ((float)adc_raw * VREF) / ADC_MAX;
			printf("Tensao lida = %.3f V \n\r", tensao);
#endif
		}

		/* Atraso curto que funciona como debounce simples do botão */
		for (volatile int i = 0; i < 20000; i++) {}
	}
#endif
}
