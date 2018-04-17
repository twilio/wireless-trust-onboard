#ifndef __USART_H__
#define __USART_H__

#include <stdint.h>

void usart_init(uint8_t handle, uint32_t baudrate, uint16_t rx_size, uint16_t tx_size);
void usart_change_baudrate(uint8_t handle, uint32_t baudrate);

void usart_open(uint8_t handle);
int32_t usart_read(uint8_t handle, uint8_t* data, uint16_t len);
int32_t usart_write(uint8_t handle, uint8_t* data, uint16_t len);
void usart_flush(uint8_t handle);
void usart_close(uint8_t handle);

#endif /* __USART_H__ */
