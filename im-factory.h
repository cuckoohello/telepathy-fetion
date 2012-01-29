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

#ifndef __IM_FACTORY_H__
#define __IM_FACTORY_H__

#include <glib-object.h>
#include <telepathy-glib/telepathy-glib.h>
#include "im-channel.h"

G_BEGIN_DECLS

typedef struct _FetionImFactory FetionImFactory;
typedef struct _FetionImFactoryClass FetionImFactoryClass;
typedef struct _FetionImFactoryPrivate FetionImFactoryPrivate;

struct _FetionImFactoryClass {
    GObjectClass parent_class;
};

struct _FetionImFactory {
    GObject parent;
    FetionImFactoryPrivate *priv;
};
struct _FetionImFactoryPrivate
{
    TpBaseConnection *conn;
    GHashTable *channels;
    gulong status_changed_id;
    gulong message_received_id;
};

GType fetion_im_factory_get_type (void);

/* TYPE MACROS */
#define FETION_TYPE_IM_FACTORY \
    (fetion_im_factory_get_type ())
#define FETION_IM_FACTORY(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), FETION_TYPE_IM_FACTORY, \
                                FetionImFactory))
#define FETION_IM_FACTORY_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), FETION_TYPE_IM_FACTORY, \
                             FetionImFactoryClass))
#define FETION_IM_FACTORY_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FETION_TYPE_IM_FACTORY, \
                                FetionImFactoryClass))

G_END_DECLS

#endif
