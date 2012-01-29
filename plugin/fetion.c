/***************************************************************************
 *   Copyright (C) 2011 by levin                                           *
 *   levin108@gmail.com                                                    *
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

#include <glib.h>
#include "util.h"
#include "account.h"
#include "module.h"
#include "blist.h"

#include "fetion.h"
#include "fx_trans.h"
#include "fx_login.h"
#include "fx_account.h"
#include "fx_group.h"
#include "fx_buddy.h"
#include "fx_msg.h"
#include "fx_util.h"

fetion_account *ac;

extern GSList *channel_list;

static gint
update_info_cb(fetion_account *ac, const gchar *sipmsg, fetion_transaction *trans)
{
    HybridBuddy      *hybrid_buddy;
    fetion_buddy     *buddy;

    if (!(buddy = fetion_buddy_parse_info(ac, trans->userid, sipmsg))) {
        return HYBRID_ERROR;
    }

    if ((hybrid_buddy = hybrid_blist_find_buddy(ac->account, buddy->userid))) {
        if(buddy->nickname && *buddy->nickname != '\0')
            hybrid_blist_set_buddy_name(hybrid_buddy, buddy->nickname);
    }

    return HYBRID_OK;
}

/**
 * Process "presence changed" message.
 */
static void
process_presence(fetion_account *ac, const gchar *sipmsg)
{
    GSList       *list;
    GSList       *pos;
    fetion_buddy *buddy;
    HybridBuddy  *imbuddy;

    list = sip_parse_presence(ac, sipmsg);

    for (pos = list; pos; pos = pos->next) {

        buddy   = (fetion_buddy*)pos->data;
        imbuddy = hybrid_blist_find_buddy(ac->account, buddy->userid);

	if(!imbuddy->name || *imbuddy->name=='\0')
	{
		if (buddy->localname && *(buddy->localname) != '\0') {
			hybrid_blist_set_buddy_name(imbuddy, buddy->localname);
		}else if (buddy->nickname && *(buddy->nickname) != '\0')
			hybrid_blist_set_buddy_name(imbuddy, buddy->nickname);
		else
			fetion_buddy_get_info(ac, buddy->userid, update_info_cb, NULL);
	}

        hybrid_blist_set_buddy_mood(imbuddy, buddy->mood_phrase);

        switch (buddy->state) {
            case P_ONLINE:
                hybrid_blist_set_buddy_state(imbuddy, HYBRID_STATE_ONLINE);
                break;
            case P_OFFLINE:
                hybrid_blist_set_buddy_state(imbuddy, HYBRID_STATE_OFFLINE);
                break;
            case P_INVISIBLE:
                hybrid_blist_set_buddy_state(imbuddy, HYBRID_STATE_OFFLINE);
                break;
            case P_AWAY:
                hybrid_blist_set_buddy_state(imbuddy, HYBRID_STATE_AWAY);
                break;
            case P_BUSY:
                hybrid_blist_set_buddy_state(imbuddy, HYBRID_STATE_BUSY);
                break;
            default:
                hybrid_blist_set_buddy_state(imbuddy, HYBRID_STATE_AWAY);
                break;
        }

        fetion_update_portrait(ac, buddy);
    }
}

/**
 * Process deregister message. When the same account logins
 * at somewhere else, this message will be received.
 */
static void
process_dereg_cb(fetion_account *ac, const gchar *sipmsg)
{
    hybrid_account_error_reason(ac->account,TP_ERROR_DISCONNECTED,
                                _("Your account has logined elsewhere."
                                  "You are forced to quit."));
}

/**
 * Process the user left message, we close the current channel,
 * and remove the session from the session list.
 */
static void
process_left_cb(fetion_account *ac, const gchar *sipmsg)
{
    extern GSList *channel_list;
    GSList        *pos;

    g_return_if_fail(ac != NULL);
    g_return_if_fail(sipmsg != NULL);

    hybrid_debug_info("fetion", "buddy left, recv:\n%s", sipmsg);

    for (pos = channel_list; pos; pos = pos->next) {
        if (pos->data == ac) {
            channel_list = g_slist_remove(channel_list, ac);

            goto channel_removed;
        }
    }

    hybrid_debug_error("fetion", "FATAL, can't find channel");

    return;

channel_removed:
    /* remove the read event source. */
    g_source_remove(ac->source);

    /* close the channel. */
    close(ac->sk);

    /* destroy the account. */
    fetion_account_destroy(ac);
}

