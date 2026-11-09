# AV2 STR — Guia de apresentação (na ordem em que foi implementado)

Projeto: `workspace_2.2.0/av2_shield_lcd_serial` · Placa: Nucleo-F446RE · Bare-metal, sem HAL · LCD Shield 16x2 (HD44780) com teclado analógico.

Arquivos: `lcd.c/.h` (parte 1), `keypad.c/.h` (parte 2), `tim.c/.h` (parte 3), `main.c` (parte 4), `uart.c/.h` (serial).

## 0. Visão geral

| Módulo | Faz o quê | Periférico |
|---|---|---|
| `tim` | Base de tempo: `delay_us` / `delay_ms` | TIM2 |
| `lcd` | Escreve texto no LCD (4 bits) | GPIOA/B/C |
| `keypad` | Descobre qual tecla está apertada | ADC1 canal 0 (PA0) |
| `uart` | Recebe caractere do PC | USART2 (PA2/PA3) |
| `main` | Junta tudo: boas-vindas, tecla, caractere serial | — |

Ordem de dependência: **tim** vem antes de tudo, porque o LCD precisa de atrasos. Por isso, no `main`, `tim2_init()` é chamado antes de `lcd_init()`.

## 1. Ligação física (shield → Nucleo)

O shield encaixa direto, sem jumper, alinhado pelo lado do **A0/VIN** (os soquetes extras D15/D14, NC e IOREF ficam de fora, do lado do USB).

| Sinal do LCD | Pino Arduino | Pino STM32 |
|---|---|---|
| RS | D8 | PA9 |
| E | D9 | PC7 |
| D4 | D4 | PB5 |
| D5 | D5 | PB4 |
| D6 | D6 | PB10 |
| D7 | D7 | PA8 |
| Teclado (analógico) | A0 | PA0 (ADC1_IN0) |

Nota para a pergunta "o enunciado diz A1": o fio físico do teclado cai no A0 da Nucleo = PA0 = canal 0; o modelo do professor também usa canal 0.

## 2. Parte 3 primeiro na lógica: base de tempo (`tim.c`)

Tudo no LCD depende de tempos (microssegundos). Então a base de tempo é a fundação.

- **TIM2** é um contador de **32 bits**. Clock = 16 MHz (HSI, padrão após o reset).
- `PSC = 15` → divide por 16 → **1 contagem por µs**.
- `ARR = 0xFFFFFFFF` → só dá a volta após ~71 minutos. Ou seja, é um contador **livre** (free-running): nunca para, nunca precisa de interrupção.
- `EGR = UG` força a aplicação do PSC (o prescaler tem registrador de "sombra"). `CEN` liga o contador.

```c
void delay_us(uint32_t us){
    uint32_t start = TIM2->CNT;
    while ((TIM2->CNT - start) < us) {}
}
```
- Guarda o instante inicial e espera passar `us` contagens. A subtração **sem sinal** continua certa mesmo se o contador der a volta (aritmética módulo 2^32).
- `delay_ms(ms)` = `ms` vezes `delay_us(1000)`.

**Por que não esperar o flag UIF?** Com ARR = 0xFFFFFFFF o flag só vem após 71 min → o programa "trava". Foi um erro achado na revisão; a solução foi ler o `CNT`.

## 3. Parte 1: driver do LCD (`lcd.c`)

### 3.1 Conceito do HD44780 em 4 bits
- Só 4 fios de dados (D4–D7) ligados → cada byte vai em **2 nibbles** (alto primeiro).
- **RS**: 0 = comando, 1 = dado (caractere).
- **E (Enable)**: o LCD **captura o dado na borda de descida** de E. Largura mínima do pulso ≈ 450 ns.

### 3.2 `lcd_gpio_init`
1. Liga o clock de GPIOA/B/C (`RCC->AHB1ENR`). Sem clock o periférico ignora escritas.
2. `(void)RCC->AHB1ENR;` é uma leitura "boba" para dar tempo do clock estabilizar.
3. Zera os pinos (`BSRR` com `<< 16`), configura *push-pull* (`OTYPER`), baixa velocidade (`OSPEEDR`), sem pull (`PUPDR`).
4. `MODER = 01` (2 bits por pino) → saída.

### 3.3 `lcd_set_data4(valor)`
Espalha os 4 bits do nibble nos pinos físicos: bit3→PA8 (D7), bit2→PB10 (D6), bit1→PB4 (D5), bit0→PB5 (D4). Usa `|=` e `&= ~` só no bit certo, sem mexer nos outros pinos da porta.

