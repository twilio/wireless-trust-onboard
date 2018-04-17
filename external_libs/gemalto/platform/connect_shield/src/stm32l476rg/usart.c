#include <stdlib.h>

#include "usart.h"

#include "stm32l4xx.h"

#define RCC_AHB_PRESCALER  1
#define RCC_APB1_PRESCALER 1
#define RCC_APB2_PRESCALER 1

typedef struct {
	// Hardware specific
	USART_TypeDef* hw;
	// Configuration
	uint32_t baudrate;
	// Buffers
	struct {
		uint16_t head;
		uint16_t tail;
		uint16_t size;
		uint8_t* data;
	} rx;
	struct {
		uint16_t head;
		uint16_t tail;
		uint16_t size;
		uint8_t* data;
	} tx;
} usart_t;

static usart_t m_usart[] = {
	{ .hw = 0 },	
	{ .hw = USART1 },
	{ .hw = USART2 },
	{ .hw = USART3 },
};

// USART3 is used for debug
#define USART_DBG 3

#define USART_IRQHandler(usart) {\
	/* Read data register is not empty */\
	if(usart.hw->ISR & USART_ISR_RXNE) {\
		uint16_t head;\
		head = (usart.rx.head + 1) % usart.rx.size;\
		if(head != usart.rx.tail) {\
			usart.rx.data[usart.rx.head] = usart.hw->RDR;\
			usart.rx.head = head;\
		}\
		else {\
			/* Buffer overflow */\
		}\
	}\
	\
	/* Transmit data register empty */\
	if((usart.hw->ISR & USART_ISR_TXE) && (usart.hw->CR1 & USART_CR1_TXEIE)) {\
		if(usart.tx.tail != usart.tx.head) {\
			usart.hw->TDR = usart.tx.data[usart.tx.tail];\
			usart.tx.tail = (usart.tx.tail + 1) % usart.tx.size;\
		}\
		else {\
			/* Nothing to send, disable TXE interrupt */\
			usart.hw->CR1 &= ~USART_CR1_TXEIE;\
		}\
	}\
}

void USART1_IRQHandler(void) {
	USART_IRQHandler(m_usart[1]);
}

void USART2_IRQHandler(void) {
	USART_IRQHandler(m_usart[2]);
}

void USART3_IRQHandler(void) {
	USART_IRQHandler(m_usart[3]);
}

static usart_t* usart_get_by_handle(uint8_t handle) {
	if(handle && (handle <= (sizeof(m_usart) / sizeof(usart_t)))) {	
		return &m_usart[handle];
	}
	return 0;
}

void usart_open(uint8_t handle) {
	usart_t* usart;
	
	usart = usart_get_by_handle(handle);
	if(!usart) {
		return;
	}
	
	usart->rx.head = 0;
	usart->rx.tail = 0;
	usart->rx.data = (uint8_t*) malloc(usart->rx.size * sizeof(uint8_t));
	
	usart->tx.head = 0;
	usart->tx.tail = 0;
	usart->tx.data = (uint8_t*) malloc(usart->tx.size * sizeof(uint8_t));
	
	usart->hw->CR1 |= USART_CR1_UE;
}

int32_t usart_read(uint8_t handle, uint8_t* data, uint16_t len) {
	uint16_t i;
	usart_t* usart;
	
	usart = usart_get_by_handle(handle);
	if(!usart) {
		return -1;
	}
	
	if(!(usart->hw->CR1 & USART_CR1_UE)) {
		return -1;
	}
	
	for(i=0; i<len; i++) {
		if(usart->rx.tail == usart->rx.head) {
			break;
		}

		data[i] = usart->rx.data[usart->rx.tail];
		usart->rx.tail = (usart->rx.tail + 1) % usart->rx.size;
	}
	
	#ifdef USART_DBG
	if(i && (handle != USART_DBG)) {
		usart_write(USART_DBG, data, i);
	}
	#endif
	
	return ((int32_t) i);
}

