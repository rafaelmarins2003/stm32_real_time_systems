# AV1 STR - USART1 + LED (PC10) + ADC1 CH1 em Volts — Log de Desenvolvimento

Este arquivo registra o raciocínio e as decisões tomadas durante a implementação,
para facilitar depois a escrita do relatório (introdução, metodologia, resultados).

## 1. Objetivo da atividade

Modificar o código da Prática 3 (USART Serial), Parte 5, para:
1. Usar **UART1** (em vez de USART2) para a comunicação serial;
2. Fazer o **toggle do LED no pino GPIOC10** (em vez de PA5) a cada byte recebido
   pela serial (mantendo a lógica de recepção por interrupção da Parte 5);
3. Adicionar a **leitura analógica do canal 1 do ADC** (PA1), convertê-la para
   **Volts** e enviar o valor continuamente pela UART1 (equivalente à Tarefa 2
   da Prática 4, adaptada para UART1).

Entrega: relatório (40%) até 18/09/2026; demonstração na placa em 16/09/2026 (60%).

## 2. Ambiente de desenvolvimento

- Projeto: `workspace_2.2.0/av1_usart_serial` (STM32CubeIDE)
- MCU configurado no projeto: **STM32F446RETx** (mesma configuração usada nas
  práticas anteriores 1, 2 e 3 — linker `STM32F446RETX_*.ld`, startup
  `startup_stm32f446retx.s`, define `STM32F446xx`)
- Estilo de código: acesso direto a registradores (bare-metal), sem HAL,
  seguindo o padrão didático das práticas 1-4 do Prof. Imbiriba
- Reference Manual usado como consulta de registradores: RM0383 (STM32F411xC/E).
  Os bits usados neste trabalho (RCC_AHB1ENR, RCC_APB2ENR, GPIOx_MODER/AFRL/AFRH,
  USART_CR1/SR/BRR/DR, ADC_SR/CR2/SQR1/SQR3/DR) são idênticos entre as variantes
  F401/F411/F446 da família STM32F4 (mesmo IP de periférico), portanto válido
  como referência mesmo a placa sendo F446RE.

## 3. Mapeamento de periféricos/pinos (decisões de projeto)

| Função | Prática 3 Parte 5 (original) | Nesta atividade |
|---|---|---|
| USART | USART2 (APB1) | **USART1 (APB2)** |
| TX | PA2 (AF7, via AFRL) | **PA9 (AF7, via AFRH)** |
| RX | PA3 (AF7, via AFRL) | **PA10 (AF7, via AFRH)** |
| LED (toggle no RX) | PA5 | **PC10** |
| Baudrate | 9600 | 9600 (mesma fórmula, clock HSI 16 MHz) |

Detalhe importante identificado no RM: os pinos 0-7 de cada porta usam o
registrador `GPIOx_AFRL`, mas os pinos 8-15 usam `GPIOx_AFRH` — como PA9 e PA10
estão nessa segunda faixa, a configuração da função alternativa (AF7) é feita
em `GPIOA->AFR[1]` (AFRH), diferente do exemplo da apostila que usa
`GPIOA->AFR[0]` (AFRL) para PA2/PA3.

Bits de habilitação de clock confirmados no RM0383 (Tabela do RCC register map):
- `RCC_AHB1ENR`: bit0 = GPIOAEN, bit2 = GPIOCEN
- `RCC_APB2ENR`: bit4 = **USART1EN**, bit8 = **ADC1EN**
  (USART2 ficaria no APB1ENR bit17 — por isso a mudança de barramento importa)

Para o ADC (canal 1 = PA1):
- `GPIOA_MODER` pino1 = `11` (modo analógico)
- `ADC1->SQR3` = 1 (canal 1 como única conversão da sequência)
- `ADC1->SQR1` = 0 (sequência de comprimento 1)
- `ADC1->CR2`: bit0 ADON liga o módulo; bit30 SWSTART dispara a conversão
- `ADC1->SR` bit1 = EOC (fim de conversão, usado em polling)
- `ADC1->DR`: resultado de 12 bits (0-4095), alinhado à direita (padrão)
- Conversão para Volts: `V = DR * 3.3 / 4095.0` (assumindo VDDA = 3.3 V, tensão
  de referência padrão da Nucleo)
