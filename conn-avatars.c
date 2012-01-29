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
#include "conn-avatars.h"

#define AVATAR_MIN_PX 64
#define AVATAR_REC_PX 96
#define AVATAR_MAX_PX 120
#define AVATAR_MAX_BYTES 16384

static const char *mimetypes[] = {
    "image/png", "image/jpeg", "image/gif", NULL };

static TpDBusPropertiesMixinPropImpl props[] = {
    { "MinimumAvatarWidth", GUINT_TO_POINTER (AVATAR_MIN_PX), NULL },
    { "RecommendedAvatarWidth", GUINT_TO_POINTER (AVATAR_REC_PX), NULL },
    { "MaximumAvatarWidth", GUINT_TO_POINTER (AVATAR_MAX_PX), NULL },
    { "MinimumAvatarHeight", GUINT_TO_POINTER (AVATAR_MIN_PX), NULL },
    { "RecommendedAvatarHeight", GUINT_TO_POINTER (AVATAR_REC_PX), NULL },
    { "MaximumAvatarHeight", GUINT_TO_POINTER (AVATAR_MAX_PX), NULL },
    { "MaximumAvatarBytes", GUINT_TO_POINTER (AVATAR_MAX_BYTES), NULL },
    /* special-cased - it's the only one with a non-guint value */
    { "SupportedAvatarMIMETypes", NULL, NULL },
    { NULL }
};

TpDBusPropertiesMixinPropImpl *conn_avatars_properties = props;

    static void
conn_avatars_get_avatar_requirements (TpSvcConnectionInterfaceAvatars *iface,
        DBusGMethodInvocation *context)
{

    TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (TP_BASE_CONNECTION (iface),
            context);

    tp_svc_connection_interface_avatars_return_from_get_avatar_requirements (
            context, mimetypes, AVATAR_MIN_PX, AVATAR_MIN_PX,
            AVATAR_MAX_PX, AVATAR_MAX_PX, AVATAR_MAX_BYTES);
}

    static void
conn_avatars_get_avatar_tokens (TpSvcConnectionInterfaceAvatars *iface,
        const GArray *contacts, DBusGMethodInvocation *invocation)
{
    FetionConnection *self =
        FETION_CONNECTION (iface);
    TpBaseConnection *base = (TpBaseConnection *) self;
    HybridAccount* account = self->priv->account;
    TpHandleRepoIface *contacts_repo = account->contact_repo;

    guint i ;
    gchar **ret;
    GError *err = NULL;

    TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, invocation);

    if (!tp_handles_are_valid (contacts_repo, contacts, FALSE, &err))
    {
        dbus_g_method_return_error (invocation, err);
        g_error_free (err);
        return;
    }

    ret = g_new0 (gchar *, contacts->len + 1);

    for (i = 0; i < contacts->len; i++)
    {
        TpHandle contact = g_array_index (contacts, TpHandle, i);
        if(contact  != base->self_handle)
        {
            HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(account, contact);
            if(buddy == NULL)
                continue;
            ret[i] = g_strup(buddy->icon_name);

        }else
        {
            ret[i] = g_strup(account->icon_name);
        }
    }

    tp_svc_connection_interface_avatars_return_from_get_avatar_tokens (
            invocation, (const gchar **)ret);
    g_strfreev (ret);
}

    static void
conn_avatars_get_known_avatar_tokens (TpSvcConnectionInterfaceAvatars *iface,
        const GArray *contacts, DBusGMethodInvocation *invocation)
{
    FetionConnection *self =
        FETION_CONNECTION (iface);
    TpBaseConnection *base = (TpBaseConnection *) self;
    HybridAccount* account = self->priv->account;

    TpHandleRepoIface *contacts_repo = account->contact_repo;

    guint i;
    GHashTable *ret;
    GError *err = NULL;

    TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, invocation);

    if (!tp_handles_are_valid (contacts_repo, contacts, FALSE, &err))
    {
        dbus_g_method_return_error (invocation, err);
        g_error_free (err);
        return;
    }

    ret = g_hash_table_new_full (NULL, NULL, NULL, g_free);

    for (i = 0; i < contacts->len; i++)
    {
        TpHandle contact;

        contact = g_array_index (contacts, TpHandle, i);

        if(contact  != base->self_handle)
        {
            HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(account, contact);
            if(buddy == NULL)
                continue;
            g_hash_table_insert (ret, GUINT_TO_POINTER (contact),
                    g_strdup (buddy->icon_name));

        }else
        {
            g_hash_table_insert (ret, GUINT_TO_POINTER (contact),
                    g_strdup (account->icon_name));
        }
    }

    tp_svc_connection_interface_avatars_return_from_get_known_avatar_tokens (
            invocation, ret);

    g_hash_table_destroy (ret);
}

    static void
