# AV1 — Sistema de Tempo Real (T569-19) — Respostas

**Disciplina:** Sistema de Tempo Real | **Professor:** Imbiriba | **Data:** 14/09/2026
**Aluno:** Rafael Marins
**Prova individual, com consulta. Duração: 100 min.**

---

## 1. Qual afirmação é verdadeira sobre sistemas em tempo real?

**Resposta: b)**

> O principal objetivo de um sistema em tempo real é garantir que todos os eventos ou processos sejam atendidos dentro de suas restrições de tempo.

**Justificativa:** um sistema em tempo real (STR) é definido pela **correção temporal**, e não apenas pela correção lógica do resultado. Um resultado correto entregue depois do prazo (*deadline*) é considerado errado (em sistemas *hard real-time*) ou degradado (em sistemas *soft real-time*). O objetivo, portanto, é a **previsibilidade**: garantir que cada tarefa/evento seja atendido dentro de sua restrição temporal.

As demais alternativas estão erradas porque:
- **a)** STR têm, por definição, restrições de tempo; "eficiência" não substitui prazo.
- **c)** STR são justamente os sistemas usados em tarefas críticas (aviação, automotivo, médico), pois são projetados para garantir os prazos.
- **d)** Gerenciar recursos (CPU, memória, periféricos) de forma eficiente e determinística é essencial para cumprir os prazos.
- **e)** STR são amplamente usados em aplicações industriais e críticas, não apenas em entretenimento.

---

## 2. Qual das seguintes afirmações é verdadeira em relação ao problema que os sistemas em tempo real procuram resolver?

**Resposta: c)**

> O problema tempo real consiste em garantir comportamentos previsíveis, atendendo às restrições temporais impostas pelo ambiente ou pelo usuário.

**Justificativa:** o "problema tempo real" é o problema de responder a estímulos do ambiente (interrupções, sensores, comandos do usuário) dentro de prazos determinados e de forma **previsível/determinística**. Isso normalmente exige **programação concorrente** (várias tarefas atendendo eventos assíncronos) e hardware que favoreça resposta rápida a eventos (interrupções, timers, DMA).

As demais alternativas estão erradas porque:
- **a)** o núcleo do problema são as restrições temporais, não só o uso de recursos.
- **b)** a concorrência é justamente a ferramenta usada para tratar múltiplos eventos simultâneos.
- **d)** hardware voltado à programação puramente sequencial dificulta o atendimento de eventos assíncronos.
- **e)** o foco é o prazo de resposta, não o volume de processamento.

---

## 3. Características do modelo de programação baseado no laço infinito (*one single loop* / *superloop*)

O modelo consiste em uma etapa de inicialização seguida de um `while(1)` que chama, em sequência, todas as rotinas da aplicação. Principais características:

1. **Sem sistema operacional (*bare-metal*)** — não há escalonador; o próprio laço é o "escalonador" da aplicação.
2. **Execução sequencial e cooperativa** — as tarefas são funções chamadas uma após a outra, na ordem fixa em que aparecem no laço; uma tarefa só começa quando a anterior termina (não há preempção entre elas).
3. **Sem prioridades reais entre tarefas** — todas as tarefas têm a mesma "prioridade" implícita, determinada apenas pela ordem de chamada.
4. **Tempo de resposta variável (*jitter*) e pouco previsível** — a latência para atender um evento depende da soma dos tempos de todas as tarefas executadas antes dele no mesmo ciclo; o pior caso é o tempo de uma volta completa do laço.
5. **Uso de interrupções apenas para sinalizar eventos** — as ISRs normalmente são curtas, apenas setam *flags* ou enfileiram dados; o processamento pesado ocorre no laço, por *polling* dessas *flags*.
6. **Baixo *overhead* e simplicidade** — não há troca de contexto, pilhas por tarefa nem *kernel*; consome pouca RAM/Flash, é fácil de entender e depurar em aplicações pequenas.
7. **Ausência de isolamento entre tarefas** — uma função que trava (laço de espera longo, `delay()` bloqueante) paralisa todo o sistema, pois nada pode retomar o controle da CPU além das interrupções.
8. **Escalabilidade limitada** — à medida que funcionalidades são acrescentadas, o ciclo do laço cresce e o tempo de resposta a eventos piora; manter o determinismo torna-se difícil, sendo geralmente o ponto em que se migra para um RTOS.
9. **Temporização por contagem de ciclos ou *timers*** — tarefas periódicas são implementadas com contadores/flags de *timer* verificados no laço, não por um escalonador de tempo.