- `printf` com `%f` exige habilitar em *Project Properties → C/C++ Build →
  Settings → MCU Settings* a opção **"Use float with printf from newlib-nano
  (-u _printf_float)"** — sem isso o valor não é impresso corretamente.

## 4. Estrutura do código (modular, como nas práticas 3/4)

- `Inc/uart.h` / `Src/uart.c`: driver da USART1 — init (TX+RX+IRQ), write, read
- `Inc/adc.h` / `Src/adc.c`: driver do ADC1 canal 1 — init, leitura bloqueante
- `Src/main.c`: aplicação — configura LED (PC10), inicializa uart1 e adc,
  `USART1_IRQHandler`/`uart_callback` (toggle do LED a cada byte recebido),
  loop principal fazendo leitura do ADC e enviando a tensão via `printf`

## 5. Progresso

- [x] Levantamento dos registradores no RM0383 e nas apostilas 3 e 4
- [x] Definição do mapeamento de pinos/periféricos
- [x] Driver UART1 (`uart.h`/`uart.c`) — TX em PA9, RX em PA10 com interrupção
      (RXNEIE + NVIC), toggle do LED em PC10 a cada byte recebido
- [x] Driver ADC1 canal 1 (`adc.h`/`adc.c`) — modo de conversão única (polling do EOC)
- [x] Integração em `main.c`: inicializa UART1 + ADC, loop principal lê o ADC,
      converte para Volts e envia via `printf` (redirecionado para `uart1_write`)
- [ ] Habilitar a flag `-u _printf_float` no projeto (**ação manual do usuário na
      IDE** — ver seção 7)
- [x] Build de verificação fora da IDE: compilado e **linkado com sucesso**
      (`arm-none-eabi-gcc` 14.3, o mesmo toolchain que acompanha o
      STM32CubeIDE 2.2.0) usando o startup file e o linker script reais do
      projeto (`startup_stm32f446retx.s`, `STM32F446RETX_FLASH.ld`) e a flag
      `-u _printf_float`. Nenhum warning com `-Wall -Wextra`. Tamanho final:
      ~17 KB de código (`.text`), bem dentro dos 512 KB de flash disponíveis.
- [x] Testar na placa: compilar pela IDE, gravar e testar via UART1 real (adaptador
      USB-TTL em PA9/PA10) — **encontrado e corrigido um bug real de ambiente**
      (ver seção 9)
- [ ] Resolver a parte física de visualização da UART1 (ver seção 6)

## 6. Decisões tomadas com o usuário

- **MCU real confirmado: Nucleo-F446RE** (mesma placa física usada nas práticas
  anteriores). Mantido `STM32F446xx` / `STM32F446RETX_*.ld` / `startup_stm32f446retx.s`
  sem alterações. O "F401RE" do enunciado é apenas uma referência genérica do
  template da atividade.
- **Monitor serial: via ST-LINK (porta COM virtual)**, sem adaptador USB-TTL
  externo — decisão do usuário, seguindo a mesma recomendação da apostila.
  Isso exige remapear os solder bridges da Nucleo para tirar a UART2 (padrão)
  do ST-LINK e colocar a UART1 no lugar (ver detalhes e ressalva na seção 7).

## 7. Fiação física da UART1 via ST-LINK (confirmado no manual oficial UM1724)

O ST-LINK da Nucleo é, na prática, dois circuitos em um: (1) o
programador/depurador SWD (usado ao gravar/debugar — não depende de UART) e
(2) um conversor USB↔serial (Virtual COM Port) fisicamente cabeado, de
fábrica, à USART2 (PA2/PA3). Como trocamos para USART1 (PA9/PA10), essa
fiação de fábrica não "escuta" nossos pinos, e por isso nada aparece num
terminal serial mesmo com o firmware gravado e rodando corretamente.

