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

#ifndef TG_DB_WRAPPER_H_
#define TG_DB_WRAPPER_H_
#include "tg_db_manager.h"
#include "tg_common.h"

#define TG_SETTINGS_TABLE_NAME "tg_settings_table"
#define TG_SETTINGS_ROW_ID "row_id"
#define TG_SETTINGS_CHAT_BG "chat_bg"

#define MESSAGE_TRANSPORT_TABLE_NAME "message_transport_table"

#define MESSAGE_TRANSPORT_TABLE_ROW_ID "row_id"
#define MESSAGE_TRANSPORT_TABLE_APP_NAME "app_name"
#define MESSAGE_TRANSPORT_TABLE_COMMAND "command"
#define MESSAGE_TRANSPORT_TABLE_BUDDY_ID "buddy_id"
#define MESSAGE_TRANSPORT_TABLE_MESSAGE_ID "message_id"
#define MESSAGE_TRANSPORT_TABLE_MESSAGE_TYPE "message_type"
#define MESSAGE_TRANSPORT_TABLE_MESSAGE_DATA "message_data"
#define MESSAGE_TRANSPORT_TABLE_TYPE_OF_CHAT "type_of_chat"

#define MEDIA_TRANSPORT_TABLE_NAME "media_transport_table"

#define MEDIA_TRANSPORT_TABLE_ROW_ID "row_id"
#define MEDIA_TRANSPORT_TABLE_APP_NAME "app_name"
#define MEDIA_TRANSPORT_TABLE_COMMAND "command"
#define MEDIA_TRANSPORT_TABLE_BUDDY_ID "buddy_id"
#define MEDIA_TRANSPORT_TABLE_MESSAGE_ID "message_id"
#define MEDIA_TRANSPORT_TABLE_MEDIA_ID "media_id"
#define MEDIA_TRANSPORT_TABLE_MESSAGE_TYPE "message_type"
#define MEDIA_TRANSPORT_TABLE_FILE_PATH "file_path"
#define MEDIA_TRANSPORT_TABLE_TYPE_OF_CHAT "type_of_chat"



#define USER_INFO_TABLE_NAME "user_info_table"

#define USER_INFO_TABLE_USER_ID "user_id"
#define USER_INFO_TABLE_PRINT_NAME "print_name"
#define USER_INFO_TABLE_STRUCTURE_VERSION "structure_version"
#define USER_INFO_TABLE_PHOTO_PATH "photo_path"
#define USER_INFO_TABLE_PHOTO_ID "photo_id"
#define USER_INFO_TABLE_FIRST_NAME "first_name"
#define USER_INFO_TABLE_LAST_NAME "last_name"
#define USER_INFO_TABLE_PHONE_NO "phone"
#define USER_INFO_TABLE_ACCESS_HASH "access_hash"
#define USER_INFO_TABLE_REAL_FIRST_NAME "real_first_name"
#define USER_INFO_TABLE_REAL_LAST_NAME "real_last_name"
#define USER_INFO_TABLE_USER_NAME "username"
#define USER_INFO_TABLE_ONLINE_STATUS "online"
#define USER_INFO_TABLE_LAST_SEEN_TIME "last_seen"
#define USER_INFO_TABLE_IS_BLOCKED "is_blocked"
#define USER_INFO_TABLE_IS_DELETED "is_deleted"
#define USER_INFO_TABLE_IS_UNKNOWN_PEER "is_unknown"

#define BUDDY_INFO_TABLE_NAME "buddy_info_table"
#if 0
#define BUDDY_INFO_TABLE_BUDDY_ID "user_id"
#define BUDDY_INFO_TABLE_PRINT_NAME "print_name"
#define BUDDY_INFO_TABLE_STRUCTURE_VERSION "structure_version"
#define BUDDY_INFO_TABLE_PHOTO_PATH "photo_path"
#define BUDDY_INFO_TABLE_PHOTO_ID "photo_id"
#define BUDDY_INFO_TABLE_FIRST_NAME "first_name"
#define BUDDY_INFO_TABLE_LAST_NAME "last_name"
#define BUDDY_INFO_TABLE_PHONE_NO "phone"
#define BUDDY_INFO_TABLE_ACCESS_HASH "access_hash"
#define BUDDY_INFO_TABLE_REAL_FIRST_NAME "real_first_name"
#define BUDDY_INFO_TABLE_REAL_LAST_NAME "real_last_name"
#define BUDDY_INFO_TABLE_BUDDY_NAME "username"
#define BUDDY_INFO_TABLE_ONLINE_STATUS "online"
#define BUDDY_INFO_TABLE_LAST_SEEN_TIME "last_seen"
#endif
#define MEDIA_INFO_TABLE_NAME "media_info_table"

