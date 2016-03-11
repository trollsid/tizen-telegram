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

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "telegramtizen.h"
#include "tg_splash_screen.h"
#include "tg_registration.h"
#include "tg_db_wrapper.h"
#include "tg_login.h"
#include "tg_first_registration.h"
#include "server_requests.h"
#include "tg_init_screen.h"
#include "tg_user_main_view.h"
#include "tg_messaging_view.h"
#include "tg_start_messaging_view.h"
#include "tg_user_info_view.h"
#include "tg_chat_info_view.h"
#include "ucol.h"
#include <notification.h>
#include <badge.h>
#include "tg_settings_view.h"
#include "device_contacts_manager.h"
#include "tg_search_peer_view.h"
#include <sys/stat.h>

static void free_app_data(appdata_s *app_data, Eina_Bool destroy_server);
static int init_service(appdata_s *app);
static void popup_block_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	evas_object_del(obj);
}

static void popup_timeout_cb(void *data, Evas_Object *obj, void *event_info)
{
	evas_object_del(obj);
}

void show_toast(appdata_s *ad, char* value)
{
	Evas_Object *popup;
	Evas_Object *win = ad->win;
	popup = elm_popup_add(win);
	elm_object_style_set(popup, "toast");
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);

	char buf[512] = {'\0'};
	snprintf(buf, sizeof(buf), "<font_size=30><color=#000000>%s</color></font_size>", value);

	//elm_object_part_text_set(popup,"elm.text", buf);
	elm_object_text_set(popup, buf);
	evas_object_smart_callback_add(popup, "block,clicked", popup_block_clicked_cb, win);
	elm_popup_timeout_set(popup, 2.0);
	evas_object_smart_callback_add(popup, "timeout", popup_timeout_cb, NULL);
	evas_object_show(popup);
}

static int _app_send_response(appdata_s *app, bundle *const msg)
{
	int res = SVC_RES_FAIL;
	res = service_client_send_message(app->service_client, msg);
	return res;
}


void load_list_of_countries(appdata_s *ad)
{
	if (!ad)
		return;

	int size_of_listed_countries = 0;
	const char *file_name = ui_utils_get_resource(TG_LIST_OF_COUNTIRES);
	if (!file_name)
		return;

	int fd;
	char *ptr;
	char *ptrToCode;
	char *ptrToShortName;
	char *ptrToFullName;
	enum parse_state {
		STATE_BEGIN,
		STATE_CODE,
		STATE_SHORT_NAME,
		STATE_FULL_NAME,
		STATE_ERROR
	} state;

	fd = open(file_name, O_RDONLY);
	if (fd > 0) {
		off_t file_size;

		file_size = lseek(fd, 0L, SEEK_END);
		if ((int)file_size == -1) {
			if (close(fd) < 0) {

			}
		} else {
			if (lseek(fd, 0L, SEEK_SET) < 0) {
				// TODO: Error
			}
			ad->country_code_buffer = malloc(file_size);
			if (!ad->country_code_buffer) {
				// TODO: Error
			}

			if (read(fd, ad->country_code_buffer, file_size) != file_size) {
				// TODO: Error
			}

			if (close(fd) < 0) {

			}
		}
	} else {
		// TODO: Error
	}

	ptr = ad->country_code_buffer;
	state = STATE_BEGIN;
	while (*ptr) {
		switch (state) {
		case STATE_BEGIN:
			ptrToCode = NULL;
			ptrToShortName = NULL;
			ptrToFullName = NULL;
			state = STATE_CODE;
		case STATE_CODE:
			if (ptrToCode == NULL) {
				if (isdigit(*ptr)) {
					ptrToCode = ptr;
				}
			} else if (*ptr == ';') {
				*ptr = '\0';
				state = STATE_SHORT_NAME;
			}
			ptr++;
			break;
		case STATE_SHORT_NAME:
			if (ptrToShortName == NULL) {
				ptrToShortName = ptr;
			} else if (*ptr == ';') {
				*ptr = '\0';
				state = STATE_FULL_NAME;
			}
			ptr++;
			break;
		case STATE_FULL_NAME:
			if (ptrToFullName == NULL) {
				ptrToFullName = ptr;
			} else if (*ptr == '\r' || *ptr == '\n') {
				*ptr = '\0';
				dlog_print(DLOG_DEBUG, LOG_TAG, "[%s] [%s] [%s]", ptrToCode, ptrToShortName, ptrToFullName);
				ad->country_codes_list = eina_list_append(ad->country_codes_list, ptrToCode);
				ad->country_names_list = eina_list_append(ad->country_names_list, ptrToFullName);
				state = STATE_BEGIN;
			}
			ptr++;
			break;
		case STATE_ERROR:
		default:
			break;
		}
	}
}

void load_registered_user_data(appdata_s *ad)
{
	Eina_List *user_info = get_registered_user_info();

	if (!user_info) {
		//error
		//show_toast(ad, "User info not loaded");
	}

	if (ad->current_user_data) {
		free_user_data(ad->current_user_data);
		ad->current_user_data = NULL;
	}

	ad->current_user_data = (user_data_s*)malloc(sizeof(user_data_s));


	Eina_List *row_vals = NULL;
	int pos = 0;
	EINA_LIST_FREE(user_info, row_vals) {
		if (!row_vals)
			continue;
		if (pos == 0) {
			int *temp_user_id = (int *)eina_list_nth(row_vals, 0);
			if (temp_user_id) {
				int user_id = *temp_user_id;
				free(temp_user_id);

				ad->user_id.id = user_id;
				ad->user_id.type = TGL_PEER_USER;

				ad->current_user_data->user_id.id = user_id;
				ad->current_user_data->user_id.type = TGL_PEER_USER;
			}

			ad->current_user_data->print_name = (char *)eina_list_nth(row_vals, 1);

			int *temp_struct_ver = (int *)eina_list_nth(row_vals, 2);
			if (temp_struct_ver) {
				ad->current_user_data->structure_version = *temp_struct_ver;
				free(temp_struct_ver);
			}

			ad->current_user_data->photo_path = (char *)eina_list_nth(row_vals, 3);

			int *temp_photo_id = (int *)eina_list_nth(row_vals, 4);
			if (temp_photo_id) {
				ad->current_user_data->photo_id = *temp_photo_id;
				free(temp_photo_id);
			}

			ad->current_user_data->first_name = (char *)eina_list_nth(row_vals, 5);

			ad->current_user_data->last_name = (char *)eina_list_nth(row_vals, 6);

			char *phone_no = (char *)eina_list_nth(row_vals, 7);
			ad->current_user_data->phone = phone_no;

			int *temp_access_hash = (int *)eina_list_nth(row_vals, 8);
			if (temp_access_hash) {
				ad->current_user_data->access_hash = *temp_access_hash;
				free(temp_access_hash);
			}

			ad->current_user_data->real_first_name = (char *)eina_list_nth(row_vals, 9);

			ad->current_user_data->real_last_name = (char *)eina_list_nth(row_vals, 10);

			char *user_name = (char *)eina_list_nth(row_vals, 11);
			ad->current_user_data->username = user_name;

			int *temp_online_status = (int *)eina_list_nth(row_vals, 12);
			if (temp_online_status) {
				ad->current_user_data->online = *temp_online_status;
				free(temp_online_status);
			}

			int *temp_last_seen = (int *)eina_list_nth(row_vals, 13);
			if (temp_last_seen) {
				ad->current_user_data->last_seen = *temp_last_seen;
				free(temp_last_seen);
			}
		} else {
			void* val = NULL;
			EINA_LIST_FREE(row_vals, val) {
				if (val)
					free(val);
			}
		}
		pos++;
	}
	eina_list_free(user_info);
}

void load_peer_data(appdata_s *ad)
{
	if (!ad) {
		return;
	}


	peer_with_pic_s* pic_item = NULL;
	EINA_LIST_FREE(ad->peer_list, pic_item) {
		if (!pic_item)
			continue;
		tg_peer_info_s* item = pic_item->use_data;
		if (item) {
			if (item->print_name) {
				free(item->print_name);
				item->print_name = NULL;
			}
			if (item->photo_path) {
				free(item->photo_path);
				item->photo_path = NULL;
			}
			pic_item->contact_icon = NULL;
			pic_item->msg_object = NULL;
			pic_item->name_object = NULL;
			free(item);
		}
		free(pic_item);
	}

	ad->peer_list = NULL;


	Eina_List *peer_details = get_all_peer_details(NULL);

	Eina_List *ts_msg = NULL;
	EINA_LIST_FREE(peer_details, ts_msg) {
		tg_peer_info_s* peer_info = (tg_peer_info_s*)malloc(sizeof(*peer_info));

		int *temp_peer_id = (int *)eina_list_nth(ts_msg, 0);
		if (temp_peer_id) {
			peer_info->peer_id  = *temp_peer_id;
			free(temp_peer_id);
		}

		int *temp_peer_type = (int *)eina_list_nth(ts_msg, 1);
		if (temp_peer_type) {
			peer_info->peer_type  = *temp_peer_type;
			free(temp_peer_type);
		}

		int *temp_flags = (int *)eina_list_nth(ts_msg, 2);
		if (temp_flags) {
			peer_info->flags = *temp_flags;
			free(temp_flags);
		}

		char *temp_msg_id = (char *)eina_list_nth(ts_msg, 3);
		if (temp_msg_id) {
			peer_info->last_msg_id  = atoll(temp_msg_id);
			free(temp_msg_id);
		} else {
			peer_info->last_msg_id = 0;
		}

		int *temp_last_msg_date = (int *)eina_list_nth(ts_msg, 4);
		if (temp_last_msg_date) {
			peer_info->last_msg_date = *temp_last_msg_date;
			free(temp_last_msg_date);
		}

		peer_info->print_name = (char *)eina_list_nth(ts_msg, 5);

		int *temp_struct_ver = (int *)eina_list_nth(ts_msg, 6);
		if (temp_struct_ver) {
			peer_info->struct_version = *temp_struct_ver;
			free(temp_struct_ver);
		}

		int *temp_no_unread = (int *)eina_list_nth(ts_msg, 7);
		if (temp_no_unread) {
			peer_info->no_of_unread_msgs = *temp_no_unread;
			free(temp_no_unread);
		}

		int *temp_last_seen = (int *)eina_list_nth(ts_msg, 8);
		if (temp_last_seen) {
			peer_info->last_seen_time = *temp_last_seen;
			free(temp_last_seen);
		}

		peer_info->photo_path = (char *)eina_list_nth(ts_msg, 9);

		int *temp_pic_id = (int *)eina_list_nth(ts_msg, 10);
		if (temp_pic_id) {
			peer_info->photo_id = *temp_pic_id;
			free(temp_pic_id);
		}
		int *temp_unknown = (int *)eina_list_nth(ts_msg, 11);
		if (temp_unknown) {
			peer_info->is_unknown = *temp_unknown;
			free(temp_unknown);
		}

		eina_list_free(ts_msg);

		peer_with_pic_s *item = (peer_with_pic_s*) malloc(sizeof(*item));
		item->use_data = peer_info;
		item->contact_icon = NULL;

		ad->peer_list = eina_list_append(ad->peer_list, item);

	}
}

void load_main_list_data(appdata_s *ad)
{
	if (!ad || !ad->peer_list)
		return;

	tg_main_list_item_s* main_item = NULL;
	EINA_LIST_FREE(ad->main_list, main_item) {

		if (main_item->peer_print_name)
			free(main_item->peer_print_name);
		main_item->peer_print_name = NULL;

		if (main_item->last_message)
			free(main_item->last_message);
		main_item->last_message = NULL;

		if (main_item->profile_pic_path)
			free(main_item->profile_pic_path);
		main_item->profile_pic_path = NULL;

		if (main_item->buddy_display_name)
			free(main_item->buddy_display_name);
		main_item->buddy_display_name = NULL;

		main_item->date_lbl = NULL;
		main_item->msg_status_lbl = NULL;
		main_item->profile_pic = NULL;
		main_item->profile_pic_path = NULL;
		main_item->status_lbl = NULL;
		main_item->user_name_lbl = NULL;
		main_item->main_item_layout = NULL;
	}
	ad->main_list = NULL;

	Eina_List *l = NULL;
	peer_with_pic_s *item = NULL;
	EINA_LIST_FOREACH(ad->peer_list, l, item) {

		tg_peer_info_s* peer_info = item->use_data;
		if (!peer_info)
			continue;

		if (peer_info->peer_type == TGL_PEER_CHAT &&
				get_buddy_unknown_status(peer_info->peer_id))
			continue;

		//if (peer_info->last_msg_id > 0) {

		// get message from message table.

		char* tablename = get_table_name_from_number(peer_info->peer_id);
		tg_message_s *msg = get_latest_message_from_message_table(tablename, EINA_FALSE);

		if (msg) {

			if (msg->service) {
				if (msg->message && strlen(msg->message) > 0) {
					item->last_message = strdup(msg->message);
				} else {
					item->last_message = strdup(" ");
				}
			} else {

				int media_type = msg->media_type;
				if (media_type == tgl_message_media_none) {
					item->last_message = strdup(msg->message ? msg->message : " ");
				} else if (media_type == tgl_message_media_photo) {
					item->last_message = strdup("Image");
				} else if (media_type == tgl_message_media_document) {
					tgl_media_s *media_msg = NULL;
					media_msg = get_media_details_from_db(atoll(msg->media_id));

					if (media_msg && media_msg->doc_type) {
						if (strlen(media_msg->doc_type) > 0) {
							item->last_message = strdup(media_msg->doc_type);
						} else {
							item->last_message = strdup("Document");
						}
						free_media_details(media_msg);
					} else {
						if (msg->message && strlen(msg->message) > 0) {
							item->last_message = strdup(msg->message);
						} else {
							item->last_message = strdup("Document");
						}
					}

				} else if (media_type == tgl_message_media_geo) {
					item->last_message = strdup("Geo location");
				} else if (media_type == tgl_message_media_contact) {
					item->last_message = strdup("Contact");
				} else if (media_type == tgl_message_media_unsupported) {
					item->last_message = strdup(" ");
				} else if (media_type == tgl_message_media_photo_encr) {
					item->last_message = strdup("Image encrypted");
				} else if (media_type == tgl_message_media_document_encr) {
					item->last_message = strdup("Document encrypted");
				} else  {
					item->last_message = strdup(" ");
				}
			}

			tg_main_list_item_s* main_list_item = (tg_main_list_item_s*)malloc(sizeof(tg_main_list_item_s));
			main_list_item->peer_id = peer_info->peer_id;
			main_list_item->peer_type = peer_info->peer_type;

			set_peer_names(peer_info, main_list_item);

			main_list_item->last_seen_time = msg->date;
			main_list_item->profile_pic = NULL;
			main_list_item->last_msg_id = msg->msg_id;
			main_list_item->last_message = strdup(item->last_message);
			main_list_item->last_msg_type = msg->media_type;
			main_list_item->is_out_msg = msg->out;
			main_list_item->last_msg_status = msg->msg_state;
			main_list_item->last_msg_service = msg->service;
			main_list_item->number_of_unread_msgs = get_unread_message_count(tablename);

			if (peer_info->photo_path)
				main_list_item->profile_pic_path = strdup(peer_info->photo_path);
			else
				main_list_item->profile_pic_path = NULL;

			main_list_item->user_name_lbl = NULL;
			main_list_item->status_lbl = NULL;
			main_list_item->date_lbl = NULL;
			main_list_item->msg_status_lbl = NULL;
			main_list_item->main_item_layout = NULL;
			ad->main_list = eina_list_append(ad->main_list, main_list_item);

			// delete message object
			if (msg->message)
				free(msg->message);
			msg->message = NULL;

			if (msg->media_id)
				free(msg->media_id);
			msg->media_id = NULL;

			free(msg);
			msg = NULL;

		} else {
			item->last_message = strdup(" ");
			if (peer_info->peer_type == TGL_PEER_CHAT) {
				tg_main_list_item_s* main_list_item = (tg_main_list_item_s*)malloc(sizeof(tg_main_list_item_s));
				main_list_item->peer_id = peer_info->peer_id;
				main_list_item->peer_type = peer_info->peer_type;

				set_peer_names(peer_info, main_list_item);

				main_list_item->last_seen_time = peer_info->last_seen_time;
				main_list_item->profile_pic = NULL;
				main_list_item->last_message = strdup(item->last_message);
				main_list_item->last_msg_type = -1;
				main_list_item->last_msg_service = 0;
				main_list_item->is_out_msg = -1;
				main_list_item->last_msg_id = -1;
				main_list_item->last_msg_status = -1;
				main_list_item->number_of_unread_msgs = 0;

				if (peer_info->photo_path) {
					main_list_item->profile_pic_path = strdup(peer_info->photo_path);
				} else {
					main_list_item->profile_pic_path = NULL;
				}
				main_list_item->user_name_lbl = NULL;
				main_list_item->status_lbl = NULL;
				main_list_item->date_lbl = NULL;
				main_list_item->msg_status_lbl = NULL;
				main_list_item->main_item_layout = NULL;
				ad->main_list = eina_list_append(ad->main_list, main_list_item);
			}
		}

		if (tablename) {
			free(tablename);
			tablename = NULL;
		}
	}
}