/**
 * Process the user entered message. The fetion prococol dont allow us
 * to send messages to an online buddy who's conversation channel is not
 * ready, so before chating with an online buddy, we should first start
 * a new conversation channel, and invite the buddy to the conversation,
 * but when the channel is ready? yes, when we got the User-Entered
 * message through the new channel established, the channel is ready!
 */
static void
process_enter_cb(fetion_account *ac, const gchar *sipmsg)
{
    GSList             *pos;
    fetion_transaction *trans;

    g_return_if_fail(ac != NULL);
    g_return_if_fail(sipmsg != NULL);

    hybrid_debug_info("fetion", "user entered:\n%s", sipmsg);

    /* Set the channel's ready flag. */
    ac->channel_ready = TRUE;

    /* Check the transaction waiting list, wakeup the sleeping transaction,
     * the transaction is either sending a SMS or sending a nudge, we got
     * the information from the transaction context, and restore the transaction. */
    while (ac->trans_wait_list) {
        pos = ac->trans_wait_list;
        trans = (fetion_transaction*)pos->data;

        if (trans->msg && *(trans->msg) != '\0') {
            fetion_message_send(ac, trans->userid, trans->msg);
        }

        transaction_wakeup(ac, trans);
    }
}

/**
 * Process the synchronization message, when the contact list or the personal info
 * changed, the server will push this message to tell the client to update its
 * local cache file, well, we will not update the local cache file, we keep the
 * old version numbers, and reload it after the next logining.
 */
static void
process_sync_info(fetion_account *ac, const gchar *sipmsg)
{
    GSList       *list;
    fetion_buddy *buddy;
    HybridBuddy  *hb;

    hybrid_debug_info("fetion", "sync info,recv:\n%s", sipmsg);

    if (!(list = sip_parse_sync(ac, sipmsg))) {
        return;
    }

    while (list) {
        buddy = (fetion_buddy*)list->data;

        list = g_slist_remove(list, buddy);

        if (buddy->status == 0) {
            continue;
        }

        if (!(hb = hybrid_blist_find_buddy(ac->account, buddy->userid))) {
            continue;
        }

        if (buddy->status == 1) {

            hybrid_blist_set_buddy_status(hb, TRUE);

        } else {
            hybrid_blist_set_buddy_status(hb, FALSE);
        }

    }

}

static gboolean
fx_buddy_req(HybridAccount *account, HybridGroup *group,
             const gchar *id, const gchar *alias, gboolean accept, const gpointer user_data);

static void
process_add_buddy(fetion_account *ac, const gchar *sipmsg)
{
    gchar                *sipuri;
    gchar                *userid;
    gchar                *desc;

    hybrid_debug_info("fetion", "received add-buddy request:\n%s", sipmsg);

    if (sip_parse_appbuddy(sipmsg, &userid, &sipuri, &desc) != HYBRID_OK) {
        return;
    }

    // TODO add buddy request always receive it here.
    fx_buddy_req(ac->account, hybrid_blist_find_group(ac->account, "0"),
            userid, desc, TRUE, sipuri);

    g_free(sipuri);
    g_free(userid);
    g_free(desc);
}

/**
 * Process notification routine.
 */
static void
process_notify_cb(fetion_account *ac, const gchar *sipmsg)
{
    gint notify_type;
    gint event_type;

    sip_parse_notify(sipmsg, &notify_type, &event_type);

    if (notify_type == NOTIFICATION_TYPE_UNKNOWN ||
            event_type == NOTIFICATION_EVENT_UNKNOWN) {

        hybrid_debug_info("fetion", "recv unknown notification:\n%s", sipmsg);
        return;
    }

    switch (notify_type) {

        case NOTIFICATION_TYPE_PRESENCE:
            if (event_type == NOTIFICATION_EVENT_PRESENCECHANGED) {
                process_presence(ac, sipmsg);
            }
            break;

        case NOTIFICATION_TYPE_CONVERSATION :
            if (event_type == NOTIFICATION_EVENT_USERLEFT) {
                process_left_cb(ac, sipmsg);
                break;

            } else     if (event_type == NOTIFICATION_EVENT_USERENTER) {
                process_enter_cb(ac, sipmsg);
                break;
            }
            break;

        case NOTIFICATION_TYPE_REGISTRATION :
            if (event_type == NOTIFICATION_EVENT_DEREGISTRATION) {
                process_dereg_cb(ac, sipmsg);
            }
            break;

        case NOTIFICATION_TYPE_SYNCUSERINFO :
            if (event_type == NOTIFICATION_EVENT_SYNCUSERINFO) {
                process_sync_info(ac, sipmsg);
            }
            break;

        case NOTIFICATION_TYPE_CONTACT :
            if (event_type == NOTIFICATION_EVENT_ADDBUDDYAPPLICATION) {
                process_add_buddy(ac, sipmsg);
            }
            break;
#if 0
        case NOTIFICATION_TYPE_PGGROUP :
            break;
#endif
        default:
            break;
    }
}

