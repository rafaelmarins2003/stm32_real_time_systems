//#include <stdint.h>
#include "stm32f4xx.h"

//#define PERIPH_BASE (0x40000000UL)
//#define AHB1PERIPH_OFFSET (0x00020000UL)
//#define AHB1PERIPH_BASE (PERIPH_BASE + AHB1PERIPH_OFFSET)
//#define GPIOA_OFFSET (0x0000UL)
//
//#define GPIOA_BASE (AHB1PERIPH_BASE + GPIOA_OFFSET)
//
//#define RCC_OFFSET (0x3800UL)
//#define RCC_BASE (AHB1PERIPH_BASE + RCC_OFFSET)
//
#define GPIOAEN (1U<<0)
//
#define PIN5 (1U<<5)
#define LED_PIN PIN5
//
//typedef struct {
//	volatile uint32_t DUMMY[12];
//	volatile uint32_t AHB1ENR;
//} RCC_TypeDef;
//
//typedef struct {
//	volatile uint32_t MODER;
//	volatile uint32_t DUMMY[4];
//	volatile uint32_t ODR;
//} GPIO_TypeDef;
//
//#define  RCC ((RCC_TypeDef*) RCC_BASE)
//#define GPIOA ((GPIO_TypeDef*) GPIOA_BASE)


int main(void){
	RCC->AHB1ENR |= GPIOAEN;
	GPIOA->MODER |= (1U<<10);
	GPIOA->MODER &=~ (1U<<11);

	while(1){
//		GPIOA->ODR ^= LED_PIN;
		GPIOA->BSRR = LED_PIN;
		for(int i = 0; i<100000; i++){}

		GPIOA->BSRR = (1U<<21);
		for(int i=0;i<100000;i++){}
	}
}
