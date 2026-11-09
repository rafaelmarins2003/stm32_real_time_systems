#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>
#include "stm32f4xx.h"

#define ADXL345_OK    0U
#define ADXL345_ERR   1U

/* Configura o I2C1 (PB8 = SCL, PB9 = SDA) e liga o ADXL345. Retorna ADXL345_OK se o sensor respondeu. */
uint8_t adxl345_init(void);

/* Le os tres eixos (valores brutos, 3,9 mg por LSB). Retorna ADXL345_OK se a leitura deu certo. */
uint8_t adxl345_read_xyz(int16_t *x, int16_t *y, int16_t *z);

#endif