int32_t usart_write(uint8_t handle, uint8_t* data, uint16_t len) {
	usart_t* usart;
	uint16_t i, head;
	
	usart = usart_get_by_handle(handle);
	if(!usart) {
		return -1;
	}
	
	if(!(usart->hw->CR1 & USART_CR1_UE)) {
		return -1;
	}
	
	for(i=0; i<len; i++) {
		head = (usart->tx.head + 1) % usart->tx.size;
		
		if(head == usart->tx.tail) {
			break;
		}
		
		usart->tx.data[usart->tx.head] = data[i];
		usart->tx.head = head;		
	}
	
	if(i) {
		// Something to send, enable TXE interrupt
		usart->hw->CR1 |= USART_CR1_TXEIE;
	}
	
	#ifdef USART_DBG
	if(i && (handle != USART_DBG)) {
		usart_write(USART_DBG, data, i);
	}
	#endif
	
	return ((int32_t) i);
}

void usart_flush(uint8_t handle) {
	usart_t* usart;
	
	usart = usart_get_by_handle(handle);
	if(!usart) {
		return;
	}
	
	do {
		
	} while(usart->tx.head != usart->tx.tail);
}

void usart_close(uint8_t handle) {
	usart_t* usart;
	
	usart = usart_get_by_handle(handle);
	if(!usart) {
		return;
	}
	
	usart->rx.head = 0;
	usart->rx.tail = 0;
	free((void*) usart->rx.data);
	
	usart->tx.head = 0;
	usart->tx.tail = 0;
	free((void*) usart->tx.data);
	
	usart->hw->CR1 &= ~USART_CR1_UE;
}

