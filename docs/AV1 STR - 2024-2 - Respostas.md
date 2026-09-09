# AV1 - Sistema de Tempo Real (T569-19) — Respostas

Disciplina: Sistema de Tempo Real | Professor: Imbiriba

---

## 1. Qual afirmação é verdadeira sobre sistemas em tempo real?

**Resposta: b)**

> O principal objetivo de um sistema em tempo real é garantir que todos os eventos ou processos sejam atendidos dentro de suas restrições de tempo.

Um sistema em tempo real não é definido pela velocidade absoluta, mas pela **previsibilidade**: cada tarefa deve ser concluída dentro de um *deadline* imposto pelo ambiente/aplicação. As demais alternativas contradizem esse conceito básico (restrições de tempo inexistentes, incompatibilidade com tarefas críticas, gestão ineficiente de recursos, uso restrito a entretenimento).

---

## 2. Qual afirmação é verdadeira sobre o problema que os sistemas em tempo real procuram resolver?

**Resposta: c)**

> O problema tempo real consiste em garantir comportamentos previsíveis, atendendo às restrições temporais impostas pelo ambiente ou pelo usuário.

O "problema tempo real" é essencialmente um problema de **previsibilidade temporal**: garantir que reações a estímulos externos (interrupções, eventos, sensores) ocorram dentro de janelas de tempo determinadas, geralmente exigindo programação concorrente (multitarefa) para atender múltiplos eventos assíncronos.

---

## 3. Características do modelo de programação em laço infinito (*single loop* / *superloop*)

1. **Sem sistema operacional (bare-metal)**: toda a aplicação roda em um único `while(1)` infinito, sem escalonador de tarefas.
2. **Execução sequencial/cooperativa**: as "tarefas" (funções chamadas dentro do loop) são executadas uma após a outra, na ordem em que aparecem no código — não há preempção real entre elas.
3. **Ausência de prioridades entre tarefas**: todas as rotinas do loop têm, na prática, a mesma prioridade implícita (a ordem de chamada).
4. **Tempo de resposta variável e pouco previsível (jitter alto)**: a latência de resposta a um evento depende do tempo total gasto pelas tarefas anteriores no mesmo ciclo do loop.
5. **Baixo overhead e simplicidade**: não há troca de contexto nem RTOS, resultando em código simples, baixo consumo de memória/RAM e fácil depuração para aplicações pequenas.
6. **Uso comum de flags/polling combinadas com interrupções**: interrupções normalmente só sinalizam eventos (setam flags), e o processamento real é feito dentro do loop, verificado por *polling*.
7. **Dificuldade de escalabilidade**: à medida que mais funcionalidades são adicionadas, o loop cresce e a latência de resposta a eventos tende a aumentar, prejudicando o determinismo.
8. **Nenhum isolamento entre tarefas**: uma tarefa mal escrita (laço travado, espera longa) pode bloquear o sistema inteiro, já que não há mecanismo de proteção/preempção.

---

## 4. O que é um banco de registradores em processadores ARM?

**Resposta: b)**

> É um grupo de registradores de propósito geral.

O *register bank* (banco de registradores) do ARM é o conjunto de registradores (r0–r15, mais CPSR/SPSR) disponíveis à CPU para armazenamento temporário de dados/operandos, sendo em sua maioria de uso geral (r0–r12), com alguns papéis convencionados (r13=SP, r14=LR, r15=PC).

---

## 5. Diferenças entre memórias flash NOR e NAND

**Verdadeiras: a, c, d**

- **a) Verdadeiro** — NOR é mais rápida em operações de **leitura** (permite *execute-in-place*, XIP).
- **b) Falso** — é o **contrário**: NAND possui maior densidade de armazenamento (menor custo por bit) do que NOR.
- **c) Verdadeiro** — NAND é o padrão para dispositivos de armazenamento de massa (cartões SD, SSDs, pendrives), devido à alta densidade e menor custo.
- **d) Verdadeiro** — NOR permite acesso aleatório (endereçável byte a byte, como uma RAM), enquanto NAND é acessada em blocos/páginas, de forma sequencial.

---

## 6. Funções dos registradores internos da CPU ARM no modo User/System

No modo **User** (não privilegiado) e **System** (privilegiado, mas que usa o mesmo banco de registradores do User — sem *banking* de registradores), estão disponíveis:

- **r0–r12 (registradores de propósito geral)**: armazenam dados temporários, operandos de instruções aritméticas/lógicas e, por convenção da ABI, r0–r3 são usados para passagem de argumentos e valor de retorno em chamadas de função/subrotina.
- **r13 (SP – Stack Pointer)**: aponta para o topo da pilha (*stack*) usada pela aplicação corrente, para armazenar variáveis locais, contexto salvo, etc.
- **r14 (LR – Link Register)**: armazena o endereço de retorno automaticamente quando uma sub-rotina é chamada via instrução `BL` (*Branch and Link*), permitindo retorno da função sem uso obrigatório da pilha.
- **r15 (PC – Program Counter)**: contém o endereço da próxima instrução a ser buscada/executada; é atualizado automaticamente a cada instrução (ou explicitamente em desvios).
- **CPSR (Current Program Status Register)**: acessível em todos os modos, contém as *flags* de condição (N, Z, C, V) e bits de controle (modo de operação, estado Thumb/ARM, máscaras de interrupção I/F).
- **SPSR**: **não existe** de forma útil em User/System, pois esses não são modos de exceção — o *banking* de SPSR só ocorre nos modos privilegiados de exceção (FIQ, IRQ, SVC, ABT, UND), para salvar o CPSR anterior à entrada na exceção.

