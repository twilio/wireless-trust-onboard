#ifndef __MODEM_H__
#define __MODEM_H__

#include <stdbool.h>

#include "types.h"

void modem_init(uint8_t io);

bool modem_bring_up(void);
bool modem_bring_down(void);

int  modem_open_tcp_socket(const char* address, const uint16_t port);
int  modem_read_socket(int handle, uint8_t* data, uint16_t len);
int  modem_write_socket(int handle, uint8_t* data, uint16_t len);
void modem_close_socket(int handle);

bool modem_send_apdu(uint8_t* apdu, uint16_t apdu_len, uint8_t* response, uint16_t* response_len);

#endif /* __MODEM_H__ */
