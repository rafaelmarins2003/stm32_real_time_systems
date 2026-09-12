#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include "stm32f4xx.h"

void uart1_init(void);
void uart1_write(int ch);
char uart1_read(void);

#endif /* UART_H_ */