void load_unknown_buddy_list_data(appdata_s *ad)
{

	if (!ad)
		return;

	user_data_with_pic_s *item = NULL;
	EINA_LIST_FREE(ad->unknown_buddy_list, item) {
		user_data_s* user_data = item->use_data;

		if (user_data->print_name)
			free(user_data->print_name);
		user_data->print_name = NULL;

		if (user_data->photo_path)
			free(user_data->photo_path);
		user_data->photo_path = NULL;

		if (user_data->first_name)
			free(user_data->first_name);
		user_data->first_name = NULL;

		if (user_data->last_name)
			free(user_data->last_name);
		user_data->last_name = NULL;

		if (user_data->phone)
			free(user_data->phone);
		user_data->phone = NULL;

		if (user_data->real_first_name)
			free(user_data->real_first_name);
		user_data->real_first_name = NULL;

		if (user_data->real_last_name)
			free(user_data->real_last_name);
		user_data->real_last_name = NULL;

		if (user_data->username)
			free(user_data->username);
		user_data->username = NULL;

		free(user_data);
		free(item);
	}
	ad->unknown_buddy_list = NULL;

	Eina_List *user_info = get_unknown_buddy_list_info();
	if (!user_info) {
		//error
		//show_toast(ad, "User info not loaded");
		return;
	}

	Eina_List *row_vals = NULL;
	EINA_LIST_FREE(user_info, row_vals) {

		int *temp_user_id = (int *)eina_list_nth(row_vals, 0);
		if (ad->user_id.id == (*temp_user_id)) {
			void* val = NULL;
			EINA_LIST_FREE(row_vals, val) {
				if (val)
					free(val);
			}
			continue;
		}

		user_data_s *user_data = (user_data_s*)malloc(sizeof(*user_data));
		user_data->is_selected = EINA_FALSE;


		if (temp_user_id) {
			user_data->user_id.id = *temp_user_id;
			user_data->user_id.type = TGL_PEER_USER;
			free(temp_user_id);
		}

		user_data->print_name = (char *)eina_list_nth(row_vals, 1);

		int *temp_struct_ver = (int *)eina_list_nth(row_vals, 2);
		if (temp_struct_ver) {
			user_data->structure_version = *temp_struct_ver;
			free(temp_struct_ver);
		}

		user_data->photo_path = (char *)eina_list_nth(row_vals, 3);

		int *temp_photo_id = (int *)eina_list_nth(row_vals, 4);
		if (temp_photo_id) {
			user_data->photo_id = *temp_photo_id;
			free(temp_photo_id);
		}

		user_data->first_name = (char *)eina_list_nth(row_vals, 5);

		user_data->last_name = (char *)eina_list_nth(row_vals, 6);

		user_data->phone = (char *)eina_list_nth(row_vals, 7);

		int *temp_access_hash = (int *)eina_list_nth(row_vals, 8);
		if (temp_access_hash) {
			user_data->access_hash = *temp_access_hash;
			free(temp_access_hash);
		}

		user_data->real_first_name = (char *)eina_list_nth(row_vals, 9);

		user_data->real_last_name = (char *)eina_list_nth(row_vals, 10);

		user_data->username = (char *)eina_list_nth(row_vals, 11);

		int *temp_online_status = (int *)eina_list_nth(row_vals, 12);
		if (temp_online_status) {
			user_data->online = *temp_online_status;
			free(temp_online_status);
		}

		int *temp_last_seen = (int *)eina_list_nth(row_vals, 13);
		if (temp_last_seen) {
			user_data->last_seen = *temp_last_seen;
			free(temp_last_seen);
		}

		int *temp_is_blocked = (int *)eina_list_nth(row_vals, 14);
		if (temp_is_blocked) {
			user_data->is_blocked = *temp_is_blocked;
			free(temp_is_blocked);
		}

		int *temp_is_deleted = (int *)eina_list_nth(row_vals, 15);
		if (temp_is_deleted) {
			user_data->is_deleted = *temp_is_deleted;
			free(temp_is_deleted);
		}

		int *temp_is_unknown = (int *)eina_list_nth(row_vals, 16);
		if (temp_is_unknown) {
			user_data->is_unknown = *temp_is_unknown;
			free(temp_is_unknown);
		}

		user_data_with_pic_s *item = (user_data_with_pic_s *)malloc(sizeof(*item));
		item->use_data = user_data;
		item->contact_icon = NULL;
		ad->unknown_buddy_list = eina_list_append(ad->unknown_buddy_list, item);

		eina_list_free(row_vals);
	}
}

void load_buddy_list_data(appdata_s *ad)
{
	if (!ad) {
		return;
	}

	user_data_with_pic_s *item = NULL;
	EINA_LIST_FREE(ad->buddy_list, item) {
		user_data_s* user_data = item->use_data;

		if (user_data->print_name)
			free(user_data->print_name);
			user_data->print_name = NULL;

		if (user_data->photo_path)
			free(user_data->photo_path);
			user_data->photo_path = NULL;

		if (user_data->first_name)
			free(user_data->first_name);
			user_data->first_name = NULL;

		if (user_data->last_name)
			free(user_data->last_name);
			user_data->last_name = NULL;

		if (user_data->phone)
			free(user_data->phone);
			user_data->phone = NULL;

		if (user_data->real_first_name)
			free(user_data->real_first_name);
			user_data->real_first_name = NULL;

		if (user_data->real_last_name)
			free(user_data->real_last_name);
			user_data->real_last_name = NULL;

		if (user_data->username)
			free(user_data->username);
			user_data->username = NULL;

		free(user_data);
		free(item);
	}
	ad->buddy_list = NULL;

	Eina_List *user_info = get_buddy_list_info();

	if (!user_info) {
		//error
		//show_toast(ad, "User info not loaded");
		return;
	}

	Eina_List *row_vals = NULL;
	EINA_LIST_FREE(user_info, row_vals) {
		if (!row_vals)
			continue;
		int *temp_user_id = (int *)eina_list_nth(row_vals, 0);
		if (ad->user_id.id == (*temp_user_id)) {
			void* val = NULL;
			EINA_LIST_FREE(row_vals, val) {
				if (val)
					free(val);
			}
			continue;
		}

		user_data_s *user_data = (user_data_s *)malloc(sizeof(*user_data));
		user_data->is_selected = EINA_FALSE;


		if (temp_user_id) {
			user_data->user_id.id = *temp_user_id;
			user_data->user_id.type = TGL_PEER_USER;
			free(temp_user_id);
		}

		user_data->print_name = (char *)eina_list_nth(row_vals, 1);

		int *temp_struct_ver = (int *)eina_list_nth(row_vals, 2);
		if (temp_struct_ver) {
			user_data->structure_version = *temp_struct_ver;
			free(temp_struct_ver);
		}

		user_data->photo_path = (char *)eina_list_nth(row_vals, 3);

		int *temp_photo_id = (int *)eina_list_nth(row_vals, 4);
		if (temp_photo_id) {
			user_data->photo_id = *temp_photo_id;
			free(temp_photo_id);
		}

		user_data->first_name = (char *)eina_list_nth(row_vals, 5);

		user_data->last_name = (char *)eina_list_nth(row_vals, 6);

		user_data->phone = (char *)eina_list_nth(row_vals, 7);

		int *temp_access_hash = (int *)eina_list_nth(row_vals, 8);
		if (temp_access_hash) {
			user_data->access_hash = *temp_access_hash;
			free(temp_access_hash);
		}

		user_data->real_first_name = (char *)eina_list_nth(row_vals, 9);

		user_data->real_last_name = (char *)eina_list_nth(row_vals, 10);

		user_data->username = (char *)eina_list_nth(row_vals, 11);

		int *temp_online_status = (int *)eina_list_nth(row_vals, 12);
		if (temp_online_status) {
			user_data->online = *temp_online_status;
			free(temp_online_status);
		}

		int *temp_last_seen = (int *)eina_list_nth(row_vals, 13);
		if (temp_last_seen) {
			user_data->last_seen = *temp_last_seen;
			free(temp_last_seen);
		}

		int *temp_is_blocked = (int *)eina_list_nth(row_vals, 14);
		if (temp_is_blocked) {
			user_data->is_blocked = *temp_is_blocked;
			free(temp_is_blocked);
		}

		int *temp_is_deleted = (int *)eina_list_nth(row_vals, 15);
		if (temp_is_deleted) {
			user_data->is_deleted = *temp_is_deleted;
			free(temp_is_deleted);
		}

		int *temp_is_unknown = (int *)eina_list_nth(row_vals, 16);
		if (temp_is_unknown) {
			user_data->is_unknown = *temp_is_unknown;
			free(temp_is_unknown);
		}

		user_data_with_pic_s *item = (user_data_with_pic_s *) malloc(sizeof(*item));
		item->use_data = user_data;
		item->contact_icon = NULL;
		ad->buddy_list = eina_list_append(ad->buddy_list, item);
		eina_list_free(row_vals);
	}
}


static int on_contacts_loaded(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {

		// Load user data

		load_registered_user_data(app);

		if (app->current_app_state == TG_LOGIN_STATE || app->current_app_state == TG_REGISTRATION_STATE || app->current_app_state == TG_PROFILE_REGISTRATION_STATE) {
			// Launch buddy list view
			load_buddy_list_data(app);
			load_unknown_buddy_list_data(app);
			load_peer_data(app);
			load_main_list_data(app);
			elm_naviframe_item_pop(app->nf);
			elm_naviframe_item_pop(app->nf);
			//launch_buddy_list_cb(app);

			//launch_user_main_view_cb(app);
			launch_start_messaging_view(app);

		} else if (app->current_app_state == TG_BUDDY_LIST_STATE) {
			////refresh_buddy_list(app);
		} else {

		}

	} else {
		// error handling
		show_toast(app, "Error: loading contacts");
	}
	hide_loading_popup(app);
	return result;
}


static int on_contacts_and_chats_loaded(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {

		// Load user data
		//Eina_List *user_info = get_registered_user_info();
		load_registered_user_data(app);

		if (app->current_app_state == TG_LOGIN_STATE || app->current_app_state == TG_REGISTRATION_STATE || app->current_app_state == TG_PROFILE_REGISTRATION_STATE) {
			// Launch buddy list view
			load_buddy_list_data(app);
			load_unknown_buddy_list_data(app);
			load_peer_data(app);
			load_main_list_data(app);
			elm_naviframe_item_pop(app->nf);
			elm_naviframe_item_pop(app->nf);
			//launch_buddy_list_cb(app);
			launch_user_main_view_cb(app);
		} else if (app->current_app_state == TG_BUDDY_LIST_STATE) {
			app->current_app_state = TG_BUDDY_LIST_STATE;
			evas_object_show(app->panel);
			//elm_panel_hidden_set(app->panel, EINA_FALSE);
			////refresh_buddy_list(app);
		} else {

		}

	} else {
		// error handling
		show_toast(app, "Error: loading contacts");
	}
	hide_loading_popup(app);
	return result;
}

static int on_group_chat_updated(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char *chat_id_str = NULL;
	result = bundle_get_str(rec_msg, "chat_id", &chat_id_str);
	int chat_id = atoi(chat_id_str);

	int buddy_id = -1;
	app->main_item_in_cahtting_data = NULL;
	app->peer_in_cahtting_data = NULL;

	Eina_List *l = NULL;
	tg_main_list_item_s *item = NULL;
	EINA_LIST_FOREACH(app->main_list, l, item) {
		if (!item)
			continue;
		if (item->peer_id == chat_id) {
			app->main_item_in_cahtting_data = item;
			break;
		}
	}

	l = NULL;
	peer_with_pic_s* pic_item = NULL;
	int i = 0;
	EINA_LIST_FOREACH(app->peer_list, l, pic_item) {
		if (!pic_item) {
			i++;
			continue;
		}
		tg_peer_info_s* item = pic_item->use_data;
		if (item->peer_id == chat_id) {
			app->peer_in_cahtting_data = pic_item;
			buddy_id = i;
			break;
		}
		i++;
	}

	launch_messaging_view_cb(app, buddy_id);
	hide_loading_popup(app);
	return result;
}

static int on_group_chat_info_changed_response(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char *chat_id_str = NULL;
	result = bundle_get_str(rec_msg, "chat_id", &chat_id_str);
	int chat_id = atoi(chat_id_str);

	char *type_of_change = NULL;
	result = bundle_get_str(rec_msg, "type_of_change", &type_of_change);


	peer_with_pic_s *peer_item = get_peer_info(chat_id);
	tg_main_list_item_s *latest_item = get_latest_item(app, peer_item);


	Eina_List *l = NULL;
	peer_with_pic_s *pitem = NULL;
	EINA_LIST_FOREACH(app->peer_list, l, pitem) {
		if (!pitem)
			continue;
		if (pitem->use_data->peer_id == chat_id) {
			app->peer_list = eina_list_remove(app->peer_list, pitem);
			break;
		}
	}

	l = NULL;
	tg_main_list_item_s *item = NULL;
	EINA_LIST_FOREACH(app->main_list, l, item) {
		if (!item)
			continue;

		if (item->peer_id == chat_id) {
			app->main_list = eina_list_remove(app->main_list, item);
			break;
		}
	}

	app->peer_list = eina_list_prepend(app->peer_list, peer_item);
	app->main_list = eina_list_prepend(app->main_list, latest_item);
	refresh_main_list_view(app, EINA_TRUE);

	if (app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE && app->peer_in_cahtting_data
			&& app->peer_in_cahtting_data->use_data->peer_id == chat_id) {

		app->peer_in_cahtting_data = peer_item;
		app->main_item_in_cahtting_data = latest_item;

		on_group_chat_info_changed(app, type_of_change);
	}

	if (app->current_app_state ==  TG_SET_CHAT_INFO_STATE && app->peer_in_cahtting_data
			&& app->peer_in_cahtting_data->use_data->peer_id == chat_id) {

		app->peer_in_cahtting_data = peer_item;
		app->main_item_in_cahtting_data = latest_item;

		on_group_chat_info_changed(app, type_of_change);

		on_group_chat_info_updated(app, type_of_change);

	}
	hide_loading_popup(app);

	return result;
}

