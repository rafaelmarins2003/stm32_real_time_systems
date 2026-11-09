# AV1 STR — Guia de apresentação (código passo a passo + perguntas prováveis)

Projeto: `workspace_2.2.0/av1_usart_serial` · Placa: Nucleo-F446RE (Cortex-M4F) · Bare-metal, sem HAL.

## 1. Fluxo completo, do RESET ao loop

1. **RESET** → CPU lê o vetor 0 (SP inicial) e o vetor 1 (`Reset_Handler`) no início da Flash (`startup_stm32f446retx.s`).
2. `Reset_Handler` copia a seção `.data` da Flash para a RAM, zera a `.bss`, chama `SystemInit()`, depois `__libc_init_array()` e por fim `main()`.
3. `SystemInit()` (`system_stm32f4xx.c`) **habilita a FPU** (`SCB->CPACR`, coprocessadores CP10/CP11) e deixa o RCC no padrão: **HSI 16 MHz**, sem PLL, prescalers AHB/APB1/APB2 = 1. Por isso `SYS_FREQ = APB2_CLK = 16 MHz` no `uart.c`.
4. `main()` chama `uart1_init()` e `adc1_ch1_init()`, imprime a mensagem de boot e entra no `while(1)`.
5. No `while(1)` (**superloop**): lê o ADC por *polling*, converte para Volts, imprime, faz um atraso.
6. **Em paralelo**, cada byte que chega na USART1 dispara a **interrupção** `USART1_IRQHandler`, que imprime a tecla e inverte o LED em PC10.

Modelo de tempo real usado: **laço infinito + interrupção**. A tarefa periódica (ADC) fica no laço; o evento assíncrono (byte recebido) é atendido por IRQ, com latência limitada (~12 ciclos de entrada na exceção no Cortex-M4 + o tempo do handler), independente de onde o laço estiver.

## 2. `main.c`

| Linha | O que faz | Por quê |
|---|---|---|
| 14–15 | `VREF = 3.3f`, `ADC_MAX_VALUE = 4095.0f` | ADC de 12 bits → 2^12 − 1 = 4095. VREF+ da Nucleo = VDDA = 3,3 V. |
| 18–22 | `__io_putchar(int ch)` chama `uart1_write(ch)` | Redireciona o `printf`. Cadeia: `printf` → newlib → `_write()` (`syscalls.c`) → `__io_putchar()` por caractere → USART1. `_write` e `__io_putchar` são `weak` no `syscalls.c`; nossa definição forte substitui. |
| 29–30 | `uart1_init(); adc1_ch1_init();` | Inicializa periféricos antes de usar. A UART vem primeiro para o `printf` já funcionar. |
| 32 | mensagem de boot | Prova visual de que o firmware reiniciou. `\r\n` porque o terminal precisa de CR+LF. |
| 36 | `adc_raw = adc1_ch1_read();` | Conversão única, bloqueante (polling do EOC). |
| 37 | `tensao = adc_raw * 3.3 / 4095` | Regra de três: 0 → 0 V, 4095 → 3,3 V. Resolução ≈ 0,8 mV/LSB. Conta em `float` na FPU de hardware. |
| 39 | `printf("... %u ... %.2f V")` | `%.2f` exige a flag `-u _printf_float` (newlib-nano não linka suporte a float por padrão). |
| 42 | `for (volatile int i...) 300000` | Atraso "burro" para não inundar o terminal (≈ 3 leituras/s). `volatile` impede o compilador de remover o laço. |

## 3. `uart.c`

### 3.1 Pinos (GPIOA, PA9 = TX, PA10 = RX)
- **Linha 33** `RCC->AHB1ENR |= GPIOAEN` (bit 0): todo periférico nasce com clock desligado para economizar energia; sem isso, escrever nos registradores do GPIOA não tem efeito.
- **Linhas 36–39** `MODER`: 2 bits por pino, pino *n* usa bits `2n+1:2n`. PA9 → bits 19:18, PA10 → bits 21:20. Valor `10` = **função alternativa** (entrega o pino ao periférico, não ao GPIO).
- **Linhas 43–51** `AFR[1]` (= **AFRH**): pinos 0–7 usam `AFR[0]` (AFRL), pinos 8–15 usam `AFR[1]` (AFRH), 4 bits por pino. PA9 → bits 7:4, PA10 → bits 11:8. Valor **AF7 = 0111** = USART1/2/3. *Diferença em relação à apostila (USART2 em PA2/PA3), que usava AFRL.*

### 3.2 LED em PC10
- **Linhas 54–56**: clock do GPIOC (bit 2 do AHB1ENR); `MODER` bits 21:20 = `01` = saída *push-pull*. PC10 não tem LED na placa, por isso o LED externo + resistor.

