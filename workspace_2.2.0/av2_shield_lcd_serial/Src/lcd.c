#include "lcd.h"
#include "tim.h"

static void lcd_write_nibble(uint8_t nibble, uint8_t rs);

void lcd_gpio_init(void)
{
    /* PA8 = D7; PA9 = RS. */
    const uint32_t pins_a = (1U << 8U) | (1U << 9U);
    const uint32_t mask_a = (3U << 16U) | (3U << 18U);

    /* PB4 = D5; PB5 = D4; PB10 = D6. */
    const uint32_t pins_b = (1U << 4U) | (1U << 5U) | (1U << 10U);
    const uint32_t mask_b = (3U << 8U) | (3U << 10U) | (3U << 20U);

    /* PC7 = E. */
    const uint32_t pins_c = (1U << 7U);
    const uint32_t mask_c = (3U << 14U);

    // AHB1ENR = registrador do módulo RCC que controla os clocks.
    /* Habilita o clock das tres portas utilizadas pelo LCD. */
    RCC->AHB1ENR = RCC->AHB1ENR | RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;

    // Forma parar gerar um pequeno delay antes de acessarmos os GPIO para dar tempo de eles serem habilitados certinho.
    // basicamente só busca um valor no registrador e não retorna nada.
    (void)RCC->AHB1ENR;

    // BSRR = Bit Set/Reset Register. É um registrador do GPIO que muda o nível de saída de pinos individuais, de forma atômica
    // do bit 0 a 15 é set, aonde se eu colocar o bit em 1, ele estará setado, do bit 16 a 31, é o reset, se o bit for selecionado lá, ele vai
    // para 0, se os 2 forem acionados, ele fica em set.
    // O << 16U jogs os bits para a metade de reset então todos os pinos vão a 0, garantido que D4-D7, RS e E comecem em nível baixo, só para
    // garantir que caso E de um pulso, não vá sair lixo no LCD..
    GPIOA->BSRR = pins_a << 16U; // pins_a = PA8 e PA9
    GPIOB->BSRR = pins_b << 16U; // PB4, PB5, PB10
    GPIOC->BSRR = pins_c << 16U; // PC7

    /* Saidas push-pull: o STM32 produz tanto nivel alto como baixo. */
    // OTYPER = Output Type Register. Define, para cada pino configurado como saída, como o circuito de saída é montado.
    // O objetivo aqui é só deixar os pinos mencionados em push-pull (que é o estado 0 nesse caso).
    GPIOA->OTYPER &= ~pins_a;
    GPIOB->OTYPER &= ~pins_b;
    GPIOC->OTYPER &= ~pins_c;

    /* Baixa velocidade de comutacao: suficiente para esta interface. */
    // OSPEEDR = Output Speed Register. Define a velocidade de comutação (slew rate) do driver de saída
    // de cada pino: o quão rápido o pino sobe de 0 para 1 e desce de 1 para 0.
    GPIOA->OSPEEDR &= ~mask_a;
    GPIOB->OSPEEDR &= ~mask_b;
    GPIOC->OSPEEDR &= ~mask_c;

    /* Sem resistores internos de pull-up ou pull-down. */
    // PUPDR = Pull-Up/Pull-Down Register. Liga um resistor interno (de aproximadamente 40 kΩ)
    // entre o pino e o VDD (pull-up) ou o GND (pull-down), ou nenhum.
    GPIOA->PUPDR &= ~mask_a;
    GPIOB->PUPDR &= ~mask_b;
    GPIOC->PUPDR &= ~mask_c;

    /* MODER: cada pino ocupa dois bits; 01 significa saida. */
    // define se o pino é entrada, saída, analógico ou função alternativa, nesse caso, todos são saida.
    GPIOA->MODER = (GPIOA->MODER & ~mask_a) | (1U << 16U) | (1U << 18U);
    GPIOB->MODER = (GPIOB->MODER & ~mask_b) | (1U << 8U) | (1U << 10U) | (1U << 20U);
    GPIOC->MODER = (GPIOC->MODER & ~mask_c) | (1U << 14U);
}

