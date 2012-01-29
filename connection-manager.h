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
#ifndef __CONNECTION_MANAGER_H__
#define __CONNECTION_MANAGER_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection-manager.h>

G_BEGIN_DECLS

typedef struct _FetionConnectionManager FetionConnectionManager;
typedef struct _FetionConnectionManagerClass FetionConnectionManagerClass;
typedef struct _FetionConnectionManagerPrivate FetionConnectionManagerPrivate;

struct _FetionConnectionManagerClass {
    TpBaseConnectionManagerClass parent_class;
};

struct _FetionConnectionManager {
    TpBaseConnectionManager parent;
    FetionConnectionManagerPrivate *priv;
};

GType fetion_connection_manager_get_type (void);

#define FETION_TYPE_CONNECTION_MANAGER \
    (fetion_connection_manager_get_type ())
#define FETION_CONNECTION_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                FETION_TYPE_CONNECTION_MANAGER, \
                                FetionConnectionManager))
#define FETION_CONNECTION_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), \
                             FETION_TYPE_CONNECTION_MANAGER, \
                             FetionConnectionManagerClass))
#define IS_FETION_CONNECTION_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                FETION_TYPE_CONNECTION_MANAGER))
#define IS_FETION_CONNECTION_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), \
                             FETION_TYPE_CONNECTION_MANAGER))
#define FETION_CONNECTION_MANAGER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                FETION_TYPE_CONNECTION_MANAGER, \
                                FetionConnectionManagerClass))

G_END_DECLS

#endif
