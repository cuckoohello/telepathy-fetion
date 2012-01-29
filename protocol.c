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
#include <telepathy-glib/telepathy-glib.h>
#include "protocol.h"
#include "connection.h"
#include "roster.h"
#include "im-factory.h"

G_DEFINE_TYPE (FetionProtocol,
        fetion_protocol,
        TP_TYPE_BASE_PROTOCOL)

    static void
fetion_protocol_init ( FetionProtocol *self)
{
}

    gboolean
fetion_protocol_check_contact_id (const gchar *id,
        gchar **normal,
        GError **error)
{
    g_return_val_if_fail (id != NULL, FALSE);

    if (id[0] == '\0')
    {
        g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_HANDLE,
                "ID must not be empty");
        return FALSE;
    }

    if (normal != NULL)
        *normal = g_utf8_normalize (id, -1, G_NORMALIZE_ALL_COMPOSE);

    return TRUE;
}

static const TpCMParamSpec fetion_params[] = {
    { "account", "s", G_TYPE_STRING,
        TP_CONN_MGR_PARAM_FLAG_REQUIRED | TP_CONN_MGR_PARAM_FLAG_REGISTER,
        NULL,                               /* no default */
        0,                                  /* unused, formerly struct offset */
        tp_cm_param_filter_string_nonempty,
        NULL,                               /* filter data, unused here */
        NULL },                             /* setter data, now unused */
    { "password", "s", G_TYPE_STRING,
        TP_CONN_MGR_PARAM_FLAG_REQUIRED | TP_CONN_MGR_PARAM_FLAG_SECRET,
        NULL,                               /* no default */
        0,                                  /* unused, formerly struct offset */
        tp_cm_param_filter_string_nonempty,
        NULL,                               /* filter data, unused here */
        NULL },                             /* setter data, now unused */
    { NULL }
};

    static const TpCMParamSpec *
get_parameters (TpBaseProtocol *self)
{
    return fetion_params;
}

    static TpBaseConnection *
new_connection (TpBaseProtocol *protocol,
        GHashTable *asv,
        GError **error)
{
    FetionConnection *conn;
    const gchar *account,*password;

    account = tp_asv_get_string (asv, "account");
    /* telepathy-glib checked this for us */
    g_assert (account != NULL);

    password = tp_asv_get_string (asv, "password");


    conn = FETION_CONNECTION (
            g_object_new (FETION_TYPE_CONNECTION,
                "account", account,
                "password", password,
                "protocol", tp_base_protocol_get_name (protocol),
                NULL));

    return (TpBaseConnection *) conn;
}

    static gchar *
normalize_contact (TpBaseProtocol *self G_GNUC_UNUSED,
        const gchar *contact,
        GError **error)
{
    gchar *normal;

    if (fetion_protocol_check_contact_id (contact, &normal, error))
        return normal;
    else
        return NULL;
}

    static gchar *
identify_account (TpBaseProtocol *self G_GNUC_UNUSED,
        GHashTable *asv,
        GError **error)
{
    const gchar *account = tp_asv_get_string (asv, "account");

    if (account != NULL)
        return normalize_contact (self, account, error);

    g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
            "'account' parameter not given");
    return NULL;
}

    static GStrv
get_interfaces (TpBaseProtocol *self)
{
    return NULL;
}

    static void
get_connection_details (TpBaseProtocol *self G_GNUC_UNUSED,
        GStrv *connection_interfaces,
        GType **channel_managers,
        gchar **icon_name,
        gchar **english_name,
        gchar **vcard_field)
{
    if (connection_interfaces != NULL)
    {
        *connection_interfaces = g_strdupv (
                (GStrv) fetion_connection_get_possible_interfaces());
    }
    // TODO

    if (channel_managers != NULL)
    {
        GType types[] = { FETION_TYPE_CONTACT_LIST, FETION_TYPE_IM_FACTORY,G_TYPE_INVALID };
        *channel_managers = g_memdup (types, sizeof (types));
    }

    if (icon_name != NULL)
        *icon_name = g_strdup ("face-smile");

    if (english_name != NULL)
        *english_name = g_strdup ("Fetion");

    if (vcard_field != NULL)
        *vcard_field = g_strdup ("x-telepathy-fetion");
}

    static GStrv
dup_authentication_types (TpBaseProtocol *self)
{
    const gchar * const types[] = {
        TP_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION,
        NULL };           

    return g_strdupv ((GStrv) types);
}       


static void
fetion_protocol_class_init (
        FetionProtocolClass *klass)
{
    TpBaseProtocolClass *base_class =
        (TpBaseProtocolClass *) klass;

    base_class->get_parameters = get_parameters;
    base_class->new_connection = new_connection;

    base_class->normalize_contact = normalize_contact;
    base_class->identify_account = identify_account;
    base_class->get_interfaces = get_interfaces;
    base_class->get_connection_details = get_connection_details;
    base_class->dup_authentication_types = dup_authentication_types;
}