/**
 * Process the sip response message.
 */
static void
process_sipc_cb(fetion_account *ac, const gchar *sipmsg)
{
    gchar              *callid;
    gint                callid0;
    fetion_transaction *trans;
    GSList             *trans_cur;

    if (!(callid = sip_header_get_attr(sipmsg, "I"))) {
        hybrid_debug_error("fetion", "invalid sipc message received\n%s",
                sipmsg);
        g_free(callid);
        return;
    }

    callid0 = atoi(callid);

    trans_cur = ac->trans_list;

    while(trans_cur) {
        trans = (fetion_transaction*)(trans_cur->data);

        if (trans->callid == callid0) {

            if (trans->callback) {
                (trans->callback)(ac, sipmsg, trans);
            }

            transaction_remove(ac, trans);

            break;
        }

        trans_cur = g_slist_next(trans_cur);
    }
}

/**
 * Process the message sip message.
 */
static void
process_message_cb(fetion_account *ac, const gchar *sipmsg)
{
    gchar        *event;
    gchar        *sysmsg_text;
    gchar        *sysmsg_url;
    //HybridNotify *notify;

    if ((event = sip_header_get_attr(sipmsg, "N")) &&
            g_strcmp0(event, "system-message") == 0) {
        if (fetion_message_parse_sysmsg(sipmsg, &sysmsg_text,
                    &sysmsg_url) != HYBRID_OK) {
            return;
        }

     //   notify = hybrid_notify_create(ac->account, _("System Message"));
      //  hybrid_notify_set_text(notify, sysmsg_text);

        g_free(sysmsg_text);
        g_free(sysmsg_url);
    }

    hybrid_debug_info("fetion", "received message:\n%s", sipmsg);

    fetion_process_message(ac, sipmsg);

    g_free(event);
}

/**
 * Process the invitation message.
 */
static void
process_invite_cb(fetion_account *ac, const gchar *sipmsg)
{
    hybrid_debug_info("fetion", "invitation message recv:\n%s", sipmsg);

    fetion_process_invite(ac, sipmsg);
}

/**
 * Process the pushed message.
 */
void
process_pushed(fetion_account *ac, const gchar *sipmsg)
{
    gint type;

    type = fetion_sip_get_msg_type(sipmsg);

    switch (type) {
        case SIP_NOTIFICATION :
            process_notify_cb(ac, sipmsg);
            break;
        case SIP_MESSAGE:
            process_message_cb(ac, sipmsg);
            break;
        case SIP_INVITATION:
            process_invite_cb(ac, sipmsg);
            break;
        case SIP_INFO:
            //process_info_cb(ac, sipmsg);
            break;
        case SIP_SIPC_4_0:
            process_sipc_cb(ac, sipmsg);
            break;
        default:
            hybrid_debug_info("fetion", "recevie unknown msg:\n%s", sipmsg);
            break;
    }
}

static gboolean
fx_login(HybridAccount *imac)
{
    HybridSslConnection *conn;

    hybrid_debug_info("fetion", "fetion is now logging in...");

    ac = fetion_account_create(imac, imac->username, imac->password);

    hybrid_account_set_protocol_data(imac, ac);

    conn = hybrid_ssl_connect(SSI_SERVER, 443, ssi_auth_action, ac);
    if(!conn->conn)
	    hybrid_account_error_reason(imac, TP_ERROR_NETWORK_ERROR ,_("Network error"));

    return TRUE;
}