void usart_init(uint8_t handle, uint32_t baudrate, uint16_t rx_size, uint16_t tx_size) {
	uint32_t clock;
	usart_t* usart;
	
	usart = usart_get_by_handle(handle);
	if(!usart) {
		return;
	}
	
	usart->baudrate = baudrate;
	usart->rx.size = rx_size;
	usart->tx.size = tx_size;
		
	if(usart->hw == USART1) {
		// Configure RCC for GPIOB and USART1
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
		RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
		
		// Configure GPIO RX (A10)
		//  * AFR:     configure alternate function 7
		//  * MODER:   configure as alternate function mode
		//  * OSPEEDR: configure a very high speed
		GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL10;
		GPIOA->AFR[1] |= (GPIO_AFRH_AFSEL10_2 | GPIO_AFRH_AFSEL10_1 | GPIO_AFRH_AFSEL10_0);
		GPIOA->MODER &= ~GPIO_MODER_MODE10;
		GPIOA->MODER |= GPIO_MODER_MODE10_1;
		GPIOA->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED10;
		GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED10_1 | GPIO_OSPEEDR_OSPEED10_0);
		
		// Configure GPIO TX (A9)
		//  * AFR:     configure alternate function 7
		//  * MODER:   configure as alternate function mode
		//  * OSPEEDR: configure a very high speed
		GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL9;
		GPIOA->AFR[1] |= (GPIO_AFRH_AFSEL9_2 | GPIO_AFRH_AFSEL9_1 | GPIO_AFRH_AFSEL9_0);
		GPIOA->MODER &= ~GPIO_MODER_MODE9;
		GPIOA->MODER |= GPIO_MODER_MODE9_1;
		GPIOA->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED9;
		GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED9_1 | GPIO_OSPEEDR_OSPEED9_0);
		
		// Configure USART1
		clock  = SystemCoreClock;
		clock /= RCC_AHB_PRESCALER;
		clock /= RCC_APB2_PRESCALER;
		
		USART1->BRR = (clock / usart->baudrate);
		USART1->CR3 = 0;
		USART1->CR2 = 0;
		USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
		NVIC->ISER[USART1_IRQn >> 5] = (uint32_t) (1 << (USART1_IRQn & (uint8_t) 0x1F));
	}
		
	else if(usart->hw == USART2) {
		// Configure RCC for GPIOA and USART2
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
		RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
		
		// Configure GPIO RX (A3)
		//  * AFR:     configure alternate function 7
		//  * MODER:   configure as alternate function mode
		//  * OSPEEDR: configure a very high speed
		GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL3;
		GPIOA->AFR[0] |= (GPIO_AFRL_AFSEL3_2 | GPIO_AFRL_AFSEL3_1 | GPIO_AFRL_AFSEL3_0);
		GPIOA->MODER &= ~GPIO_MODER_MODE3;
		GPIOA->MODER |= GPIO_MODER_MODE3_1;
		GPIOA->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED3;
		GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED3_1 | GPIO_OSPEEDR_OSPEED3_0);
		
		// Configure GPIO TX (A2)
		//  * AFR:     configure alternate function 7
		//  * MODER:   configure as alternate function mode
		//  * OSPEEDR: configure a very high speed
		GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2;
		GPIOA->AFR[0] |= (GPIO_AFRL_AFSEL2_2 | GPIO_AFRL_AFSEL2_1 | GPIO_AFRL_AFSEL2_0);
		GPIOA->MODER &= ~GPIO_MODER_MODE2;
		GPIOA->MODER |= GPIO_MODER_MODE2_1;
		GPIOA->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED2;
		GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED2_1 | GPIO_OSPEEDR_OSPEED2_0);
		
		// Configure USART2
		clock  = SystemCoreClock;
		clock /= RCC_AHB_PRESCALER;
		clock /= RCC_APB1_PRESCALER;
		
		USART2->BRR = (clock / usart->baudrate);
		USART2->CR3 = 0;
		USART2->CR2 = 0;
		USART2->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
		NVIC->ISER[USART2_IRQn >> 5] = (uint32_t) (1 << (USART2_IRQn & (uint8_t) 0x1F));
	}
		
	else if(usart->hw == USART3) {
		// Configure RCC for GPIOC and USART3
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
		RCC->APB1ENR1 |= RCC_APB1ENR1_USART3EN;
		
		// Configure GPIO RX (C11)
		//  * AFR:     configure alternate function 7
		//  * MODER:   configure as alternate function mode
		//  * OSPEEDR: configure a very high speed
		GPIOC->AFR[1] &= ~GPIO_AFRH_AFSEL11;
		GPIOC->AFR[1] |= (GPIO_AFRH_AFSEL11_2 | GPIO_AFRH_AFSEL11_1 | GPIO_AFRH_AFSEL11_0);
		GPIOC->MODER &= ~GPIO_MODER_MODE11;
		GPIOC->MODER |= GPIO_MODER_MODE11_1;
		GPIOC->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED11;
		GPIOC->OSPEEDR |= (GPIO_OSPEEDR_OSPEED11_1 | GPIO_OSPEEDR_OSPEED11_0);
		
		// Configure GPIO TX (C10)
		//  * AFR:     configure alternate function 7
		//  * MODER:   configure as alternate function mode
		//  * OSPEEDR: configure a very high speed
		GPIOC->AFR[1] &= ~GPIO_AFRH_AFSEL10;
		GPIOC->AFR[1] |= (GPIO_AFRH_AFSEL10_2 | GPIO_AFRH_AFSEL10_1 | GPIO_AFRH_AFSEL10_0);
		GPIOC->MODER &= ~GPIO_MODER_MODE10;
		GPIOC->MODER |= GPIO_MODER_MODE10_1;
		GPIOC->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED10;
		GPIOC->OSPEEDR |= (GPIO_OSPEEDR_OSPEED10_1 | GPIO_OSPEEDR_OSPEED10_0);
		
		// Configure USART2
		clock  = SystemCoreClock;
		clock /= RCC_AHB_PRESCALER;
		clock /= RCC_APB1_PRESCALER;
		
		USART3->BRR = (clock / usart->baudrate);
		USART3->CR3 = 0;
		USART3->CR2 = 0;
		USART3->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
		NVIC->ISER[USART3_IRQn >> 5] = (uint32_t) (1 << (USART3_IRQn & (uint8_t) 0x1F));
	}
}

void usart_change_baudrate(uint8_t handle, uint32_t baudrate) {
	uint32_t clock;
	usart_t* usart;
	
	usart = usart_get_by_handle(handle);
	if(!usart) {
		return;
	}
	
	if(usart->baudrate != baudrate) {
		usart->baudrate = baudrate;
		usart->hw->CR1 &= ~USART_CR1_UE;
	
		if(usart->hw == USART1) {
			clock  = SystemCoreClock;
			clock /= RCC_AHB_PRESCALER;
			clock /= RCC_APB2_PRESCALER;			
			USART1->BRR = (clock / usart->baudrate);
		}
		
		else if(usart->hw == USART2) {
			clock  = SystemCoreClock;
			clock /= RCC_AHB_PRESCALER;
			clock /= RCC_APB1_PRESCALER;
			USART2->BRR = (clock / usart->baudrate);
		}
		
		else if(usart->hw == USART3) {
			clock  = SystemCoreClock;
			clock /= RCC_AHB_PRESCALER;
			clock /= RCC_APB1_PRESCALER;
			USART3->BRR = (clock / usart->baudrate);
		}
		
		usart->hw->CR1 |= USART_CR1_UE;
	}
}
