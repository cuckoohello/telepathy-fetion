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

#ifndef __IM_CHANNEL_H__
#define __IM_CHANNEL_H__

#include <telepathy-glib/base-channel.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/message-mixin.h>

G_BEGIN_DECLS

typedef struct _FetionImChannel FetionImChannel;
typedef struct _FetionImChannelClass FetionImChannelClass;
typedef struct _FetionImChannelPrivate FetionImChannelPrivate;

GType fetion_im_channel_get_type (void);

#define FETION_TYPE_IM_CHANNEL \
    (fetion_im_channel_get_type ())
#define FETION_IM_CHANNEL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FETION_TYPE_IM_CHANNEL, \
                                 FetionImChannel))
#define FETION_IM_CHANNEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FETION_TYPE_IM_CHANNEL, \
                              FetionImChannelClass))

struct _FetionImChannelClass {
    TpBaseChannelClass parent_class;
};

struct _FetionImChannel {
    TpBaseChannel parent;
    TpMessageMixin text;
    FetionImChannelPrivate *priv;
};

#define FETION_IM_CHANNEL_MAX_SMS_LENGTH 100

void fetion_im_channel_set_sms (FetionImChannel *self,
        gboolean sms);

G_END_DECLS

#endif
