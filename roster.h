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

#ifndef __ROSTER_H__
#define __ROSTER_H__

#include <glib-object.h>

#include <telepathy-glib/base-contact-list.h>
#include <telepathy-glib/channel-manager.h>
#include <telepathy-glib/handle.h>
#include <telepathy-glib/presence-mixin.h>

G_BEGIN_DECLS

typedef struct _FetionContactList FetionContactList;
typedef struct _FetionContactListClass FetionContactListClass;
typedef struct _FetionContactListPrivate FetionContactListPrivate;

struct _FetionContactListClass {
    TpBaseContactListClass parent_class;
};

struct _FetionContactList {
    TpBaseContactList parent;

    FetionContactListPrivate *priv;
};

GType fetion_contact_list_get_type (void);

#define FETION_TYPE_CONTACT_LIST \
    (fetion_contact_list_get_type ())
#define FETION_CONTACT_LIST(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), FETION_TYPE_CONTACT_LIST, \
                                FetionContactList))
#define FETION_CONTACT_LIST_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), FETION_TYPE_CONTACT_LIST, \
                             FetionContactListClass))
#define FETION_IS_CONTACT_LIST(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), FETION_TYPE_CONTACT_LIST))
#define FETION_IS_CONTACT_LIST_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), FETION_TYPE_CONTACT_LIST))
#define FETION_CONTACT_LIST_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FETION_TYPE_CONTACT_LIST, \
                                FetionContactListClass))

/* this enum must be kept in sync with the array _statuses in
 * contact-list.c */
typedef enum {
    FETION_CONTACT_LIST_PRESENCE_OFFLINE = 0,
    FETION_CONTACT_LIST_PRESENCE_UNKNOWN,
    FETION_CONTACT_LIST_PRESENCE_ERROR,
    FETION_CONTACT_LIST_PRESENCE_AWAY,
    FETION_CONTACT_LIST_PRESENCE_AVAILABLE
} FetionContactListPresence;

const TpPresenceStatusSpec *fetion_contact_list_presence_statuses (
        void);

FetionContactListPresence fetion_contact_list_get_presence (
        FetionContactList *self, TpHandle contact);
const gchar *fetion_contact_list_get_alias (
        FetionContactList *self, TpHandle contact);
void fetion_contact_list_set_alias (
        FetionContactList *self, TpHandle contact, const gchar *alias);

G_END_DECLS

#endif