### 3.3 USART1
- **Linha 59** `RCC->APB2ENR |= USART1EN` (bit 4). **USART1 fica no barramento APB2**; a USART2 da apostila fica no APB1 (bit 17 do APB1ENR). Por isso o clock usado no cálculo do baud é o do APB2.
- **Linha 61** baud rate → `BRR = (16 000 000 + 4800) / 9600 = 1667 = 0x683`. Interpretação do hardware: mantissa `0x68` = 104, fração `3`/16 → USARTDIV = 104,1875 → baud real = 16 MHz / (16 × 104,1875) = **9598 bps** (erro 0,02 %). O `+ BaudRate/2` é arredondamento para o inteiro mais próximo. Escrever `fck/baud` direto no BRR funciona porque, com *oversampling* 16, `BRR = 16 × USARTDIV`.
- **Linha 64** `CR1 = TE | RE` (bits 3 e 2): habilita transmissor e receptor. Usa `=` (não `|=`) para partir de um estado limpo (8 bits, sem paridade, 1 stop bit são os defaults).
- **Linha 67** `CR1 |= RXNEIE` (bit 5): a flag RXNE passa a gerar pedido de interrupção.
- **Linha 68** `NVIC_EnableIRQ(USART1_IRQn)`: USART1 é a **IRQ 37**; a função CMSIS seta o bit 5 de `NVIC->ISER[1]`. Dois "portões": o do periférico (RXNEIE) e o do NVIC (ISER).
- **Linha 71** `CR1 |= UE` (bit 13): liga a USART por último, depois de configurada.

### 3.4 Transmissão (`uart1_write`, linhas 74–81)
Espera `SR.TXE` (bit 7, *transmit data register empty*), depois escreve em `DR`. O hardware move para o *shift register* e serializa (start, 8 dados, stop). Bloqueante: ~1,04 ms por caractere a 9600 bps.

### 3.5 Recepção por interrupção (linhas 92–108)
1. Byte chega no pino PA10 → USART monta o byte → seta `SR.RXNE` (bit 5).
2. Como RXNEIE = 1 e a IRQ 37 está habilitada no NVIC, a CPU salva contexto (r0–r3, r12, LR, PC, xPSR na pilha) e salta para o endereço no vetor 16+37 da tabela.
3. `USART1_IRQHandler` (nome fixo, definido `.weak` no startup apontando para `Default_Handler`; nossa função forte substitui) confere `SR.RXNE` e chama `uart_callback`.
4. `uart_callback` lê `DR` — **a leitura do DR limpa RXNE** (sem isso a IRQ dispararia para sempre) — imprime a tecla e faz `GPIOC->ODR ^= LED_PIN` (**XOR = toggle**).
5. Retorno da exceção restaura o contexto; o `while(1)` continua de onde parou.

`uart1_read()` (polling) ficou no driver por completude/compatibilidade com a apostila, mas não é usada: no modo por interrupção quem lê o DR é a ISR.

## 4. `adc.c`

- **Linhas 20–24**: clock do GPIOA; PA1 → `MODER` bits 3:2 = `11` = **modo analógico** (desliga o buffer digital de entrada, deixando o pino só para o ADC).
- **Linha 27**: `RCC->APB2ENR |= ADC1EN` (bit 8). ADC1 também está no APB2.
- **Linha 30** `SQR3 = 1`: o campo `SQ1` (primeira conversão da sequência regular) recebe o **canal 1** (canal 1 do ADC1 é fisicamente o PA1 — pino A1 do header Arduino).
- **Linha 33** `SQR1 = 0`: campo `L[3:0]` = 0 → sequência de **1 conversão**.
- **Linha 36** `CR2 |= ADON` (bit 0): liga o ADC. Defaults mantidos: 12 bits (`CR1.RES = 00`), alinhado à direita, modo de conversão única (`CONT = 0`), tempo de amostragem 3 ciclos, clock do ADC = PCLK2/2 = 8 MHz.
- **`adc1_ch1_read`**: `CR2 |= SWSTART` (bit 30) dispara; espera `SR.EOC` (bit 1); lê `DR` (0–4095), o que **também limpa EOC**. Tempo de conversão ≈ (3 + 12) ciclos / 8 MHz ≈ **1,9 µs**.

## 5. Mapa de memória usado (de `stm32f446xx.h`)

| Periférico | Endereço base | Barramento |
|---|---|---|
| RCC | 0x4002 3800 | AHB1 |
| GPIOA | 0x4002 0000 | AHB1 |
| GPIOC | 0x4002 0800 | AHB1 |
| USART1 | 0x4001 1000 | APB2 |
| ADC1 | 0x4001 2000 | APB2 |
| USART2 (apostila) | 0x4000 4400 | APB1 |