static int on_new_buddy_added_or_deleted_from_group_chat(appdata_s *app, bundle *const rec_msg, char *rec_key_val)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	Eina_Bool is_success = EINA_FALSE;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
	} else {
		is_success = EINA_FALSE;
	}

	if (app->current_app_state ==  TG_SET_CHAT_INFO_STATE && app->peer_in_cahtting_data
			&& app->peer_in_cahtting_data->use_data->peer_id == buddy_id) {
		hide_loading_popup(app);
		if (strcmp(rec_key_val, "group_chat_new_buddy_added_response") == 0) {
			show_toast(app, "Failed to add new buddy to chat.");
		} else if (strcmp(rec_key_val, "group_chat_buddy_deleted_response") == 0) {
			show_toast(app, "Failed to delete buddy to chat.");
		}
	}
	hide_loading_popup(app);
	return result;
}

static int on_group_chat_name_changed(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	Eina_Bool is_success = EINA_FALSE;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
	} else {
		is_success = EINA_FALSE;
	}

	if (app->current_app_state ==  TG_SET_CHAT_INFO_STATE && app->peer_in_cahtting_data
			&& app->peer_in_cahtting_data->use_data->peer_id == buddy_id) {

		hide_loading_popup(app);
		show_toast(app, "Failed to rename the chat title.");
	}
	hide_loading_popup(app);

	return result;
}

static int on_server_connection_status_changed(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	Eina_Bool is_success = EINA_FALSE;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "connection_status", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
	} else {
		is_success = EINA_FALSE;
	}

	app->is_server_ready = is_success;
	return result;
}

static int on_buddy_profile_pic_updated(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* user_id_str = NULL;
	result = bundle_get_str(rec_msg, "user_id", &user_id_str);
	int user_id = atoi(user_id_str);
	char* pic_file_path = NULL;
	result = bundle_get_str(rec_msg, "file_path", &pic_file_path);
	if (result == SVC_RES_FAIL || !pic_file_path || strlen(pic_file_path) <= 0 || access(pic_file_path, F_OK) == -1) {
		//pic_file_path = (char *)ui_utils_get_resource(DEFAULT_PROFILE_PIC);
		return result;
	}

	// update profile pic if buddy list view is present.
	if (user_id == app->user_id.id) {
		// registerd user.
		// to be handled
	} else {
		// update buddy list in appdata.

		Eina_List *l = NULL;
		user_data_with_pic_s *uitem = NULL;
		EINA_LIST_FOREACH(app->buddy_list, l, uitem) {
			if (!uitem)
				continue;
			if (uitem->use_data->user_id.id == user_id) {
				if (uitem->use_data->photo_path) {
					free(uitem->use_data->photo_path);
					uitem->use_data->photo_path = NULL;
				}
				uitem->use_data->photo_path = strdup(pic_file_path);
				if (uitem->contact_icon) {
					elm_image_file_set(uitem->contact_icon, pic_file_path, NULL);
					evas_object_color_set(uitem->contact_icon, 0, 0, 0, 0);
				}
				break;
			}
		}


		l = NULL;
		peer_with_pic_s *pitem = NULL;
		EINA_LIST_FOREACH(app->peer_list, l, pitem) {
			if (!pitem)
				continue;

			if (pitem->use_data->peer_id == user_id) {
				if (pitem->use_data->photo_path) {
					free(pitem->use_data->photo_path);
					pitem->use_data->photo_path = NULL;
				}
				pitem->use_data->photo_path = strdup(pic_file_path);
				if (pitem->contact_icon) {
					elm_image_file_set(pitem->contact_icon, pic_file_path, NULL);
					evas_object_color_set(pitem->contact_icon, 0, 0, 0, 0);
				}
				break;
			}
		}


		l = NULL;
		tg_main_list_item_s *item = NULL;
		EINA_LIST_FOREACH(app->main_list, l, item) {
			if (!item)
				continue;

			if (item->peer_id == user_id) {
				if (item->profile_pic_path) {
					free(item->profile_pic_path);
					item->profile_pic_path = NULL;
				}
				item->profile_pic_path = strdup(pic_file_path);
				if (item->profile_pic) {
					elm_image_file_set(item->profile_pic, pic_file_path, NULL);
					evas_object_color_set(item->profile_pic, 0, 0, 0, 0);
				}
				break;
			}
		}
		if (app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE && app->peer_in_cahtting_data && app->peer_in_cahtting_data->use_data
				&& app->peer_in_cahtting_data->use_data->peer_id == user_id) {
			on_chat_profile_pic_changed(app, pic_file_path);
		}
		if (app->current_app_state ==  TG_SET_CHAT_INFO_STATE && app->peer_in_cahtting_data && app->peer_in_cahtting_data->use_data
				&& app->peer_in_cahtting_data->use_data->peer_id == user_id) {
			//on_chat_profile_pic_changed(app, pic_file_path);
			//show_toast(app, "Chat profile picture updated successfully.");
		}
	}
	return result;
}


static int on_message_received_from_buddy(appdata_s *app, bundle *const rec_msg, Eina_Bool with_date)
{
	int result = SVC_RES_FAIL;

#if 0
	if (!app->peer_in_cahtting_data || !app->peer_in_cahtting_data->use_data) {
		LOGE("Wrong status, previous chatting data are freed!!!");
		return result;
	}
#endif

	char* from_id_str = NULL;
	result = bundle_get_str(rec_msg, "from_id", &from_id_str);
	int from_id = atoi(from_id_str);

	char* to_id_str = NULL;
	result = bundle_get_str(rec_msg, "to_id", &to_id_str);
	int to_id = atoi(to_id_str);

	char* msg_id_str = NULL;
	result = bundle_get_str(rec_msg, "message_id", &msg_id_str);
	long long message_id = atoll(msg_id_str);

	char* type_of_chat_str = NULL;
	result = bundle_get_str(rec_msg, "type_of_chat", &type_of_chat_str);
	int type_of_chat = atoi(type_of_chat_str);

	if (from_id == app->user_id.id) {
		// sent by the user. May be from another device.
	}

	//load message from the received info.
	// get phone number using buddy id.

	Eina_Bool is_user_present_in_main_list = EINA_FALSE;
	int id_to_check;
	if (type_of_chat == TGL_PEER_USER) {
		id_to_check = from_id;
	} else {
		id_to_check = to_id;
	}

	if (type_of_chat == TGL_PEER_USER) {
		if ((app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE || app->current_app_state ==  TG_SET_USER_INFO_STATE) &&
				app->peer_in_cahtting_data && app->peer_in_cahtting_data->use_data->peer_id == from_id) {
			if (with_date) {
				char* date_id_str = NULL;
				result = bundle_get_str(rec_msg, "date_id", &date_id_str);
				int date_id = atoi(date_id_str);
				on_text_message_received_from_buddy(app, date_id, type_of_chat);
				wait_for(1);
			}
			on_text_message_received_from_buddy(app, message_id, type_of_chat);

			return result;
		}

	} else if (type_of_chat == TGL_PEER_CHAT) {
		if ((app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE || app->current_app_state ==  TG_SET_CHAT_INFO_STATE) &&
				app->peer_in_cahtting_data && app->peer_in_cahtting_data->use_data->peer_id == to_id) {
			char* tablename = get_table_name_from_number(to_id);
			tg_message_s* msg = get_message_from_message_table(message_id, tablename);
			if (msg) {
				// To be handled.
				if (with_date) {
					char* date_id_str = NULL;
					result = bundle_get_str(rec_msg, "date_id", &date_id_str);
					int date_id = atoi(date_id_str);
					on_text_message_received_from_buddy(app, date_id, type_of_chat);
					wait_for(1);
					on_text_message_received_from_buddy(app, message_id, type_of_chat);
				} else {
					on_text_message_received_from_buddy(app, message_id, type_of_chat);
				}
				if (msg->message) {
					free(msg->message);
					msg->message = NULL;
				}

				if (msg->media_id) {
					free(msg->media_id);
					msg->media_id = NULL;
				}
			}
			free(msg);
			free(tablename);
			return result;
		}
	}

	Eina_List *l = NULL;
	tg_main_list_item_s *sel_item = NULL;
	EINA_LIST_FOREACH(app->main_list, l, sel_item) {

		if (sel_item->peer_id != id_to_check) {
			continue;
		}

		// update last message
		char* tablename = get_table_name_from_number(sel_item->peer_id);
		tg_message_s* msg = get_message_from_message_table(message_id, tablename);

		if (msg) {
			if (msg->service == 2) {
				if (msg->message) {
					free(msg->message);
					msg->message = NULL;
				}

				if (msg->media_id) {
					free(msg->media_id);
					msg->media_id = NULL;
				}
				free(msg);
				return SVC_RES_OK;
			}

			if (msg->service == 1) {
				if (msg->message && strlen(msg->message) > 0) {
					sel_item->last_message = strdup(msg->message);
				} else {
					sel_item->last_message = strdup(" ");
				}
			} else {

				int media_type = msg->media_type;
				if (media_type == tgl_message_media_none) {
					if (msg->message && strlen(msg->message) > 0) {
						sel_item->last_message = strdup(msg->message);
					} else {
						sel_item->last_message = strdup(" ");
					}
				} else if (media_type == tgl_message_media_photo) {
					sel_item->last_message = strdup("Image");
				} else if (media_type == tgl_message_media_document) {

#if 0
					if (msg->message && strlen(msg->message) > 0) {
						sel_item->last_message = strdup(msg->message);
					} else {
						sel_item->last_message = strdup("Document");
					}
#else
					tgl_media_s *media_msg = NULL;
					media_msg = get_media_details_from_db(atoll(msg->media_id));
					sel_item->last_message = strdup(media_msg->doc_type);
					free_media_details(media_msg);
#endif
				} else if (media_type == tgl_message_media_geo) {
					sel_item->last_message = strdup("Geo location");
				} else if (media_type == tgl_message_media_contact) {
					sel_item->last_message = strdup("Contact");
				} else if (media_type == tgl_message_media_unsupported) {
					sel_item->last_message = strdup(" ");
				} else if (media_type == tgl_message_media_photo_encr) {
					sel_item->last_message = strdup("Image encrypted");
				} else if (media_type == tgl_message_media_document_encr) {
					sel_item->last_message = strdup("Document encrypted");
				} else  {
					sel_item->last_message = strdup(" ");
				}
			}
			sel_item->last_msg_id = msg->msg_id;
			sel_item->last_msg_type = msg->media_type;
			sel_item->is_out_msg = msg->out;
			sel_item->last_msg_status = msg->msg_state;
			sel_item->last_seen_time = msg->date;
			sel_item->number_of_unread_msgs = get_unread_message_count(tablename);

			if (sel_item->profile_pic) {

			}

			if (sel_item->user_name_lbl) {

			}

			if (sel_item->status_lbl) {
				char* org_msg = NULL;
				if (sel_item->last_message) {
					org_msg = sel_item->last_message;
				} else {
					org_msg = "";
				}
				elm_object_text_set(sel_item->status_lbl, "");

				char status_buf[126] = {'\0'};
				const char *color;

				if (msg->service == 1 || msg->service == 2) {
					sel_item->last_msg_service = 1;
					color = "#158CB0";
				} else {
					sel_item->last_msg_service = 0;
					color = "#A4A4A4";
				}

				snprintf(status_buf, sizeof(status_buf), "<font=Tizen:style=Bold color=%s align=left><font_size=26>%.*s</font_size></font>", color, 40, org_msg);
				elm_object_text_set(sel_item->status_lbl, status_buf);
			}

			if (sel_item->date_lbl) {
				Eina_Bool is_today = compare_date_with_current_date(sel_item->last_seen_time);

				if (is_today) {
					elm_object_text_set(sel_item->date_lbl, "<font=Tizen:style=Normal color=#000000 align=center><font_size=25>Today</font_size></font>");
				} else {
					struct tm lt;
					char res[256];
					char time_str[256] = { 0, };
					time_t t;

					t = sel_item->last_seen_time;
					(void)localtime_r(&t, &lt);
					if (strftime(res, sizeof(res), "%d/%b/%Y", &lt) == 0) {
						strcpy(res, "unknown");
					}

					snprintf(time_str, sizeof(time_str), "<font=Tizen:style=Normal color=#000000 align=center><font_size=25>%s</font_size></font>", res);
					elm_object_text_set(sel_item->date_lbl, time_str);
				}
			}
			if (sel_item->msg_status_lbl) {
				Evas_Object* temp_lbl = elm_object_part_content_get(sel_item->msg_status_lbl, "swallow.status");
				if (temp_lbl) {
					evas_object_del(temp_lbl);
				}

				Evas_Object* num_lbl = elm_label_add(app->nf);
				elm_object_style_set(num_lbl, "transparent");
				char unread_msgs[256];
				sprintf(unread_msgs, "<font=Tizen:style=Bold color=#ffffff align=center><font_size=25>%d</font_size></font>", sel_item->number_of_unread_msgs);
				elm_object_text_set(num_lbl, unread_msgs);
				evas_object_size_hint_weight_set(num_lbl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
				evas_object_size_hint_align_set(num_lbl, EVAS_HINT_FILL, EVAS_HINT_FILL);
				evas_object_show(num_lbl);

				char edj_path[PATH_MAX] = {0, };
				app_get_resource(TELEGRAM_INIT_VIEW_EDJ, edj_path, (int)PATH_MAX);
				Evas_Object* msg_status = elm_layout_add(app->nf);
				elm_layout_file_set(msg_status, edj_path, "circle_item");
				evas_object_size_hint_weight_set(msg_status, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
				evas_object_size_hint_align_set(msg_status, EVAS_HINT_FILL, EVAS_HINT_FILL);
				evas_object_show(msg_status);
				elm_object_part_content_set(msg_status, "content", num_lbl);

				elm_object_part_content_set(sel_item->msg_status_lbl, "swallow.status", msg_status);

			}
			if (msg->message) {
				free(msg->message);
				msg->message = NULL;
			}

			if (msg->media_id) {
				free(msg->media_id);
				msg->media_id = NULL;
			}
			free(msg);
		}

		free(tablename);
		is_user_present_in_main_list = EINA_TRUE;

		app->main_list = eina_list_remove(app->main_list, sel_item);
		app->main_list = eina_list_prepend(app->main_list, sel_item);
		refresh_main_list_view(app, EINA_FALSE);
		break;
	}

	if (!is_user_present_in_main_list) {
		 peer_with_pic_s *peer_item = get_peer_info(id_to_check);
		 if (peer_item) {
			 tg_main_list_item_s* latest_item = get_latest_item(app, peer_item);
			 if (latest_item) {
				 app->main_list = eina_list_prepend(app->main_list, latest_item);
				 refresh_main_list_view(app, EINA_TRUE);
			 }
		 }
	}

	if (app->s_app_visible_state == APP_STATE_IN_BACKGROUND || app->current_app_state !=  TG_USER_MAIN_VIEW_STATE) {
		// show notification
		char *icon_path = (char *)ui_utils_get_resource(DEFAULT_TELEGRAM_ICON);
		char *title = "Telegram";
		char content[512];

		int unread_msg_cnt = get_number_of_unread_messages();

		if (unread_msg_cnt <= 0) {
			badge_set_count(TELEGRAM_APP_ID, 0);
			return result;
		}

		sprintf(content, "%d new messages received.", unread_msg_cnt);

		char *sound_track = NULL;
		char *app_id = TELEGRAM_APP_ID;
		tg_notification_create(app, icon_path, title, content, sound_track, app_id);
		int err = badge_set_count(TELEGRAM_APP_ID, unread_msg_cnt);
		if (BADGE_ERROR_NONE != err) {

		}
	}

	return result;
}

static int on_message_with_date_received_from_buddy(appdata_s *app, bundle *const rec_msg)
{
	return on_message_received_from_buddy(app, rec_msg, EINA_TRUE);
}

static int on_new_contact_added(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	Eina_Bool is_success = EINA_FALSE;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
	} else {
		is_success = EINA_FALSE;
	}
	if (is_success) {
		load_buddy_list_data(app);
		load_peer_data(app);
	} else {
		//show_toast(app, "Failed to add contact. Please try after sometime.");
	}
	hide_loading_popup(app);
	if (app->current_app_state == TG_ADD_CONTACT_STATE) {
		on_new_contact_added_response_received(app, buddy_id, is_success);
	}
	return result;
}

static int on_buddy_re_added_to_chat(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	Eina_Bool is_success = EINA_FALSE;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
	} else {
		is_success = EINA_FALSE;
	}

	if (app->current_app_state == TG_SET_USER_INFO_STATE) {
		on_user_added_response_received(app, buddy_id, is_success);
	}
	hide_loading_popup(app);
	return result;
}

