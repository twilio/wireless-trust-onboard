#ifndef _AT_PARSER_H_
#define _AT_PARSER_H_

#include "types.h"

#define AT_GENERIC_EVENT_MASK  0x80
#define AT_SPECIFIC_EVENT_MASK 0x40

#define AT_NO_EVENT                   (AT_GENERIC_EVENT_MASK | 0)
#define AT_IDLE_EVENT                 (AT_GENERIC_EVENT_MASK | 1)
#define AT_TIMEOUT_EVENT              (AT_GENERIC_EVENT_MASK | 2)
#define AT_COMMAND_SUCCESS_EVENT      (AT_GENERIC_EVENT_MASK | 3)
#define AT_COMMAND_FAILURE_EVENT      (AT_GENERIC_EVENT_MASK | 4)
#define AT_COMMAND_NOTIFICATION_EVENT (AT_GENERIC_EVENT_MASK | 5)

/*
function djb2(str) {
	var hash = 5381;

    for(var i = 0; i < str.length; i++) {	
			hash = ((hash << 5) + hash) + str.charCodeAt(i);        
    }

    return ('0x' + (new Uint32Array([hash]))[0].toString(16));
}
*/

typedef uint8_t at_event_t;

typedef struct {
	void *ctx;
	void (*fct)(void*, at_event_t);
} at_callback_t;

typedef struct {
	uint32_t hash;
	at_event_t evt;
	at_callback_t cb;
} at_map_entry_t;

typedef at_map_entry_t at_hash_map_t[];

typedef struct {
	uint8_t locked;
	uint8_t io;
	
	const at_map_entry_t* generic_table;
	uint8_t generic_table_size;
	
	const at_map_entry_t* specific_table;
	uint8_t specific_table_size;
	
	at_callback_t async_cb;
} at_t;

void at_init(at_t* at, uint8_t io, const at_map_entry_t* generic_table, uint8_t generic_table_size);
void at_lock(at_t* at);

at_event_t at_process_recv(at_t* at);
void at_wait_for_timeout(at_t* at);
at_event_t at_wait_for_notification(at_t* at);
at_event_t at_wait_for_response(at_t* at);
at_event_t at_wait_for_event(at_t* at, at_event_t evt);

#define at_set_specific(at, table) {\
	((at_t*) at)->specific_table = table;\
	((at_t*) at)->specific_table_size = sizeof(table) / sizeof(at_map_entry_t);\
}

#define at_set_async_cb(at, context, callback) {\
	((at_t*) at)->async_cb.ctx = context;\
	((at_t*) at)->async_cb.fct = callback;\
}

#define at_print(at, msg, ...) {\
	fprintf(&((FILE) { .handle = ((at_t*) at)->io}), msg, ##__VA_ARGS__);\
}
uint16_t at_write(at_t* at, uint8_t* data, uint16_t len);
uint16_t at_read(at_t* at, uint8_t* data, uint16_t len);

#endif /* _AT_PARSER_H_ */