---

## 4. O que é um banco de registradores em processadores ARM?

**Resposta: b)**

> É um grupo de registradores de propósito geral.

**Justificativa:** o *register bank* (banco de registradores) do ARM é o conjunto de registradores internos de 32 bits (r0–r15) diretamente acessíveis pelas instruções, além dos registradores de estado CPSR/SPSR. A maior parte deles (r0–r12) é de **propósito geral**; r13, r14 e r15 têm papéis convencionados (SP, LR e PC). O banco é ainda "*banked*": alguns registradores possuem cópias próprias em determinados modos de exceção (FIQ, IRQ, SVC, ABT, UND). Eles ficam dentro da CPU, não em RAM (**c** errada), não servem para armazenar dados da aplicação de forma permanente (**e** errada) e não são "específicos" nem exclusivos para a pilha (**a** e **d** erradas).

---

## 5. Diferenças entre memórias flash NOR e NAND — marque as verdadeiras

**Verdadeiras: a), c), d)**

| Item | V/F | Comentário |
|---|---|---|
| **a)** NOR é mais rápida na leitura do que NAND | **V** | A NOR possui linhas de endereço/dados completas e leitura aleatória rápida (permite *Execute-In-Place*, XIP). A NAND lê em páginas, com latência inicial maior. |
| **b)** NOR possui maior densidade de armazenamento do que NAND | **F** | É o contrário: a NAND tem células menores e maior densidade (menor custo por bit). |
| **c)** NAND é usada em dispositivos de armazenamento de massa | **V** | Cartões SD, pendrives, SSDs e eMMC usam NAND justamente pela densidade e custo. |
| **d)** NOR permite acesso aleatório; NAND acesso sequencial | **V** | NOR é endereçável byte a byte, como uma ROM/RAM. NAND é acessada por páginas/blocos, de forma sequencial, através de uma interface de comandos. |

Resumo: **NOR** → firmware/código (leitura rápida, XIP, menor densidade, mais cara); **NAND** → dados em massa (alta densidade, escrita/apagamento rápidos por bloco, exige controlador com correção de erros e gerência de blocos defeituosos).

---

## 6. Funções dos registradores internos da CPU ARM no modo User/System

Os modos **User** (não privilegiado) e **System** (privilegiado) compartilham exatamente o **mesmo banco de registradores** — nenhum registrador é *banked* nesses modos. Os registradores visíveis e suas funções são:

| Registrador | Nome | Função |
|---|---|---|
| **r0–r3** | Argumentos / *scratch* | Registradores de propósito geral. Por convenção da ABI (AAPCS), transportam os **argumentos** de chamadas de função e o **valor de retorno** (em r0/r1). Não precisam ser preservados pela função chamada. |
| **r4–r11** | Variáveis locais | Propósito geral; por convenção, devem ser **preservados** pela função chamada (*callee-saved*), sendo usados para variáveis de longa duração dentro de uma função. r11 costuma ser usado como *Frame Pointer* (FP). |
| **r12** | IP (*Intra-Procedure call scratch*) | Propósito geral; usado como temporário pelo *linker* em chamadas entre módulos (*veneers*). |
| **r13** | **SP** (*Stack Pointer*) | Aponta para o topo da **pilha** do programa corrente, onde são salvos variáveis locais, registradores em chamadas de função e contexto. |
| **r14** | **LR** (*Link Register*) | Recebe automaticamente o **endereço de retorno** quando uma sub-rotina é chamada com `BL`/`BLX`; o retorno é feito com `BX LR` ou `MOV PC, LR`. |
| **r15** | **PC** (*Program Counter*) | Contém o **endereço da próxima instrução** a ser buscada. Devido ao pipeline, ao ser lido vale (endereço atual + 8) em estado ARM ou (+4) em Thumb. Escrever nele provoca um desvio. |
| **CPSR** | *Current Program Status Register* | Guarda as ***flags* de condição** (N, Z, C, V), as **máscaras de interrupção** (I, F), o **estado** ARM/Thumb (T) e o **modo** de operação (M[4:0]). Em modo User o programa só pode alterar as *flags*; os bits de controle são somente leitura. |

