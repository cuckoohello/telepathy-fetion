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
#include "conn-aliasing.h"

    static void
get_alias_flags (TpSvcConnectionInterfaceAliasing *aliasing,
        DBusGMethodInvocation *context)
{
    TpBaseConnection *base = TP_BASE_CONNECTION (aliasing);

    TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);
    tp_svc_connection_interface_aliasing_return_from_get_alias_flags (context,
            TP_CONNECTION_ALIAS_FLAG_USER_SET);
}

    static void
get_aliases (TpSvcConnectionInterfaceAliasing *aliasing,
        const GArray *contacts, DBusGMethodInvocation *context)
{
    FetionConnection *self =
        FETION_CONNECTION (aliasing);
    TpBaseConnection *base = TP_BASE_CONNECTION (aliasing);
    TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
            TP_HANDLE_TYPE_CONTACT);
    GHashTable *result;
    GError *error = NULL;
    guint i;

    TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);

    if (!tp_handles_are_valid (contact_repo, contacts, FALSE, &error))
    {
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    result = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);

    for (i = 0; i < contacts->len; i++)
    {
        TpHandle contact = g_array_index (contacts, TpHandle, i);
        const gchar *alias;
        if (contact == base->self_handle)
        {
            alias = self->priv->account->nickname;
        }else
        {
            HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(self->priv->account, contact);
            if(buddy == NULL)
                continue;

            if (buddy->name && *(buddy->name) != '\0') {
                alias = buddy->name;

            } else {
                alias = buddy->id;
            }   
        }

        g_hash_table_insert (result, GUINT_TO_POINTER (contact),
                (gchar *) alias);
    }

    tp_svc_connection_interface_aliasing_return_from_get_aliases (context,
            result);
    g_hash_table_destroy (result);
}

    static void
request_aliases (TpSvcConnectionInterfaceAliasing *aliasing,
        const GArray *contacts, DBusGMethodInvocation *context)
{
    FetionConnection *self =
        FETION_CONNECTION (aliasing);
    TpBaseConnection *base = TP_BASE_CONNECTION (aliasing);
    TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
            TP_HANDLE_TYPE_CONTACT);
    GPtrArray *result;
    gchar **strings;
    GError *error = NULL;
    guint i;

    TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);

    if (!tp_handles_are_valid (contact_repo, contacts, FALSE, &error))
    {
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    result = g_ptr_array_sized_new (contacts->len + 1);

    for (i = 0; i < contacts->len; i++)
    {
        TpHandle contact = g_array_index (contacts, TpHandle, i);
        const gchar *alias ;
        if (contact == base->self_handle)
        {
            alias = self->priv->account->nickname;
        }else
        {
            HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(self->priv->account, contact);
            if (buddy->name && *(buddy->name) != '\0') {
                alias = buddy->name;

            } else {
                alias = buddy->id;
            }   
        }

        g_ptr_array_add (result, (gchar *) alias);
    }

    g_ptr_array_add (result, NULL);
    strings = (gchar **) g_ptr_array_free (result, FALSE);
    tp_svc_connection_interface_aliasing_return_from_request_aliases (context,
            (const gchar **) strings);
    g_free (strings);
}

    static void
set_aliases (TpSvcConnectionInterfaceAliasing *aliasing,
        GHashTable *aliases, DBusGMethodInvocation *context)
{
    FetionConnection *self =
        FETION_CONNECTION (aliasing);
    TpBaseConnection *base = TP_BASE_CONNECTION (aliasing);
    TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base,
            TP_HANDLE_TYPE_CONTACT);
    HybridAccount* account = self->priv->account;
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, aliases);

    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        GError *error = NULL;

        if (!tp_handle_is_valid (contact_repo, GPOINTER_TO_UINT (key),
                    &error))
        {
            dbus_g_method_return_error (context, error);
            g_error_free (error);
            return;
        }
    }

    g_hash_table_iter_init (&iter, aliases);

    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        if(GPOINTER_TO_UINT(key) == base->self_handle)
            account->proto->info->im_ops->modify_name(account,value);
        else
        {
            HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(self->priv->account, GPOINTER_TO_UINT(key));
            if (buddy != NULL)
                account->proto->info->im_ops->buddy_rename(account,buddy,value);
        }
    }

    tp_svc_connection_interface_aliasing_return_from_set_aliases (context);
}

    static void
aliasing_fill_contact_attributes (GObject *object,
        const GArray *contacts,
        GHashTable *attributes)
{
    FetionConnection *self =
        FETION_CONNECTION (object);
    TpBaseConnection *base = TP_BASE_CONNECTION (object);
    guint i;

    for (i = 0; i < contacts->len; i++)
    {
        TpHandle contact = g_array_index (contacts, guint, i);
        const gchar *name;
        if (contact == base->self_handle)
        {
            name = self->priv->account->nickname;
        }else
        {
            HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(self->priv->account, contact);
            if (buddy == NULL)
                continue;

            if (buddy->name && *(buddy->name) != '\0') {
                name = buddy->name;

            } else {
                name = buddy->id;
            }   
        }

        tp_contacts_mixin_set_contact_attribute (attributes, contact,
                TP_TOKEN_CONNECTION_INTERFACE_ALIASING_ALIAS,
                tp_g_value_slice_new_string (name));
    }
}

    void
conn_aliasing_iface_init(gpointer iface,
        gpointer iface_data G_GNUC_UNUSED)
{
    TpSvcConnectionInterfaceAliasingClass *klass = iface;

#define IMPLEMENT(x) tp_svc_connection_interface_aliasing_implement_##x (\
        klass, x)
    IMPLEMENT(get_alias_flags);
    IMPLEMENT(request_aliases);
    IMPLEMENT(get_aliases);
    IMPLEMENT(set_aliases);
#undef IMPLEMENT
}

    static void 
alias_updated_cb( FetionConnection *conn,
        TpHandle contact, gpointer user_data)
{
    TpBaseConnection *base = (TpBaseConnection *) conn;
    const gchar *name;
    if (contact == base->self_handle)
    {
        name = conn->priv->account->nickname;
    }else
    {
        HybridBuddy *buddy = hybrid_blist_find_buddy_by_handle(conn->priv->account, contact);
        if (buddy == NULL)
            return;
        if (buddy->name && *(buddy->name) != '\0') {
            name = buddy->name;
        } else {
            name = buddy->id;
        }   
    }
    GPtrArray *aliases;
    GValueArray *pair;

    pair = g_value_array_new (2);
    g_value_array_append (pair, NULL);
    g_value_array_append (pair, NULL);
    g_value_init (pair->values + 0, G_TYPE_UINT);
    g_value_init (pair->values + 1, G_TYPE_STRING);
    g_value_set_uint (pair->values + 0, contact);
    g_value_set_string (pair->values + 1,name);

    aliases = g_ptr_array_sized_new (1);
    g_ptr_array_add (aliases, pair);

    tp_svc_connection_interface_aliasing_emit_aliases_changed (conn, aliases);

    g_ptr_array_free (aliases, TRUE);
    g_value_array_free (pair);
}

    void
conn_aliasing_init(GObject *object)
{
    g_signal_connect (object,
            "alias-updated", (GCallback) alias_updated_cb, NULL);
    tp_contacts_mixin_add_contact_attributes_iface (object,
            TP_IFACE_CONNECTION_INTERFACE_ALIASING,
            aliasing_fill_contact_attributes);
}
