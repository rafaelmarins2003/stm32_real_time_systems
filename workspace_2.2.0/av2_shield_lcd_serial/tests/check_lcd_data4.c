/* Rodar na pasta do projeto:
 * cc -Wall -Wextra -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
 *   -DSTM32F446xx -IDrivers/CMSIS/Include \
 *   -IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
 *   tests/check_lcd_data4.c -o /tmp/check_lcd_data4 && /tmp/check_lcd_data4
 * Usa a funcao real com registradores em memoria, sem acessar a placa.
 */
#include <assert.h>
#include "stm32f4xx.h"

static GPIO_TypeDef port_a, port_b;
#undef GPIOA
#undef GPIOB
#define GPIOA (&port_a)
#define GPIOB (&port_b)
#define main firmware_main
#include "../Src/lcd.c"
#undef main

int main(void)
{
    const uint32_t mask_a = (1U << 8U);
    const uint32_t mask_b = (1U << 10U) | (1U << 4U) | (1U << 5U);

    for (uint32_t initial = 0U; initial < 2U; initial++)
    {
        uint32_t previous = initial ? UINT32_MAX : 0U;
        for (uint32_t value = 0U; value < 16U; value++)
        {
            port_a.ODR = previous;
            port_b.ODR = previous;
            lcd_set_data4(value);

            uint32_t received = (((port_a.ODR >> 8U) & 1U) << 3U)
                              | (((port_b.ODR >> 10U) & 1U) << 2U)
                              | (((port_b.ODR >> 4U) & 1U) << 1U)
                              | ((port_b.ODR >> 5U) & 1U);
            assert(received == value);
            assert((port_a.ODR & ~mask_a) == (previous & ~mask_a));
            assert((port_b.ODR & ~mask_b) == (previous & ~mask_b));
        }
    }
    return 0;
}