**SPSR:** **não existe** nos modos User/System. O *Saved Program Status Register* só é disponibilizado nos modos de exceção (FIQ, IRQ, SVC, ABT, UND), onde recebe uma cópia do CPSR no momento em que a exceção é tomada, para ser restaurado no retorno.

Diferença entre os dois modos: o **User** não pode alterar os bits de controle do CPSR nem acessar recursos privilegiados; o **System** é privilegiado e serve para rodar código do S.O. usando os registradores do usuário, sem risco de corromper o LR/SP dos modos de exceção.

---

## 7. Função de cada bit do registrador CPSR (*Current Program Status Register*)

O CPSR é um registrador de 32 bits dividido em *flags* de condição (parte alta) e bits de controle (parte baixa):

```
 31  30  29  28  27      24      19..16     9   8   7   6   5   4..0
+---+---+---+---+---+---+---+---+--------+---+---+---+---+---+------+
| N | Z | C | V | Q | . | J | . | GE[3:0]| E | A | I | F | T | M4..0|
+---+---+---+---+---+---+---+---+--------+---+---+---+---+---+------+
```

| Bit(s) | Nome | Função |
|---|---|---|
| **31 – N** | *Negative* | Setado quando o resultado da última operação (com flags) foi **negativo** (bit 31 do resultado = 1). |
| **30 – Z** | *Zero* | Setado quando o resultado da última operação foi **zero**. Usado em comparações (`CMP`) e desvios condicionais (`BEQ`, `BNE`). |
| **29 – C** | *Carry* | Setado quando houve **vai-um** (*carry*) na soma, **não houve empréstimo** (*borrow*) na subtração, ou pelo bit expulso em deslocamentos. Base da aritmética sem sinal e multiprecisão. |
| **28 – V** | *Overflow* | Setado quando houve **estouro aritmético com sinal** (resultado não cabe em 32 bits em complemento de dois). |
| **27 – Q** | *Saturation* (ARMv5TE+) | Fica "grudado" em 1 quando uma instrução DSP/saturada (`QADD`, `SMLAxy`, `SSAT`…) satura ou estoura. |
| **24 – J** | *Jazelle* | Indica execução no estado Jazelle (bytecode Java), em núcleos com esse suporte. |
| **19–16 – GE[3:0]** | *Greater-or-Equal* (ARMv6) | Flags por *lane* das instruções SIMD de 8/16 bits, usadas pela instrução `SEL`. |
| **9 – E** | *Endianness* (ARMv6) | Define se os acessos a dados são *little-endian* (0) ou *big-endian* (1). |
| **8 – A** | *Asynchronous abort disable* (ARMv6) | Quando 1, mascara os *aborts* de dados assíncronos (imprecisos). |
| **7 – I** | *IRQ disable* | Quando 1, **desabilita** as interrupções IRQ. |
| **6 – F** | *FIQ disable* | Quando 1, **desabilita** as interrupções rápidas FIQ. |
| **5 – T** | *Thumb state* | 1 = CPU executando instruções **Thumb** (16 bits); 0 = estado **ARM** (32 bits). Alterado por `BX`/`BLX`. |
| **4–0 – M[4:0]** | *Mode bits* | Definem o **modo de processamento** atual: `10000` User, `10001` FIQ, `10010` IRQ, `10011` Supervisor, `10111` Abort, `11011` Undefined, `11111` System. |
| demais | reservados | Não utilizados/reservados (nos núcleos mais antigos, como ARM7, só existem N, Z, C, V, I, F, T e M[4:0]). |

