#include "adxl345.h"
#include "tim.h"

/* Endereco de 7 bits: 0x53 com o pino SDO em GND, 0x1D com SDO em VCC. */
#define ADXL_ADDR_LOW     0x53U
#define ADXL_ADDR_HIGH    0x1DU

#define REG_DEVID         0x00U   /* Sempre le 0xE5. */
#define REG_BW_RATE       0x2CU
#define REG_POWER_CTL     0x2DU
#define REG_DATA_FORMAT   0x31U
#define REG_DATAX0        0x32U   /* X0 X1 Y0 Y1 Z0 Z1: 6 bytes seguidos, byte baixo primeiro. */

#define DEVID_VALUE       0xE5U

/* Tempo maximo de espera por qualquer evento do I2C: sem sensor, o programa nao trava. */
#define I2C_TIMEOUT_US    5000U

static uint8_t adxl_addr = ADXL_ADDR_LOW;

/* Reconfigura o periferico I2C1 do zero (tambem serve para destravar o barramento). */
static void i2c1_config(void)
{
    I2C1->CR1 = I2C_CR1_SWRST;     /* Reset do periferico. */
    I2C1->CR1 = 0U;

    I2C1->CR2 = 16U;               /* FREQ = 16 MHz (clock do APB1). */
    I2C1->CCR = 80U;               /* 100 kHz: meio periodo = 80 x 62,5 ns = 5 us. */
    I2C1->TRISE = 17U;             /* 1000 ns / 62,5 ns + 1. */
    I2C1->CR1 |= I2C_CR1_PE;
}

static void i2c1_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->APB1ENR;

    /* PB8 e PB9: open-drain (o I2C so puxa para 0, o resistor leva a 1). */
    GPIOB->OTYPER |= (1U << 8U) | (1U << 9U);

    /* Pull-up interno de apoio (o modulo ja costuma ter os seus). */
    GPIOB->PUPDR = (GPIOB->PUPDR & ~((3U << 16U) | (3U << 18U)))
                 | (1U << 16U) | (1U << 18U);

    /* AF4 = I2C1: PB8 nos bits 3:0 e PB9 nos bits 7:4 do AFR[1]. */
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~0xFFU) | 0x44U;

    /* Modo funcao alternativa (10). */
    GPIOB->MODER = (GPIOB->MODER & ~((3U << 16U) | (3U << 18U)))
                 | (2U << 16U) | (2U << 18U);

    i2c1_config();
}

/* Espera uma flag do SR1. Retorna 1 se deu timeout ou o sensor deu NACK. */
static uint8_t i2c_wait(uint32_t flag)
{
    uint32_t start = TIM2->CNT;

    while ((I2C1->SR1 & flag) == 0U)
    {
        if ((I2C1->SR1 & I2C_SR1_AF) != 0U)
            return 1U;
        if ((TIM2->CNT - start) > I2C_TIMEOUT_US)
            return 1U;
    }
    return 0U;
}

/* Em caso de erro: encerra a transferencia e limpa o flag de NACK. */
static void i2c_abort(void)
{
    I2C1->CR1 |= I2C_CR1_STOP;
    I2C1->SR1 &= ~I2C_SR1_AF;
}

/* Gera START e envia o endereco. Retorna 1 em erro. */
static uint8_t i2c_start_addr(uint8_t addr, uint8_t read)
{
    I2C1->CR1 |= I2C_CR1_START;
    if (i2c_wait(I2C_SR1_SB))
        return 1U;

    I2C1->DR = (uint8_t)((addr << 1U) | read);
    if (i2c_wait(I2C_SR1_ADDR))
        return 1U;

    return 0U;
}

/* Escreve um valor em um registrador do sensor. */
static uint8_t adxl_write_reg(uint8_t reg, uint8_t value)
{
    /* Se o barramento ficou ocupado de um erro anterior, reinicia o periferico. */
    if ((I2C1->SR2 & I2C_SR2_BUSY) != 0U)
        i2c1_config();

    if (i2c_start_addr(adxl_addr, 0U)) { i2c_abort(); return ADXL345_ERR; }
    (void)I2C1->SR1; (void)I2C1->SR2;            /* Ler SR1 e SR2 limpa o ADDR. */

    I2C1->DR = reg;
    if (i2c_wait(I2C_SR1_TXE)) { i2c_abort(); return ADXL345_ERR; }
    I2C1->DR = value;
    if (i2c_wait(I2C_SR1_BTF)) { i2c_abort(); return ADXL345_ERR; }

    I2C1->CR1 |= I2C_CR1_STOP;
    return ADXL345_OK;
}

