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
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <glib-object.h>
#include <telepathy-glib/base-protocol.h>

G_BEGIN_DECLS

typedef struct _FetionProtocol FetionProtocol;
typedef struct _FetionProtocolPrivate FetionProtocolPrivate;
typedef struct _FetionProtocolClass FetionProtocolClass;
typedef struct _FetionProtocolClassPrivate FetionProtocolClassPrivate;

struct _FetionProtocolClass {
    TpBaseProtocolClass parent_class;
    FetionProtocolClassPrivate *priv;
};

struct _FetionProtocol {
    TpBaseProtocol parent;
    FetionProtocolPrivate *priv;
};

GType fetion_protocol_get_type (void);

#define FETION_TYPE_PROTOCOL \
    (fetion_protocol_get_type ())
#define FETION_PROTOCOL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                 FETION_TYPE_PROTOCOL, \
                                 FetionProtocol))
#define FETION_PROTOCOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                              FETION_TYPE_PROTOCOL, \
                              FetionProtocolClass))
#define IS_FETION_PROTOCOL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                 FETION_TYPE_PROTOCOL))
#define IS_FETION_PROTOCOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                              FETION_TYPE_PROTOCOL))
#define FETION_PROTOCOL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                FETION_TYPE_PROTOCOL, \
                                FetionProtocolClass))

gboolean fetion_protocol_check_contact_id (const gchar *id,
        gchar **normal,
        GError **error);

G_END_DECLS

#endif
