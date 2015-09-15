/*
 * tg_common.h
 *
 *  Created on: May 19, 2015
 *      Author: sandeep
 */

#ifndef TG_COMMON_H_
#define TG_COMMON_H_


#include <Elementary.h>

#define DEFAULT_TELEGRAM_PATH "/opt/usr/media/telegram/"
#define DEFAULT_TG_DATABASE_PATH "/opt/usr/media/telegram/tg_data_base.db"

typedef enum MESSAGE_STATE {
	TG_MESSAGE_STATE_SENDING = 1,
	TG_MESSAGE_STATE_SENT,
	TG_MESSAGE_STATE_DELIVERED,
	TG_MESSAGE_STATE_FAILED,
	TG_MESSAGE_STATE_RECEIVED,
	TG_MESSAGE_STATE_READ,
	TG_MESSAGE_STATE_UNKNOWN
} TG_MESSAGE_STATE;

extern const char *tg_common_to_string(const char *fmt, ...);

//static char* get_table_name_from_number(const char* phone_no)
static inline char *get_table_name_from_number(const int id)
{
	char *msg_table;

	msg_table = (char *)malloc(32);
	if (!msg_table) {
		return NULL;
	}

	snprintf(msg_table, 32, "tg_%d_msg", id);

	return msg_table;
}

static inline char *replace(const char *s, char ch, const char *repl)
{
	int count = 0;
	const char *t;

	for(t = s; *t; t++) {
		count += (*t == ch);
	}

	size_t rlen = strlen(repl);
	char *res = malloc(strlen(s) + (rlen - 1) * count + 1);
	char *ptr = res;

	for(t = s; *t; t++) {
		if(*t == ch) {
			memcpy(ptr, repl, rlen);
			ptr += rlen;
		} else {
			*ptr++ = *t;
		}
	}

	*ptr = 0;
	return res;
}

#endif /* TG_COMMON_H_ */