#define MEDIA_INFO_TABLE_MEDIA_ID "media_id"
#define MEDIA_INFO_TABLE_MEDIA_TYPE "media_type"
#define MEDIA_INFO_TABLE_ACCESS_HASH "access_hash"
#define MEDIA_INFO_TABLE_USER_ID "user_id"
#define MEDIA_INFO_TABLE_DATE "date"
#define MEDIA_INFO_TABLE_CAPTION "caption"
#define MEDIA_INFO_TABLE_LONGITUDE "longitude"
#define MEDIA_INFO_TABLE_LATITUDE "latitude"
#define MEDIA_INFO_TABLE_SIZES_NUM "sizes_num"

#define MEDIA_INFO_TABLE_PHOTO_TYPE1 "photo_type1" //text
#define MEDIA_INFO_TABLE_PHOTO_LOC_DC1 "photo_loc_dc1" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_VOL1 "photo_loc_vol1" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_ID1 "photo_loc_id1" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_SECRET1 "photo_loc_secret1" //int
#define MEDIA_INFO_TABLE_PHOTO_WIDTH1 "photo_width1" //int
#define MEDIA_INFO_TABLE_PHOTO_HEIGHT1 "photo_height1" //int
#define MEDIA_INFO_TABLE_PHOTO_SIZE1 "photo_size1" //int
#define MEDIA_INFO_TABLE_PHOTO_DATA1 "photo_data1" //text

#define MEDIA_INFO_TABLE_PHOTO_TYPE2 "photo_type2" //text
#define MEDIA_INFO_TABLE_PHOTO_LOC_DC2 "photo_loc_dc2" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_VOL2 "photo_loc_vol2" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_ID2 "photo_loc_id2" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_SECRET2 "photo_loc_secret2" //int
#define MEDIA_INFO_TABLE_PHOTO_WIDTH2 "photo_width2" //int
#define MEDIA_INFO_TABLE_PHOTO_HEIGHT2 "photo_height2" //int
#define MEDIA_INFO_TABLE_PHOTO_SIZE2 "photo_size2" //int
#define MEDIA_INFO_TABLE_PHOTO_DATA2 "photo_data2" //text

#define MEDIA_INFO_TABLE_PHOTO_TYPE3 "photo_type3" //text
#define MEDIA_INFO_TABLE_PHOTO_LOC_DC3 "photo_loc_dc3" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_VOL3 "photo_loc_vol3" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_ID3 "photo_loc_id3" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_SECRET3 "photo_loc_secret3" //int
#define MEDIA_INFO_TABLE_PHOTO_WIDTH3 "photo_width3" //int
#define MEDIA_INFO_TABLE_PHOTO_HEIGHT3 "photo_height3" //int
#define MEDIA_INFO_TABLE_PHOTO_SIZE3 "photo_size3" //int
#define MEDIA_INFO_TABLE_PHOTO_DATA3 "photo_data3" //text

#define MEDIA_INFO_TABLE_PHOTO_TYPE4 "photo_type4" //text
#define MEDIA_INFO_TABLE_PHOTO_LOC_DC4 "photo_loc_dc4" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_VOL4 "photo_loc_vol4" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_ID4 "photo_loc_id4" //int
#define MEDIA_INFO_TABLE_PHOTO_LOC_SECRET4 "photo_loc_secret4" //int
#define MEDIA_INFO_TABLE_PHOTO_WIDTH4 "photo_width4" //int
#define MEDIA_INFO_TABLE_PHOTO_HEIGHT4 "photo_height4" //int
#define MEDIA_INFO_TABLE_PHOTO_SIZE4 "photo_size4" //int
#define MEDIA_INFO_TABLE_PHOTO_DATA4 "photo_data4" //text

