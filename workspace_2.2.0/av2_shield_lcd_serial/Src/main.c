#include <stdint.h>
#include "stm32f4xx.h"
#include "lcd.h"
#include "tim.h"
#include "keypad.h"
#include "uart.h"
#include "adxl345.h"

/* Coluna (0-15) do caractere recebido pela serial na linha 2: "NONE  X" -> 7a posicao. */
#define SERIAL_COL  6

/* Todos com 6 letras para apagar o nome anterior ao reescrever a linha 2. */
static const char *key_name[] = {"RIGHT ", "UP    ", "DOWN  ", "LEFT  ", "SELECT", "NONE  "};

static uint8_t adxl_ok = 0U;

/* Envia "X=.. (.. mg)  Y=.. (.. mg)  Z=.. (.. mg)" pela serial. Cada LSB vale 3,9 mg. */
static void write_axis(const char *name, int16_t raw)
{
    uart2_write_string(name);
    uart2_write_int(raw);
    uart2_write_string(" (");
    uart2_write_int(((int32_t)raw * 39) / 10);
    uart2_write_string(" mg)");
}

/* Comando serial 'A': le os 3 eixos do ADXL345 e responde pela serial. */
static void accel_report(void)
{
    int16_t x, y, z;

    /* Se o sensor nao respondeu no boot (ex.: fio solto), tenta de novo a cada comando. */
    if (!adxl_ok)
        adxl_ok = (adxl345_init() == ADXL345_OK);

    if (!adxl_ok || adxl345_read_xyz(&x, &y, &z) != ADXL345_OK)
    {
        adxl_ok = 0U;
        uart2_write_string("ADXL345: sem resposta (confira SCL/SDA/3V3/GND)\r\n");
        return;
    }

    write_axis("X=", x);
    write_axis("  Y=", y);
    write_axis("  Z=", z);
    uart2_write_string("\r\n");
}

int main(void)
{
    uint8_t key = KEY_NONE;
    uint8_t cand = KEY_NONE;
    uint8_t prev_cand = KEY_NONE;

    lcd_gpio_init();
    tim2_init();          /* antes de qualquer delay */
    lcd_init();
    keypad_init();
    uart2_init();
    adxl_ok = (adxl345_init() == ADXL345_OK);

    /* Tela de boas-vindas por 3 s. */
    lcd_put_cur(0, 0);
    lcd_send_string("SIST. TEMPO REAL");
    lcd_put_cur(1, 0);
    lcd_send_string("* U N I F O R *");
    delay_ms(3000U);

    /* Tela de espera de tecla. */
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("TECLA ACIONADA:");
    lcd_put_cur(1, 0);
    lcd_send_string(key_name[KEY_NONE]);

    while (1)
    {
        /* Debounce simples: so aceita a tecla se a leitura se repetir duas vezes seguidas. */
        cand = keypad_read_key();
        if (cand == prev_cand && cand != key)
        {
            key = cand;
            lcd_put_cur(1, 0);
            lcd_send_string(key_name[key]);   /* NONE ao soltar a tecla */
        }
        prev_cand = cand;

        /* Caractere recebido pela serial vai para a linha 2, depois do nome da tecla. */
        if (uart2_rx_ready())
        {
            char c = uart2_read();
            if (c >= ' ' && c <= '~')         /* ignora Enter e outros nao imprimiveis */
            {
                lcd_put_cur(1, SERIAL_COL);
                lcd_send_data((uint8_t)c);
            }

            if (c == 'A' || c == 'a')
                accel_report();
        }

        delay_ms(10U);
    }
}