static int on_buddy_deleted_from_friends_list(appdata_s *ad, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	Eina_Bool is_success = EINA_FALSE;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
	} else {
		is_success = EINA_FALSE;
	}

	if (is_success && ad->current_app_state == TG_SET_USER_INFO_STATE) {
		//on_user_deleted_response_received(app, buddy_id, is_success);
		show_loading_popup(ad);
		elm_naviframe_item_pop(ad->nf);
		ad->current_app_state = TG_CHAT_MESSAGING_VIEW_STATE;
		ad->peer_in_cahtting_data = NULL;
		ad->main_item_in_cahtting_data = NULL;
		ad->buddy_in_cahtting_data = NULL;
		elm_naviframe_item_pop(ad->nf);
		load_registered_user_data(ad);
		load_buddy_list_data(ad);
		load_unknown_buddy_list_data(ad);
		load_peer_data(ad);
		load_main_list_data(ad);

		if (buddy_id > 0) {
			Eina_List *l = NULL;
			tg_main_list_item_s* main_list_item = NULL;
			EINA_LIST_FOREACH(ad->main_list, l, main_list_item) {
				if (main_list_item && main_list_item->peer_id == buddy_id) {
					ad->main_list = eina_list_remove(ad->main_list, main_list_item);
					ad->main_list = eina_list_prepend(ad->main_list, main_list_item);
					break;
				}
			}
		}
		ecore_timer_add(1, on_load_main_view_requested, ad);
	} else {
		hide_loading_popup(ad);
	}
	return result;
}

static int on_buddy_blocked(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;

	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	Eina_Bool is_success = EINA_FALSE;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
	} else {
		is_success = EINA_FALSE;
	}

	if (app->current_app_state == TG_SET_USER_INFO_STATE) {
		on_user_block_response_received(app, buddy_id, is_success);
	}
	hide_loading_popup(app);

	return result;
}

static int on_buddy_unblocked(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;

	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	Eina_Bool is_success = EINA_FALSE;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
	} else {
		is_success = EINA_FALSE;
	}

	if (app->current_app_state == TG_SET_USER_INFO_STATE) {
		on_user_unblock_response_received(app, buddy_id, is_success);
	}
	hide_loading_popup(app);

	return result;
}

static int on_delete_selected_group_chats(appdata_s *ad, bundle *const rec_msg)
{
	int result = SVC_RES_OK;
#if 0
	if (app->current_app_state == TG_USER_MAIN_VIEW_SELECTION_STATE) {
		elm_naviframe_item_pop(app->nf);
		app->current_app_state = TG_USER_MAIN_VIEW_STATE;
		show_floating_button(app);
		load_buddy_list_data(app);
		load_unknown_buddy_list_data(app);
		load_peer_data(app);
		load_main_list_data(app);
		refresh_main_list_view(app, EINA_FALSE);
	}
	hide_loading_popup(app);
#else
	load_registered_user_data(ad);
	load_buddy_list_data(ad);
	load_unknown_buddy_list_data(ad);
	load_peer_data(ad);
	load_main_list_data(ad);
	if (ad->main_item) {
		elm_naviframe_item_pop_to(ad->main_item);
	}
	ecore_timer_add(1, on_load_main_view_requested, ad);
#endif
	return result;
}

static int on_group_chat_deleted_response(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;

	char* chat_id_str = NULL;
	result = bundle_get_str(rec_msg, "chat_id", &chat_id_str);
	int chat_id = atoi(chat_id_str);

	Eina_Bool is_success = EINA_FALSE;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
	} else {
		is_success = EINA_FALSE;
	}

	if (is_success) {


		Eina_List *l = NULL;
		peer_with_pic_s *pitem = NULL;
		EINA_LIST_FOREACH(app->peer_list, l, pitem) {
			if (!pitem)
				continue;

			if (pitem->use_data->peer_id == chat_id) {
				app->peer_list = eina_list_remove(app->peer_list, pitem);
				break;
			}
		}

		l = NULL;
		tg_main_list_item_s *item = NULL;
		EINA_LIST_FOREACH(app->main_list, l, item) {
			if (item->peer_id == chat_id) {
				app->main_list = eina_list_remove(app->main_list, item);
				break;
			}
		}

		refresh_main_list_view(app, EINA_FALSE);

		if (app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE && app->peer_in_cahtting_data
				&& app->peer_in_cahtting_data->use_data->peer_id == chat_id) {

			app->peer_in_cahtting_data = NULL;
			app->main_item_in_cahtting_data = NULL;

			elm_naviframe_item_pop(app->nf);
			app->current_app_state = TG_USER_MAIN_VIEW_STATE;
			show_floating_button(app);
		}

		if (app->current_app_state ==  TG_SET_CHAT_INFO_STATE && app->peer_in_cahtting_data
							&& app->peer_in_cahtting_data->use_data->peer_id == chat_id) {
			on_group_chat_deleted(app, chat_id);
		}

	} else {
		show_toast(app, "Failed to delete chat");
	}
	hide_loading_popup(app);

	return result;
}

static int on_message_sent_to_buddy(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;

	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	char* msg_id_str = NULL;
	result = bundle_get_str(rec_msg, "message_id", &msg_id_str);
	int message_id = atoi(msg_id_str);

	char* table_name = NULL;
	result = bundle_get_str(rec_msg, "table_name", &table_name);


	Eina_Bool is_success = EINA_FALSE;
	char* is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
	} else {
		is_success = EINA_FALSE;
		show_toast(app, "message sent failed");
	}

	char* type_of_chat_str = NULL;
	result = bundle_get_str(rec_msg, "type_of_chat", &type_of_chat_str);
	int type_of_chat = atoi(type_of_chat_str);

	tg_message_s* msg = get_message_from_message_table(message_id, table_name);

	if (msg) {
		if (app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE && app->peer_in_cahtting_data && app->peer_in_cahtting_data->use_data->peer_id == buddy_id) {
			// update message to sent state
			on_text_message_state_changed(app, msg, type_of_chat);
		}

		if (app->current_app_state ==  TG_USER_MAIN_VIEW_STATE) {
			Eina_List *l = NULL;
			tg_main_list_item_s *sel_item = NULL;
			EINA_LIST_FOREACH(app->main_list, l, sel_item) {
				if (sel_item->peer_id == buddy_id && sel_item->last_msg_id == message_id) {
					if (sel_item->msg_status_lbl) {
						elm_image_file_set(sel_item->msg_status_lbl, ui_utils_get_resource(MESSAGE_SENT_ICON), NULL);
					}

					break;
				}
			}
		}
		if (msg->message) {
			free(msg->message);
			msg->message = NULL;
		}

		if (msg->media_id) {
			free(msg->media_id);
			msg->media_id = NULL;
		}
		free(msg);
	}
	return result;
}

static int on_self_profile_pic_updated(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	Eina_Bool is_success = EINA_FALSE;
	char *is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
		show_toast(app, "profile picture updated successfully.");
	} else {
		is_success = EINA_FALSE;
		show_toast(app, "profile picture not updated.");
	}

	if (is_success) {
		char *file_path = NULL;
		result = bundle_get_str(rec_msg, "file_path", &file_path);

		if (app->current_user_data->photo_path) {
			free(app->current_user_data->photo_path);
			app->current_user_data->photo_path = NULL;
		}

		app->current_user_data->photo_path = strdup(file_path);
		Evas_Object* profile_pic = evas_object_data_get(app->nf, "main_user_profile_pic");
		if (profile_pic) {
			elm_image_file_set(profile_pic, file_path, NULL);
		}

		if (app->current_app_state == TG_SETTINGS_SCREEN_STATE) {
			Evas_Object* set_pro_pic = evas_object_data_get(app->nf, "settings_user_profile_pic");
			if (set_pro_pic) {
				elm_image_file_set(set_pro_pic, file_path, NULL);
			}
		}
	}
	hide_loading_popup(app);
	return result;
}

static int on_self_profile_name_updated(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	Eina_Bool is_success = EINA_FALSE;
	char *is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
		show_toast(app, "Username updated successfully.");
	} else {
		is_success = EINA_FALSE;
		show_toast(app, "Username not updated. Username already exist or please check your network connection.");
	}

	if (is_success) {

		char *first_name = NULL;
		result = bundle_get_str(rec_msg, "first_name", &first_name);
		if (!first_name) {
			first_name = "";
		}

		char *last_name = NULL;
		result = bundle_get_str(rec_msg, "last_name", &last_name);
		if (!last_name) {
			last_name = "";
		}

		if (app->current_user_data->first_name) {
			free(app->current_user_data->first_name);
			app->current_user_data->first_name = NULL;
		}

		if (app->current_user_data->last_name) {
			free(app->current_user_data->last_name);
			app->current_user_data->last_name = NULL;
		}

		if (first_name) {
			app->current_user_data->first_name = strdup(first_name);
		}

		if (last_name) {
			app->current_user_data->last_name = strdup(last_name);
		}

		if (app->current_app_state == TG_SETTINGS_EDIT_NAME_STATE) {
			elm_naviframe_item_pop(app->nf);
			app->current_app_state = TG_SETTINGS_SCREEN_STATE;
			refresh_settings_screen(app);
		}

	} else {

	}
	hide_loading_popup(app);
	return result;
}

static int on_self_username_updated(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	Eina_Bool is_success = EINA_FALSE;
	char *is_success_val = NULL;
	result = bundle_get_str(rec_msg, "is_success", &is_success_val);
	if (strncmp("true", is_success_val, strlen("true")) == 0) {
		is_success = EINA_TRUE;
		show_toast(app, "Username updated successfully.");
	} else {
		is_success = EINA_FALSE;
		show_toast(app, "Username not updated. Username already exist or please check your network connection.");
	}

	if (is_success) {
		char *username = NULL;
		result = bundle_get_str(rec_msg, "username", &username);

		if (app->current_user_data->username) {
			free(app->current_user_data->username);
			app->current_user_data->username = NULL;
		}

		app->current_user_data->username = strdup(username);


		if (app->current_app_state == TG_SETTINGS_SCREEN_STATE || app->current_app_state == TG_SET_USERNAME_STATE) {
			Evas_Object *name_lbl = evas_object_data_get(app->nf, "settings_user_name_label");
			if (name_lbl) {
				char buf[512] = {'\0'};
				snprintf(buf, 512, "<font=Tizen:style=Bold color=#000000 align=left><font_size=40>%s</font_size></font>", app->current_user_data->username);
				elm_object_text_set(name_lbl, buf);
			}
		}
		if (app->current_app_state == TG_SET_USERNAME_STATE) {
			elm_naviframe_item_pop(app->nf);
			app->current_app_state = TG_SETTINGS_SCREEN_STATE;
		}
	}
	hide_loading_popup(app);

	return result;
}

static int on_message_read_by_buddy(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	char* msg_id_str = NULL;
	result = bundle_get_str(rec_msg, "message_id", &msg_id_str);
	int message_id = atoi(msg_id_str);

	char* table_name = NULL;
	result = bundle_get_str(rec_msg, "table_name", &table_name);

	char* phone_number = NULL;
	result = bundle_get_str(rec_msg, "phone_number", &phone_number);

	char* type_of_chat_str = NULL;
	result = bundle_get_str(rec_msg, "type_of_chat", &type_of_chat_str);
	int type_of_chat = atoi(type_of_chat_str);

	tg_message_s* msg = get_message_from_message_table(message_id, table_name);

	if (msg) {
		if (app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE && app->peer_in_cahtting_data && app->peer_in_cahtting_data->use_data->peer_id == buddy_id) {
			// update message to sent state
			on_text_message_state_changed(app, msg, type_of_chat);
		}

		if (app->current_app_state ==  TG_USER_MAIN_VIEW_STATE) {

			Eina_List *l = NULL;
			tg_main_list_item_s *sel_item = NULL;
			EINA_LIST_FOREACH(app->main_list, l, sel_item) {

				if (sel_item->peer_id == buddy_id && sel_item->last_msg_id == message_id) {

					if (sel_item->msg_status_lbl) {
						elm_image_file_set(sel_item->msg_status_lbl, ui_utils_get_resource(MESSAGE_READ_ICON), NULL);
					}

					break;
				}
			}
		}

		if (msg->message) {
			free(msg->message);
			msg->message = NULL;
		}

		if (msg->media_id) {
			free(msg->media_id);
			msg->media_id = NULL;
		}
		free(msg);
	}

	return result;
}