Os bits **N, Z, C, V** são os que controlam a **execução condicional** das instruções ARM (sufixos `EQ`, `NE`, `GT`, `LT`, etc.). Os bits **I, F, T, M** são bits de **controle**, alteráveis apenas em modos privilegiados.

---

## 8. Verdadeiro (V) ou Falso (F) — arquitetura ARM (com correção das falsas)

**( F )** *A arquitetura lógica do processador ARM possui desempenho inferior quando comparada a um processador CISC, pois somente é possível executar a instrução seguinte após a execução da instrução atual.*
➡️ **Correção:** A arquitetura RISC do ARM utiliza **pipeline de instruções**, de modo que a instrução seguinte já é buscada e decodificada **enquanto** a atual é executada. Isso permite desempenho igual ou superior ao de um CISC de complexidade comparável, e não obriga a esperar o término de uma instrução para iniciar a próxima.

**( V )** *Na arquitetura ARM não existe a figura de um registrador chamado ACC (acumulador). Nesse sentido todos os registradores da CPU são de uso geral.*
➡️ **Verdadeiro:** o ARM é uma arquitetura *load-store* com banco uniforme de registradores; qualquer registrador r0–r12 pode ser operando ou destino de uma operação da ULA, diferentemente das arquiteturas baseadas em acumulador (8051, 6502). *Observação:* r13 (SP), r14 (LR) e r15 (PC) possuem funções convencionadas, mas continuam sendo endereçáveis como os demais.

**( F )** *Uma arquitetura de CPU superescalar permite o pipeline de instruções visando aumentar a taxa de execução de instruções por ciclo de clock, mas não a execução paralela de instruções.*
➡️ **Correção:** Uma arquitetura **superescalar** possui **múltiplas unidades de execução e múltiplos pipelines**, permitindo despachar e executar **mais de uma instrução em paralelo no mesmo ciclo de clock** (IPC > 1). O pipeline simples sobrepõe estágios de instruções diferentes; a superescalaridade adiciona a execução paralela.

**( F )** *A memória ROM utilizada para armazenamento do firmware é uma memória FLASH do tipo NAND.*
➡️ **Correção:** A memória usada para armazenar o firmware é uma FLASH do tipo **NOR**, pois permite acesso aleatório byte a byte e execução direta do código (*Execute-In-Place*, XIP). A NAND é usada para armazenamento de dados em massa.

**( F )** *A memória Cache é uma memória não volátil existente dentro da CPU e é utilizada para armazenar dados de forma permanente.*
➡️ **Correção:** A memória Cache é uma memória **volátil** (SRAM) existente dentro da CPU ou junto a ela, utilizada para armazenar **temporariamente** cópias dos dados e instruções mais recentemente/frequentemente usados, reduzindo o tempo médio de acesso à memória principal.

**( V )** *Na família ARM7 os processadores possuem pipeline de 3 estágios e podem operar a 120 MIPS.*
➡️ **Verdadeiro:** o núcleo ARM7 (ex.: ARM7TDMI) usa um pipeline de **3 estágios** (*Fetch*, *Decode*, *Execute*) e, com clock na faixa de 100–133 MHz, atinge desempenho da ordem de **~120–130 MIPS** (≈ 0,9 MIPS/MHz).

**( F )** *A CPU ARM possui 7 modos de processamento sendo 1 privilegiado e 6 não privilegiados.*
➡️ **Correção:** A CPU ARM possui 7 modos de processamento, sendo **6 privilegiados** (FIQ, IRQ, Supervisor, Abort, Undefined e System) e **apenas 1 não privilegiado** (User).

