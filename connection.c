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
#include "connection.h"
#include <dbus/dbus-glib.h>
#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/handle-repo-dynamic.h>
#include <telepathy-glib/handle-repo-static.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/handle-repo.h>

#include "roster.h"
#include "protocol.h"
#include "im-factory.h"
#include "account.h"
#include "conn-aliasing.h"
#include "conn-avatars.h"
#include "conn-presence.h"

G_DEFINE_TYPE_WITH_CODE (FetionConnection,
        fetion_connection,
        TP_TYPE_BASE_CONNECTION,
        G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_ALIASING,
            conn_aliasing_iface_init);
        G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_AVATARS,
            conn_avatars_iface_init);
        G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
            tp_dbus_properties_mixin_iface_init);
        G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACTS,
            tp_contacts_mixin_iface_init);
        G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACT_LIST,
            tp_base_contact_list_mixin_list_iface_init);
        G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACT_GROUPS,
            tp_base_contact_list_mixin_groups_iface_init);
        G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_PRESENCE,
            tp_presence_mixin_iface_init);
        G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
            tp_presence_mixin_simple_presence_iface_init))

enum
{
    PROP_ACCOUNT = 1,
    PROP_PASSWORD,
    N_PROPS
};

guint signals[N_SIGNALS] = { 0 };

    static void
fetion_connection_init (FetionConnection *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
            FETION_TYPE_CONNECTION,
            FetionConnectionPrivate);
}

    static void
get_property (GObject *object, guint property_id,
        GValue *value, GParamSpec *spec)
{
    FetionConnection *self =
        FETION_CONNECTION (object);

    g_return_if_fail(self->priv->account != NULL);

    switch (property_id)
    {
        case PROP_ACCOUNT:
            g_value_set_string (value, self->priv->account->username);
            break;

        case PROP_PASSWORD:
            g_value_set_string (value, self->priv->account->password);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, spec);
    }
}

    static void
set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *spec)
{
    FetionConnection *self =
        FETION_CONNECTION (object);

    switch (property_id)
    {
        case PROP_ACCOUNT:
            self->priv->account = hybrid_account_get("fetion",g_value_dup_string(value));
            self->priv->account->conn = (TpBaseConnection*) self;
            break;

        case PROP_PASSWORD:
            g_return_if_fail(self->priv->account != NULL);
            hybrid_account_set_password(self->priv->account,g_value_dup_string(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, spec);
    }
}

    static void
finalize (GObject *object)
{
    FetionConnection *self G_GNUC_UNUSED =
        FETION_CONNECTION (object);

    tp_contacts_mixin_finalize (object);

    G_OBJECT_CLASS (fetion_connection_parent_class)->finalize (
            object);
}

    static gchar *
get_unique_connection_name (TpBaseConnection *conn)
{
    FetionConnection *self = FETION_CONNECTION (conn);

    return g_strdup_printf ("%s@%p", self->priv->account->username, self);
}

    gchar *
fetion_normalize_contact (TpHandleRepoIface *repo, const gchar *id,
        gpointer context, GError **error)
{
    gchar *normal = NULL;
    if (fetion_protocol_check_contact_id (id, &normal, error))
        return normal;
    else
        return NULL;
}

    static void
create_handle_repos (TpBaseConnection *conn,
        TpHandleRepoIface *repos[NUM_TP_HANDLE_TYPES])
{
    repos[TP_HANDLE_TYPE_CONTACT] = tp_dynamic_handle_repo_new
        (TP_HANDLE_TYPE_CONTACT, fetion_normalize_contact, NULL);
}


    static GPtrArray *
create_channel_managers (TpBaseConnection *conn)
{
    FetionConnection *self =
        FETION_CONNECTION (conn);
    GPtrArray *ret = g_ptr_array_sized_new (1);

    self->priv->contact_list = FETION_CONTACT_LIST (g_object_new (
                FETION_TYPE_CONTACT_LIST,
                "connection", conn,
                NULL));

    g_ptr_array_add (ret, self->priv->contact_list);

    self->priv->im_factory = FETION_IM_FACTORY( g_object_new (FETION_TYPE_IM_FACTORY,
                "connection", conn,
                NULL));
    g_ptr_array_add(ret,self->priv->im_factory);

    return ret;
}

    static gboolean
start_connecting (TpBaseConnection *conn, GError **error)
{
    FetionConnection *self = FETION_CONNECTION (conn);
    TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (conn,
            TP_HANDLE_TYPE_CONTACT);

    g_return_val_if_fail(self->priv->account != NULL, FALSE);

    conn->self_handle = tp_handle_ensure (contact_repo, self->priv->account->username,
            NULL, error);

    if (conn->self_handle == 0)
        return FALSE;

    self->priv->account->contact_repo =  contact_repo;
    self->priv->account->contacts = tp_handle_set_new (contact_repo);
    hybrid_account_set_enabled(self->priv->account,TRUE);
    hybrid_account_enable(self->priv->account);

    return TRUE;
}

    static void
shut_down (TpBaseConnection *conn)
{
    FetionConnection *self = FETION_CONNECTION (conn);

    hybrid_account_close(self->priv->account);
    // TODO tp_clear_pointer ( self->priv->account->contacts, tp_handle_set_destroy);
    //TODO self_handle
    tp_base_connection_finish_shutdown (conn);
}

    static void
constructed (GObject *object)
{
    TpBaseConnection *base = TP_BASE_CONNECTION (object);
    void (*chain_up) (GObject *) =
        G_OBJECT_CLASS (fetion_connection_parent_class)->constructed;

    if (chain_up != NULL)
        chain_up (object);

    tp_contacts_mixin_init (object,
            G_STRUCT_OFFSET (FetionConnection, contacts_mixin));

    tp_base_connection_register_with_contacts_mixin (base);
    tp_base_contact_list_mixin_register_with_contacts_mixin (base);

    conn_aliasing_init(object);
    conn_avatars_init(object);
    conn_presence_init(object);
}

static const gchar *interfaces_always_present[] = {
    TP_IFACE_CONNECTION_INTERFACE_ALIASING,
    TP_IFACE_CONNECTION_INTERFACE_AVATARS,
    TP_IFACE_CONNECTION_INTERFACE_CONTACTS,
    TP_IFACE_CONNECTION_INTERFACE_CONTACT_LIST,
    TP_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS,
    TP_IFACE_CONNECTION_INTERFACE_PRESENCE,
    TP_IFACE_CONNECTION_INTERFACE_REQUESTS,
    TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
    NULL };

    const gchar * const *
fetion_connection_get_possible_interfaces (void)
{
    return interfaces_always_present;
}


static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
    { TP_IFACE_CONNECTION_INTERFACE_AVATARS,
        conn_avatars_properties_getter,
        NULL,
        NULL,
    },
    { NULL }
};

    static void