static int on_media_message_download_completed(appdata_s *app, bundle *const rec_msg)
{
	LOGE("Media download14");
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	char* to_id_str = NULL;
	result = bundle_get_str(rec_msg, "to_peer_id", &to_id_str);
	int to_id = atoi(to_id_str);

	char* media_id_str = NULL;
	result = bundle_get_str(rec_msg, "media_id", &media_id_str);
	long long media_id = atoll(media_id_str);

	char* file_name = NULL;
	result = bundle_get_str(rec_msg, "file_name", &file_name);

	//file_name == "failed_to_load"

	if (file_name && strstr(file_name, "failed_to_load") != NULL) {
		// download failed.
		show_toast(app, "media download failed.");
	}

	char* caption = NULL;
	result = bundle_get_str(rec_msg, "caption", &caption);


	if (file_name && app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE && app->peer_in_cahtting_data
			&& app->peer_in_cahtting_data->use_data->peer_id == to_id) {
		// update media to sent state
		LOGE("Media download15");
		on_media_download_completed(app, buddy_id, media_id, file_name, caption);
	}
	LOGE("Media download16");
	return result;
}

static int on_video_message_thumb_download_completed(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	char* to_id_str = NULL;
	result = bundle_get_str(rec_msg, "to_peer_id", &to_id_str);
	int to_id = atoi(to_id_str);

	char* media_id_str = NULL;
	result = bundle_get_str(rec_msg, "media_id", &media_id_str);
	long long media_id = atoll(media_id_str);

	char* file_name = NULL;
	result = bundle_get_str(rec_msg, "file_name", &file_name);

	char* caption = NULL;
	result = bundle_get_str(rec_msg, "caption", &caption);

	if (file_name && app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE && app->peer_in_cahtting_data
			&& app->peer_in_cahtting_data->use_data->peer_id == to_id) {
		// update thumbnail icon
		on_video_thumb_download_completed(app, buddy_id, media_id, file_name, caption);
	}
	return result;
}

static int on_new_buddy_added_to_contacts(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);

	int buddy_id = atoi(buddy_id_str);
	peer_with_pic_s *peer_item = get_peer_info(buddy_id);
	tg_main_list_item_s* latest_item = get_latest_item(app, peer_item);
	if (latest_item) {
		app->main_list = eina_list_prepend(app->main_list, latest_item);
		refresh_main_list_view(app, EINA_TRUE);
	}
	hide_loading_popup(app);

	return result;
}

static int on_new_group_added(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;


#if 0
	char* chat_id_str = NULL;
	result = bundle_get_str(rec_msg, "chat_id", &chat_id_str);

	 int chat_id = atoi(chat_id_str);
	 peer_with_pic_s *peer_item = get_peer_info(chat_id);
	if (app->current_app_state == TG_USER_MAIN_VIEW_STATE) {
		tg_main_list_item_s* latest_item = get_latest_item(app, peer_item);
		if (latest_item) {
			app->main_list = eina_list_prepend(app->main_list, latest_item);
			hide_loading_popup(app);
			refresh_main_list_view(app, EINA_FALSE);
		}
	} else {

		hide_loading_popup(app);
		if (app->main_item) {
			elm_naviframe_item_pop_to(app->main_item);
			app->current_app_state = TG_USER_MAIN_VIEW_STATE;
		}

		tg_main_list_item_s* latest_item = get_latest_item(app, peer_item);
		if (latest_item) {
			app->main_list = eina_list_prepend(app->main_list, latest_item);
			refresh_main_list_view(app, EINA_FALSE);
		}

	}
	app->peer_list = eina_list_prepend(app->peer_list, peer_item);
#else
	load_registered_user_data(app);
	load_buddy_list_data(app);
	load_unknown_buddy_list_data(app);
	load_peer_data(app);
	load_main_list_data(app);

	if (app->main_item) {
		elm_naviframe_item_pop_to(app->main_item);
	}

	ecore_timer_add(1, on_load_main_view_requested, app);
#endif
	return result;
}

static int on_buddy_contact_updated(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	//int buddy_id = atoi(buddy_id_str);

	char* update_msg = NULL;
	result = bundle_get_str(rec_msg, "update_message", &update_msg);

	// reload contact

	return result;
}

static int on_buddy_status_updated(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);
	// update to online or last seen, if current view is conversation.

	// sandeep
	if (app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE && app->buddy_in_cahtting_data && app->buddy_in_cahtting_data->use_data->user_id.id == buddy_id) {
		on_user_presence_state_changed(app, buddy_id);
	}
	return result;
}

static int on_typing_status_updated(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	char* buddy_name = NULL;
	result = bundle_get_str(rec_msg, "buddy_name", &buddy_name);

	const char *type_status_str;

	result = bundle_get_str(rec_msg, "type_status", &type_status_str);
	enum tgl_typing_status type_status = atoi(type_status_str);

	// update UI
	char* type_status_message = NULL;

	switch (type_status) {
		case tgl_typing_none:
			type_status_message = "doing nothing";
			break;
		case tgl_typing_typing:
			type_status_message = "typing...";
			break;
		case tgl_typing_cancel:
			type_status_message = "deleting typed message";
			break;
		case tgl_typing_record_video:
			type_status_message = "recording video";
			break;
		case tgl_typing_upload_video:
			type_status_message = "uploading video";
			break;
		case tgl_typing_record_audio:
			type_status_message = "recording audio";
			break;
		case tgl_typing_upload_audio:
			type_status_message = "uploading audio";
			break;
		case tgl_typing_upload_photo:
			type_status_message = "uploading photo";
			break;
		case tgl_typing_upload_document:
			type_status_message = "uploading document";
			break;
		case tgl_typing_geo:
			type_status_message = "choosing location";
			break;
		case tgl_typing_choose_contact:
			type_status_message = "choosing contact";
			break;
		default:
			type_status_message = "";
			break;
	}

	// update UI. may be contacts view or conversation view.
	if (app->current_app_state ==  TG_USER_MAIN_VIEW_STATE) {
		Eina_List *l = NULL;
		tg_main_list_item_s *sel_item = NULL;
		EINA_LIST_FOREACH(app->main_list, l, sel_item) {
			if (sel_item->peer_id != buddy_id) {
				continue;
			}
			// update last message

			if (sel_item->profile_pic) {

			}

			if (sel_item->user_name_lbl) {

			}

			if (sel_item->status_lbl) {
				char status_buf[512] = {'\0'};
				snprintf(status_buf, sizeof(status_buf), "<font=Tizen:style=Bold color=#158CB0 align=left><font_size=30>%.*s</font_size></font>", 25, type_status_message);
				elm_object_text_set(sel_item->status_lbl, status_buf);
			}

			if (sel_item->date_lbl) {

			}
			if (sel_item->msg_status_lbl) {

			}
			break;
		}

	}

	if (app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE && app->buddy_in_cahtting_data && app->buddy_in_cahtting_data->use_data->user_id.id == buddy_id) {
		on_user_status_changed(app, type_status_message);
	}

	return result;
}


static int on_user_status_updated(appdata_s *app, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	char* buddy_id_str = NULL;
	result = bundle_get_str(rec_msg, "buddy_id", &buddy_id_str);
	int buddy_id = atoi(buddy_id_str);

	char* buddy_name = NULL;
	result = bundle_get_str(rec_msg, "buddy_name", &buddy_name);

	if (app->current_app_state ==  TG_USER_MAIN_VIEW_STATE) {
		Eina_List *l = NULL;
		tg_main_list_item_s *sel_item = NULL;
		EINA_LIST_FOREACH(app->main_list, l, sel_item) {
			if (sel_item->peer_id != buddy_id) {
				continue;
			}
			// update last message

			if (sel_item->profile_pic) {

			}

			if (sel_item->user_name_lbl) {

			}

			if (sel_item->status_lbl) {
				char status_buf[126] = {'\0'};
				snprintf(status_buf, sizeof(status_buf), "<font=Tizen:style=Normal color=#A4A4A4 align=left><font_size=26>%.*s</font_size></font>", 29, sel_item->last_message ? sel_item->last_message : "");
				elm_object_text_set(sel_item->status_lbl, status_buf);
			}

			if (sel_item->date_lbl) {

			}
			if (sel_item->msg_status_lbl) {

			}

			break;
		}
	}
	if (app->current_app_state ==  TG_CHAT_MESSAGING_VIEW_STATE && app->buddy_in_cahtting_data && app->buddy_in_cahtting_data->use_data->user_id.id == buddy_id) {
		on_user_presence_state_changed(app, buddy_id);
	}

	return result;
}

Eina_Bool on_logout_completed(void *data)
{
	appdata_s *ad = data;
	if (ad) {
		elm_naviframe_item_pop(ad->nf);
		ad->current_app_state = TG_REGISTRATION_STATE;
		launch_init_screen(ad);
	}
    return ECORE_CALLBACK_CANCEL;
}

Eina_Bool on_start_server_requested(void *data)
{
	appdata_s *ad = data;
	if (ad) {
		launch_tg_server(ad);
	}
    return ECORE_CALLBACK_CANCEL;
}


static int _on_service_client_msg_received_cb(void *data, bundle *const rec_msg)
{
	int result = SVC_RES_FAIL;
	RETVM_IF(!data, result, "Data is NULL");
	appdata_s *app = data;
	char *rec_key_val = NULL;


	result = bundle_get_str(rec_msg, "command", &rec_key_val);

	LOGI("MSG recv = %s", rec_key_val);

	if (strcmp(rec_key_val, "server_not_initialized") == 0) {
		hide_loading_popup(app);
		return result;
	} else if (strcmp(rec_key_val, "server_restart_notification") == 0) {
		hide_loading_popup(app);
		show_toast(app, i18n_get_text("IDS_NFC_POP_INITIALISING_PLEASE_WAIT_ING"));
		ecore_timer_add(1, on_start_server_requested, app);
	} else if (strcmp(rec_key_val, "registration_done") == 0) {

		char* is_success_val = NULL;
		result = bundle_get_str(rec_msg, "is_success", &is_success_val);

		if (strncmp("true", is_success_val, strlen("true")) == 0) {
    		//show_toast(app, is_success_val);
			// Launch login view
			//elm_naviframe_item_pop(app->nf);
			launch_login_cb(data);
		} else {
			// error handling
			show_toast(app, "Error: registration");
		}
		hide_loading_popup(app);
		return result;
	} else if (strcmp(rec_key_val, "name_registration_request") == 0) {

		elm_naviframe_item_pop(app->nf);
		elm_naviframe_item_pop(app->nf);
		launch_first_registration_cb(app);
		hide_loading_popup(app);
		return result;
	} else if (strcmp(rec_key_val, "request_reg_code_again") == 0) {
		show_toast(app, "Please enter a valid code.");
		hide_loading_popup(app);
	} else if (strcmp(rec_key_val, "request_phone_num_again") == 0) {
		show_toast(app, "Please enter a valid phone number.");
		hide_loading_popup(app);
	} else if (strcmp(rec_key_val, "server_connection_failed") == 0) {
		show_toast(app, "Server connection failed. please check network connection");
		hide_loading_popup(app);
	} else if (strcmp(rec_key_val, "logout_completed") == 0) {

		free_app_data(app, EINA_TRUE);
		hide_loading_popup(app);
		app->phone_number = NULL;
		app->buddy_list = NULL;
		app->unknown_buddy_list = NULL;
		app->known_buddy_list = NULL;
		app->main_list = NULL;
		app->peer_list = NULL;
		app->search_peer_list = NULL;
		app->is_first_time_registration = EINA_FALSE;
		app->panel = NULL;
		app->loaded_msg_list = NULL;
		app->loading_popup = NULL;
		app->current_user_data = NULL;
		app->is_tg_initilized = EINA_FALSE;
		app->chat_background = NULL;
		app->msg_popup = NULL;
		app->s_notififcation = NULL;
		app->panel = NULL;
		app->is_server_ready = EINA_FALSE;
		app->contact_list = NULL;

		char *chat_bg = NULL;
		preference_get_string(TG_CHAT_BG_PREFERENCE, &chat_bg);
		if (chat_bg) {
			app->chat_background = strdup(chat_bg);
		}

		if (!app->chat_background) {
			app->chat_background = strdup(ui_utils_get_resource(TG_CHAT_DEFAULT_BG));
			preference_set_string(TG_CHAT_BG_PREFERENCE, app->chat_background);
		}

		init_service(app);

		elm_naviframe_item_pop(app->nf);
		//ecore_timer_add(1, on_logout_completed, app);
		elm_naviframe_item_pop(app->nf);
		app->current_app_state = TG_REGISTRATION_STATE;
		launch_init_screen(app);
		return result;
	} else if (strcmp(rec_key_val, "server_connection_status") == 0) {
		return on_server_connection_status_changed(data, rec_msg);
	}

	if (strcmp(rec_key_val, "contacts_and_chats_load_done") == 0) {
		app->is_tg_initilized = EINA_TRUE;
	}
	if (!app->is_tg_initilized) {
		return result;
	}

	if (strcmp(rec_key_val, "contacts_load_done") == 0) {

		return on_contacts_loaded(data, rec_msg);

	} else if (strcmp(rec_key_val, "contacts_and_chats_load_done") == 0) {

		return on_contacts_and_chats_loaded(data, rec_msg);

	} else if (strcmp(rec_key_val, "response_group_chat_updated") == 0) {

		return on_group_chat_updated(data, rec_msg);

	} else if (strcmp(rec_key_val, "group_chat_updated") == 0) {

		return on_group_chat_info_changed_response(data, rec_msg);

	} else if ((strcmp(rec_key_val, "group_chat_new_buddy_added_response") == 0) || (strcmp(rec_key_val, "group_chat_buddy_deleted_response") == 0)) {

		return on_new_buddy_added_or_deleted_from_group_chat(data, rec_msg, rec_key_val);

	} else if (strcmp(rec_key_val, "group_chat_rename_response") == 0) {

		return on_group_chat_name_changed(data, rec_msg);

	} else if (strcmp(rec_key_val, "buddy_profile_pic_updated") == 0) {

		return on_buddy_profile_pic_updated(data, rec_msg);

	} else if (strcmp(rec_key_val, "message_received") == 0) {

		return on_message_received_from_buddy(data, rec_msg, EINA_FALSE);

	} else if (strcmp(rec_key_val, "message_received_with_date") == 0) {

		return on_message_with_date_received_from_buddy(data, rec_msg);

	} else if (strcmp(rec_key_val, "buddy_readded") == 0) {

		return on_buddy_re_added_to_chat(data, rec_msg);

	} else if (strcmp(rec_key_val, "new_contact_added") == 0) {

		return on_new_contact_added(data, rec_msg);

	} else if (strcmp(rec_key_val, "buddy_deleted") == 0) {

		return on_buddy_deleted_from_friends_list(data, rec_msg);

	} else if (strcmp(rec_key_val, "buddy_blocked") == 0) {

		return on_buddy_blocked(data, rec_msg);

	} else if (strcmp(rec_key_val, "buddy_unblocked") == 0) {

		return on_buddy_unblocked(data, rec_msg);

	} else if (strcmp(rec_key_val, "selected_group_chats_deleted_response") == 0) {

		return on_delete_selected_group_chats(data, rec_msg);

	} else if (strcmp(rec_key_val, "group_chat_deleted_response") == 0) {

		return on_group_chat_deleted_response(data, rec_msg);

	} else if (strcmp(rec_key_val, "message_sent_to_buddy") == 0) {

		return on_message_sent_to_buddy(data, rec_msg);

	} else if (strcmp(rec_key_val, "self_profile_picture_updated") == 0) {

		return on_self_profile_pic_updated(data, rec_msg);

	} else if (strcmp(rec_key_val, "self_profile_name_updated") == 0) {

		return on_self_profile_name_updated(data, rec_msg);

	} else if (strcmp(rec_key_val, "self_username_updated") == 0) {

		return on_self_username_updated(data, rec_msg);

	} else if (strcmp(rec_key_val, "message_read_by_buddy") == 0) {

		return on_message_read_by_buddy(data, rec_msg);

	} else if (strcmp(rec_key_val, "media_download_completed") == 0) {

		return on_media_message_download_completed(data, rec_msg);

	} else if (strcmp(rec_key_val, "video_thumb_download_completed") == 0) {

		return on_video_message_thumb_download_completed(data, rec_msg);

	} else if (strcmp(rec_key_val, "add_contacts_request") == 0) {
		// Not to be handled.
	} else if (strcmp(rec_key_val, "new_buddy_added") == 0) {

		return on_new_buddy_added_to_contacts(data, rec_msg);

	} else if (strcmp(rec_key_val, "new_group_added") == 0) {

		return on_new_group_added(data, rec_msg);

	} else if (strcmp(rec_key_val, "contact_updated") == 0) {

		return on_buddy_contact_updated(data, rec_msg);

	} else if (strcmp(rec_key_val, "buddy_status_updated") == 0) {

		return on_buddy_status_updated(data, rec_msg);

	} else if (strcmp(rec_key_val, "type_status_updated") == 0) {

		return on_typing_status_updated(data, rec_msg);

	} else if (strcmp(rec_key_val, "user_status_updated") == 0) {

		return on_user_status_updated(data, rec_msg);

	} else {

	}

	return result;
}