void lcd_set_data4(uint8_t value)
{
    /* Bit 3 do grupo -> D7 do LCD -> PA8 da placa STM32. */
    if (value & (1U << 3U))
        GPIOA->ODR |= (1U << 8U);
    else
        GPIOA->ODR &= ~(1U << 8U);

    /* Bit 2 do grupo -> D6 do LCD -> PB10 da placa STM32. */
    if (value & (1U << 2U))
        GPIOB->ODR |= (1U << 10U);
    else
        GPIOB->ODR &= ~(1U << 10U);

    /* Bit 1 do grupo -> D5 do LCD -> PB4 da placa STM32. */
    if (value & (1U << 1U))
        GPIOB->ODR |= (1U << 4U);
    else
        GPIOB->ODR &= ~(1U << 4U);

    /* Bit 0 do grupo -> D4 do LCD -> PB5 da placa STM32. */
    if (value & (1U << 0U))
        GPIOB->ODR |= (1U << 5U);
    else
        GPIOB->ODR &= ~(1U << 5U);
}

static void lcd_write_nibble(uint8_t nibble, uint8_t rs){
	// Controla se será um comando (0) ou envio de dado (1)
	if (rs)
		GPIOA->ODR |= (1U << 9U); // Coloca o bit 9 (PA9) em 1
	else
		GPIOA->ODR &= ~(1U << 9U); // Coloca o bit 9 (PA9) em 0

	lcd_set_data4(nibble); // Pulso em E (PC7)

    GPIOC->ODR |= (1U << 7U);     /* E = 1 */
    delay_us(1);
    GPIOC->ODR &= ~(1U << 7U);    /* E = 0: o LCD captura na borda de descida */
    delay_us(50);                 /* tempo para o LCD processar */

}

void lcd_send_cmd(uint8_t cmd)
{
    lcd_write_nibble(cmd >> 4, 0U);      /* nibble alto */
    lcd_write_nibble(cmd & 0x0FU, 0U);   /* nibble baixo */
}

void lcd_send_data(uint8_t data)
{
    lcd_write_nibble(data >> 4, 1U);
    lcd_write_nibble(data & 0x0FU, 1U);
}

void lcd_init(void)
{
    /* Espera a tensao e o LCD estabilizarem apos ligar. */
    delay_ms(50U);

    /* O LCD pode estar em modo 8 bits: so D4-D7 estao ligados, entao
     * enviamos meio byte (nibble 0x3) tres vezes para leva-lo a um estado conhecido. */
    lcd_write_nibble(0x3U, 0U);
    delay_ms(5U);                /* > 4,1 ms */
    lcd_write_nibble(0x3U, 0U);
    delay_us(150U);              /* > 100 us */
    lcd_write_nibble(0x3U, 0U);
    delay_us(150U);

    /* Nibble 0x2: passa o LCD para o modo 4 bits. */
    lcd_write_nibble(0x2U, 0U);
    delay_us(150U);

    /* A partir daqui ja em 4 bits: cada comando sao dois nibbles. */
    lcd_send_cmd(0x28U);         /* Function set: 4 bits, 2 linhas, fonte 5x8 */
    lcd_send_cmd(0x08U);         /* Display off */
    lcd_send_cmd(0x01U);         /* Clear display */
    delay_ms(2U);                /* Clear demora ate 1,52 ms */
    lcd_send_cmd(0x06U);         /* Entry mode: cursor incrementa, sem deslocar a tela */
    lcd_send_cmd(0x0CU);         /* Display on, cursor off, blink off */
}

void lcd_clear(void)
{
    lcd_send_cmd(0x01U);         /* Clear display */
    delay_ms(2U);                /* Clear demora ate 1,52 ms */
}

// Posiciona cursor do LCD, escolhe em qual linha e coluna o próximo caractere será escrito.
void lcd_put_cur(int row, int col)
{
    /* Set DDRAM address: bit 7 em 1 + endereco. Linha 0 comeca em 0x00, linha 1 em 0x40. */
    uint8_t addr = (uint8_t)col & 0x3FU;

    if (row == 0)
        addr |= 0x80U;
    else
        addr |= 0xC0U;

    lcd_send_cmd(addr);
}

void lcd_send_string(const char *str)
{
    while (*str)
        lcd_send_data((uint8_t)*str++);
}
