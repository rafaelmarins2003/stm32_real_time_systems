#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include "stm32f4xx.h"


void lcd_gpio_init(void);
void lcd_set_data4(uint8_t);
void lcd_send_cmd(uint8_t);
void lcd_send_data(uint8_t);
void lcd_init(void);
void lcd_clear(void);
void lcd_put_cur(int, int);
void lcd_send_string(const char *);

#endif