int init_service(appdata_s *app)
{
	int result = SVC_RES_FAIL;
	RETVM_IF(!app, result, "Application data is NULL");

	app->service_client = service_client_create();
	RETVM_IF(!app->service_client, result, "Failed to create service client");

	result = service_client_register_port(app->service_client, TELEGRAM_CLIENT_PORT_NAME);
	if (result != SVC_RES_OK) {
		ERR("Failed to register service client port");
		service_client_destroy(app->service_client);
		app->service_client = NULL;
		return result;
	}

	result = _service_client_set_remote_data(app->service_client, TELEGRAM_SERVER_APP_NAME, TELEGRAM_SERVER_PORT_NAME);

	result = service_client_register_msg_receive_callback(app->service_client, _on_service_client_msg_received_cb, app);
	if (result != SVC_RES_OK) {
		ERR("Failed to register service client on message receive callback");
		service_client_destroy(app->service_client);
		app->service_client = NULL;
		return result;
	}
	return result;
}

void win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}


tg_main_list_item_s* get_latest_item(appdata_s *ad,  peer_with_pic_s *item)
{
	tg_main_list_item_s* main_list_item = NULL;
	if (!item)
		return NULL;

	tg_peer_info_s* peer_info = item->use_data;
	if (!peer_info)
		return NULL;

	// get message from message table.

	char* tablename = get_table_name_from_number(peer_info->peer_id);
	tg_message_s* msg = get_latest_message_from_message_table(tablename, EINA_FALSE);

	if (msg) {

		if (msg->service) {
			if (msg->message && strlen(msg->message) > 0) {
				item->last_message = strdup(msg->message);
			} else {
				item->last_message = strdup(" ");
			}
		} else {

			int media_type = msg->media_type;
			if (media_type == tgl_message_media_none) {
				if (msg->message && strlen(msg->message) > 0) {
					item->last_message = strdup(msg->message);
				} else {
					item->last_message = strdup(" ");
				}
			} else if (media_type == tgl_message_media_photo) {
				item->last_message = strdup("Image");
			} else if (media_type == tgl_message_media_document) {
				tgl_media_s *media_msg = NULL;
				media_msg = get_media_details_from_db(atoll(msg->media_id));

				if (media_msg && media_msg->doc_type) {
					if (strlen(media_msg->doc_type) > 0) {
						item->last_message = strdup(media_msg->doc_type);
					} else {
						item->last_message = strdup("Document");
					}
					free_media_details(media_msg);
				} else {
					if (msg->message && strlen(msg->message) > 0) {
						item->last_message = strdup(msg->message);
					} else {
						item->last_message = strdup("Document");
					}
				}

			} else if (media_type == tgl_message_media_geo) {
				item->last_message = strdup("Geo location");
			} else if (media_type == tgl_message_media_contact) {
				item->last_message = strdup("Contact");
			} else if (media_type == tgl_message_media_unsupported) {
				item->last_message = strdup(" ");
			} else if (media_type == tgl_message_media_photo_encr) {
				item->last_message = strdup("Image encrypted");
			} else if (media_type == tgl_message_media_document_encr) {
				item->last_message = strdup("Document encrypted");
			} else  {
				item->last_message = strdup(" ");
			}
		}

		main_list_item = (tg_main_list_item_s*)malloc(sizeof(tg_main_list_item_s));
		main_list_item->peer_id = peer_info->peer_id;
		main_list_item->peer_type = peer_info->peer_type;

		set_peer_names(peer_info, main_list_item);

		main_list_item->last_seen_time = msg->date;
		main_list_item->profile_pic = NULL;
		main_list_item->last_msg_id = msg->msg_id;
		main_list_item->last_message = strdup(item->last_message);
		main_list_item->last_msg_type = msg->media_type;
		main_list_item->is_out_msg = msg->out;
		main_list_item->last_msg_status = msg->msg_state;
		main_list_item->last_msg_service = msg->service;
		main_list_item->number_of_unread_msgs = get_unread_message_count(tablename);

		if (peer_info->photo_path) {
			main_list_item->profile_pic_path = strdup(peer_info->photo_path);
		} else {
			main_list_item->profile_pic_path = NULL;
		}
		main_list_item->user_name_lbl = NULL;
		main_list_item->status_lbl = NULL;
		main_list_item->date_lbl = NULL;
		main_list_item->msg_status_lbl = NULL;
		main_list_item->main_item_layout = NULL;

		// delete message object
		if (msg->message) {
			free(msg->message);
			msg->message = NULL;
		}

		if (msg->media_id) {
			free(msg->media_id);
			msg->media_id = NULL;
		}

		free(msg);
		msg = NULL;

	} else {
		item->last_message = strdup(" ");
		if (peer_info->peer_type == TGL_PEER_CHAT) {
			main_list_item = (tg_main_list_item_s*)malloc(sizeof(tg_main_list_item_s));
			main_list_item->peer_id = peer_info->peer_id;
			main_list_item->peer_type = peer_info->peer_type;

			set_peer_names(peer_info, main_list_item);

			main_list_item->last_seen_time = peer_info->last_seen_time;
			main_list_item->profile_pic = NULL;
			main_list_item->last_message = strdup(item->last_message);
			main_list_item->last_msg_type = -1;
			main_list_item->last_msg_service = 0;
			main_list_item->is_out_msg = -1;
			main_list_item->last_msg_id = -1;
			main_list_item->last_msg_status = -1;
			main_list_item->number_of_unread_msgs = 0;

			if (peer_info->photo_path && strlen(peer_info->photo_path) > 0) {
				main_list_item->profile_pic_path = strdup(peer_info->photo_path);
			} else {
				main_list_item->profile_pic_path = NULL;
			}
			main_list_item->user_name_lbl = NULL;
			main_list_item->status_lbl = NULL;
			main_list_item->date_lbl = NULL;
			main_list_item->msg_status_lbl = NULL;
			main_list_item->main_item_layout = NULL;

		}
	}

	if (tablename) {
		free(tablename);
		tablename = NULL;
	}


	return main_list_item;
}

void app_nf_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	if (ad->loading_popup) {
		show_toast(ad, "Please wait. request in progress");
		return;
	}

	switch (ad->current_app_state) {
		case TG_INIT_SCREEN_STATE:
			elm_win_lower(ad->win);
			elm_exit();
			break;
		case TG_USER_MAIN_VIEW_STATE:
			if (!ad->menu_popup) {
				elm_win_lower(ad->win);
				elm_exit();
			}
			break;
		case TG_CHAT_MESSAGING_VIEW_STATE:
            // to be handled
			if (ad->main_item_in_cahtting_data) {
				int buddy_id = ad->main_item_in_cahtting_data->peer_id;
				char* tablename = get_table_name_from_number(buddy_id);

				Eina_Bool res = set_all_rows_read(tablename);
				if (!res) {
					//failed.
				}
				free(tablename);

				tg_main_list_item_s* old_item = ad->main_item_in_cahtting_data;
				ad->main_list = eina_list_remove(ad->main_list,  ad->main_item_in_cahtting_data);
				if (old_item->peer_print_name) {
					free(old_item->peer_print_name);
					old_item->peer_print_name = NULL;
				}
				if (old_item->last_message) {
					free(old_item->last_message);
					old_item->last_message = NULL;
				}
				if (old_item->profile_pic_path) {
					free(old_item->profile_pic_path);
					old_item->profile_pic_path = NULL;
				}
				if (old_item->buddy_display_name) {
					free(old_item->buddy_display_name);
					old_item->buddy_display_name = NULL;
				}

				old_item->date_lbl = NULL;
				old_item->msg_status_lbl = NULL;
				old_item->main_item_layout = NULL;
				old_item->profile_pic = NULL;
				old_item->profile_pic_path = NULL;
				old_item->status_lbl = NULL;
				old_item->user_name_lbl = NULL;
				ad->main_item_in_cahtting_data = NULL;
				if (ad->peer_in_cahtting_data) {
					peer_with_pic_s *item = ad->peer_in_cahtting_data;
					tg_main_list_item_s* latest_item = get_latest_item(ad, item);
					if (latest_item) {
						ad->main_list = eina_list_prepend(ad->main_list, latest_item);
					}
				}
				refresh_main_list_view(ad, EINA_FALSE);
			} else {
				if (ad->peer_in_cahtting_data) {
					peer_with_pic_s *item = ad->peer_in_cahtting_data;
					if (item) {
						int buddy_id = item->use_data->peer_id;
						char* tablename = get_table_name_from_number(buddy_id);
						Eina_Bool res = set_all_rows_read(tablename);
						if (!res) {
							//failed.
						}
						free(tablename);

						tg_peer_info_s* peer_info = item->use_data;
						if (peer_info) {
							//if (peer_info->last_msg_id > 0) {

							// get message from message table.

							char* tablename = get_table_name_from_number(peer_info->peer_id);
							//tg_message_s* msg = get_message_from_message_table(peer_info->last_msg_id, tablename);
							tg_message_s* msg = get_latest_message_from_message_table(tablename, EINA_FALSE);

							if (msg) {

								if (msg->service) {
									if (msg->message && strlen(msg->message) > 0) {
										item->last_message = strdup(msg->message);
									} else {
										item->last_message = strdup(" ");
									}
								} else {

									int media_type = msg->media_type;
									if (media_type == tgl_message_media_none) {
										if (msg->message && strlen(msg->message) > 0) {
											item->last_message = strdup(msg->message);
										} else {
											item->last_message = strdup(" ");
										}
									} else if (media_type == tgl_message_media_photo) {
										item->last_message = strdup("Image");
									} else if (media_type == tgl_message_media_document) {
										tgl_media_s *media_msg = NULL;
										media_msg = get_media_details_from_db(atoll(msg->media_id));

										if (media_msg && media_msg->doc_type) {
											if (strlen(media_msg->doc_type) > 0) {
												item->last_message = strdup(media_msg->doc_type);
											} else {
												item->last_message = strdup("Document");
											}
											free_media_details(media_msg);
										} else {
											if (msg->message && strlen(msg->message) > 0) {
												item->last_message = strdup(msg->message);
											} else {
												item->last_message = strdup("Document");
											}
										}

									} else if (media_type == tgl_message_media_geo) {
										item->last_message = strdup("Geo location");
									} else if (media_type == tgl_message_media_contact) {
										item->last_message = strdup("Contact");
									} else if (media_type == tgl_message_media_unsupported) {
										item->last_message = strdup(" ");
									} else if (media_type == tgl_message_media_photo_encr) {
										item->last_message = strdup("Image encrypted");
									} else if (media_type == tgl_message_media_document_encr) {
										item->last_message = strdup("Document encrypted");
									} else  {
										item->last_message = strdup(" ");
									}
								}

								tg_main_list_item_s* main_list_item = (tg_main_list_item_s*)malloc(sizeof(tg_main_list_item_s));
								main_list_item->peer_id = peer_info->peer_id;
								main_list_item->peer_type = peer_info->peer_type;

								set_peer_names(peer_info, main_list_item);

								main_list_item->last_seen_time = msg->date;
								main_list_item->profile_pic = NULL;
								main_list_item->last_msg_id = msg->msg_id;
								main_list_item->last_message = strdup(item->last_message);
								main_list_item->last_msg_type = msg->media_type;
								main_list_item->is_out_msg = msg->out;
								main_list_item->last_msg_status = msg->msg_state;
								main_list_item->last_msg_service = msg->service;
								main_list_item->number_of_unread_msgs = get_unread_message_count(tablename);

								if (peer_info->photo_path) {
									main_list_item->profile_pic_path = strdup(peer_info->photo_path);
								} else {
									main_list_item->profile_pic_path = NULL;
								}
								main_list_item->user_name_lbl = NULL;
								main_list_item->status_lbl = NULL;
								main_list_item->date_lbl = NULL;
								main_list_item->msg_status_lbl = NULL;
								main_list_item->main_item_layout = NULL;
								ad->main_list = eina_list_prepend(ad->main_list, main_list_item);

								// delete message object
								if (msg->message) {
									free(msg->message);
									msg->message = NULL;
								}

								if (msg->media_id) {
									free(msg->media_id);
									msg->media_id = NULL;
								}

								free(msg);
								msg = NULL;

							} else {
								item->last_message = strdup(" ");
								if (peer_info->peer_type == TGL_PEER_CHAT) {
									tg_main_list_item_s* main_list_item = (tg_main_list_item_s*)malloc(sizeof(tg_main_list_item_s));
									main_list_item->peer_id = peer_info->peer_id;
									main_list_item->peer_type = peer_info->peer_type;

									set_peer_names(peer_info, main_list_item);

									main_list_item->last_seen_time = peer_info->last_seen_time;
									main_list_item->profile_pic = NULL;
									main_list_item->last_message = strdup(item->last_message);
									main_list_item->last_msg_type = -1;
									main_list_item->last_msg_service = 0;
									main_list_item->is_out_msg = -1;
									main_list_item->last_msg_id = -1;
									main_list_item->last_msg_status = -1;
									main_list_item->number_of_unread_msgs = 0;

									if (peer_info->photo_path) {
										main_list_item->profile_pic_path = strdup(peer_info->photo_path);
									} else {
										main_list_item->profile_pic_path = NULL;
									}
									main_list_item->user_name_lbl = NULL;
									main_list_item->status_lbl = NULL;
									main_list_item->date_lbl = NULL;
									main_list_item->msg_status_lbl = NULL;
									main_list_item->main_item_layout = NULL;
									ad->main_list = eina_list_prepend(ad->main_list, main_list_item);
								}
							}

							if (tablename) {
								free(tablename);
								tablename = NULL;
							}
							//}
						}
					}
					refresh_main_list_view(ad, EINA_TRUE);
				}
			}

			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_USER_MAIN_VIEW_STATE;
			show_floating_button(ad);
			break;
		case TG_SELECT_BUDDY_VIEW:
			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_SET_CHAT_INFO_STATE;
			break;
		case TG_SET_USERNAME_STATE:
			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_SETTINGS_SCREEN_STATE;
			break;
#if 0
		case TG_ADD_CONTACT_STATE:
			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_SET_USER_INFO_STATE;
			break;
#endif
		case TG_SET_CHAT_INFO_STATE:
		case TG_SET_USER_INFO_STATE:
			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_CHAT_MESSAGING_VIEW_STATE;
			break;
		case TG_USER_MAIN_VIEW_SELECTION_STATE:
		case TG_PEER_SEARCH_VIEW_STATE:
		case TG_START_MESSAGING_VIEW_STATE:
		case TG_SETTINGS_SCREEN_STATE:
			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_USER_MAIN_VIEW_STATE;
			show_floating_button(ad);
			//evas_object_show(ad->panel);
			//elm_panel_hidden_set(ad->panel, EINA_FALSE);
			break;
		case TG_SETTINGS_EDIT_NAME_STATE:
			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_SETTINGS_SCREEN_STATE;
			delete_floating_button(ad);
			break;
		case TG_COUNTRY_SELECTION_VIEW:
			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_REGISTRATION_STATE;
			break;
		case TG_REGISTRATION_STATE:
			elm_win_lower(ad->win);
			elm_exit();
			break;
		case TG_LOGIN_STATE:
			send_request_for_restart_server(ad, ad->service_client);
			if (ad->timer_value > 0) {
				Ecore_Timer* timer = evas_object_data_get(ad->nf, "code_timer");
				if (timer)
					ecore_timer_del(timer);
			}

			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_REGISTRATION_STATE;
			break;
		case TG_PROFILE_REGISTRATION_STATE:
			/*			ad->current_app_state = TG_REGISTRATION_STATE;
						elm_naviframe_item_pop(ad->nf);*/
			elm_win_lower(ad->win);
			elm_exit();
			break;
		case TG_BUDDY_LIST_STATE:
			evas_object_data_set(ad->nf, "buddy_list", NULL);
			elm_win_lower(ad->win);
			elm_exit();
			break;
		case TG_BUDDY_CHAT_CONV_STATE:
			ad->buddy_in_cahtting_data = NULL;
			if (ad->loaded_msg_list) {
				eina_list_free(ad->loaded_msg_list);
				ad->loaded_msg_list = NULL;
			}
			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_BUDDY_LIST_STATE;
			//evas_object_show(ad->panel);
			//refresh_buddy_list(ad);
			break;
		case TG_ADD_CONTACT_STATE:
			elm_naviframe_item_pop(ad->nf);
			if (ad->is_loading_from_profile_view) {
				ad->is_loading_from_profile_view = EINA_FALSE;
				ad->current_app_state = TG_SET_USER_INFO_STATE;
			} else if (ad->is_loading_from_msg_view) {
				ad->is_loading_from_msg_view = EINA_FALSE;
				ad->current_app_state = TG_CHAT_MESSAGING_VIEW_STATE;
			} else {
				ad->current_app_state = TG_PEER_SEARCH_VIEW_STATE;
				show_floating_button(ad);
			}
			break;
		case TG_BUDDY_LIST_SELECTION_STATE:
		{
			Eina_List *l = NULL;
			user_data_with_pic_s *item = NULL;
			EINA_LIST_FOREACH(ad->buddy_list, l, item) {
				if (!item)
					continue;
				user_data_s* user = item->use_data;
				user->is_selected = EINA_FALSE;
			}
			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_PEER_SEARCH_VIEW_STATE;
		}
			break;
		case TG_GROUP_CHAT_NAME_ENTRY_STATE:
		{
			Eina_List *l = NULL;
			user_data_with_pic_s *item = NULL;
			EINA_LIST_FOREACH(ad->buddy_list, l, item) {
				if (!item)
					continue;

				user_data_s* user = item->use_data;
				user->is_selected = EINA_FALSE;
			}
			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_BUDDY_LIST_SELECTION_STATE;
			//evas_object_show(ad->panel);
			//elm_panel_hidden_set(ad->panel, EINA_FALSE);
			//refresh_buddy_list(ad);
		}
			break;
		default:
			break;
	}
}

