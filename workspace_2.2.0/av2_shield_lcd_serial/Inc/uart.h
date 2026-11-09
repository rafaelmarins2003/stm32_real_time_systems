#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "stm32f4xx.h"

void uart2_init(void);
uint8_t uart2_rx_ready(void);
char uart2_read(void);
void uart2_write_char(char);
void uart2_write_string(const char *);
void uart2_write_int(int32_t);

#endif
