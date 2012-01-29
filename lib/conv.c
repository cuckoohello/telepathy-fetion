/***************************************************************************
 *   Copyright (C) 2012 by cuckoo                                          *
 *                                                                         *
 *   cuckoohello@gmail.com                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.            *
 ***************************************************************************/

#include "conv.h"
#include "module.h"
#include "connection.h"
#include <sys/time.h>

    void
hybrid_conv_got_message(HybridAccount *account,
        const gchar *buddy_id, const gchar *message,
        time_t recv_time)
{
    HybridBuddy        *buddy;

    g_return_if_fail(account != NULL);
    g_return_if_fail(buddy_id != NULL);
    g_return_if_fail(message != NULL);

    if (!(buddy = hybrid_blist_find_buddy(account, buddy_id))) {
        hybrid_debug_error("conv", "buddy doesn't exist.");
        return;
    }
    TpHandle contact =  tp_handle_lookup(buddy->account->contact_repo, buddy->id, NULL,NULL);
    g_signal_emit(account->conn,signals[MESSAGE_RECEIVED],0,contact,hybrid_strip_html(message));
}

    void
hybrid_conv_send_message(HybridAccount *account,HybridBuddy *buddy,const gchar *message)
{
    g_return_if_fail(account != NULL);
    g_return_if_fail(buddy != NULL);
    g_return_if_fail(message != NULL);

    if (*message == '\0') {
        /* Yes, nothing was input, just return. */
        return;
    }

    HybridIMOps   *ops    = account->proto->info->im_ops;

    if (ops->chat_send) {
        ops->chat_send(account, buddy, message);
    }
}
