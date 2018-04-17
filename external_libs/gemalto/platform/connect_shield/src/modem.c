#include "at_parser.h"
#include "delay.h"
#include "modem.h"
#include "pwm.h"
#include "usart.h"

#define MODEM_SYSLOADING_EVENT         (AT_SPECIFIC_EVENT_MASK | 0)
#define MODEM_SYSSTART_EVENT           (AT_SPECIFIC_EVENT_MASK | 1)
#define MODEM_CME_ERROR_EVENT          (AT_SPECIFIC_EVENT_MASK | 2)
#define MODEM_CPIN_READY_EVENT         (AT_SPECIFIC_EVENT_MASK | 3)
#define MODEM_CPIN_SIM_PIN_EVENT       (AT_SPECIFIC_EVENT_MASK | 4)
#define MODEM_CREG_EVENT               (AT_SPECIFIC_EVENT_MASK | 5)
#define MODEM_SISI_1_EVENT             (AT_SPECIFIC_EVENT_MASK | 6)
#define MODEM_SISR_1_EVENT             (AT_SPECIFIC_EVENT_MASK | 7)
#define MODEM_SISW_1_EVENT             (AT_SPECIFIC_EVENT_MASK | 8)
#define MODEM_CSIM_EVENT               (AT_SPECIFIC_EVENT_MASK | 9)

#define MODEM_MAX_PACKET_SIZE 1500

static at_t m_at;

static const at_hash_map_t m_modem_generic_table = {	
	{ .hash = 0x7c89a236, .evt = AT_COMMAND_SUCCESS_EVENT }, // OK\r\n
	{ .hash = 0x88718ac6, .evt = AT_COMMAND_FAILURE_EVENT }, // ERROR\r\n
	
	{ .hash = 0xca2f91d7, .evt = MODEM_SYSLOADING_EVENT, }, // ^SYSLOADING\r\n
	{ .hash = 0x09565007, .evt = MODEM_SYSSTART_EVENT }, // ^SYSSTART\r\n
	
	{ .hash = 0x5e224a29, .evt = AT_COMMAND_FAILURE_EVENT /*MODEM_CME_ERROR_EVENT*/ }, // +CME ERROR:
	
	{ .hash = 0x8f0ad8e0, .evt = MODEM_CPIN_READY_EVENT }, // +CPIN: READY\r\n
	{ .hash = 0xe9e8a4fb, .evt = MODEM_CPIN_SIM_PIN_EVENT }, // +CPIN: SIM PIN\r\n
	
	{ .hash = 0x7117678b, .evt = MODEM_CREG_EVENT }, // +CREG:

	{ .hash = 0x0b66e692, .evt = MODEM_SISI_1_EVENT }, // ^SISI: 1,
	{ .hash = 0x0c09c31b, .evt = MODEM_SISR_1_EVENT }, // ^SISR: 1,
	{ .hash = 0x0c643da0, .evt = MODEM_SISW_1_EVENT }, // ^SISW: 1,
	
	{ .hash = 0x711805b6, .evt = MODEM_CSIM_EVENT }, // +CSIM:
};

/*
#define modem_send_simple_at_command(cmd) ({\
	at_lock(&m_at);\
	at_print(&m_at, "%s", cmd);\
	at_wait_for_response(&m_at);\
})

#define modem_send_simple_at_command_no_wait_response(cmd) {\
	at_lock(&m_at);\
	at_print(&m_at, "%s", cmd);\
}
*/

#define modem_send_at_command(cmd, ...) {\
	at_lock(&m_at);\
	at_print(&m_at, cmd, ##__VA_ARGS__);\
	at_wait_for_response(&m_at);\
}

#define modem_send_at_command_no_wait_response(cmd, ...) {\
	at_lock(&m_at);\
	at_print(&m_at, cmd, ##__VA_ARGS__);\
}

static bool modem_send_query_at_sisi(uint8_t* srv_state) {
	uint8_t c;
	at_event_t evt;
	
	*srv_state = 0;

	modem_send_at_command_no_wait_response("AT^SISI?\r\n");
	
	do {
		evt = at_process_recv(&m_at);
		if(evt == MODEM_SISI_1_EVENT) {
			at_read(&m_at, &c, 1);
			while((c >= '0') && (c <= '9')) {
				*srv_state *= 10;
				*srv_state += c - '0';
				at_read(&m_at, &c, 1);
			}
			while(c != '\n') {
				at_read(&m_at, &c, 1);
			}
		}
	} while((evt != AT_COMMAND_SUCCESS_EVENT) && (evt != AT_COMMAND_FAILURE_EVENT));

	return (evt ? true : false);
}

