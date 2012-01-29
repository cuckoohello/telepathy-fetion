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
#ifndef __CONN_AVATARS_H__
#define __CONN_AVATARS_H__

#include "connection.h"

extern TpDBusPropertiesMixinPropImpl *conn_avatars_properties;

G_BEGIN_DECLS

void conn_avatars_iface_init (gpointer g_iface, gpointer iface_data);
void conn_avatars_fill_contact_attributes (GObject *obj,
        const GArray *contacts, GHashTable *attributes_hash);
void conn_avatars_properties_getter (GObject *object, GQuark interface,
        GQuark name, GValue *value, gpointer getter_data);
void conn_avatars_init(GObject *object);

G_END_DECLS

#endif
