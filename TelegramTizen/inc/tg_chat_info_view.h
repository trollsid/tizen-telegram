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

#ifndef TG_CHAT_INFO_VIEW_H_
#define TG_CHAT_INFO_VIEW_H_

#include "tg_common.h"

extern void launch_chat_info_screen(appdata_s* ad, int peer_id);
extern void on_group_chat_info_updated(appdata_s *ad, char *type_of_change);
extern void on_group_chat_deleted(appdata_s *ad, int chat_id);

#endif /* TG_CHAT_INFO_VIEW_H_ */