/* Le n bytes (n = 1 ou n >= 3) a partir de um registrador, seguindo a sequencia do manual (RM0383). */
static uint8_t adxl_read_regs(uint8_t reg, uint8_t *buf, uint8_t n)
{
    uint8_t i;

    if ((I2C1->SR2 & I2C_SR2_BUSY) != 0U)
        i2c1_config();

    /* 1) Escrita: diz ao sensor qual registrador queremos ler. */
    if (i2c_start_addr(adxl_addr, 0U)) { i2c_abort(); return ADXL345_ERR; }
    (void)I2C1->SR1; (void)I2C1->SR2;
    I2C1->DR = reg;
    if (i2c_wait(I2C_SR1_BTF)) { i2c_abort(); return ADXL345_ERR; }

    /* 2) START repetido, agora em modo leitura. */
    I2C1->CR1 |= I2C_CR1_ACK;
    if (i2c_start_addr(adxl_addr, 1U)) { i2c_abort(); return ADXL345_ERR; }

    if (n == 1U)
    {
        I2C1->CR1 &= ~I2C_CR1_ACK;               /* NACK no unico byte. */
        (void)I2C1->SR1; (void)I2C1->SR2;        /* Limpa o ADDR. */
        I2C1->CR1 |= I2C_CR1_STOP;
        if (i2c_wait(I2C_SR1_RXNE)) { i2c_abort(); return ADXL345_ERR; }
        buf[0] = (uint8_t)I2C1->DR;
        return ADXL345_OK;
    }

    (void)I2C1->SR1; (void)I2C1->SR2;            /* Limpa o ADDR. */

    /* Bytes 0 .. n-4: leitura normal, com ACK. */
    for (i = 0U; i < (uint8_t)(n - 3U); i++)
    {
        if (i2c_wait(I2C_SR1_RXNE)) { i2c_abort(); return ADXL345_ERR; }
        buf[i] = (uint8_t)I2C1->DR;
    }

    /* Tres ultimos: o ultimo precisa de NACK + STOP, entao o ACK e desligado antes. */
    if (i2c_wait(I2C_SR1_BTF)) { i2c_abort(); return ADXL345_ERR; }
    I2C1->CR1 &= ~I2C_CR1_ACK;
    buf[n - 3U] = (uint8_t)I2C1->DR;
    I2C1->CR1 |= I2C_CR1_STOP;
    buf[n - 2U] = (uint8_t)I2C1->DR;
    if (i2c_wait(I2C_SR1_RXNE)) { i2c_abort(); return ADXL345_ERR; }
    buf[n - 1U] = (uint8_t)I2C1->DR;

    return ADXL345_OK;
}

uint8_t adxl345_init(void)
{
    uint8_t id = 0U;
    uint8_t found = 0U;

    i2c1_init();

    /* Procura o sensor nos dois enderecos possiveis e confere o ID (0xE5). */
    adxl_addr = ADXL_ADDR_LOW;
    if (adxl_read_regs(REG_DEVID, &id, 1U) == ADXL345_OK && id == DEVID_VALUE)
        found = 1U;
    else
    {
        adxl_addr = ADXL_ADDR_HIGH;
        if (adxl_read_regs(REG_DEVID, &id, 1U) == ADXL345_OK && id == DEVID_VALUE)
            found = 1U;
    }

    if (!found)
        return ADXL345_ERR;

    if (adxl_write_reg(REG_BW_RATE, 0x0AU)) return ADXL345_ERR;       /* 100 Hz. */
    if (adxl_write_reg(REG_DATA_FORMAT, 0x08U)) return ADXL345_ERR;   /* FULL_RES, +-2 g: 3,9 mg/LSB. */
    if (adxl_write_reg(REG_POWER_CTL, 0x08U)) return ADXL345_ERR;     /* Measure = 1: sai do standby. */

    return ADXL345_OK;
}

uint8_t adxl345_read_xyz(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t b[6];

    if (adxl_read_regs(REG_DATAX0, b, 6U) != ADXL345_OK)
        return ADXL345_ERR;

    /* Cada eixo vem em complemento de dois, byte baixo primeiro. */
    *x = (int16_t)((uint16_t)b[1] << 8U | b[0]);
    *y = (int16_t)((uint16_t)b[3] << 8U | b[2]);
    *z = (int16_t)((uint16_t)b[5] << 8U | b[4]);

    return ADXL345_OK;
}