**( V )** *O modo de processamento SUPERVISOR (svc) é selecionado após o RESET e permite a carga de um Sistema Operacional (S.O.) quando necessário.*
➡️ **Verdadeiro:** após o RESET a CPU entra em modo **Supervisor**, com IRQ e FIQ desabilitadas, executando a partir do vetor de reset. É o modo usado para inicializar o sistema/carregar o S.O. e para atender chamadas de sistema (instrução `SWI`/`SVC`).

**( F )** *O modo de processamento ABORT (abt) é invocado quando a CPU não reconhece uma instrução.*
➡️ **Correção:** O modo **ABORT** é invocado em **falhas de acesso à memória** (*Prefetch Abort* na busca de instrução ou *Data Abort* no acesso a dados). Quando a CPU não reconhece uma instrução, é invocado o modo **UNDEFINED (und)**.

**( V )** *A unidade de MMU (Memory Management Unit) permite o isolamento de processos uns dos outros, protegendo a memória de cada processo e garantindo que um processo não acesse a memória de outro processo.*
➡️ **Verdadeiro:** a MMU traduz endereços virtuais em físicos por meio de tabelas de páginas e aplica permissões de acesso por região. Cada processo enxerga seu próprio espaço de endereçamento, e qualquer acesso indevido gera uma exceção (*abort*), garantindo o isolamento exigido por S.O. como Linux (Cortex-A).

**Gabarito resumido:** F – V – F – F – F – V – F – V – F – V

---

## 9. Diferenças entre ARM Cortex-A, Cortex-R e Cortex-M do ponto de vista das aplicações

| | **Cortex-A** (*Application*) | **Cortex-R** (*Real-time*) | **Cortex-M** (*Microcontroller*) |
|---|---|---|---|
| **Objetivo** | Máximo desempenho para rodar S.O. completo | Tempo real crítico com alto desempenho | Baixo custo, baixo consumo e resposta rápida a eventos |
| **Gerência de memória** | **MMU** (memória virtual, isolamento de processos) | **MPU** (proteção, sem memória virtual) — memória determinística, TCM | MPU opcional, sem MMU |
| **Determinismo** | Baixo (cache, previsão de desvio, S.O. com *swap*) | Alto — latência de interrupção **previsível e limitada** | Alto — NVIC com latência de interrupção fixa e muito baixa (≈12 ciclos) |
| **Conjunto de instruções** | ARM/Thumb-2/NEON, 32/64 bits (ARMv7-A/ARMv8-A) | ARM/Thumb-2 (ARMv7-R/ARMv8-R) | Somente **Thumb/Thumb-2** (ARMv6-M/ARMv7-M/ARMv8-M) |
| **Confiabilidade** | Padrão | Recursos de segurança funcional: *dual-core lock-step*, ECC, tolerância a falhas | Simples; versões com TrustZone-M e MPU |
| **Software típico** | Linux, Android, Windows, hipervisores | RTOS certificados (safety), *bare-metal* | *Bare-metal*, superloop, FreeRTOS/Zephyr/CMSIS-RTOS |
| **Aplicações** | Smartphones, tablets, set-top boxes, automotivo *infotainment*, servidores | Freio ABS/ADAS, controladores de HDD/SSD, *baseband* de modems, controle industrial e médico crítico | Microcontroladores (ex.: STM32), IoT, sensores, controle de motores, eletrodomésticos, *wearables* |

**Resumo:**
- **Cortex-A** → prioriza **desempenho e riqueza de software** (S.O. completo com memória virtual); não é determinístico.
- **Cortex-R** → prioriza **previsibilidade temporal e confiabilidade** em sistemas críticos que ainda exigem muito desempenho.
- **Cortex-M** → prioriza **custo, simplicidade, eficiência energética e latência de interrupção baixa** em sistemas embarcados de pequeno porte; é a família usada nos microcontroladores STM32 desta disciplina.

---

## 10.

### a) Pipeline de instruções em um processador RISC