#define MEDIA_INFO_TABLE_PHONE_NO "phone"
#define MEDIA_INFO_TABLE_FIRST_NAME "first_name"
#define MEDIA_INFO_TABLE_LAST_NAME "last_name"
#define MEDIA_INFO_TABLE_FILE_PATH "file_path"

#define MEDIA_INFO_TABLE_MIME_TYPE "mime_type" //text
#define MEDIA_INFO_TABLE_DOCUMENT_TYPE "doc_type" //text
#define MEDIA_INFO_TABLE_DOCUMENT_WIDTH "doc_width" //text
#define MEDIA_INFO_TABLE_DOCUMENT_HEIGHT "doc_height" //text
#define MEDIA_INFO_TABLE_DOCUMENT_DURATION "doc_duration" //text
#define MEDIA_INFO_TABLE_DOCUMENT_SIZE "doc_size" //text
#define MEDIA_INFO_TABLE_DOCUMENT_DC "doc_dc" //text
#define MEDIA_INFO_TABLE_DOCUMENT_THUMB_FILE "video_thumb" //text

//#define MESSAGE_INFO_TABLE_MESSAGE_ROW_ID "msg_row_id"
#define MESSAGE_INFO_TABLE_MESSAGE_ID "msg_id"
#define MESSAGE_INFO_TABLE_FLAGS "flags"
#define MESSAGE_INFO_TABLE_FWD_FROM_ID "fwd_from_id"
#define MESSAGE_INFO_TABLE_FWD_DATE "fwd_date"
#define MESSAGE_INFO_TABLE_FROM_ID "from_id"
#define MESSAGE_INFO_TABLE_TO_ID "to_id"
#define MESSAGE_INFO_TABLE_OUT_MSG "out"
#define MESSAGE_INFO_TABLE_UNREAD "unread"
#define MESSAGE_INFO_TABLE_DATE "date"
#define MESSAGE_INFO_TABLE_SERVICE "service"
#define MESSAGE_INFO_TABLE_MESSAGE "message"
#define MESSAGE_INFO_TABLE_MESSAGE_STATE "message_state"
#define MESSAGE_INFO_TABLE_MESSAGE_LENGTH "message_len"
#define MESSAGE_INFO_TABLE_MEDIA_TYPE "media_type"
#define MESSAGE_INFO_TABLE_MEDIA_ID "media_id"
#define MESSAGE_INFO_TABLE_UNIQUE_ID "unique_id"
#define MESSAGE_INFO_TABLE_MARKED_FOR_DELETE "is_marked_delete"

#define CHAT_INFO_TABLE_NAME "chat_info_table"

#define CHAT_INFO_TABLE_CHAT_ID "chat_id"
#define CHAT_INFO_TABLE_FLAGS "flags"
#define CHAT_INFO_TABLE_PRINT_TITLE "print_title"
#define CHAT_INFO_TABLE_STRUCT_VERSION "struct_version"
#define CHAT_INFO_TABLE_PHOTO_ID "photo_id"
#define CHAT_INFO_TABLE_PHOTO_PATH "photo_path"
#define CHAT_INFO_TABLE_TITLE "title"
#define CHAT_INFO_TABLE_NO_OF_USERS "users_num"
#define CHAT_INFO_TABLE_USER_LIST_SIZE "user_list_size"
#define CHAT_INFO_TABLE_USER_LIST_VERSION "user_list_version"
#define CHAT_INFO_TABLE_INVITER_ID "inviter_id"
#define CHAT_INFO_TABLE_CHAT_USERS "chat_users"
#define CHAT_INFO_TABLE_DATE "date"
#define CHAT_INFO_TABLE_VERSION "version"
#define CHAT_INFO_TABLE_ADMIN_ID "admin_id"


#define PEER_INFO_TABLE_NAME "peer_info_table" //user or group chat or encrypted chat

