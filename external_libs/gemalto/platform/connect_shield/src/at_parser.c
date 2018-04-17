#include "at_parser.h"
#include "delay.h"
#include "usart.h"

#define AT_NULL_ENTRY 0

#define at_read_c(at, c) usart_read(((at_t*) at)->io, c, 1)

void at_init(at_t* at, uint8_t io, const at_map_entry_t* generic_table, uint8_t generic_table_size) {
	at->io = io;
	at->locked = 0;
	at->generic_table = generic_table;
	at->generic_table_size = generic_table_size;
	at->specific_table = 0;
	at->specific_table_size = 0;
}

void at_lock(at_t* at) {
	while(at->locked) {
		at_process_recv(at);
	}
	while(at_process_recv(at) != AT_TIMEOUT_EVENT);
	at->locked = 1;
}

static void at_unlock(at_t* at) {
	at->specific_table = 0;
	at->specific_table_size = 0;
	at->locked = 0;
}

static const at_map_entry_t* at_hash_map_contains(uint32_t hash, at_t* at) {
	uint8_t i;

	// ToDo: be sure table is sorted by hash and perform a binary search to check for a match
	
	for(i = 0; i < at->generic_table_size; i++) {
		if(hash == at->generic_table[i].hash) {
			return &at->generic_table[i];
		}
	}
	
	for(i = 0; i < at->specific_table_size; i++) {
		if(hash == at->specific_table[i].hash) {
			return &at->specific_table[i];
		}
	}

	return AT_NULL_ENTRY;
}

at_event_t at_process_recv(at_t* at) {
	uint8_t c;
	uint16_t read;
	uint32_t hash; // Hash algorithm: DJB2
	at_event_t evt;

	read = 0;
	hash = 5381;
	evt = AT_TIMEOUT_EVENT;
	
	do {
		if(at_read_c(at, &c) == 1) {
			read++;
			if((read == 1) && ((c == ' ') || (c == '\r') || (c == '\n'))) {
				read = 0;
				continue;
			}
			
			hash = ((hash << 5) + hash) + c;

			if(((c == '\n') || (c == '>') || (c == ',') || (c == ':'))) {
				const at_map_entry_t* entry;

				if((entry = at_hash_map_contains(hash, at)) != AT_NULL_ENTRY) {				
					if((entry->evt == AT_COMMAND_SUCCESS_EVENT) || (entry->evt == AT_COMMAND_FAILURE_EVENT)) {
						// We are supposed to wait at least 100ms between sending two commands
						delay_ms(100);
						at_unlock(at);
					}
										
					if(entry->cb.fct) {
						entry->cb.fct(entry->cb.ctx, entry->evt);
					}
					
					if(at->async_cb.fct) {
						if((entry->evt == AT_COMMAND_SUCCESS_EVENT) || (entry->evt == AT_COMMAND_FAILURE_EVENT)) {
							at->async_cb.fct(at->async_cb.ctx, entry->evt);
							at->async_cb.fct = 0;
							at->async_cb.ctx = 0;
						}
						else if(entry->evt == AT_COMMAND_NOTIFICATION_EVENT) {
							at->async_cb.fct(at->async_cb.ctx, entry->evt);
						}
					}
					
					evt = entry->evt;
					break;
				}
			}
		}
	} while(read && (c != '\n'));
	
	return evt;
}

void at_wait_for_timeout(at_t* at) {
	at_event_t event;
	
	do {
		event = at_process_recv(at);
	} while(event != AT_TIMEOUT_EVENT);
}

at_event_t at_wait_for_notification(at_t* at) {
	at_event_t event;
	
	do {
		event = at_process_recv(at);
	} while(event != AT_COMMAND_NOTIFICATION_EVENT);
	
	return event;
}

at_event_t at_wait_for_response(at_t* at) {
	at_event_t event;
	
	do {
		event = at_process_recv(at);
	} while((event != AT_COMMAND_SUCCESS_EVENT) && (event != AT_COMMAND_FAILURE_EVENT));
	
	return event;
}

at_event_t at_wait_for_event(at_t* at, at_event_t evt) {
	at_event_t event;
	
	do {
		event = at_process_recv(at);
	} while(event != evt);
	
	return event;
}

uint16_t at_write(at_t* at, uint8_t* data, uint16_t len) {
	uint16_t i;
	int32_t written;	

	for(i=0; i<len;) {
		written = usart_write(at->io, &data[i], len - i);
		if(written == -1) {
			break;
		}
		if(written > 0) {
			i += written;
		}
	}
	
	return len;
}

uint16_t at_read(at_t* at, uint8_t* data, uint16_t len) {
	uint16_t i;
	int32_t read;	

	for(i=0; i<len;) {	
		read = usart_read(at->io, &data[i], len - i);
		if(read == -1) {
			break;
		}
		if(read > 0) {
			i += read;
		}
	}
	
	return len;
}
