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
#include "conn-presence.h"
static const TpPresenceStatusSpec _statuses[] = {
    { "offline", TP_CONNECTION_PRESENCE_TYPE_OFFLINE, FALSE, NULL },
    { "invisible", TP_CONNECTION_PRESENCE_TYPE_HIDDEN, TRUE, NULL },
    { "away", TP_CONNECTION_PRESENCE_TYPE_AWAY, TRUE, NULL },
    { "busy", TP_CONNECTION_PRESENCE_TYPE_BUSY, TRUE, NULL },
    { "available", TP_CONNECTION_PRESENCE_TYPE_AVAILABLE, TRUE, NULL },
    { NULL }
};

    const TpPresenceStatusSpec *
fetion_contact_list_presence_statuses (void)
{
    return _statuses;
}

    static gboolean
status_available (GObject *object,
        guint index_)
{
    TpBaseConnection *base = TP_BASE_CONNECTION (object);

    if (base->status != TP_CONNECTION_STATUS_CONNECTED)
        return FALSE;

    return TRUE;
}

    static GHashTable *
get_contact_statuses (GObject *object,
        const GArray *contacts,
        GError **error)
{
    FetionConnection *self =
        FETION_CONNECTION (object);
    TpBaseConnection *base = TP_BASE_CONNECTION (object);
    guint i;
    GHashTable *result = g_hash_table_new_full (g_direct_hash, g_direct_equal,
            NULL, (GDestroyNotify) tp_presence_status_free);

    for (i = 0; i < contacts->len; i++)
    {
        TpHandle contact = g_array_index (contacts, guint, i);
        HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(self->priv->account, contact);
        GHashTable *parameters;
        GValue *message;
        guint presence;
        const gchar *presence_message;
        /* we get our own status from the connection, and everyone else's status
         * from the contact lists */
        if (contact == base->self_handle)
        {
            presence = self->priv->account->state;
            presence_message = self->priv->account->status_text;
        }
        else
        {
            presence = buddy->state;
            presence_message = buddy->mood;
        }

        parameters = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
                (GDestroyNotify) tp_g_value_slice_free);

        if (presence_message != NULL)
        {    
            message = tp_g_value_slice_new (G_TYPE_STRING);
            g_value_set_static_string (message, presence_message);
            g_hash_table_insert (parameters, "message", message);
        }    

        g_hash_table_insert (result, GUINT_TO_POINTER (contact),
                tp_presence_status_new (presence, parameters)); 
        g_hash_table_destroy (parameters);
    }

    return result;
}

    static gboolean
set_own_status (GObject *object,
        const TpPresenceStatus *status,
        GError **error)
{
    FetionConnection *self =
        FETION_CONNECTION (object);
    TpBaseConnection *base = TP_BASE_CONNECTION (object);
    HybridAccount* account = self->priv->account;
    GHashTable *presences;
    GValue *message ;

    if (status->optional_arguments != NULL)
    {    
        message = g_hash_table_lookup (status->optional_arguments, "message");

        if (message != NULL)
            account->proto->info->im_ops->modify_status(account,g_value_get_string(message));
    }  


    if (self->priv->account->state == status->index)
        return TRUE;

    if (status->index == 0)
        hybrid_account_close(account);
    else
    {
        hybrid_account_set_state(account,status->index);
        if(!account->proto->info->im_ops->change_state(account,status->index))
            return FALSE;
    }

    presences = g_hash_table_new_full (g_direct_hash, g_direct_equal,
            NULL, NULL);
    g_hash_table_insert (presences, GUINT_TO_POINTER (base->self_handle),
            (gpointer) status);
    tp_presence_mixin_emit_presence_update (object, presences);
    g_hash_table_destroy (presences);
    return TRUE;
}

    static guint
openfetion_get_maximum_status_message_length_cb (GObject *obj)
{
    return 512;
}

    void
conn_presence_class_init (FetionConnectionClass  *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    TpPresenceMixinClass *mixin_class;
    tp_presence_mixin_class_init (object_class,
            G_STRUCT_OFFSET (FetionConnectionClass, presence_mixin),
            status_available, get_contact_statuses, set_own_status,
            fetion_contact_list_presence_statuses ());
    mixin_class = TP_PRESENCE_MIXIN_CLASS(klass);
    mixin_class->get_maximum_status_message_length =
        openfetion_get_maximum_status_message_length_cb;

    tp_presence_mixin_simple_presence_init_dbus_properties (object_class);
}

    static void 
presence_updated_cb( FetionConnection *conn,
        TpHandle contact, gpointer user_data)
{
    TpBaseConnection *base = (TpBaseConnection *) conn;
    guint presence;
    const gchar *presence_message;
    if (contact == base->self_handle)
    {
        presence = conn->priv->account->state;
        presence_message = conn->priv->account->status_text;
    }
    else
    {
        HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(conn->priv->account, contact);
        if(buddy == NULL)
            return;
        presence = buddy->state;
        presence_message = buddy->mood;
    }
    GHashTable *parameters;
    GValue *message;
    parameters = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
            (GDestroyNotify) tp_g_value_slice_free);

    message = tp_g_value_slice_new (G_TYPE_STRING);
    g_value_set_static_string (message, presence_message);
    g_hash_table_insert (parameters, "message", message);

    TpPresenceStatus* status = tp_presence_status_new (presence, parameters);
    tp_presence_mixin_emit_one_presence_update ((GObject *)base,
            contact, status);
    g_hash_table_destroy (parameters);
    tp_presence_status_free (status);
}

    void
conn_presence_init(GObject *object)
{
    g_signal_connect (object,
            "presence-updated", (GCallback) presence_updated_cb, NULL);
    tp_presence_mixin_init (object,
            G_STRUCT_OFFSET (FetionConnection, presence_mixin));
    tp_presence_mixin_simple_presence_register_with_contacts_mixin (object);
}
