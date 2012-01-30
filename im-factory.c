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
#include "im-factory.h"
#include <dbus/dbus-glib.h>
#include <telepathy-glib/telepathy-glib.h>
#include "im-channel.h"
#include "connection.h"

static void channel_manager_iface_init (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (FetionImFactory,
        fetion_im_factory,
        G_TYPE_OBJECT,
        G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_MANAGER,
            channel_manager_iface_init))

enum
{
    PROP_CONNECTION = 1,
    N_PROPS
};

    static void
fetion_im_factory_init (FetionImFactory *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, FETION_TYPE_IM_FACTORY,
            FetionImFactoryPrivate);

    self->priv->channels = g_hash_table_new_full (g_direct_hash, g_direct_equal,
            NULL, g_object_unref);
}

static void fetion_im_factory_close_all (FetionImFactory *self);

    static void
dispose (GObject *object)
{
    FetionImFactory *self = FETION_IM_FACTORY (object);

    fetion_im_factory_close_all (self);
    g_assert (self->priv->channels == NULL);

    ((GObjectClass *) fetion_im_factory_parent_class)->dispose (object);
}

    static void
get_property (GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec)
{
    FetionImFactory *self = FETION_IM_FACTORY (object);

    switch (property_id)
    {
        case PROP_CONNECTION:
            g_value_set_object (value, self->priv->conn);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

    static void
set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec)
{
    FetionImFactory *self = FETION_IM_FACTORY (object);

    switch (property_id)
    {
        case PROP_CONNECTION:
            /* We don't ref the connection, because it owns a reference to the
             * channel manager, and it guarantees that the manager's lifetime is
             * less than its lifetime */
            self->priv->conn = g_value_get_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

    static void
status_changed_cb (TpBaseConnection *conn,
        guint status,
        guint reason,
        FetionImFactory *self)
{
    if (status == TP_CONNECTION_STATUS_DISCONNECTED)
        fetion_im_factory_close_all (self);
}

FetionImChannel* new_channel (FetionImFactory *self,
        TpHandle handle, TpHandle initiator, gpointer request_token);

    static void 
message_received_cb(TpBaseConnection *conn ,
        TpHandle contact,const gchar *message,FetionImFactory *im_factory )
{
    FetionImChannel *chan = g_hash_table_lookup (im_factory->priv->channels, GUINT_TO_POINTER (contact));
    if(chan == NULL)
        chan = new_channel (im_factory,contact,contact,NULL);

    TpMessage * received =tp_cm_message_new_text(conn,
            contact,TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL,message);

    tp_message_set_int64(received,0,"message-sent",time(NULL));
    tp_message_set_int64(received,0,"message-received",time(NULL));

    tp_message_mixin_take_received (G_OBJECT(chan), received);
}

    static void
constructed (GObject *object)
{
    FetionImFactory *self = FETION_IM_FACTORY (object);
    void (*chain_up) (GObject *) =
        ((GObjectClass *) fetion_im_factory_parent_class)->constructed;

    if (chain_up != NULL)
    {
        chain_up (object);
    }

    self->priv->status_changed_id = g_signal_connect (self->priv->conn,
            "status-changed", (GCallback) status_changed_cb, self);
    self->priv->message_received_id = g_signal_connect(self->priv->conn,"message-received",(GCallback)message_received_cb,self);
}

    static void
fetion_im_factory_class_init (FetionImFactoryClass *klass)
{
    GParamSpec *param_spec;
    GObjectClass *object_class = (GObjectClass *) klass;

    object_class->constructed = constructed;
    object_class->dispose = dispose;
    object_class->get_property = get_property;
    object_class->set_property = set_property;

    param_spec = g_param_spec_object ("connection", "Connection object",
            "The connection that owns this channel manager",
            TP_TYPE_BASE_CONNECTION,
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
            G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB);
    g_object_class_install_property (object_class, PROP_CONNECTION, param_spec);

    g_type_class_add_private (klass, sizeof (FetionImFactoryPrivate));

}

    static void
fetion_im_factory_close_all (FetionImFactory *self)
{
    if (self->priv->channels != NULL)
    {
        GHashTable *tmp = self->priv->channels;

        self->priv->channels = NULL;
        g_hash_table_destroy (tmp);
    }

    if (self->priv->status_changed_id != 0)
    {
        g_signal_handler_disconnect (self->priv->conn,
                self->priv->status_changed_id);
        self->priv->status_changed_id = 0;
    }

    if (self->priv->message_received_id != 0)
    {
        g_signal_handler_disconnect (self,
                self->priv->message_received_id);
        self->priv->message_received_id = 0;
    }
}

    static void
fetion_im_factory_foreach_channel (TpChannelManager *iface,
        TpExportableChannelFunc callback,
        gpointer user_data)
{
    FetionImFactory *self = FETION_IM_FACTORY (iface);
    GHashTableIter iter;
    gpointer handle, channel;

    g_hash_table_iter_init (&iter, self->priv->channels);

    while (g_hash_table_iter_next (&iter, &handle, &channel))
    {
        callback (TP_EXPORTABLE_CHANNEL (channel), user_data);
    }
}

    static void
channel_closed_cb (FetionImChannel *chan,
        FetionImFactory *self)
{
    tp_channel_manager_emit_channel_closed_for_object (self,
            TP_EXPORTABLE_CHANNEL (chan));

    if (self->priv->channels != NULL)
    {
        TpHandle handle;
        gboolean really_destroyed;

        g_object_get (chan,
                "handle", &handle,
                "channel-destroyed", &really_destroyed,
                NULL);

        /* Re-announce the channel if it's not yet ready to go away (pending
         * messages) */
        if (really_destroyed)
        {
            g_hash_table_remove (self->priv->channels,
                    GUINT_TO_POINTER (handle));
        }
        else
        {
            tp_channel_manager_emit_new_channel (self,
                    TP_EXPORTABLE_CHANNEL (chan), NULL);
        }
    }
}

    FetionImChannel*
new_channel (FetionImFactory *self,
        TpHandle handle,
        TpHandle initiator,
        gpointer request_token)
{
    FetionImChannel *chan;
    gchar *object_path;
    GSList *requests = NULL;

    g_return_val_if_fail (handle != 0, NULL);
    g_return_val_if_fail (initiator != 0, NULL);

    object_path = g_strdup_printf ("%s/FetionImChannel%u",
            self->priv->conn->object_path, handle);

    chan = g_object_new (FETION_TYPE_IM_CHANNEL,
            "connection", self->priv->conn,
            "object-path", object_path,
            "handle", handle,
            "initiator-handle", initiator,
            NULL);

    g_free (object_path);

    g_signal_connect (chan, "closed", (GCallback) channel_closed_cb, self);

    g_hash_table_insert (self->priv->channels, GUINT_TO_POINTER (handle), chan);

    if (request_token != NULL)
        requests = g_slist_prepend (requests, request_token);

    tp_channel_manager_emit_new_channel (self, TP_EXPORTABLE_CHANNEL (chan),
            requests);
    g_slist_free (requests);

    return chan;
}


static const gchar * const fixed_properties[] = {
    TP_PROP_CHANNEL_CHANNEL_TYPE,
    TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
    NULL
};

static const gchar * const allowed_properties[] = {
    TP_PROP_CHANNEL_TARGET_HANDLE,
    TP_PROP_CHANNEL_TARGET_ID,
    NULL
};

    static void
fetion_im_factory_type_foreach_channel_class (GType type,
        TpChannelManagerTypeChannelClassFunc func,
        gpointer user_data)
{
    GHashTable *table = tp_asv_new (
            TP_PROP_CHANNEL_CHANNEL_TYPE,
            G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
            TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_CONTACT,
            NULL);

    func (type, table, allowed_properties, user_data);

    g_hash_table_destroy (table);
}

    static gboolean
fetion_im_factory_request (FetionImFactory *self,
        gpointer request_token,
        GHashTable *request_properties,
        gboolean require_new)
{
    TpHandle handle;
    FetionImChannel *chan;
    GError *error = NULL;

    if (tp_strdiff (tp_asv_get_string (request_properties,
                    TP_PROP_CHANNEL_CHANNEL_TYPE),
                TP_IFACE_CHANNEL_TYPE_TEXT))
    {
        return FALSE;
    }

    if (tp_asv_get_uint32 (request_properties,
                TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, NULL) != TP_HANDLE_TYPE_CONTACT)
    {
        return FALSE;
    }

    handle = tp_asv_get_uint32 (request_properties,
            TP_PROP_CHANNEL_TARGET_HANDLE, NULL);
    g_assert (handle != 0);

    if (tp_channel_manager_asv_has_unknown_properties (request_properties,
                fixed_properties, allowed_properties, &error))
    {
        goto error;
    }

    chan = g_hash_table_lookup (self->priv->channels, GUINT_TO_POINTER (handle));

    if (chan == NULL)
    {
        new_channel (self, handle, self->priv->conn->self_handle,
                request_token);
    }
    else if (require_new)
    {
        g_set_error (&error, TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
                "An fetion im channel to contact #%u already exists", handle);
        goto error;
    }
    else
    {
        tp_channel_manager_emit_request_already_satisfied (self,
                request_token, TP_EXPORTABLE_CHANNEL (chan));
    }

    return TRUE;

error:
    tp_channel_manager_emit_request_failed (self, request_token,
            error->domain, error->code, error->message);
    g_error_free (error);
    return TRUE;
}

    static gboolean
fetion_im_factory_create_channel (TpChannelManager *manager,
        gpointer request_token,
        GHashTable *request_properties)
{
    return fetion_im_factory_request (FETION_IM_FACTORY (manager),
            request_token, request_properties, TRUE);
}

    static gboolean
fetion_im_factory_ensure_channel (TpChannelManager *manager,
        gpointer request_token,
        GHashTable *request_properties)
{
    return fetion_im_factory_request (FETION_IM_FACTORY (manager),
            request_token, request_properties, FALSE);
}

    static void
channel_manager_iface_init (gpointer g_iface,
        gpointer data G_GNUC_UNUSED)
{
    TpChannelManagerIface *iface = g_iface;

    iface->foreach_channel = fetion_im_factory_foreach_channel;
    iface->type_foreach_channel_class = fetion_im_factory_type_foreach_channel_class;
    iface->create_channel = fetion_im_factory_create_channel;
    iface->ensure_channel = fetion_im_factory_ensure_channel;
    /* In this channel manager, Request has the same semantics as Ensure */
    iface->request_channel = fetion_im_factory_ensure_channel;
}

    void 
telepathy_message_received(TpBaseConnection *conn ,
        TpHandle contact,const gchar *message,time_t recv_time)
{
    FetionImFactory *im_factory = ((FetionConnection*)conn)->priv->im_factory;

    FetionImChannel *chan = g_hash_table_lookup (im_factory->priv->channels, GUINT_TO_POINTER (contact));
    if(chan == NULL)
        chan = new_channel (im_factory,contact,contact,NULL);

    TpMessage * received =tp_cm_message_new_text(conn,
            contact,TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL,message);

    tp_message_set_int64(received,0,"message-sent",recv_time);
    tp_message_set_int64(received,0,"message-received",time(NULL));

    tp_message_mixin_take_received (G_OBJECT(chan), received);
}