static gboolean
fx_change_state(HybridAccount *account, gint state)
{
    fetion_account *ac;
    gint            fetion_state;

    ac = hybrid_account_get_protocol_data(account);

    switch(state) {
        case HYBRID_STATE_ONLINE:
            fetion_state = P_ONLINE;
            break;
        case HYBRID_STATE_AWAY:
            fetion_state = P_AWAY;
            break;
        case HYBRID_STATE_BUSY:
            fetion_state = P_BUSY;
            break;
        case HYBRID_STATE_INVISIBLE:
            fetion_state = P_INVISIBLE;
            break;
        default:
            fetion_state = P_ONLINE;
            break;
    }

    if (fetion_account_update_state(ac, fetion_state) != HYBRID_OK) {
        return FALSE;
    }

    return TRUE;
}

static gboolean
fx_keep_alive(HybridAccount *account)
{
    fetion_account *ac;
    GSList         *pos;

    ac = hybrid_account_get_protocol_data(account);

    if (fetion_account_keep_alive(ac) != HYBRID_OK) {
        return FALSE;
    }

    for (pos = channel_list; pos; pos = pos->next) {
        ac = (fetion_account*)pos->data;
        fetion_account_keep_alive(ac);
    }

    return TRUE;
}

static gboolean
fx_buddy_move(HybridAccount *account, HybridBuddy *buddy,
        HybridGroup *new_group)
{
    fetion_account *ac;

    ac = hybrid_account_get_protocol_data(account);

    fetion_buddy_move_to(ac, buddy->id, new_group->id);
    return TRUE;
}

static gboolean
fx_modify_name(HybridAccount *account, const gchar *text)
{
    fetion_account *ac;

    ac = hybrid_account_get_protocol_data(account);

    if (fetion_account_modify_name(ac, text) != HYBRID_OK) {
        return FALSE;
    }

    return TRUE;
}

static gboolean
fx_modify_status(HybridAccount *account, const gchar *text)
{
    fetion_account *ac;

    ac = hybrid_account_get_protocol_data(account);

    if (fetion_account_modify_status(ac, text) != HYBRID_OK) {
        return FALSE;
    }

    return TRUE;
}

static gboolean
fx_remove(HybridAccount *account, HybridBuddy *buddy)
{
    fetion_account *ac;

    ac = hybrid_account_get_protocol_data(account);

    if (fetion_buddy_remove(ac, buddy->id) != HYBRID_OK) {
        return FALSE;
    }

    return TRUE;
}

static gboolean
fx_rename(HybridAccount *account, HybridBuddy *buddy, const gchar *text)
{
    fetion_account *ac;

    ac = hybrid_account_get_protocol_data(account);

    if (fetion_buddy_rename(ac, buddy->id, text) != HYBRID_OK) {
        return FALSE;
    }

    return TRUE;
}

static gboolean
fx_buddy_req(HybridAccount *account, HybridGroup *group,
             const gchar *id, const gchar *alias,
             gboolean accept, const gpointer user_data)
{
    fetion_account *ac;
    const gchar    *sipuri;

    ac = hybrid_account_get_protocol_data(account);

    sipuri = (const gchar*)user_data;

    fetion_buddy_handle_request(ac, sipuri, id, alias, group->id, accept);

    return TRUE;
}

static gboolean
fx_buddy_add(HybridAccount *account, HybridGroup *group, const gchar *name,
             const gchar *alias, const gchar *tips)
{
    fetion_account *ac;

    ac = hybrid_account_get_protocol_data(account);

    if (fetion_buddy_add(ac, group->id, name, alias) != HYBRID_OK) {
        return FALSE;
    }

    return TRUE;
}

static gboolean
fx_group_rename(HybridAccount *account, HybridGroup *group, const gchar *text)
{
    fetion_account *ac;

    ac = hybrid_account_get_protocol_data(account);

    if (fetion_group_edit(ac, group->id, text) != HYBRID_OK) {
        return FALSE;
    }

    return TRUE;
}

static gboolean
fx_group_remove(HybridAccount *account, HybridGroup *group)
{
    fetion_account *ac;

    ac = hybrid_account_get_protocol_data(account);

    if (fetion_group_remove(ac, group->id) != HYBRID_OK) {
        return FALSE;
    }

    return TRUE;
}

static void
fx_group_add(HybridAccount *account, const gchar *text)
{
    fetion_account *ac;

    ac = hybrid_account_get_protocol_data(account);

    fetion_group_add(ac, text);
}

