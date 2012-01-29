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

#include "im-channel.h"
#include <string.h>

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/message.h>
#include <telepathy-glib/svc-channel.h>
#include <glib/garray.h>
#include "connection.h"
#include "conv.h"

static void destroyable_iface_init (gpointer iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (FetionImChannel,
        fetion_im_channel,
        TP_TYPE_BASE_CHANNEL,
        G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_TEXT,
            tp_message_mixin_text_iface_init)
        G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_MESSAGES,
            tp_message_mixin_messages_iface_init)
        G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_DESTROYABLE,
            destroyable_iface_init)
        )

/* type definition stuff */

static const char * fetion_im_channel_interfaces[] = {
    TP_IFACE_CHANNEL_INTERFACE_MESSAGES,
    TP_IFACE_CHANNEL_INTERFACE_DESTROYABLE,
    NULL };

enum
{
    PROP_SMS = 1,
    N_PROPS
};

struct _FetionImChannelPrivate
{
    gboolean sms;
};

    static void
fetion_im_channel_init (FetionImChannel *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
            FETION_TYPE_IM_CHANNEL, FetionImChannelPrivate);
}


    static void
send_message (GObject *object,
        TpMessage *message,
        TpMessageSendingFlags flags)
{
    FetionImChannel *self = FETION_IM_CHANNEL (object);
    TpBaseChannel *base = TP_BASE_CHANNEL (self);
    if (tp_asv_get_string (tp_message_peek (message, 0), "interface") != NULL)
    {
        /* this message is interface-specific - let's not echo it */
        goto finally;
    }

    FetionConnection *conn =
        FETION_CONNECTION (tp_base_channel_get_connection(base));

    HybridAccount *account = conn->priv->account;

    TpHandle from = tp_base_channel_get_target_handle (base);
    const GHashTable *input = tp_message_peek (message, 1);
    const gchar *send_message = tp_asv_get_string (input, "content");
    HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(account, from);
    hybrid_conv_send_message(account,buddy,send_message);

finally:
    /* "OK, we've sent the message" (after calling this, message must not be
     * dereferenced) */
    tp_message_mixin_sent (object, message, flags, "", NULL);

}


    static GObject *
constructor (GType type,
        guint n_props,
        GObjectConstructParam *props)
{
    GObject *object =
        G_OBJECT_CLASS (fetion_im_channel_parent_class)->constructor (type,
                n_props, props);
    FetionImChannel *self = FETION_IM_CHANNEL (object);
    TpBaseChannel *base = TP_BASE_CHANNEL (self);

    static TpChannelTextMessageType const types[] = {
        TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL,
        TP_CHANNEL_TEXT_MESSAGE_TYPE_ACTION,
        TP_CHANNEL_TEXT_MESSAGE_TYPE_NOTICE
    };
    static const char * const content_types[] = { "text/plain", NULL };

    tp_base_channel_register (base);

    tp_message_mixin_init (object, G_STRUCT_OFFSET (FetionImChannel, text),
            tp_base_channel_get_connection (base));

    tp_message_mixin_implement_sending (object, send_message,
            G_N_ELEMENTS (types), types,0,
            TP_DELIVERY_REPORTING_SUPPORT_FLAG_RECEIVE_FAILURES,
            content_types);

    return object;
}

    static void
finalize (GObject *object)
{
    tp_message_mixin_finalize (object);

    ((GObjectClass *) fetion_im_channel_parent_class)->finalize (object);
}

    static void
fetion_im_channel_close (TpBaseChannel *self)
{
    GObject *object = (GObject *) self;

    if (!tp_base_channel_is_destroyed (self))
    {
        TpHandle first_sender;

        /* The manager wants to be able to respawn the channel if it has pending
         * messages. When respawned, the channel must have the initiator set
         * to the contact who sent us those messages (if it isn't already),
         * and the messages must be marked as having been rescued so they
         * don't get logged twice. */
        if (tp_message_mixin_has_pending_messages (object, &first_sender))
        {
            tp_message_mixin_set_rescued (object);
            tp_base_channel_reopened (self, first_sender);
        }
        else
        {
            tp_base_channel_destroyed (self);
        }
    }
}

    static void
fetion_im_channel_fill_immutable_properties (TpBaseChannel *chan,
        GHashTable *properties)
{
    TpBaseChannelClass *klass = TP_BASE_CHANNEL_CLASS (
            fetion_im_channel_parent_class);

    klass->fill_immutable_properties (chan, properties);

    tp_dbus_properties_mixin_fill_properties_hash (
            G_OBJECT (chan), properties,
            TP_IFACE_CHANNEL_INTERFACE_MESSAGES, "MessagePartSupportFlags",
            TP_IFACE_CHANNEL_INTERFACE_MESSAGES, "DeliveryReportingSupport",
            TP_IFACE_CHANNEL_INTERFACE_MESSAGES, "SupportedContentTypes",
            TP_IFACE_CHANNEL_INTERFACE_MESSAGES, "MessageTypes",
            NULL);
}

    static void
set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec)
{
    FetionImChannel *self G_GNUC_UNUSED = (FetionImChannel *) object;

    switch (property_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

    static void
get_property (GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec)
{
    FetionImChannel *self G_GNUC_UNUSED = (FetionImChannel *) object;

    switch (property_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

    static void
fetion_im_channel_class_init (FetionImChannelClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    TpBaseChannelClass *base_class = (TpBaseChannelClass *) klass;

    g_type_class_add_private (klass, sizeof (FetionImChannelPrivate));

    object_class->constructor = constructor;
    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->finalize = finalize;

    base_class->channel_type = TP_IFACE_CHANNEL_TYPE_TEXT;
    base_class->target_handle_type = TP_HANDLE_TYPE_CONTACT;
    base_class->interfaces = fetion_im_channel_interfaces;
    base_class->close = fetion_im_channel_close;
    base_class->fill_immutable_properties =
        fetion_im_channel_fill_immutable_properties;

    tp_message_mixin_init_dbus_properties (object_class);
}

    static void
destroyable_destroy (TpSvcChannelInterfaceDestroyable *iface,
        DBusGMethodInvocation *context)
{
    TpBaseChannel *self = TP_BASE_CHANNEL (iface);

    tp_message_mixin_clear ((GObject *) self);
    fetion_im_channel_close (self);
    g_assert (tp_base_channel_is_destroyed (self));
    tp_svc_channel_interface_destroyable_return_from_destroy (context);
}

    static void
destroyable_iface_init (gpointer iface,
        gpointer data)
{
    TpSvcChannelInterfaceDestroyableClass *klass = iface;

#define IMPLEMENT(x) \
    tp_svc_channel_interface_destroyable_implement_##x (klass, destroyable_##x)
    IMPLEMENT (destroy);
#undef IMPLEMENT
}