void modem_init(uint8_t io) {
	static uint8_t first = 1;
	
	at_event_t evt;
	
	if(!first) {
		return;
	}
	first = 0;
	
	at_init(&m_at, io, m_modem_generic_table, sizeof(m_modem_generic_table) / sizeof(at_map_entry_t));
	pwm_init();
	pwm_start();
	
	while(at_process_recv(&m_at) != MODEM_SYSSTART_EVENT);
	
	modem_send_at_command("AT\r\n");
	modem_send_at_command("ATE0\r\n");
	modem_send_at_command("AT^SPOW=1,0,0\r\n");
	modem_send_at_command("AT+CMEE=2\r\n");
	
	modem_send_at_command_no_wait_response("AT+CPIN?\r\n");	
	do {
		evt = at_process_recv(&m_at);
		if(evt == MODEM_CPIN_SIM_PIN_EVENT) {
			if(at_wait_for_response(&m_at) == AT_COMMAND_SUCCESS_EVENT) {
				modem_send_at_command("AT+CPIN=\"%s\"\r\n", (char*) "1111");
				evt = AT_COMMAND_SUCCESS_EVENT;
			}
		}
		else if(evt == MODEM_CPIN_READY_EVENT) {
			evt = at_wait_for_response(&m_at);
		}
	} while((evt != AT_COMMAND_SUCCESS_EVENT) && (evt != AT_COMMAND_FAILURE_EVENT));
		
	modem_send_at_command("AT^SCFG=\"Tcp/WithURCs\",\"off\"\r\n");
	modem_send_at_command("AT^SICS=1,\"conType\",\"GPRS0\"\r\n");
	modem_send_at_command("AT^SICS=1,\"alphabet\",\"1\"\r\n");
	modem_send_at_command("AT^SICS=1,\"APN\",\"internet\"\r\n");
}

bool modem_bring_up(void) {
	at_event_t evt;
	uint8_t c, mode, status;

	do {
		mode = 0;
		status = 0;
		modem_send_at_command_no_wait_response("AT+CREG?\r\n");
		do {
			evt = at_process_recv(&m_at);
			if(evt == MODEM_CREG_EVENT) {
				// Find mode
				while(!((c >= '0') && (c <= '9'))) {
					at_read(&m_at, &c, 1);
					if(c == '\n') {
						return false;
					}
				}
				
				// Extract mode
				while((c >= '0') && (c <= '9')) {
					mode *= 10;
					mode += c - '0';
					at_read(&m_at, &c, 1);
					if(c == '\n') {
						return false;
					}
				}
				
				// Find status
				while(!((c >= '0') && (c <= '9'))) {
					at_read(&m_at, &c, 1);
					if(c == '\n') {
						return false;
					}
				}
				
				// Extract mode
				while((c >= '0') && (c <= '9')) {
					status *= 10;
					status += c - '0';
					at_read(&m_at, &c, 1);
					if(c == '\n') {
						return false;
					}
				}
				
				// Wait end of line
				while(c != '\n') {
					at_read(&m_at, &c, 1);
				}

			}
		} while((evt != AT_COMMAND_SUCCESS_EVENT) && (evt != AT_COMMAND_FAILURE_EVENT));
	} while((status != 1) && (status != 5));
	
	printf("Net interface up!");
	return true;
}

int  modem_open_tcp_socket(const char* address, const uint16_t port) {
	int handle = 1;
	uint8_t srv_state;
	
	modem_send_at_command("AT^SISS=%d,\"srvtype\",\"socket\"\r\n", 1);
	modem_send_at_command("AT^SISS=%d,\"address\",\"socktcp://%s:%d\"\r\n", 1, address, port);
	modem_send_at_command("AT^SISS=%d,\"conId\",\"%d\"\r\n", 1, handle);
	modem_send_at_command("AT^SISO=%d\r\n", handle);
	
	do {
		modem_send_query_at_sisi(&srv_state);
		if((srv_state == 5) || (srv_state == 6)) {
			// Socket closed
			return -1;
		}
	} while(srv_state != 4);
	
	return handle;
}

int  modem_read_socket(int handle, uint8_t* data, uint16_t len) {
	int32_t read;
	at_event_t evt;
	uint16_t i, toread;
	uint8_t c, srv_state;
	
	if(handle != 1) {
		return -1;
	}
	
	read = 0;
	
	for(i=0; i<len;) {
		modem_send_query_at_sisi(&srv_state);
		if((srv_state == 5) || (srv_state == 6)) {
			// Socket closed
			return -1;
		}
		
		toread = len - i;
		if(toread > MODEM_MAX_PACKET_SIZE) {
			toread = MODEM_MAX_PACKET_SIZE;
		}
				
		modem_send_at_command_no_wait_response("AT^SISR=%d,%d\r\n", handle, toread);
		
		do {
			evt = at_process_recv(&m_at);
			if((evt == MODEM_CME_ERROR_EVENT) || (evt == AT_COMMAND_FAILURE_EVENT)) {
				return -1;
			}
		} while(((handle == 1) && (evt != MODEM_SISR_1_EVENT)));
		
		toread = 0;
		at_read(&m_at, &c, 1);
		while((c >= '0') && (c <= '9')) {
			toread *= 10;
			toread += c - '0';
			at_read(&m_at, &c, 1);
		}
		while(c != '\n') {
			at_read(&m_at, &c, 1);
		}

		if(toread) {
			at_read(&m_at, &data[i], toread);	
		}
		
		evt = at_wait_for_response(&m_at);
		
		if(evt == AT_COMMAND_FAILURE_EVENT) {
			return -1;
		}
		
		i += toread;
		read += toread;
	}
	
	return read;
}