fetion_connection_class_init (FetionConnectionClass *klass)
{
    TpBaseConnectionClass *base_class = (TpBaseConnectionClass *) klass;
    GObjectClass *object_class = (GObjectClass *) klass;
    GParamSpec *param_spec;

    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->finalize = finalize;
    g_type_class_add_private (klass,
            sizeof (FetionConnectionPrivate));

    base_class->create_handle_repos = create_handle_repos;
    base_class->get_unique_connection_name = get_unique_connection_name;
    base_class->create_channel_managers = create_channel_managers;
    base_class->start_connecting = start_connecting;
    base_class->shut_down = shut_down;
    base_class->interfaces_always_present = interfaces_always_present;

    param_spec = g_param_spec_string ("account", "Account name",
            "The username of this user", NULL,
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
            G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB);
    g_object_class_install_property (object_class, PROP_ACCOUNT, param_spec);

    param_spec = g_param_spec_string ("password", "Password",
            "The password of this user", NULL,
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
            G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB);
    g_object_class_install_property (object_class, PROP_PASSWORD,
            param_spec);

    tp_contacts_mixin_class_init (object_class,
            G_STRUCT_OFFSET (FetionConnectionClass, contacts_mixin));

    conn_presence_class_init(klass);

    prop_interfaces[0].props = conn_avatars_properties;
    klass->properties_class.interfaces = prop_interfaces;
    tp_dbus_properties_mixin_class_init (object_class,
            G_STRUCT_OFFSET (FetionConnectionClass , properties_class));

    tp_base_contact_list_mixin_class_init (base_class);

    signals[ALIAS_UPDATED] = g_signal_new ("alias-updated",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);

    signals[PRESENCE_UPDATED] = g_signal_new ("presence-updated",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);

    /* "avatar-updated" is used by system */
    signals[AVATAR_UPDATED] = g_signal_new ("fetion-avatar-updated",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            g_cclosure_marshal_VOID__UINT_POINTER, G_TYPE_NONE, 2, G_TYPE_UINT,G_TYPE_POINTER);

    signals[MESSAGE_RECEIVED] = g_signal_new ("message-received",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            g_cclosure_marshal_VOID__UINT_POINTER, G_TYPE_NONE, 2, G_TYPE_UINT,G_TYPE_POINTER);
}