### 3.4 `lcd_write_nibble(nibble, rs)` (função estática, interna)
1. Põe RS (PA9) em 0 ou 1.
2. Coloca o nibble nos pinos D4–D7.
3. **Pulso em E**: E=1 → `delay_us(1)` → E=0 (LCD captura aqui) → `delay_us(50)` para o LCD processar.

`lcd_send_cmd` = `write_nibble(cmd>>4, 0)` + `write_nibble(cmd&0xF, 0)`. `lcd_send_data` é igual com RS=1.

### 3.5 `lcd_init` — a sequência de reset
O LCD não sabe em qual modo ligou (pode estar em 8 bits). A sequência do datasheet leva ele a um estado conhecido:

| Passo | Ação | Espera | Por quê |
|---|---|---|---|
| 1 | `delay_ms(50)` | 50 ms | Tensão estabilizar |
| 2 | nibble `0x3` | 5 ms (>4,1) | Reset por "software" (1ª vez) |
| 3 | nibble `0x3` | 150 µs (>100) | 2ª vez |
| 4 | nibble `0x3` | 150 µs | 3ª vez: agora com certeza num modo conhecido |
| 5 | nibble `0x2` | 150 µs | Troca para **4 bits** |
| 6 | cmd `0x28` | — | 4 bits, 2 linhas, fonte 5x8 |
| 7 | cmd `0x08` | — | Display off |
| 8 | cmd `0x01` | 2 ms | Clear (demora ~1,52 ms) |
| 9 | cmd `0x06` | — | Entry mode: cursor avança à direita |
| 10 | cmd `0x0C` | — | Display on, sem cursor, sem blink |

Os três `0x3` seguidos são enviados só como **nibble** (meio byte) porque, enquanto não está em 4 bits, o LCD lê 8 bits e só temos 4 fios.

### 3.6 `lcd_put_cur(linha, coluna)` e `lcd_send_string`
- Comando "Set DDRAM address": bit 7 em 1 + endereço. Linha 0 começa em `0x00` → `0x80 | col`. Linha 1 começa em `0x40` → `0xC0 | col`.
- `lcd_send_string` percorre a string enviando cada caractere com `lcd_send_data`.
- `lcd_clear` = comando `0x01` + 2 ms de espera.

## 4. Parte 2: teclado (`keypad.c`)

### 4.1 Como o shield lê 5 teclas com 1 fio
Cada tecla liga um resistor diferente num **divisor resistivo**. Cada tecla gera uma tensão diferente em A0, e o ADC converte para número.

### 4.2 `keypad_init`
- Clock de GPIOA e ADC1 (`APB2ENR`).
- `MODER = 11` (analógico) no PA0.
- `SMPR2 = 7` → **480 ciclos** de amostragem: a rede resistiva tem impedância alta, precisa de mais tempo para o capacitor interno carregar.
- `SQR1 = 0` (1 conversão na sequência), `SQR3 = canal`. `ADON` liga o ADC.

### 4.3 `keypad_read_raw`
`SWSTART` inicia a conversão → espera `EOC` → lê `DR` (a leitura limpa o EOC). Valor de 12 bits (0–4095), referência 3,3 V.

### 4.4 Calibração (problema real)
A primeira versão usava limites teóricos (cálculo com resistores) e errou (LEFT lido como DOWN, DOWN como UP, SELECT como NONE). Medimos o valor cru de cada tecla:

| Tecla | Valor medido | Faixa usada |
|---|---|---|
| RIGHT | ≈ 1 | < 300 |
| UP | ≈ 570 | < 1020 |
| DOWN | ≈ 1471 | < 1900 |
| LEFT | ≈ 2332 | < 3000 |
| SELECT | ≈ 3679 | < 3890 |
| NONE | 4095 | resto |

Os limites são o **ponto médio** entre valores vizinhos, o que dá a maior margem para ruído dos dois lados. Em repouso o pino vai a 5 V via pull-up do shield e o ADC satura em 4095.

## 5. Serial (`uart.c`)

- PA2 (TX) / PA3 (RX) em **AF7** (= USART2). `MODER = 10` (função alternativa) e `AFR[0]` (pinos 0–7, 4 bits cada).
- USART2 fica no **APB1** (`RCC->APB1ENR`). O ST-LINK da Nucleo liga esses pinos ao USB → aparece como `/dev/ttyACM0` no PC.
- **BRR = 16 MHz / 115200 = 139** (oversampling de 16, arredondado).
- `CR1 = TE | RE` (transmissor e receptor) e depois `UE` (liga a USART). Formato padrão 8N1.
- `uart2_rx_ready()` testa o flag **RXNE** (chegou byte). `uart2_read()` lê o `DR` (isso limpa o flag).
- É **não bloqueante**: o `main` só consulta, nunca fica esperando.