int modem_write_socket(int handle, uint8_t* data, uint16_t len) {
	at_event_t evt;
	int32_t written;
	uint16_t i, towrite;
	uint8_t c, srv_state;
	
	written = 0;
	
	for(i=0; i<len;) {
		modem_send_query_at_sisi(&srv_state);
		if((srv_state == 5) || (srv_state == 6)) {
			// Socket closed
			return -1;
		}
		
		towrite = len - i;
		if(towrite > MODEM_MAX_PACKET_SIZE) {
			towrite = MODEM_MAX_PACKET_SIZE;
		}
		
		modem_send_at_command_no_wait_response("AT^SISW=%d,%d\r\n", handle, towrite);
		
		do {
			evt = at_process_recv(&m_at);
			if((evt == MODEM_CME_ERROR_EVENT) || (evt == AT_COMMAND_FAILURE_EVENT)) {
				return -1;
			}
		} while(((handle == 1) && (evt != MODEM_SISW_1_EVENT)));
		
		towrite = 0;
		at_read(&m_at, &c, 1);
		while((c >= '0') && (c <= '9')) {
			towrite *= 10;
			towrite += c - '0';
			at_read(&m_at, &c, 1);
		}
		while(c != '\n') {
			at_read(&m_at, &c, 1);
		}

		if(towrite) {
			at_write(&m_at, &data[i], towrite);	
		}
					
		evt = at_wait_for_response(&m_at);
		
		if(evt == AT_COMMAND_FAILURE_EVENT) {
			return -1;
		}
		
		i += towrite;
		written += towrite;
	}
	
	return written;
}

void modem_close_socket(int handle) {
	if(handle != 1) {
		return;
	}	
	modem_send_at_command("AT^SISC=%d\r\n", handle);
}

bool modem_send_apdu(uint8_t* apdu, uint16_t apdu_len, uint8_t* response, uint16_t* response_len) {
	uint16_t i;
	at_event_t evt;
	
	*response_len = 0;
	
	if(apdu_len > 256) {
		return false;
	}
	
	at_lock(&m_at);
	at_print(&m_at, "AT+CSIM=%d,\"", apdu_len * 2);	
	for(i=0; i<apdu_len; i++) {
		at_print(&m_at, "%02X", apdu[i]);
	}
	at_print(&m_at, "\"\r\n");
	
	do {
		evt = at_process_recv(&m_at);
		if(evt == MODEM_CSIM_EVENT) {
			uint8_t c = 0;

			// Find length
			while(!((c >= '0') && (c <= '9'))) {
				at_read(&m_at, &c, 1);
				if(c == '\n') {
					return false;
				}
			}
			
			// Extract length
			while((c >= '0') && (c <= '9')) {
				at_read(&m_at, &c, 1);
				if(c == '\n') {
					return false;
				}
			}
			
			// Find response
			while(!(((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F')) || ((c >= 'a') && (c <= 'f')))) {
				at_read(&m_at, &c, 1);
				if(c == '\n') {
					return false;
				}
			}
			
			// Extract response
			while(((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F')) || ((c >= 'a') && (c <= 'f'))) {
				*response = 0;
			
				if((c >= '0') && (c <= '9')) {
					*response = c - '0';
				}
				else if((c >= 'a') && (c <= 'f')) {
					*response = c - 'a' + 10;
				}
				else if((c >= 'A') && (c <= 'F')) {
					*response = c - 'A' + 10;
				}
				
				at_read(&m_at, &c, 1);
				if(c == '\n') {
					return false;
				}
				
				*response <<= 4;
				
				if((c >= '0') && (c <= '9')) {
					*response |= c - '0';
				}
				else if((c >= 'a') && (c <= 'f')) {
					*response |= c - 'a' + 10;
				}
				else if((c >= 'A') && (c <= 'F')) {
					*response |= c - 'A' + 10;
				}
				
				response++;
				*response_len += 1;
				
				at_read(&m_at, &c, 1);
				if(c == '\n') {
					return false;
				}
			}
			
			// Wait end of line
			while(c != '\n') {
				at_read(&m_at, &c, 1);
			}
		}
	} while((evt != AT_COMMAND_SUCCESS_EVENT) && (evt != AT_COMMAND_FAILURE_EVENT));

	return (evt ? true : false);
}