Consultei o manual oficial da placa (STMicroelectronics, **UM1724 "User
manual STM32 Nucleo boards"**, DocID025833) para confirmar o procedimento
exato (a apostila 3 só dá um exemplo genérico com USART3/PC10-11 na
NUCLEO-F103RB). Trecho relevante (seção 5.8, "USART communication"):

> "By default the USART2 communication between the target MCU and ST-LINK MCU
> is enabled [...] (SB13 and SB14 ON, SB62 and SB63 OFF). [...] it is possible
> to connect another USART to ST-LINK MCU using flying wires between Morpho
> connector and CN3."

No esquemático da placa, o conector **CN3 tem dois pinos literalmente
rotulados "TX" e "RX"** (não numerados). Procedimento para a UART1:

1. **Abrir (remover) os solder bridges SB13 e SB14** — desconecta o ST-LINK
   de PA2/PA3 (USART2, padrão de fábrica). É o único passo realmente
   necessário para liberar as linhas do ST-LINK no CN3.
2. **SB62/SB63 não precisam ser alterados** — eles só serviriam para levar
   PA2/PA3 até o conector Arduino/shield, o que não é usado aqui.
3. **Soldar 2 fios**: CN3 pino **"RX"** → **PA9** (TX da UART1, disponível no
   Morpho/Arduino D8); CN3 pino **"TX"** → **PA10** (RX da UART1, D2).
4. Não precisa de fio de GND (mesma placa, terra já compartilhado).

Essa seção do manual descreve o circuito do próprio módulo ST-LINK, que é
idêntico em toda a família Nucleo-64 (mesma placa-mãe MB1136), então vale
também para a F446RE, mesmo a revisão do manual consultada sendo anterior ao
lançamento dessa variante. **Ressalva**: a posição física exata de
SB13/SB14/CN3 no layout da F446RE não foi verificada visualmente (nenhuma
foto da placa real foi conferida) — antes de soldar, confirmar
visualmente esses componentes na placa física (ou revisão mais nova do UM1724
que já lista a F446RE) para não arriscar solda no lugar errado.

**Alternativa sem solda**: usar um adaptador USB-serial externo emprestado
(mesmo que só durante a demonstração), ligando diretamente RX↔PA9, TX↔PA10,
GND↔GND — o firmware é idêntico nos dois casos.

Fontes consultadas:
- STMicroelectronics, UM1724 "User manual STM32 Nucleo boards", DocID025833
  (cópia consultada via https://files.amperka.ru/datasheets/nucleo-usermanual.pdf)
- https://community.st.com/t5/stm32-mcus-boards-and-hardware/nucleo-board-look-at-user-manual-um1724-paragraph-6-8-any-help/td-p/78640

## 8. Diferenças conscientes em relação à Parte 5 da apostila (para o relatório)

Além das mudanças pedidas pela atividade (USART1 em vez de USART2, LED em
PC10 em vez de PA5), há duas decisões de projeto tomadas por nós que não
estavam na apostila original:

1. **Callback de RX simplificado**: a apostila também imprime o caractere
   recebido (`printf("tecla pressionada : %c\n\r", key)`) além de fazer o
   toggle do LED. Na nossa versão o callback só faz o toggle e descarta o
   byte — decisão para não misturar esse print com os prints contínuos de
   tensão do loop principal (pendente de confirmação com o usuário se deve
   voltar a imprimir o caractere recebido).
2. **Reorganização de arquivos**: na apostila, a configuração do LED e o
   `USART_IRQHandler`/callback ficam em `main.c`, e `uart.c` cuida só do
   barramento serial. Na nossa versão, tudo isso (LED + callback + IRQHandler)
   foi movido para dentro de `uart.c`, deixando `main.c` só com a
   orquestração (init + loop do ADC). Escolha de organização, não exigência
   da atividade.

O `while(1)` de `main.c` deixou de ficar vazio (como na Parte 5) porque agora
faz a leitura contínua do ADC e o envio da tensão — isso é esperado, pois vem
da tarefa adicional pedida (leitura do ADC), que a Parte 5 sozinha não tinha.

## 9. Bug real encontrado em bancada: FPU nunca habilitada (`system_stm32f4xx.c` faltando)

Ao testar na placa via UART1 real (adaptador USB-TTL em PA9/PA10, ver seção 7),
o firmware imprimia a mensagem de boot e travava silenciosamente assim que
chegava a primeira leitura de tensão — nenhum print seguinte aparecia, nem
mesmo um `printf("oi")` sem `%f`, usado como teste de bisseção.

**Diagnóstico** (feito só com prints "espiões" na serial, já que "Debug As"
não estava disponível na IDE nesse momento):
1. Prints de diagnóstico logo após `adc1_ch1_init()` confirmaram que os
   registradores do ADC estavam corretos (`APB2ENR` com `ADC1EN`/`USART1EN`
   ligados, `CR2.ADON=1`, `SQR3=1`).
2. Prints dentro de `adc1_ch1_read()` confirmaram que a conversão **terminava
   normalmente** (`EOC` setava, valor lido com sucesso).
3. O travamento acontecia **depois** do ADC funcionar, exatamente na primeira
   conta com números de ponto flutuante do programa inteiro:
   `tensao = (adc_raw * VREF) / ADC_MAX_VALUE;` — mesmo sem nenhum `printf`
   de float por perto (o teste do usuário trocando `%.2f` por `"oi"` não
   eliminou essa conta, só a formatação — por isso continuou travando).

**Causa raiz**: o projeto está configurado com **Floating-point ABI = hard**
(usa a FPU de hardware de verdade, não emulação por software). Para a FPU
poder ser usada, é preciso habilitar o acesso a ela escrevendo no registrador
`SCB->CPACR` — isso é feito dentro de `SystemInit()`. Só que o arquivo
`system_stm32f4xx.c` (que contém essa implementação real) nunca tinha sido
copiado para dentro da pasta `Src/` do projeto — só existia dentro de
`Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/`, uma pasta de
"modelo" que não é compilada automaticamente. O arquivo de startup
(`startup_stm32f446retx.s`) prevê essa ausência e declara um `SystemInit`
"fraco" (`.weak`) vazio só para o link não falhar — mas esse vazio não
habilita a FPU. Resultado: a primeira instrução de ponto flutuante do
programa disparava uma falha de hardware (coprocessador não habilitado), que
cai num laço infinito de tratamento de erro (o `HardFault_Handler` padrão),
sem imprimir nada.

**Correção**: copiado `Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c`
para `Src/system_stm32f4xx.c`. Validado com `nm` no binário linkado que agora
existe um símbolo `SystemInit` forte (`T`, não mais `W` fraco).

**Lição para o relatório**: esse é um erro clássico de "ambiente incompleto"
em projetos bare-metal do STM32CubeIDE montados manualmente (sem usar o
assistente/CubeMX) — o arquivo `system_stm32f4xx.c` é gerado automaticamente
quando se cria um projeto pelo assistente da IDE, mas como o ambiente deste
projeto foi montado copiando pastas manualmente, esse arquivo específico
(escondido dentro de `Templates/`) passou despercebido. Vale mencionar na
metodologia/resultados como parte real do processo de depuração.

**Confirmado em bancada**: após adicionar `system_stm32f4xx.c` e regravar, o
loop passou a rodar continuamente sem travar (leituras do ADC e prints
repetindo sem parar). Diagnósticos temporários removidos e `printf` final
restaurado para `"ADC1 CH1 (PA1): bruto = %u, tensao = %.2f V\r\n"`.
Recompilado fora da IDE com as flags reais do projeto (`-mfloat-abi=hard
-mfpu=fpv4-sp-d16`) para confirmar: build limpo, sem nenhum warning.

## 10. Estado atual

Firmware funcional e testado de ponta a ponta na placa real: USART1 (via
adaptador USB-TTL em PA9/PA10) transmite a leitura do ADC1 canal 1 (PA1) já
convertida em Volts, continuamente. Falta apenas:
- [ ] Ligar um LED externo (+ resistor ~220-330Ω) entre PC10 e GND para
      demonstrar visualmente o toggle na recepção serial (PC10 não tem LED de
      fábrica — ver seção 6)
- [ ] Decidir se reativa o eco do caractere recebido no callback (comentado
      em `uart.c`, controlado manualmente pelo usuário)
- [ ] Escrever o relatório (introdução, metodologia, revisão bibliográfica,
      resultados/conclusões) usando este log como base
- [ ] Ensaiar a demonstração para 16/09/2026
