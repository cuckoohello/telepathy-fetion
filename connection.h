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
#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/contacts-mixin.h>
#include <telepathy-glib/presence-mixin.h>
#include "account.h"
#include "roster.h"
#include "im-factory.h"

G_BEGIN_DECLS

typedef struct _FetionConnection FetionConnection;
typedef struct _FetionConnectionClass FetionConnectionClass;
typedef struct _FetionConnectionPrivate FetionConnectionPrivate;

struct _FetionConnectionClass {
    TpBaseConnectionClass parent_class;
    TpDBusPropertiesMixinClass properties_class;
    TpPresenceMixinClass presence_mixin;
    TpContactsMixinClass contacts_mixin;
};

enum
{
    ALIAS_UPDATED=0,
    PRESENCE_UPDATED,
    AVATAR_UPDATED,
    MESSAGE_RECEIVED,
    N_SIGNALS
};

extern guint signals[N_SIGNALS] ;
struct _FetionConnectionPrivate
{
    HybridAccount *account;
    FetionContactList *contact_list;
    FetionImFactory *im_factory;
};

struct _FetionConnection {
    TpBaseConnection parent;
    TpPresenceMixin presence_mixin;
    TpContactsMixin contacts_mixin;

    FetionConnectionPrivate *priv;
};

GType fetion_connection_get_type (void);

#define FETION_TYPE_CONNECTION \
    (fetion_connection_get_type ())
#define FETION_CONNECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), FETION_TYPE_CONNECTION, \
                                FetionConnection))
#define FETION_CONNECTION_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), FETION_TYPE_CONNECTION, \
                             FetionConnectionClass))
#define IS_FETION_CONNECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), FETION_TYPE_CONNECTION))
#define IS_FETION_CONNECTION_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), FETION_TYPE_CONNECTION))
#define FETION_CONNECTION_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FETION_TYPE_CONNECTION, \
                                FetionConnectionClass))

gchar *fetion_normalize_contact (TpHandleRepoIface *repo,
        const gchar *id, gpointer context, GError **error);

const gchar * const * fetion_connection_get_possible_interfaces (void);

G_END_DECLS

#endif