---

## 7. Função de cada bit do registrador CPSR

| Bits | Campo | Função |
|---|---|---|
| 31 | **N** | *Negative* — indica se o resultado da última operação foi negativo (bit mais significativo do resultado = 1). |
| 30 | **Z** | *Zero* — indica se o resultado da última operação foi igual a zero. |
| 29 | **C** | *Carry* — indica ocorrência de *carry*/*borrow* (vai-um / empresta-um) na última operação aritmética ou deslocamento. |
| 28 | **V** | *Overflow* — indica estouro (*overflow*) aritmético em operações com sinal. |
| 27–8 | reservados | Não utilizados nesta representação simplificada (em versões mais completas armazenam bits como Q, J, GE[3:0], etc.). |
| 7 | **I** | *IRQ disable* — quando em 1, desabilita (mascara) interrupções IRQ. |
| 6 | **F** | *FIQ disable* — quando em 1, desabilita (mascara) interrupções FIQ. |
| 5 | **T** | *Thumb state* — indica se a CPU está executando no estado Thumb (1) ou ARM (0). |
| 4–0 | **Modo** | *Mode bits (M[4:0])* — definem o modo de operação corrente da CPU (User, FIQ, IRQ, Supervisor, Abort, Undefined, System). |

---

## 8. Verdadeiro/Falso — Arquitetura ARM (com correção das falsas)

**( F )** *A arquitetura lógica do processador ARM possui desempenho inferior quando comparada a um processador CISC, pois somente é possível executar a instrução seguinte após a execução da instrução atual.*
➡️ **Correção**: A arquitetura RISC do ARM utiliza **pipeline de instruções**, permitindo que múltiplas instruções estejam em estágios diferentes de execução simultaneamente (busca, decodificação, execução se sobrepõem), o que geralmente resulta em desempenho igual ou superior ao de um CISC de complexidade comparável, e não impede o início da instrução seguinte antes do término da atual.

**( V )** *Na arquitetura ARM não existe a figura de um registro chamado ACC (acumulador). Nesse sentido todos os registradores da CPU são de uso geral.*
➡️ Considerado **verdadeiro** no sentido pretendido: diferente de arquiteturas baseadas em acumulador (ex.: 8051, x86 clássico), o ARM é *load-store* com um banco uniforme r0–r15. *Observação*: na prática r13 (SP), r14 (LR) e r15 (PC) têm papéis convencionados/especiais, então "uso geral" se aplica plenamente a r0–r12.

**( F )** *Uma arquitetura de CPU superescalar permite o pipeline de instruções visando aumentar a taxa de execução de instruções por ciclo de clock, mas não a execução paralela de instruções.*
➡️ **Correção**: Uma arquitetura **superescalar** é justamente aquela que possui **múltiplas unidades de execução**, permitindo despachar e executar **mais de uma instrução em paralelo por ciclo de clock** (IPC > 1), indo além do simples pipeline (que sobrepõe estágios, mas processa uma instrução por estágio).

**( F )** *A memória ROM utilizada para armazenamento do firmware é uma memória FLASH do tipo NAND.*
➡️ **Correção**: O firmware normalmente é armazenado em memória FLASH do tipo **NOR**, pois permite acesso aleatório e execução direta do código (*Execute In Place* – XIP), o que não é viável de forma eficiente em memórias NAND.

**( F )** *A memória Cache é uma memória não volátil existente dentro da CPU e é utilizada para armazenar dados de forma permanente.*
➡️ **Correção**: A memória Cache é uma memória **volátil** (tipicamente SRAM), localizada dentro ou próxima do núcleo da CPU, utilizada para armazenar **temporariamente** dados e instruções frequentemente/recentemente acessados, reduzindo o tempo médio de acesso à memória principal — e não para armazenamento permanente.

**( V )** *Na família ARM7 os processadores possuem pipeline de 3 estágios e podem operar a 120 MIPS.*
➡️ **Verdadeiro**: o núcleo ARM7 (ex.: ARM7TDMI) utiliza pipeline clássico de **3 estágios** (Fetch, Decode, Execute) e, dependendo do processo de fabricação e clock, atinge desempenhos da ordem de **até ~120–130 MIPS**.

**( F )** *A CPU ARM possui 7 modos de processamento sendo 1 privilegiado e 6 não privilegiados.*
➡️ **Correção**: É o **inverso**: a CPU ARM possui 7 modos de processamento, sendo **6 privilegiados** (FIQ, IRQ, Supervisor, Abort, Undefined, System) e **apenas 1 não privilegiado** (User).

**( V )** *O modo de processamento SUPERVISOR (svc) é selecionado após o RESET e permite a carga de um Sistema Operacional (S.O) quando necessário.*
➡️ **Verdadeiro**: após o RESET, a CPU ARM entra automaticamente em modo Supervisor (privilegiado), usado tipicamente para inicialização do sistema e chamadas de sistema (via instrução `SWI`/`SVC`).

**( F )** *O modo de processamento ABORT (abt) é invocado quando a CPU não reconhece uma instrução.*
➡️ **Correção**: O modo **ABORT** é invocado em falhas de **acesso à memória** (*prefetch abort* ou *data abort*). O reconhecimento de uma instrução inválida/não implementada invoca, em vez disso, o modo **UNDEFINED (und)**.

**( V )** *A unidade de MMU (Memory Management Unit) permite o isolamento de processos uns dos outros, protegendo a memória de cada processo e garantindo que um processo não acesse a memória de outro processo.*
➡️ **Verdadeiro**: a MMU realiza tradução de endereços virtuais para físicos e aplica permissões de acesso por região de memória, permitindo isolamento e proteção entre processos (essencial para SOs com memória virtual, como os que rodam em Cortex-A).

---

## 9. Diferenças entre ARM Cortex-A, Cortex-R e Cortex-M (do ponto de vista das aplicações)

| Família | Foco de aplicação | Características principais |
|---|---|---|
| **Cortex-A** (*Application*) | Sistemas com SO completo (Linux, Android, Windows) | Alto desempenho, possui **MMU** (memória virtual, isolamento de processos), suporta multitarefa complexa; usado em smartphones, tablets, computadores embarcados de alto desempenho. |
| **Cortex-R** (*Real-time*) | Tempo real crítico de alto desempenho | Foco em **determinismo e baixa latência de interrupção**, possui **MPU** (não MMU completa), alta confiabilidade; usado em controladores de freio ABS, discos rígidos/SSD, sistemas industriais críticos. |
| **Cortex-M** (*Microcontroller*) | Controle embarcado simples/IoT | Baixo custo e **baixíssimo consumo de energia**, arquitetura simplificada, resposta rápida a interrupções via NVIC, geralmente sem MMU (podendo ter MPU opcional); usado em microcontroladores, sensores, controle de motores, IoT. |

Em resumo: **Cortex-A** prioriza desempenho e riqueza de software (SO completo); **Cortex-R** prioriza previsibilidade temporal em aplicações críticas de alto desempenho; **Cortex-M** prioriza custo, simplicidade e eficiência energética em sistemas embarcados de pequeno porte.

---

## 10. 

### a) Pipeline de instruções em um processador RISC

*Pipeline* é uma técnica de organização da CPU que divide a execução de cada instrução em múltiplos estágios independentes (tipicamente: **Fetch** – busca, **Decode** – decodificação, **Execute** – execução, e em pipelines maiores também **Memory Access** e **Write-back**). Cada estágio é executado por um hardware dedicado, permitindo que, a cada ciclo de clock, uma nova instrução entre no primeiro estágio enquanto instruções anteriores avançam pelos estágios seguintes — ou seja, **várias instruções ficam "em voo" simultaneamente**, cada uma em um estágio diferente.

Isso não reduz a latência de uma instrução individual, mas aumenta o **throughput** (taxa de instruções concluídas por ciclo, idealmente aproximando-se de 1 instrução/ciclo). A arquitetura RISC favorece o pipeline porque suas instruções têm **tamanho fixo, formato uniforme e poucos modos de endereçamento**, simplificando a decodificação e reduzindo conflitos (*hazards*) entre estágios. Ainda assim, podem ocorrer *hazards* estruturais, de dados e de controle (desvios), tratados com técnicas como *stall* (bolhas), *forwarding* e previsão de desvio.

### b) Memória CACHE — função e tipo de memória utilizada

A memória **Cache** existe para reduzir o impacto da diferença de velocidade entre a CPU (muito rápida) e a memória principal — RAM (mais lenta), armazenando **temporariamente** cópias de dados e instruções frequentemente ou recentemente acessados, aproveitando os princípios de **localidade temporal** (dado acessado tende a ser acessado novamente em breve) e **localidade espacial** (dados próximos tendem a ser acessados em sequência).

Com isso, a CPU consegue obter grande parte dos dados/instruções necessários diretamente da cache, evitando acessos repetidos e mais lentos à memória principal, o que aumenta significativamente o desempenho médio do sistema. Caches costumam ser organizadas em níveis (**L1**, mais rápida e próxima do núcleo; **L2**/**L3**, maiores porém mais lentas).

O tipo de memória utilizado para cache é a **SRAM (Static RAM)**, pois é muito mais rápida que a DRAM (usada na memória principal) — não requer ciclos de *refresh* e tem tempos de acesso muito menores — embora seja mais cara e menos densa por bit, o que a torna inviável para uso como memória principal de grande capacidade.