static gint
fx_chat_word_limit(HybridAccount *account)
{
    return 180;
}

static gboolean
fx_chat_start(HybridAccount *account, HybridBuddy *buddy)
{
    ac = hybrid_account_get_protocol_data(account);

    if (!hybrid_blist_get_buddy_authorized(buddy)) {

        return FALSE;
    }

    return TRUE;
}

static void
fx_chat_send(HybridAccount *account, HybridBuddy *buddy, const gchar *text)
{
    fetion_account *ac;
    fetion_account *tmp_ac;
    extern GSList  *channel_list;
    GSList         *pos;

    ac = hybrid_account_get_protocol_data(account);

    if (BUDDY_IS_OFFLINE(buddy) || BUDDY_IS_INVISIBLE(buddy)) {
        fetion_message_send(ac, buddy->id, text);

    } else {
        /*
         * If the buddy's state is greater than 0, then we should
         * invite the buddy first to start a new socket channel,
         * then we can send the message through the new channel.
         * Now, we check whether a channel related to this buddy has
         * been established, if so, just send the message through
         * the existing one, otherwise, start a new channel.
         */
        for (pos = channel_list; pos; pos = pos->next) {
            tmp_ac = (fetion_account*)pos->data;

            if (g_strcmp0(tmp_ac->who, buddy->id) == 0) {
                /* yes, we got one. */
                fetion_message_send(tmp_ac, tmp_ac->who, text);

                return;
            }
        }

        fetion_message_new_chat(ac, buddy->id, text);
    }
}

static void
fx_close(HybridAccount *account)
{
    GSList         *pos;
    fetion_account *ac;
    fetion_buddy   *buddy;
    fetion_group   *group;

    ac = hybrid_account_get_protocol_data(account);

    /* close the socket */
    if (ac->source) {
        hybrid_event_remove(ac->source);
    }

    if (ac->sk) {
        close(ac->sk);
    }

    /* destroy the group list */
    while (ac->groups) {
        pos = ac->groups;
        group = (fetion_group*)pos->data;
        ac->groups = g_slist_remove(ac->groups, group);
        fetion_group_destroy(group);
    }

    /* destroy the buddy list */
    while (ac->buddies) {
        pos = ac->buddies;
        buddy = (fetion_buddy*)pos->data;
        ac->buddies = g_slist_remove(ac->buddies, buddy);
        fetion_buddy_destroy(buddy);
    }
}

HybridIMOps im_ops = {
    fx_login,                   /**< login */
    NULL,                /**< get_info */
    fx_modify_name,             /**< modify_name */
    fx_modify_status,           /**< modify_status */
    NULL,                       /**< modify_photo */
    fx_change_state,            /**< change_state */
    fx_keep_alive,              /**< keep_alive */
//  fx_account_tooltip,         /**< account_tooltip */
//  fx_buddy_tooltip,           /**< buddy_tooltip */
    fx_buddy_move,              /**< buddy_move */
    fx_remove,                  /**< buddy_remove */
    fx_rename,                  /**< buddy_rename */
    fx_buddy_add,               /**< buddy_add */
    fx_buddy_req,               /**< buddy_req */
    fx_group_rename,            /**< group_rename */
    fx_group_remove,            /**< group_remove */
    fx_group_add,               /**< group_add */
    fx_chat_word_limit,         /**< chat_word_limit */
    fx_chat_start,              /**< chat_start */
    NULL,                       /**< chat_send_typing */
    fx_chat_send,               /**< chat_send */
    fx_close,                   /**< close */
};

HybridModuleInfo module_info = {
    "fetion",                   /**< name */
    "levin108",                 /**< author */
    N_("fetion client"),        /**< summary */
    /* description */
    N_("hybrid plugin implementing Fetion Protocol version 4"),
    "http://basiccoder.com",      /**< homepage */
    "0","1",                    /**< major version, minor version */
    "fetion",                   /**< icon name */
    MODULE_TYPE_IM,
    &im_ops,
    NULL,
    NULL,             /**< actions */
    NULL,
};

void
fetion_module_init(HybridModule *module)
{

}

HYBRID_MODULE_INIT(fetion_module_init, &module_info);


void fetion_module_register(HybridModule *module)
{
        module->info = &module_info;
        fetion_module_init(module); 
        hybrid_module_register(module);
	if (module->info->options) {
		module->option_list = module->info->options();
	}   
}

