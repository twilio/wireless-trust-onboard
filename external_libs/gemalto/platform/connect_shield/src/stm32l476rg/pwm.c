#include "pwm.h"

#include "stm32l4xx.h"

void pwm_init(void) {
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	
	// Configure TIM2
	TIM2->CR1 = 0;
	TIM2->ARR = 8000;
	TIM2->PSC = 1000;
	TIM2->EGR = TIM_EGR_UG;
	TIM2->SMCR = 0;
	TIM2->CR2 = 0;
	TIM2->CCMR1 = TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
	TIM2->CCR2 = TIM2->ARR / 2; // 50%
	TIM2->CCER = 0;
	TIM2->CCMR1 |= TIM_CCMR1_OC2PE;
		
	// Configure TIM2 Channel2 (B3)
	//  * AFR:     configure alternate function 1
	//  * MODER:   configure as alternate function mode
	//  * OSPEEDR: configure a low speed
	GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL3;
	GPIOB->AFR[0] |= GPIO_AFRL_AFSEL3_0;
	GPIOB->MODER &= ~GPIO_MODER_MODE3;
	GPIOB->MODER |= GPIO_MODER_MODE3_1;
	GPIOB->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED3;
}

void pwm_start(void) {
	TIM2->CCER |= TIM_CCER_CC2E;
	TIM2->CR1 |= TIM_CR1_CEN;
}

void pwm_stop(void) {
	TIM2->CCER &= ~TIM_CCER_CC2E;
	TIM2->CR1 &= ~TIM_CR1_CEN;
}
