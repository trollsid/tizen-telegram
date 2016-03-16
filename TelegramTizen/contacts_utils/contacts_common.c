/*
    This file is part of Telegram application for tizen

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "contacts_common.h"
#include "logger.h"
#include <contacts.h>
#include <stdio.h>
#include <stdlib.h>
#include <Eina.h>

#define YEAR_BASE           1900
#define YEAR_FACTOR         10000
#define MONTH_BASE          1
#define MONTH_FACTOR        100

#define DATE_STRING_LENGTH  11

static const char * const _common_contact_fields_names[] =
{
	"Phone",
	"First Name",
	"Last Name",
	"Phone Number",
	"Email",
	"Url",
	"Company",
	"Birthday",
	"Note"
};

const char *sc_common_get_contact_field_name(sc_common_contact_fields field_type)
{
	RETVM_IF(CF_MAX <= field_type || CF_ADDRESSBOOK > field_type, NULL, "incorrect field type");
	return _common_contact_fields_names[field_type];
}

void sc_common_get_contact_properties(sc_common_contact_fields field_type, unsigned *parent_property_id, const char **child_uri, unsigned *child_property_id)
{
	unsigned temp_parent_property_id = 0;
	const char *temp_child_uri = NULL;
	unsigned temp_child_property_id = 0;

	switch(field_type) {
	case CF_ADDRESSBOOK:
		//Not needed
		break;
	case CF_FIRST_NAME:
		temp_parent_property_id = _contacts_contact.name;
		temp_child_uri = _contacts_name._uri;
		temp_child_property_id = _contacts_name.first;
		break;
	case CF_LAST_NAME:
		temp_parent_property_id = _contacts_contact.name;
		temp_child_uri = _contacts_name._uri;
		temp_child_property_id = _contacts_name.last;
		break;
	case CF_PHONE_NUMBER:
		temp_parent_property_id = _contacts_contact.number;
		temp_child_uri = _contacts_number._uri;
		temp_child_property_id = _contacts_number.number;
		break;
	case CF_EMAIL:
		temp_parent_property_id = _contacts_contact.email;
		temp_child_uri = _contacts_email._uri;
		temp_child_property_id = _contacts_email.email;
		break;
	case CF_URL:
		temp_parent_property_id = _contacts_contact.url;
		temp_child_uri = _contacts_url._uri;
		temp_child_property_id = _contacts_url.url;
		break;
	case CF_COMPANY:
		temp_parent_property_id = _contacts_contact.company;
		temp_child_uri = _contacts_company._uri;
		temp_child_property_id = _contacts_company.name;
		break;
	case CF_BIRTHDAY:
		temp_parent_property_id = _contacts_contact.event;
		temp_child_uri = _contacts_event._uri;
		temp_child_property_id = _contacts_event.date;
		break;
	case CF_NOTE:
		temp_parent_property_id = _contacts_contact.note;
		temp_child_uri = _contacts_note._uri;
		temp_child_property_id = _contacts_note.note;
		break;
	case CF_MAX:
		//Not correct field type
		break;
	}

	if(parent_property_id) {
		*parent_property_id = temp_parent_property_id;
	}

	if(child_uri) {
		*child_uri = temp_child_uri;
	}

	if(child_property_id) {
		*child_property_id = temp_child_property_id;
	}
}

int sc_common_date_to_int(const struct tm *date)
{
	if(!date)
	{
		time_t current_time = time(NULL);
		date = localtime(&current_time);
	}
	return (date->tm_year + YEAR_BASE) * YEAR_FACTOR
		+ (date->tm_mon + MONTH_BASE) * MONTH_FACTOR
		+  date->tm_mday;
}

struct tm sc_common_int_to_date(int date_value)
{
	struct tm date = {
		.tm_mday = date_value % MONTH_FACTOR,
		.tm_mon = (date_value % YEAR_FACTOR) / MONTH_FACTOR - 1,
		.tm_year = date_value / YEAR_FACTOR - 1900
	};
	return date;
}

char *sc_common_date_to_str(struct tm *date)
{
	char *ret_str = malloc(DATE_STRING_LENGTH);
	if (ret_str) {
		snprintf(ret_str, DATE_STRING_LENGTH, "%d/%d/%d", date->tm_year + 1900, date->tm_mon + 1, date->tm_mday);
	}
	return ret_str;
}

char *sc_common_trim_spaces_in_string(const char *str)
{
	Eina_Strbuf *strbuff = eina_strbuf_new();
	RETVM_IF(!strbuff, NULL, "strbuff is not created");
	eina_strbuf_append(strbuff, str);
	eina_strbuf_trim(strbuff);
	char *ret_str = strdup(eina_strbuf_string_get(strbuff));
	eina_strbuf_free(strbuff);
	return ret_str;
}
