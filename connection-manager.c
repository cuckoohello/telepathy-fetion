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
#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>
#include <telepathy-glib/telepathy-glib.h>
#include "connection-manager.h"
#include "connection.h"
#include "protocol.h"

G_DEFINE_TYPE (FetionConnectionManager,
        fetion_connection_manager,
        TP_TYPE_BASE_CONNECTION_MANAGER)

struct _FetionConnectionManagerPrivate
{
    int dummy;
};

static void
fetion_connection_manager_init (
        FetionConnectionManager *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
            FETION_TYPE_CONNECTION_MANAGER,
            FetionConnectionManagerPrivate);
};

    static void
fetion_connection_manager_constructed (GObject *object)
{
    FetionConnectionManager *self =
        FETION_CONNECTION_MANAGER (object);
    TpBaseConnectionManager *base = (TpBaseConnectionManager *) self;
    void (*constructed) (GObject *) =
        ((GObjectClass *) fetion_connection_manager_parent_class)->constructed;
    TpBaseProtocol *protocol;

    if (constructed != NULL)
        constructed (object);

    protocol = g_object_new (FETION_TYPE_PROTOCOL,
            "name", "fetion",
            NULL);
    tp_base_connection_manager_add_protocol (base, protocol);
    g_object_unref (protocol);
}

static void
fetion_connection_manager_class_init (
        FetionConnectionManagerClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    TpBaseConnectionManagerClass *base_class =
        (TpBaseConnectionManagerClass *) klass;

    g_type_class_add_private (klass,
            sizeof (FetionConnectionManagerPrivate));

    object_class->constructed = fetion_connection_manager_constructed;
    base_class->cm_dbus_name = "openfetion";
}