conn_avatars_request_avatar (TpSvcConnectionInterfaceAvatars *iface,
        guint contact, DBusGMethodInvocation *context)
{
    FetionConnection *self =
        FETION_CONNECTION (iface);
    TpBaseConnection *base = (TpBaseConnection *) self;
    HybridAccount* account = self->priv->account;

    TpHandleRepoIface *contacts_repo = account->contact_repo;

    GError *err = NULL;

    TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);

    if (!tp_handle_is_valid (contacts_repo, contact, &err))
    {
        dbus_g_method_return_error (context, err);
        g_error_free (err);
        return;
    }

    if(contact  != base->self_handle)
    {
        GArray *arr;
        HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(account, contact);
        if(buddy == NULL)
            return;

        arr = g_array_new (FALSE, FALSE, sizeof (gchar));
        g_array_append_vals (arr, buddy->icon_data , buddy->icon_data_length);
        tp_svc_connection_interface_avatars_return_from_request_avatar (
                context, arr, "");
        g_array_free (arr, TRUE);
    }else
    {
        GArray *arr;

        arr = g_array_new (FALSE, FALSE, sizeof (gchar));
        g_array_append_vals (arr, account->icon_data , account->icon_data_len);
        tp_svc_connection_interface_avatars_return_from_request_avatar (
                context, arr, "");
        g_array_free (arr, TRUE);
    }

}

    static void
conn_avatars_request_avatars (TpSvcConnectionInterfaceAvatars *iface,
        const GArray *contacts, DBusGMethodInvocation *context)
{
    FetionConnection *self =
        FETION_CONNECTION (iface);
    TpBaseConnection *base = (TpBaseConnection *) self;
    HybridAccount* account = self->priv->account;

    TpHandleRepoIface *contacts_repo = account->contact_repo;

    GError *error = NULL;
    guint i;

    TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);

    if (!tp_handles_are_valid (contacts_repo, contacts, FALSE, &error))
    {
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    for (i = 0; i < contacts->len; i++)
    {
        TpHandle contact = g_array_index (contacts, TpHandle, i);

        if(contact  != base->self_handle)
        {
            HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(account, contact);
            if(buddy == NULL)
                continue;

            gchar *sha1;
            GArray *arr;

            sha1 = buddy->icon_name;
            arr = g_array_new (FALSE, FALSE, sizeof (gchar));
            g_array_append_vals (arr, buddy->icon_data , buddy->icon_data_length);
            tp_svc_connection_interface_avatars_emit_avatar_retrieved (iface, contact,
                    sha1, arr, mimetypes[1]);
            g_array_free (arr, TRUE);
            //g_free (sha1);
        }else
        {
            gchar *sha1;
            GArray *arr;

            sha1 = account->icon_name;
            arr = g_array_new (FALSE, FALSE, sizeof (gchar));
            g_array_append_vals (arr, account->icon_data , account->icon_data_len);
            tp_svc_connection_interface_avatars_emit_avatar_retrieved (iface, contact,
                    sha1, arr, mimetypes[1]);
            g_array_free (arr, TRUE);
            //g_free (sha1);
        }
    }

    tp_svc_connection_interface_avatars_return_from_request_avatars (context);
}

    void
conn_avatars_fill_contact_attributes (GObject *obj,
        const GArray *contacts, GHashTable *attributes_hash)
{
    guint i;

    FetionConnection *self =
        FETION_CONNECTION (obj);
    TpBaseConnection *base = (TpBaseConnection *) self;
    HybridAccount* account = self->priv->account;

    for (i = 0; i < contacts->len; i++)
    {
        TpHandle contact = g_array_index (contacts, guint, i);
        GValue *val = tp_g_value_slice_new (G_TYPE_STRING);

        if(contact  != base->self_handle)
        {
            HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(account, contact);
            if(buddy == NULL)
                continue;
            g_value_set_string (val, buddy->icon_name);

        }else
        {
            g_value_set_string (val, account->icon_name);
        }

        tp_contacts_mixin_set_contact_attribute (attributes_hash, contact,
                TP_IFACE_CONNECTION_INTERFACE_AVATARS"/token", val);
    }
}

    void
conn_avatars_properties_getter (GObject *object,
        GQuark interface, GQuark name,
        GValue *value, gpointer getter_data)
{
    GQuark q_mime_types = g_quark_from_static_string (
            "SupportedAvatarMIMETypes");

    if (name == q_mime_types)
    {
        g_value_set_static_boxed (value, mimetypes);
    }
    else
    {
        g_value_set_uint (value, GPOINTER_TO_UINT (getter_data));
    }
}

    void
conn_avatars_iface_init (gpointer g_iface, gpointer iface_data)
{
    TpSvcConnectionInterfaceAvatarsClass *klass = g_iface;

#define IMPLEMENT(x) tp_svc_connection_interface_avatars_implement_##x (\
        klass, conn_avatars_##x)
    IMPLEMENT(get_avatar_requirements);
    IMPLEMENT(get_avatar_tokens);
    IMPLEMENT(get_known_avatar_tokens);
    IMPLEMENT(request_avatar);
    IMPLEMENT(request_avatars);
#undef IMPLEMENT
}

    static void 
avatar_updated_cb( FetionConnection *conn,
        TpHandle contact,const gchar *shal, gpointer user_data)
{
    g_assert(shal != NULL);
    tp_svc_connection_interface_avatars_emit_avatar_updated(conn,contact,shal);
}

    void
conn_avatars_init(GObject *object)
{
    g_signal_connect (object,
            "fetion-avatar-updated", (GCallback) avatar_updated_cb, NULL);
    tp_contacts_mixin_add_contact_attributes_iface (object,
            TP_IFACE_CONNECTION_INTERFACE_AVATARS,
            conn_avatars_fill_contact_attributes);
}
