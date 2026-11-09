#ifndef UART_H_
#define UART_H_
#include <stdint.h>

#include "stm32f4xx.h"

/* Lê um byte da serial (espera até chegar um dado). */
char uart2_read(void);
/* Inicializa a UART2 (PA2 = TX, PA3 = RX) a 115200 bps. */
void uart2_rxtx_init(void);

#endif /* UART_H_ */