`USART1->CR1` é açúcar sintático para `*(volatile uint32_t *)(0x40011000 + 0x0C)`; as structs (`USART_TypeDef`, `GPIO_TypeDef`…) vêm do CMSIS.

## 6. Perguntas prováveis e respostas curtas

1. **Por que USART1 e não USART2?** Exigência da atividade. Mudou barramento (APB1 → APB2), pinos (PA2/3 → PA9/10), registrador de AF (AFRL → AFRH) e IRQ (38 → 37).
2. **Por que o ST-LINK não mostra nada?** O VCP do ST-LINK é cabeado de fábrica na USART2 (PA2/PA3, SB13/SB14). Para a USART1 precisaria refazer os solder bridges; usei um conversor USB-TTL externo em PA9/PA10.
3. **Como o `printf` chega na serial?** `printf` → `_write` (newlib, `syscalls.c`) → `__io_putchar` → `uart1_write`. Sobrescrevi a função `weak`.
4. **De onde vem o 16 MHz?** HSI interno, default após reset; `SystemInit` não configura PLL. APB2 = SYSCLK/1.
5. **Como calcula o BRR?** 16 MHz / 9600 = 1666,7 → 1667 = 0x683 (mantissa 104, fração 3/16). Erro 0,02 %.
6. **O que é AF7?** Função alternativa 7 = USART1/2/3 no STM32F4 (tabela de AF do datasheet).
7. **Por que ler o DR na interrupção?** Pega o byte e limpa RXNE. Se não ler, a IRQ reentra sem parar.
8. **O que acontece se a IRQ chegar no meio do `printf` do loop?** A ISR preempta, imprime "tecla recebida" e volta; as linhas podem sair intercaladas no terminal. Limitação conhecida.
9. **Chamar `printf` dentro da ISR é boa prática?** Não. ISR deve ser curta; o ideal é só setar uma flag/enfileirar e deixar o laço imprimir. Mantive por fidelidade à apostila e para demonstração.
10. **Por que `ODR ^= bit`?** XOR inverte só aquele bit (toggle) sem mexer nos outros pinos da porta.
11. **Por que `volatile` no laço de atraso?** Sem ele o otimizador elimina o laço vazio.
12. **Por que dividir por 4095 e não 4096?** 4095 é o maior código (fim de escala = VREF). Com 4096 o código máximo daria 3,299 V. As duas convenções existem; usei a da apostila.
13. **Qual a resolução?** 3,3 V / 4095 ≈ 0,806 mV por LSB.
14. **Por que os valores oscilam com PA1 solto?** Entrada flutuante de alta impedância capta ruído. Ligado no 3V3/GND/potenciômetro fica estável.
15. **O que é `SQR3`/`SQR1`?** Registradores de sequência regular: `SQR3.SQ1` = qual canal converte primeiro; `SQR1.L` = quantas conversões na sequência.
16. **Polling × interrupção — onde usou cada um?** ADC: polling do EOC (laço principal, periódico). UART RX: interrupção (evento assíncrono). TX: polling do TXE.
17. **Qual bug encontrou?** Firmware travava na primeira conta em `float`: `system_stm32f4xx.c` não estava no `Src/`, então o `SystemInit` era o *weak* vazio do startup e a FPU nunca era habilitada → UsageFault/HardFault (coprocessador desabilitado). Corrigido copiando o arquivo do `Templates/`.
18. **Qual o processador?** Cortex-M4F, ARMv7E-M, só Thumb-2, NVIC, FPU simples precisão (FPv4-SP). Compilado com `-mfloat-abi=hard -mfpu=fpv4-sp-d16`.
19. **Qual a latência de interrupção?** ~12 ciclos para entrar na exceção no Cortex-M4 (empilhamento automático), mais o tempo do handler.
20. **O que é tempo real aqui?** Não é velocidade, é previsibilidade: o byte recebido é atendido em tempo limitado via IRQ, independentemente do que o laço esteja fazendo.
21. **Por que `-u _printf_float`?** newlib-nano exclui o formatador de float para economizar Flash; a flag força o link.
22. **Como o hardware sabe que `USART1_IRQHandler` é o handler?** O nome está na tabela de vetores do startup (`.word USART1_IRQHandler`, vetor 53 = 16 + 37). Está declarado `weak` para `Default_Handler`; ao definir uma função com esse nome, o linker usa a minha.

## 7. Limitações que vale admitir se perguntarem
- `printf` dentro da ISR (longo e newlib não é reentrante) — funciona, mas não é o ideal.
- Atraso por laço de software em vez de timer/SysTick.
- Sem tratamento de erros da USART (ORE, FE, NE) nem *debounce*.
- ADC com tempo de amostragem mínimo (3 ciclos) e fonte de alta impedância → ruído.
- `(void)key;` sobrou de quando o eco estava comentado; é inofensivo.