void
layout_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

void _btn_clicked(void* data, Evas_Object* btn, void* ev)
{
	appdata_s *ad = data;
	bundle *msg = bundle_create();
	if (bundle_add_str(msg, "tizen_app", "Hello service") != 0) {
		ERR("Failed to add data by key to bundle");
		bundle_free(msg);
	}

	int result = _app_send_response(ad, msg);
	if (result != SVC_RES_OK) {
		bundle_free(msg);
	}
	bundle_free(msg);
}

Eina_Bool on_load_main_view_requested(void *data)
{
	appdata_s *ad = data;
	if (ad) {
		elm_naviframe_item_pop(ad->nf);
		hide_loading_popup(ad);
		ad->is_loading_from_msg_view = EINA_FALSE;
		ad->is_loading_from_profile_view = EINA_FALSE;
		launch_user_main_view_cb(ad);
		int unread_msg_cnt = get_number_of_unread_messages();
		if (unread_msg_cnt <= 0) {
			badge_set_count(TELEGRAM_APP_ID, 0);
			return ECORE_CALLBACK_CANCEL;
		}

		int err = badge_set_count(TELEGRAM_APP_ID, unread_msg_cnt);
		if (BADGE_ERROR_NONE != err) {

		}
	}
    return ECORE_CALLBACK_CANCEL;
}

Eina_Bool on_init_view_requested(void *data)
{
	appdata_s *ad = data;
	if (ad) {
		Eina_List *user_info = get_registered_user_info();
		if (!user_info) {
			elm_naviframe_item_pop(ad->nf);
			ad->current_app_state = TG_REGISTRATION_STATE;
			launch_init_screen(ad);
		} else {
			//show_toast(ad, "user already registered");
			load_registered_user_data(ad);
			load_buddy_list_data(ad);
			load_unknown_buddy_list_data(ad);
			load_peer_data(ad);
			load_main_list_data(ad);
			eina_list_free(user_info);
			ecore_timer_add(2, on_load_main_view_requested, ad);
		}
	}
    return ECORE_CALLBACK_CANCEL;
}

void on_tg_service_result_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	appdata_s *ad = user_data;
	if (result == APP_CONTROL_RESULT_SUCCEEDED) {
		if (ad) {
			show_toast(ad, "Server launched successfully.");
		} else {
			show_toast(ad, "Server not launched.");
		}
	}
}

void launch_tg_server(void *data)
{
	appdata_s *ad = data;
	app_control_h app_control = NULL;
	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_create(&app_control);
	ret_if (APP_CONTROL_ERROR_NONE != ret);

	ret = app_control_set_app_id(app_control, TELEGRAM_SERVER_APP_NAME);
	goto_if (APP_CONTROL_ERROR_NONE != ret, out);

	ret = app_control_send_launch_request(app_control, &on_tg_service_result_cb, ad);
	goto_if (APP_CONTROL_ERROR_NONE != ret, out);

out:
	app_control_destroy(app_control);
}

static void create_base_gui(appdata_s *ad)
{
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_conformant_set(ad->win, EINA_TRUE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	/* Telegram guideline 720 x 1280 */
	elm_app_base_scale_set(2.6);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[2] = {0, 180};
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), sizeof(rots) / sizeof(int));
	}
	ad->target_direction = tg_get_device_orientation();
	char edj_path[PATH_MAX] = {0, };
	app_get_resource(TELEGRAM_CUSTOM_WINSET_EDJ, edj_path, (int)PATH_MAX);

	Elm_Theme *theme = elm_theme_new();
	elm_theme_extension_add(NULL, edj_path);
	ad->theme = theme;
	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);

	ad->conform = elm_conformant_add(ad->win);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	ad->layout = elm_layout_add(ad->conform);
	evas_object_size_hint_weight_set(ad->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	//elm_layout_theme_set(ad->layout, "layout", "drawer", "panel");
	elm_layout_theme_set(ad->layout, "layout", "application", "default");
	evas_object_show(ad->layout);

	elm_object_content_set(ad->conform, ad->layout);
	ad->nf = elm_naviframe_add(ad->layout);

	ad->current_app_state = TG_SPLASH_SCREEN_STATE;
	launch_splash_screen_cb(ad);
	elm_object_part_content_set(ad->layout, "elm.swallow.content", ad->nf);
	eext_object_event_callback_add(ad->nf, EEXT_CALLBACK_BACK, app_nf_back_cb, ad);

	evas_object_show(ad->win);

	// check for user info

	//
	char *chat_bg = NULL;
	preference_get_string(TG_CHAT_BG_PREFERENCE, &chat_bg);
	if (chat_bg) {
		ad->chat_background = strdup(chat_bg);
	}

	if (!ad->chat_background) {
		ad->chat_background = strdup(ui_utils_get_resource(TG_CHAT_DEFAULT_BG));
		preference_set_string(TG_CHAT_BG_PREFERENCE, ad->chat_background);
	}
	launch_tg_server(ad);
	ecore_timer_add(5, on_init_view_requested, ad);
	ucol_init();
}


char *build_a_path(const char *path, const char *filename)
{
	char *ret;
	int len;

	len = strlen(path) + strlen(filename) + 2;
	ret = malloc(len);
	if (!ret) {
		return NULL;
	}

	snprintf(ret, len, "%s/%s", path, filename);
	return ret;
}

int remove_directory(const char *path)
{
	DIR *d = opendir(path);
	size_t path_len = strlen(path);
	int r = -1;

	if (d)
	{
		struct dirent *p;

		r = 0;

		while (!r && (p=readdir(d)))
		{
			int r2 = -1;
			char *buf;
			size_t len;

			/* Skip the names "." and ".." as we don't want to recurse on them. */
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
			{
				continue;
			}

			len = path_len + strlen(p->d_name) + 2;
			buf = malloc(len);

			if (buf)
			{
				struct stat statbuf;

				snprintf(buf, len, "%s/%s", path, p->d_name);

				if (!stat(buf, &statbuf))
				{
					if (S_ISDIR(statbuf.st_mode))
					{
						r2 = remove_directory(buf);
					}
					else
					{
						r2 = unlink(buf);
					}
				}

				free(buf);
			}

			r = r2;
		}

		closedir(d);
	}

	if (!r)
	{
		r = rmdir(path);
	}

	return r;
}

Eina_Bool copy_file_another_dir(const char *source_file, const char *target_file)
{
	FILE *source, *target;

	source = fopen(source_file, "rb+");

	if (source == NULL)
	{
		return EINA_FALSE;
	}

	target = fopen(target_file, "wb+");

	if (target == NULL)
	{
		fclose(source);
		return EINA_FALSE;
	}

	char buffer[1024] = {0};    /*Buffer to store files content*/

	int size = 0;
	while((size = fread(buffer, 1, 1024, source)) != 0)
	{
		fwrite(buffer, 1, size, target);
	}

	fclose(source);
	fclose(target);
	return EINA_TRUE;
}