#define PEER_INFO_TABLE_CHAT_ID "user_id"
#define PEER_INFO_TABLE_PEER_TYPE "peer_type"
#define PEER_INFO_TABLE_FLAGS "flags"
#define PEER_INFO_TABLE_LAST_MESSAGE_ID "message_id"
#define PEER_INFO_TABLE_LAST_MESSAGE_DATE "message_date"
#define PEER_INFO_TABLE_PRINT_NAME "print_name"
#define PEER_INFO_TABLE_STRUCT_VERSION "struct_version"
#define PEER_INFO_TABLE_NO_OF_UNREAD_MESSAGES "unread_message_count"
#define PEER_INFO_TABLE_LAST_SEEN_TIME "last_seen"

#define TG_DB_COLUMN_INTEGER "INTEGER"
#define TG_DB_COLUMN_INTEGER_PRIMARY_KEY "INTEGER PRIMARY KEY NOT NULL"
#define TG_DB_COLUMN_INTEGER_PRIMARY_AUTO_INC_KEY "INTEGER PRIMARY KEY   AUTOINCREMENT"
#define TG_DB_COLUMN_TEXT "TEXT"


void create_data_base_tables();

Eina_List* get_registered_user_info();

Eina_List* get_buddy_list_info();

Eina_List* get_buddy_info(int buddy_id);

char* get_profile_pic_path(int buddy_id);

tg_message_s* get_message_from_message_table(long long msg_id, char* table_name);

char* get_video_thumb_path_from_db(long long media_id);

char* get_media_path_from_db(long long media_id);

tg_chat_info_s* get_chat_info(int chat_id);

Eina_List* get_group_chat_details();

Eina_List* get_all_peer_details(char* start_name);

peer_with_pic_s* get_peer_info(int peer_id);

void insert_or_update_peer_into_database(tg_peer_info_s* UC);

void insert_msg_into_db(tg_message_s *M, char* table_name, int unique_id);
void insert_media_info_to_db(tg_message_s *M, char *file_path, int width, int height, int size, char *latitude, char *longitude, char *first_name, char *last_name, char *phone_number);

Eina_List* get_image_details_from_db(long long media_id);
Eina_List* get_image_sizes_from_db(long long media_id);

tg_message_s* get_latest_message_from_message_table(char* table_name, Eina_Bool is_for_date);
int get_unread_message_count(char* table_name);

extern void update_msg_into_db(tg_message_s *M, char* table_name);

extern Eina_Bool set_all_rows_read(const char *table_name);

void get_buddy_contact_details_from_db(int buddy_id, char **first_name, char **last_name, char **phone_number);

char* get_buddy_name_from_id(int buddy_id);

char* get_buddy_phone_num_from_id(int buddy_id);

int  get_buddy_unknown_status(int buddy_id);

Eina_List* get_unknown_buddy_list_info();

int  get_buddy_online_status(int buddy_id);

int  get_buddy_delete_status(int buddy_id);

int  get_buddy_block_status(int buddy_id);

int insert_current_date_to_table(char* tb_name);

Eina_List* load_peer_data_by_name(char* name);

tgl_media_s* get_media_details_from_db(long long media_id);

void free_media_details(tgl_media_s *media_msg);

int get_media_size_from_db(long long media_id);

int get_number_of_unread_messages();

Eina_List* load_buddy_data_by_name(int current_user, char* name);
Eina_List* get_all_buddy_details(char* name);

void create_buddy_msg_table(const char* table_name);

Eina_Bool mark_all_records_for_deletion(char *tablename);

Eina_Bool delete_date_messages_from_table(char *tablename);

Eina_Bool is_phone_number_exists_in_buddy_list(const char *phone_num);

Eina_Bool insert_unsent_message_to_db(const char *app_name, const char *command, const char *buddy_id, const char *msg_id, const char *msg_type, const char * msg_data, const char *type_of_chat);
Eina_Bool insert_unsent_media_message_to_db(const char *app_name, const char *command, const char *buddy_id, const char *msg_id, const char *media_id, const char *msg_type, const char *file_path, const char *type_of_chat);
#if 0
void set_chat_bg(char *file_path);
char* get_chat_bg();
#endif
#endif /* TG_DB_WRAPPER_H_ */