Para testar no PC: `screen /dev/ttyACM0 115200` e digitar.

## 6. Parte 4: `main.c`

```
init: lcd_gpio_init → tim2_init → lcd_init → keypad_init → uart2_init
tela 1 (3 s): "SIST. TEMPO REAL" / "* U N I F O R *"
lcd_clear
tela 2: "TECLA ACIONADA:" / "NONE"
loop (10 ms):
    debounce + mostrar tecla na linha 2
    se chegou caractere serial → escrever na linha 2, coluna 7
```

- **Debounce**: só aceita a tecla se **duas leituras seguidas** (10 ms entre elas) forem iguais **e** diferentes da tecla atual. Ignora a trepidação do contato. Só reescreve o LCD quando muda (evita piscar).
- Nomes todos com 6 letras (`"UP    "`) para **apagar o nome anterior** sem precisar de `lcd_clear`.
- **Serial**: aceita só caracteres imprimíveis (`' '` a `'~'`), ignora Enter etc. Escreve em `lcd_put_cur(1, 6)` (coluna 6 = **7ª posição**, depois de "SELECT"/"NONE  ").
- O caractere não é guardado: ele só existe na tela. Reiniciando a placa, o LCD é limpo e some.

Modelo de tempo real: **superloop com polling** (ciclo de 10 ms). Não usa interrupção; a latência pior caso é ~10 ms + tempo de escrita no LCD.

## 7. Perguntas prováveis (e respostas curtas)

1. **Por que 4 bits e não 8?** Economiza pinos (4 em vez de 8) e o shield só liga D4–D7. Custo: cada byte são 2 envios.
2. **Para que serve o pulso em E?** O LCD só lê os pinos quando E cai (borda de descida). Sem o pulso, nada é capturado.
3. **Por que três vezes `0x3` no init?** Leva o LCD a um estado conhecido, seja qual for o modo em que ele ligou. Só então se passa para 4 bits (`0x2`).
4. **Por que o `clear` espera 2 ms?** É o comando mais lento (~1,52 ms). Mandar outro antes é ignorado.
5. **Como funciona o `delay_us`?** Lê o contador livre do TIM2 (1 µs por contagem) e espera a diferença chegar a `us`. A subtração sem sinal trata a volta do contador.
6. **Por que 480 ciclos de amostragem no ADC?** A fonte (rede resistiva) tem impedância alta; amostragem curta lê tensão errada.
7. **Como distingue as teclas?** Cada tecla gera uma tensão diferente (divisor resistivo). Comparamos o valor do ADC com limites no ponto médio das leituras medidas.
8. **Por que a leitura "NONE" é 4095?** Sem tecla, o pino fica puxado a 5 V pelo shield; o ADC (ref. 3,3 V) satura no máximo.
9. **O que é o debounce?** Contatos mecânicos oscilam ao apertar. Exigimos duas leituras iguais seguidas para aceitar a mudança.
10. **Por que a serial é por polling?** Simples e suficiente: o loop roda a cada 10 ms e o UART de 115200 bps tem 1 byte por ~87 µs, mas o PC digita bem mais devagar. (Se enviar rajada, perderia bytes: com interrupção ou DMA seria robusto.)
11. **Por que o enunciado diz A1 e vocês usaram canal 0?** O shield encaixado põe o teclado em A0 da Nucleo (PA0 = ADC1_IN0).
12. **O que foi mais difícil?** Calibração do teclado (valores teóricos não bateram, medimos) e o `delay_us` (versão com UIF travava).

## 8. Conceitos de C que apareceram

- `#ifndef / #define / #endif` (include guard): evita incluir o mesmo `.h` duas vezes; convenção `NOME_H`.
- Incluir `.h` e não `.c`; funções declaradas no `.h` **não** podem ser `static` no `.c`; função `static` fica privada ao arquivo (ex.: `lcd_write_nibble`).
- `uint32_t` sem sinal e subtração com volta (wrap).
- `volatile`/leitura de registrador: registradores de periféricos podem mudar fora do programa.

## 9. Roteiro sugerido de demonstração

1. Mostrar o hardware (shield encaixado, sem jumpers) e a tabela de pinos.
2. Reset → tela de boas-vindas (3 s) → "TECLA ACIONADA: NONE".
3. Apertar cada tecla (RIGHT, UP, DOWN, LEFT, SELECT) e mostrar o nome mudando.
4. `screen /dev/ttyACM0 115200` → digitar um caractere → aparece na linha 2.
5. Explicar a calibração e o `delay_us` se perguntarem.