O **pipeline** é uma técnica de organização do processador que divide a execução de uma instrução em vários **estágios** independentes, cada um executado por um bloco de hardware dedicado. No ARM7, por exemplo, são 3 estágios: **Fetch** (busca da instrução na memória), **Decode** (decodificação e leitura dos registradores) e **Execute** (operação na ULA / acesso à memória / escrita do resultado). Pipelines maiores (ARM9: 5 estágios; Cortex-A: 8 a 13+) separam ainda **Memory** e **Write-back**.

A ideia é semelhante a uma linha de montagem: a cada ciclo de clock, uma nova instrução entra no primeiro estágio enquanto as anteriores avançam para os estágios seguintes. Assim, **várias instruções estão em execução simultaneamente**, cada uma em um estágio diferente:

```
ciclo:     1      2      3      4      5
instr 1:  Fetch  Decode Exec
instr 2:         Fetch  Decode Exec
instr 3:                Fetch  Decode Exec
```

O pipeline **não reduz a latência** de uma instrução individual (ela ainda leva 3 ciclos), mas aumenta o ***throughput***: em regime, conclui-se idealmente **uma instrução por ciclo de clock**. Isso explica por que o PC do ARM, ao ser lido, aponta duas instruções à frente (PC + 8).

A arquitetura **RISC** favorece o pipeline porque suas instruções têm **tamanho fixo (32 bits), formato regular, execução em um ciclo e acesso à memória apenas por `LDR`/`STR`** (*load-store*), o que torna cada estágio simples e uniforme. Os problemas do pipeline são os ***hazards***: **estruturais** (dois estágios disputam o mesmo recurso), **de dados** (uma instrução depende do resultado da anterior ainda não escrita) e **de controle** (desvios invalidam as instruções já buscadas, obrigando a esvaziar o pipeline — *flush*). Eles são tratados com *stalls* (bolhas), *forwarding*, execução condicional (característica do ARM, que evita desvios curtos) e previsão de desvios.

### b) Para que serve a memória CACHE e que tipo de memória é utilizada

A **memória cache** existe para reduzir o "gargalo" entre a CPU, que executa instruções em poucos nanossegundos, e a memória principal (DRAM ou Flash), cujo tempo de acesso é dezenas de vezes maior. Ela é uma memória **pequena e muito rápida**, colocada entre o núcleo e a memória principal, que guarda **cópias temporárias** das instruções e dados **usados recentemente ou com maior frequência**.

Seu funcionamento se apoia no **princípio da localidade**:
- **Localidade temporal** — um dado acessado agora tende a ser acessado novamente em breve (laços, variáveis de controle).
- **Localidade espacial** — dados próximos ao acessado tendem a ser acessados em seguida (vetores, código sequencial). Por isso a cache carrega **linhas** inteiras (ex.: 32/64 bytes), não palavras isoladas.

Quando a CPU pede um dado e ele está na cache ocorre um ***hit*** (acesso rápido); caso contrário ocorre um ***miss***, o dado é buscado da memória principal e uma cópia fica na cache. Com taxas de *hit* elevadas (> 90 %), o tempo médio de acesso à memória aproxima-se do tempo da cache, elevando muito o desempenho. As caches são organizadas em níveis — **L1** (separada em instruções e dados, dentro do núcleo, a mais rápida), **L2** e **L3** (maiores, mais lentas, muitas vezes compartilhadas entre núcleos). Em sistemas de tempo real a cache é uma fonte de **não determinismo** (o tempo de acesso varia com *hit/miss*), razão pela qual Cortex-R e Cortex-M usam também memórias TCM/SRAM de acesso fixo ou *flash accelerators* (como o ART Accelerator do STM32).

O tipo de memória utilizado para cache é a **SRAM (Static RAM)**. Ela é feita de células com *flip-flops* (6 transistores), que não precisam de ciclos de *refresh* e possuem tempo de acesso muito menor que o da **DRAM** (usada na memória principal por ser mais densa e barata). A SRAM, porém, ocupa mais área de silício e custa mais por bit, por isso é usada apenas em capacidades pequenas (KB a poucos MB), como cache, e não como memória principal.