void recursive_dir_copy(const char *source_dir, const char *target_dir)
{
	DIR *open_src;
	struct dirent *src_files;
	Eina_List *src_dir_stack;
	Eina_List *tar_dir_stack;

	char *src_dir;
	char *tar_dir;


	src_dir_stack = NULL;
	tar_dir_stack = NULL;

	src_dir = strdup(source_dir);
	tar_dir = strdup(target_dir);

	do {
		open_src = opendir(src_dir);
		while((src_files = readdir(open_src))) {
			if (src_files->d_type == DT_DIR) {
				// create directory
				// push to stack
				if (src_files->d_name[0] == '.' && (src_files->d_name[1] == '\0' || (src_files->d_name[1] == '.' && src_files->d_name[2] == '\0'))) {
					continue;
				}

				char *temp_dest = build_a_path(tar_dir, src_files->d_name);
				mkdir(temp_dest, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

				char *temp_src = build_a_path(src_dir, src_files->d_name);

				src_dir_stack = eina_list_prepend(src_dir_stack, temp_src);
				tar_dir_stack = eina_list_prepend(tar_dir_stack, temp_dest);

			} else {
				// copy file to target src_dir.
				char *temp_src = build_a_path(src_dir, src_files->d_name);
				char *temp_dest = build_a_path(tar_dir, src_files->d_name);
				//int ret = rename(temp_src, temp_dest);
				Eina_Bool ret = copy_file_another_dir(temp_src, temp_dest);
				if (ret) {
					// file copy success
				} else {
					// file copy failed.
				}
				free(temp_src);
				free(temp_dest);
			}
		}

		if (open_src) {
			closedir(open_src);
		}

		free(src_dir);
		free(tar_dir);
		src_dir = eina_list_nth(src_dir_stack, 0);
		if (src_dir) {
			src_dir_stack = eina_list_remove(src_dir_stack, src_dir);
		}
		tar_dir = eina_list_nth(tar_dir_stack, 0);
		if (tar_dir) {
			tar_dir_stack = eina_list_remove(tar_dir_stack, tar_dir);
		}
	} while (src_dir != NULL);
}

void move_downloaded_files()
{
	// create download directory.
	struct stat st = {0};
	char dest_download_dir[512] = {0,};
	sprintf(dest_download_dir, "%s/%s", app_get_shared_data_path(), "downloads");
	if (stat(dest_download_dir, &st) == -1) {
		mkdir(dest_download_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}

	LOGE("dest Download directory: %s", dest_download_dir);
	// create download directory.
	char target_download_dir[512] = {0,};
	sprintf(target_download_dir, "%s/%s/%s", app_get_data_path(), "telegram_tizen", "downloads");

	LOGE("target Download directory: %s", target_download_dir);
	recursive_dir_copy(target_download_dir, dest_download_dir);
	remove_directory(target_download_dir);
}


Eina_Bool dirExists(const char *path)
{
    struct stat info;

    if (stat(path, &info) != 0)
        return EINA_FALSE;
    else if (info.st_mode & S_IFDIR)
        return EINA_TRUE;
    else
        return EINA_FALSE;
}

static void update_downloaded_video_thumb_paths(const char *table_name, const char *col_one, const char *column_two)
{
	LOGE("Update media info table thumb files");
	if (!table_name || !col_one || !column_two)
		return;

	Eina_List *col_types = NULL;
	col_types = eina_list_append(col_types, TG_DB_COLUMN_INTEGER);
	col_types = eina_list_append(col_types, TG_DB_COLUMN_TEXT);

	Eina_List *col_names = NULL;
	col_names = eina_list_append(col_names, col_one);
	col_names = eina_list_append(col_names, column_two);

	Eina_List *vals = get_values_from_table_sync(table_name, col_names, col_types, NULL, TG_DBMGR_NOLIMITED, TG_DBMGR_NOLIMITED);
	int val_size = eina_list_count(vals);
	if (vals && val_size > 0) {
		Eina_List *row_vals = NULL;
		EINA_LIST_FREE(vals, row_vals) {
			int user_id = 0;
			char *file_path = NULL;

			int *temp_user_id = (int *)eina_list_nth(row_vals, 0);
			if (temp_user_id) {
				user_id  = *temp_user_id;
				free(temp_user_id);
			}

			file_path = (char *)eina_list_nth(row_vals, 1);

			if (!file_path) {
				eina_list_free(row_vals);
				continue;
			}

			if (strlen(file_path) > 0 && strncmp(file_path, "/opt/usr/media/telegram/telegram_tizen", strlen("/opt/usr/media/telegram/telegram_tizen")) == 0) {
				// update file name in db
				char *new_path = str_replace(new_path, "/opt/usr/media/telegram/telegram_tizen", app_get_shared_data_path());
				if (!new_path) {
					free(file_path);
					eina_list_free(row_vals);
					continue;
				}
				LOGE("Old file path: %s", file_path);
				LOGE("New file path: %s", new_path);

				//update database.
				Eina_List *new_col_types = NULL;
				new_col_types = eina_list_append(new_col_types, TG_DB_COLUMN_TEXT);

				Eina_List *new_col_names = NULL;
				new_col_names = eina_list_append(new_col_names, column_two);

				Eina_List *new_col_values = NULL;
				new_col_values = eina_list_append(new_col_values, new_path);

				char* where_clause = NULL;

				int str_size = strlen(col_one) + strlen(" = ") + 10 + 1;
				where_clause = (char *)malloc(str_size);
				snprintf(where_clause, str_size, "%s = %d", col_one, user_id);
				Eina_Bool ret = update_table(table_name, new_col_names, new_col_types, new_col_values, where_clause);
				if (!ret) {
					LOGE("Media info table update failed.");
				} else {
					LOGE("Media info table update success.");
				}
				free(where_clause);
				eina_list_free(new_col_types);
				eina_list_free(new_col_names);
				eina_list_free(new_col_values);
			}

			free(file_path);

			eina_list_free(row_vals);
		}
	}
	eina_list_free(col_types);
	eina_list_free(col_names);
}

void update_downloaded_video_thumb_paths_in_media_info_table()
{
	update_downloaded_video_thumb_paths(MEDIA_INFO_TABLE_NAME, MEDIA_INFO_TABLE_MEDIA_ID, MEDIA_INFO_TABLE_DOCUMENT_THUMB_FILE);
}


void update_downloaded_file_paths_in_media_info_table()
{
	update_downloaded_video_thumb_paths(MEDIA_INFO_TABLE_NAME, MEDIA_INFO_TABLE_MEDIA_ID, MEDIA_INFO_TABLE_FILE_PATH);
}


void update_downloaded_file_paths_in_chat_info_table()
{
	update_downloaded_video_thumb_paths(CHAT_INFO_TABLE_NAME, CHAT_INFO_TABLE_CHAT_ID, CHAT_INFO_TABLE_PHOTO_PATH);

}

void update_downloaded_file_paths_in_buddy_info_table(const char *table_name)
{
	LOGE("Update %s", table_name);
	update_downloaded_video_thumb_paths(table_name, USER_INFO_TABLE_USER_ID, USER_INFO_TABLE_PHOTO_PATH);
}

static bool app_create(void *data)
{
	/*
	   Hook to take necessary actions before main event loop starts
	   Initialize UI resources and application's data
	   If this function returns true, the main loop of application starts
	   If this function returns false, the application is terminated
	 */

	appdata_s *ad = data;
	if (dirExists(OLD_DIR)) {
		char *new_dir = app_get_data_path();
		recursive_dir_copy(OLD_DIR, new_dir);
		remove_directory(OLD_DIR);
		move_downloaded_files();
		// update downloaded image paths in DB
		update_downloaded_file_paths_in_buddy_info_table(BUDDY_INFO_TABLE_NAME);
		update_downloaded_file_paths_in_buddy_info_table(USER_INFO_TABLE_NAME);
		update_downloaded_file_paths_in_buddy_info_table(PEER_INFO_TABLE_NAME);
		update_downloaded_file_paths_in_chat_info_table();
		update_downloaded_file_paths_in_media_info_table();
		update_downloaded_video_thumb_paths_in_media_info_table();
	}

	tg_db_init();
	bindtextdomain(PACKAGE, "/opt/usr/apps/org.tizen.telegram_tizen/res/locale");

	ad->phone_number = NULL;
	ad->buddy_list = NULL;
	ad->unknown_buddy_list = NULL;
	ad->known_buddy_list = NULL;
	ad->main_list = NULL;
	ad->peer_list = NULL;
	ad->search_peer_list = NULL;
	ad->is_first_time_registration = EINA_FALSE;
	ad->panel = NULL;
	ad->loaded_msg_list = NULL;
	ad->loading_popup = NULL;
	ad->current_user_data = NULL;
	ad->is_tg_initilized = EINA_FALSE;
	ad->chat_background = NULL;
	ad->msg_popup = NULL;
	ad->s_notififcation = NULL;
	ad->panel = NULL;
	ad->is_server_ready = EINA_FALSE;
	ad->contact_list = NULL;
	ad->main_item = NULL;
	ad->country_codes_list = NULL;
	ad->country_names_list = NULL;
	//ad->msg_count = 0;
	create_base_gui(ad);
	int err = badge_new(TELEGRAM_APP_ID);
	if (BADGE_ERROR_NONE != err) {

	}
	init_service(ad);
	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void
app_pause(void *data)
{
	appdata_s *app_data = data;
	if (app_data) {
		app_data->s_app_visible_state = APP_STATE_IN_BACKGROUND;
		int unread_msg_cnt = get_number_of_unread_messages();
		if (unread_msg_cnt <= 0) {
			badge_set_count(TELEGRAM_APP_ID, 0);
			return;
		}
		int err = badge_set_count(TELEGRAM_APP_ID, unread_msg_cnt);
		if (BADGE_ERROR_NONE != err) {

		}
	}
}

static void
app_resume(void *data)
{
	appdata_s *app_data = data;
	if (app_data) {
		app_data->s_app_visible_state = APP_STATE_IN_FOREGROUND;
	}
}


void free_app_data(appdata_s *app_data, Eina_Bool destroy_server)
{
	if (!app_data) {
		return;
	}

	int unread_msg_cnt = get_number_of_unread_messages();
	if (unread_msg_cnt > 0) {
		int err = badge_set_count(TELEGRAM_APP_ID, unread_msg_cnt);
		if (BADGE_ERROR_NONE != err) {

		}
	} else {
		badge_set_count(TELEGRAM_APP_ID, 0);
	}
	eina_list_free(app_data->country_codes_list);
	eina_list_free(app_data->country_names_list);
	free(app_data->country_code_buffer);

	if (app_data->panel) {
		Evas_Object *panel_list = evas_object_data_get(app_data->panel, "panel_list");
		if (panel_list) {
			evas_object_del(panel_list);
		}
		evas_object_del(app_data->panel);
		app_data->panel = NULL;
	}

	if (app_data->phone_number) {
		free(app_data->phone_number);
		app_data->phone_number = NULL;
	}

	if (app_data->sms_code) {
		free(app_data->sms_code);
		app_data->sms_code = NULL;
	}

	if (app_data->current_user_data) {
		free_user_data(app_data->current_user_data);
	}
	if (app_data->chat_background) {
		app_data->chat_background = NULL;
	}
	if (app_data && app_data->s_notififcation) {
		notification_delete_all(NOTIFICATION_TYPE_NOTI);
		app_data->s_notififcation = NULL;
	}

	peer_with_pic_s* pic_item = NULL;
	EINA_LIST_FREE(app_data->peer_list, pic_item) {
		if (!pic_item)
			continue;
		tg_peer_info_s* item = pic_item->use_data;
		if (item) {
			if (item->print_name) {
				free(item->print_name);
				item->print_name = NULL;
			}
			if (item->photo_path) {
				free(item->photo_path);
				item->photo_path = NULL;
			}
			pic_item->contact_icon = NULL;
			pic_item->msg_object = NULL;
			pic_item->name_object = NULL;
			free(item);
		}
		free(pic_item);
	}
	app_data->peer_list = NULL;

	tg_main_list_item_s* main_item = NULL;
	EINA_LIST_FREE(app_data->main_list, main_item) {

		if (main_item->peer_print_name) {
			free(main_item->peer_print_name);
			main_item->peer_print_name = NULL;
		}
		if (main_item->last_message) {
			free(main_item->last_message);
			main_item->last_message = NULL;
		}
		if (main_item->profile_pic_path) {
			free(main_item->profile_pic_path);
			main_item->profile_pic_path = NULL;
		}
		if (main_item->buddy_display_name) {
			free(main_item->buddy_display_name);
			main_item->buddy_display_name = NULL;
		}
		main_item->date_lbl = NULL;
		main_item->msg_status_lbl = NULL;
		main_item->main_item_layout = NULL;
		main_item->profile_pic = NULL;
		main_item->profile_pic_path = NULL;
		main_item->status_lbl = NULL;
		main_item->user_name_lbl = NULL;
	}
	app_data->main_list = NULL;

	user_data_with_pic_s* item = NULL;
	EINA_LIST_FREE(app_data->buddy_list, item) {
		if (!item)
			continue;
		user_data_s* user_data = item->use_data;

		if (user_data->print_name) {
			free(user_data->print_name);
			user_data->print_name = NULL;
		}

		if (user_data->photo_path) {
			free(user_data->photo_path);
			user_data->photo_path = NULL;
		}

		if (user_data->first_name) {
			free(user_data->first_name);
			user_data->first_name = NULL;
		}

		if (user_data->last_name) {
			free(user_data->last_name);
			user_data->last_name = NULL;
		}

		if (user_data->phone) {
			free(user_data->phone);
			user_data->phone = NULL;
		}
		if (user_data->real_first_name) {
			free(user_data->real_first_name);
			user_data->real_first_name = NULL;
		}

		if (user_data->real_last_name) {
			free(user_data->real_last_name);
			user_data->real_last_name = NULL;
		}

		if (user_data->username) {
			free(user_data->username);
			user_data->username = NULL;
		}
		free(user_data);
		free(item);
	}
	app_data->buddy_list = NULL;


	EINA_LIST_FREE(app_data->unknown_buddy_list, item) {

		if (!item)
			continue;

		user_data_s* user_data = item->use_data;

		if (user_data->print_name) {
			free(user_data->print_name);
			user_data->print_name = NULL;
		}

		if (user_data->photo_path) {
			free(user_data->photo_path);
			user_data->photo_path = NULL;
		}

		if (user_data->first_name) {
			free(user_data->first_name);
			user_data->first_name = NULL;
		}

		if (user_data->last_name) {
			free(user_data->last_name);
			user_data->last_name = NULL;
		}

		if (user_data->phone) {
			free(user_data->phone);
			user_data->phone = NULL;
		}
		if (user_data->real_first_name) {
			free(user_data->real_first_name);
			user_data->real_first_name = NULL;
		}

		if (user_data->real_last_name) {
			free(user_data->real_last_name);
			user_data->real_last_name = NULL;
		}

		if (user_data->username) {
			free(user_data->username);
			user_data->username = NULL;
		}
		free(user_data);
		free(item);
	}
	app_data->unknown_buddy_list = NULL;

	EINA_LIST_FREE(app_data->known_buddy_list, item) {
		user_data_s* user_data = item->use_data;

		if (user_data->print_name)
			free(user_data->print_name);
			user_data->print_name = NULL;

		if (user_data->photo_path)
			free(user_data->photo_path);
			user_data->photo_path = NULL;

		if (user_data->first_name)
			free(user_data->first_name);
			user_data->first_name = NULL;

		if (user_data->last_name)
			free(user_data->last_name);
			user_data->last_name = NULL;

		if (user_data->phone)
			free(user_data->phone);
			user_data->phone = NULL;

		if (user_data->real_first_name)
			free(user_data->real_first_name);
			user_data->real_first_name = NULL;

		if (user_data->real_last_name)
			free(user_data->real_last_name);
			user_data->real_last_name = NULL;

		if (user_data->username)
			free(user_data->username);
			user_data->username = NULL;

		free(user_data);
		free(item);
	}
	app_data->known_buddy_list = NULL;

	eina_list_free(app_data->loaded_msg_list);
	app_data->loaded_msg_list = NULL;

	if (app_data->service_client && destroy_server) {
		service_client_destroy(app_data->service_client);
		app_data->service_client = NULL;
	}
	free_contact_list(app_data->contact_list);
	tg_db_fini();
}

static void
app_terminate(void *data)
{
	free_app_data(data, EINA_TRUE);
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	if (locale) {
		setlocale(LC_ALL, locale);
		elm_language_set(locale);
	}
	free(locale);
	return;
}


TelegramAppDirection tg_get_device_orientation()
{
	TelegramAppDirection target_direction;

	switch (app_get_device_orientation()) {
	case APP_DEVICE_ORIENTATION_90:
		target_direction = TELEGRAM_TARGET_DIRECTION_LANDSCAPE_INVERSE;
		break;
	case APP_DEVICE_ORIENTATION_180:
		target_direction = TELEGRAM_TARGET_DIRECTION_PORTRAIT_INVERSE;
		break;
	case APP_DEVICE_ORIENTATION_270:
		target_direction = TELEGRAM_TARGET_DIRECTION_LANDSCAPE;
		break;
	case APP_DEVICE_ORIENTATION_0:
	default:
		target_direction = TELEGRAM_TARGET_DIRECTION_PORTRAIT;
		break;
	}

	return target_direction;
}

void update_current_view(appdata_s *ad)
{
	if (!ad) {
		return;
	}
	switch (ad->current_app_state) {
	case TG_USER_MAIN_VIEW_STATE:
		refresh_main_list_view(ad, EINA_FALSE);
		break;
	case TG_CHAT_MESSAGING_VIEW_STATE:
		refresh_messaging_view(ad);
		break;
	default:
		break;
	}

}


static void ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	appdata_s *ad = user_data;
	if (!ad) {
		return;
	}
	TelegramAppDirection target_direction;
	app_device_orientation_e orientation;

	if (app_event_get_device_orientation(event_info, &orientation) != 0) {
		return;
	}

	switch (orientation) {
	case APP_DEVICE_ORIENTATION_90:
		target_direction = TELEGRAM_TARGET_DIRECTION_LANDSCAPE_INVERSE;
		break;
	case APP_DEVICE_ORIENTATION_180:
		target_direction = TELEGRAM_TARGET_DIRECTION_PORTRAIT_INVERSE;
		break;
	case APP_DEVICE_ORIENTATION_270:
		target_direction = TELEGRAM_TARGET_DIRECTION_LANDSCAPE;
		break;
	case APP_DEVICE_ORIENTATION_0:
	default:
		target_direction = TELEGRAM_TARGET_DIRECTION_PORTRAIT;
		break;
	}

	if (target_direction != ad->target_direction) {
		ad->target_direction = target_direction;
		update_current_view(ad);
	}

	return;
}

static void ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
}

static void ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
}

static void ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
}

int main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);
	ui_app_remove_event_handler(handlers[APP_EVENT_LOW_MEMORY]);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "ui_app_main() is failed. err = %d", ret);
	}

	return ret;
}
