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
#include "connection.h"
#include "roster.h"
#include <string.h>

#include <dbus/dbus-glib.h>

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/telepathy-glib.h>

static void mutable_contact_list_iface_init (TpMutableContactListInterface *);
static void contact_group_list_iface_init (TpContactGroupListInterface *);
static void mutable_contact_group_list_iface_init (TpMutableContactGroupListInterface *);

G_DEFINE_TYPE_WITH_CODE (FetionContactList,
        fetion_contact_list,
        TP_TYPE_BASE_CONTACT_LIST,
        G_IMPLEMENT_INTERFACE (TP_TYPE_CONTACT_GROUP_LIST,
            contact_group_list_iface_init);
        G_IMPLEMENT_INTERFACE (TP_TYPE_MUTABLE_CONTACT_GROUP_LIST,
            mutable_contact_group_list_iface_init);
        G_IMPLEMENT_INTERFACE (TP_TYPE_MUTABLE_CONTACT_LIST,
            mutable_contact_list_iface_init))


struct _FetionContactListPrivate
{
    TpBaseConnection *conn;
    gulong status_changed_id;
};

    static void
fetion_contact_list_init (FetionContactList *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
            FETION_TYPE_CONTACT_LIST, FetionContactListPrivate);
}

    static void
fetion_contact_list_close_all (FetionContactList *self)
{
    if (self->priv->status_changed_id != 0)
    {
        g_signal_handler_disconnect (self->priv->conn,
                self->priv->status_changed_id);
        self->priv->status_changed_id = 0;
    }
}

    static void
dispose (GObject *object)
{
    FetionContactList *self = FETION_CONTACT_LIST (object);
    fetion_contact_list_close_all(self);

    ((GObjectClass *) fetion_contact_list_parent_class)->dispose (
        object);
}

    static void
fetion_contact_list_set_contact_groups_async (TpBaseContactList *contact_list,
        TpHandle contact,
        const gchar * const *names,
        gsize n,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn =
        FETION_CONNECTION (self->priv->conn);
    HybridAccount *account = conn->priv->account;


    if(n!=1)
        return ; // can only have one group

    //tp_base_contact_list_groups_created (contact_list, names, n); //here we assume the groups is already exist

    //tp_base_contact_list_one_contact_changed (contact_list, contact);

    HybridBuddy* buddy = hybrid_blist_find_buddy_by_handle(account,contact);

    // here we assume the account must have default group and only one
    gchar *oldgroup = buddy->parent->name; 



    if (!tp_strdiff (oldgroup,names[0] ))
        return; // the same group nothing need to do 

    HybridGroup *group = hybrid_blist_find_group_by_name(account, names[0]);
    if (group == NULL)
        return ; // the group must existed
    if(!account->proto->info->im_ops->buddy_move(account,buddy,group))
        return; // move group failed

    tp_base_contact_list_one_contact_groups_changed (contact_list, contact,
            (const gchar * const *) names[0], 1,
            (const gchar * const *) oldgroup, 1);
    tp_simple_async_report_success_in_idle ((GObject *) self, callback,
            user_data, fetion_contact_list_set_contact_groups_async);
}

    static void
status_changed_cb (TpBaseConnection *conn,
        guint status,
        guint reason,
        FetionContactList *self)
{
    switch (status)
    {
        case TP_CONNECTION_STATUS_CONNECTED:
            {
                /* Do network I/O to get the contact list. */
            }
            break;

        case TP_CONNECTION_STATUS_DISCONNECTED:
            {
                fetion_contact_list_close_all (self);
                tp_clear_object (&self->priv->conn);
            }
            break;
    }
}

    static void
constructed (GObject *object)
{
    FetionContactList *self = FETION_CONTACT_LIST (object);
    void (*chain_up) (GObject *) =
        ((GObjectClass *) fetion_contact_list_parent_class)->constructed;

    if (chain_up != NULL)
    {
        chain_up (object);
    }

    g_object_get (self,
            "connection", &self->priv->conn,
            NULL);
    g_assert (self->priv->conn != NULL);
    self->priv->status_changed_id = g_signal_connect (self->priv->conn,
            "status-changed", (GCallback) status_changed_cb, self);

}

    static void
fetion_contact_list_set_group_members_async (TpBaseContactList *contact_list,
        const gchar *group,
        TpHandleSet *contacts,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn =
        FETION_CONNECTION (self->priv->conn);
    HybridAccount *account = conn->priv->account;
    HybridGroup *dest_group = hybrid_blist_find_group_by_name(account, group);
    if (dest_group == NULL)
        return ; // the group must existed

    TpHandleSet *added = tp_handle_set_new (account->contact_repo);
    TpHandleSet *removed = tp_handle_set_new (account->contact_repo);

    TpIntsetFastIter iter;
    TpHandle member;

    tp_intset_fast_iter_init (&iter, tp_handle_set_peek (contacts));

    while (tp_intset_fast_iter_next (&iter, &member))
    {

        HybridBuddy* buddy = hybrid_blist_find_buddy_by_handle(account,member);
        if (buddy == NULL)
            continue; // must existed
        HybridGroup *origin_group = buddy->parent;
        if (origin_group == dest_group)
            continue;

        if(!account->proto->info->im_ops->buddy_move(account,buddy,dest_group))
            continue;

        tp_handle_set_add (added, member);
    }

    tp_intset_fast_iter_init (&iter, tp_handle_set_peek (account->contacts));

    HybridGroup *default_group = hybrid_blist_find_group(account, "0"); 
    g_assert (default_group != NULL);
    while (tp_intset_fast_iter_next (&iter, &member))
    {
        if (tp_handle_set_is_member (contacts, member))
            continue;

        HybridBuddy* buddy = hybrid_blist_find_buddy_by_handle(account,member);
        if (buddy == NULL)
            continue; // must existed
        HybridGroup *origin_group = buddy->parent;
        if (origin_group != dest_group)
            continue;

        if(!account->proto->info->im_ops->buddy_move(account,buddy,default_group))
            continue;

        tp_handle_set_add (removed, member);
    }


    if (!tp_handle_set_is_empty (added))
        tp_base_contact_list_groups_changed (contact_list, added, &group, 1,
                NULL, 0);

    if (!tp_handle_set_is_empty (removed))
        tp_base_contact_list_groups_changed (contact_list, removed, NULL, 0,
                &group, 1);

    tp_handle_set_destroy (added);
    tp_handle_set_destroy (removed);
    tp_simple_async_report_success_in_idle ((GObject *) self, callback,
            user_data, fetion_contact_list_set_group_members_async);
}

    static void
fetion_contact_list_add_to_group_async (TpBaseContactList *contact_list,
        const gchar *group,
        TpHandleSet *contacts,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn =
        FETION_CONNECTION (self->priv->conn);
    HybridAccount *account = conn->priv->account;
    HybridGroup *dest_group = hybrid_blist_find_group_by_name(account, group);
    if(dest_group == NULL)
        return ; // group must exist

    TpHandleSet *new_to_group = tp_handle_set_new (account->contact_repo);

    TpIntsetFastIter iter;
    TpHandle member;

    tp_intset_fast_iter_init (&iter, tp_handle_set_peek (contacts));

    while (tp_intset_fast_iter_next (&iter, &member))
    {
        HybridBuddy* buddy = hybrid_blist_find_buddy_by_handle(account,member);
        if (buddy == NULL)
            continue; // must existed
        HybridGroup *origin_group = buddy->parent;
        if (origin_group == dest_group)
            continue;

        if(!account->proto->info->im_ops->buddy_move(account,buddy,dest_group))
            continue;

        tp_handle_set_add (new_to_group, member);

    }


    if (!tp_handle_set_is_empty (new_to_group))
        tp_base_contact_list_groups_changed (contact_list, new_to_group, &group, 1,
                NULL, 0);

    tp_handle_set_destroy (new_to_group);
    tp_simple_async_report_success_in_idle ((GObject *) self, callback,
            user_data, fetion_contact_list_add_to_group_async);
}

    static void
fetion_contact_list_remove_from_group_async (TpBaseContactList *contact_list,
        const gchar *group,
        TpHandleSet *contacts,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn =
        FETION_CONNECTION (self->priv->conn);
    HybridAccount *account = conn->priv->account;
    HybridGroup *origin_group = hybrid_blist_find_group_by_name(account, group);
    HybridGroup *default_group = hybrid_blist_find_group(account, "0");
    g_assert (default_group != NULL);


    TpHandleSet *changed = tp_handle_set_new (account->contact_repo);
    TpIntsetFastIter iter;
    TpHandle member;

    tp_intset_fast_iter_init (&iter, tp_handle_set_peek (contacts));

    while (tp_intset_fast_iter_next (&iter, &member))
    {
        HybridBuddy* buddy = hybrid_blist_find_buddy_by_handle(account,member);
        if(buddy == NULL)
            continue;
        HybridGroup *current_group = buddy->parent;
        if (current_group != origin_group)
            continue;
        if(!account->proto->info->im_ops->buddy_move(account,buddy,default_group))
            continue;

        tp_handle_set_add (changed, member);
    }

    if (!tp_handle_set_is_empty (changed))
        tp_base_contact_list_groups_changed (contact_list, changed, NULL, 0,
                &group, 1);

    tp_handle_set_destroy (changed);
    tp_simple_async_report_success_in_idle ((GObject *) self, callback,
            user_data, fetion_contact_list_remove_from_group_async);
}

    static TpHandleSet *
fetion_contact_list_dup_contacts (TpBaseContactList *contact_list)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn = FETION_CONNECTION (self->priv->conn);
    return tp_handle_set_copy (conn->priv->account->contacts); 
}

    static TpHandleSet *
fetion_contact_list_dup_group_members (TpBaseContactList *contact_list,
        const gchar *group)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn = FETION_CONNECTION (self->priv->conn);
    HybridAccount* account = conn->priv->account;

    HybridGroup *lookup_group = hybrid_blist_find_group_by_name(account, group);

    TpIntsetFastIter iter;
    TpHandle member;
    TpHandleSet *members = tp_handle_set_new (account->contact_repo);

    tp_intset_fast_iter_init (&iter, tp_handle_set_peek (account->contacts));

    while (tp_intset_fast_iter_next (&iter, &member))
    {
        HybridBuddy* buddy = hybrid_blist_find_buddy_by_handle(account,member);
        if (buddy == NULL)
            continue;
        if (lookup_group != buddy->parent)
            continue;

        tp_handle_set_add (members, member);
    }

    return members;
}


    static inline TpSubscriptionState
compose_presence (gboolean full,
        gboolean ask,
        gboolean removed_remotely)
{
    // TODO remove
    if (full)
        return TP_SUBSCRIPTION_STATE_YES;
    else if (ask)
        return TP_SUBSCRIPTION_STATE_ASK;
    else if (removed_remotely)
        return TP_SUBSCRIPTION_STATE_REMOVED_REMOTELY;
    else
        return TP_SUBSCRIPTION_STATE_NO;
}

    static void
fetion_contact_list_dup_states (TpBaseContactList *contact_list,
        TpHandle contact,
        TpSubscriptionState *subscribe,
        TpSubscriptionState *publish,
        gchar **publish_request)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn = FETION_CONNECTION (self->priv->conn);
    HybridAccount* account = conn->priv->account;
    HybridBuddy* buddy = hybrid_blist_find_buddy_by_handle(account,contact);
    if(buddy != NULL && hybrid_blist_get_buddy_authorized(buddy))
    {
        if (subscribe != NULL)
            *subscribe = TP_SUBSCRIPTION_STATE_YES ;
        if (publish != NULL)
            *publish = TP_SUBSCRIPTION_STATE_YES;
    }else
    {
        if (subscribe != NULL)
            *subscribe = TP_SUBSCRIPTION_STATE_NO ;
        if (publish != NULL)
            *publish = TP_SUBSCRIPTION_STATE_NO;
    }
}

static void
fetion_contact_list_request_subscription_async (
        TpBaseContactList *contact_list,
        TpHandleSet *contacts,
        const gchar *message,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn = FETION_CONNECTION (self->priv->conn);
    HybridAccount* account = conn->priv->account;

    TpIntsetFastIter iter;
    TpHandle member;

    tp_intset_fast_iter_init (&iter, tp_handle_set_peek (contacts));

    while (tp_intset_fast_iter_next (&iter, &member))
    {
        HybridBuddy* buddy = hybrid_blist_find_buddy_by_handle(account,member);

        /* if they already authorized us, it's a no-op */
        if (buddy != NULL)
            continue;
        HybridGroup *default_group = hybrid_blist_find_group(account, "0"); 
        g_assert (default_group != NULL);

        account->proto->info->im_ops->buddy_add(account,default_group,tp_handle_inspect(account->contact_repo,member),NULL,message);

    }

    tp_simple_async_report_success_in_idle ((GObject *) self, callback,
            user_data, fetion_contact_list_request_subscription_async);
}

static void
fetion_contact_list_authorize_publication_async (
        TpBaseContactList *contact_list,
        TpHandleSet *contacts,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    //TODO fix
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);

    tp_simple_async_report_success_in_idle ((GObject *) self, callback,
            user_data, fetion_contact_list_authorize_publication_async);
}

static void
fetion_contact_list_store_contacts_async (
        TpBaseContactList *contact_list,
        TpHandleSet *contacts,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);

    tp_simple_async_report_success_in_idle ((GObject *) self, callback,
            user_data, fetion_contact_list_store_contacts_async);
}

    static void
fetion_contact_list_remove_contacts_async (TpBaseContactList *contact_list,
        TpHandleSet *contacts,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn = FETION_CONNECTION (self->priv->conn);
    HybridAccount* account = conn->priv->account;

    TpHandleSet *removed = tp_handle_set_new (account->contact_repo);
    TpIntsetFastIter iter;
    TpHandle member;

    tp_intset_fast_iter_init (&iter, tp_handle_set_peek (contacts));

    while (tp_intset_fast_iter_next (&iter, &member))
    {
        HybridBuddy* buddy = hybrid_blist_find_buddy_by_handle(account,member);
        if (buddy == NULL)
            continue;
        if(!account->proto->info->im_ops->buddy_remove(account,buddy))
            continue;

        tp_handle_set_add (removed, member);
    }

    tp_base_contact_list_contacts_changed (contact_list, NULL, removed);

    tp_simple_async_report_success_in_idle ((GObject *) self, callback,
            user_data, fetion_contact_list_remove_contacts_async);
}

    static void
fetion_contact_list_unsubscribe_async (TpBaseContactList *contact_list,
        TpHandleSet *contacts,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    // TODO
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);

    tp_simple_async_report_success_in_idle ((GObject *) self, callback,
            user_data, fetion_contact_list_unsubscribe_async);
}

    static void
fetion_contact_list_unpublish_async (TpBaseContactList *contact_list,
        TpHandleSet *contacts,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    // TODO
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    tp_simple_async_report_success_in_idle ((GObject *) self, callback,
            user_data, fetion_contact_list_unpublish_async);
}

static guint
fetion_contact_list_get_group_storage (
        TpBaseContactList *contact_list G_GNUC_UNUSED)
{
    return TP_CONTACT_METADATA_STORAGE_TYPE_ANYONE;
}

    static GStrv
fetion_contact_list_dup_groups (TpBaseContactList *contact_list)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn = FETION_CONNECTION (self->priv->conn);
    HybridAccount* account = conn->priv->account;

    GPtrArray *tags = g_ptr_array_sized_new (
            g_hash_table_size (account->group_list) + 1);
    GHashTableIter iter;
    gpointer tag;
    HybridGroup    *group;

    g_hash_table_iter_init (&iter, account->group_list);

    while (g_hash_table_iter_next (&iter, &tag, (gpointer*)&group))
        g_ptr_array_add (tags, g_strdup (group->name));

    g_ptr_array_add (tags, NULL);
    return (GStrv) g_ptr_array_free (tags, FALSE);
}

    static GStrv
fetion_contact_list_dup_contact_groups (TpBaseContactList *contact_list,
        TpHandle contact)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn = FETION_CONNECTION (self->priv->conn);
    HybridAccount* account = conn->priv->account;

    GPtrArray *tags = g_ptr_array_sized_new (2);

    HybridBuddy* buddy = hybrid_blist_find_buddy_by_handle(account,contact);
    if(buddy != NULL)
    {

        g_ptr_array_add (tags, g_strdup (buddy->parent->name));
    }

    g_ptr_array_add (tags, NULL);
    return (GStrv) g_ptr_array_free (tags, FALSE);
}

    static void
fetion_contact_list_remove_group_async (TpBaseContactList *contact_list,
        const gchar *group,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn = FETION_CONNECTION (self->priv->conn);
    HybridAccount* account = conn->priv->account;

    /* signal the deletion */
    g_message ("deleting group %s", group);

    /* apply the change to our model of the contacts too; we don't need to signal
     * the change, because TpBaseContactList already did */
    HybridGroup *delete_group = hybrid_blist_find_group_by_name(account, group);
    if(delete_group == NULL || !hybrid_blist_get_group_renamable(delete_group))
        return;

    if(!account->proto->info->im_ops->group_remove(account,delete_group))
        return;

    tp_base_contact_list_groups_removed (contact_list, &group, 1);

    tp_simple_async_report_success_in_idle ((GObject *) contact_list, callback,
            user_data, fetion_contact_list_remove_group_async);
}

    static gchar *
fetion_contact_list_normalize_group (TpBaseContactList *contact_list,
        const gchar *id)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn = FETION_CONNECTION (self->priv->conn);
    HybridAccount* account = conn->priv->account;
    if(hybrid_blist_find_group_by_name(account,id))
        return g_utf8_normalize (id, -1, G_NORMALIZE_ALL_COMPOSE);
    else
        return NULL;
}

    static void
fetion_contact_list_rename_group_async (TpBaseContactList *contact_list,
        const gchar *old_name,
        const gchar *new_name,
        GAsyncReadyCallback callback,
        gpointer user_data)
{
    FetionContactList *self = FETION_CONTACT_LIST (contact_list);
    FetionConnection *conn = FETION_CONNECTION (self->priv->conn);
    HybridAccount* account = conn->priv->account;

    HybridGroup *rename_group = hybrid_blist_find_group_by_name(account, old_name);
    if(rename_group == NULL || !hybrid_blist_get_group_renamable(rename_group))
        return;

    if(!account->proto->info->im_ops->group_rename(account,rename_group,new_name))
        return;

    g_print ("renaming group %s to %s", old_name, new_name);

    tp_base_contact_list_group_renamed (contact_list, old_name, new_name); 

    /* update our model (this doesn't need to signal anything because
     * TpBaseContactList already did) */

    tp_simple_async_report_success_in_idle ((GObject *) contact_list, callback,
            user_data, fetion_contact_list_rename_group_async);
}

    static void
fetion_contact_list_class_init (FetionContactListClass *klass)
{
    TpBaseContactListClass *contact_list_class =
        (TpBaseContactListClass *) klass;
    GObjectClass *object_class = (GObjectClass *) klass;

    object_class->constructed = constructed;
    object_class->dispose = dispose;

    contact_list_class->dup_contacts = fetion_contact_list_dup_contacts;
    contact_list_class->dup_states = fetion_contact_list_dup_states;
    /* for this fetion CM we pretend there is a server-stored contact list,
     * like in XMPP, even though there obviously isn't really */
    contact_list_class->get_contact_list_persists =
        tp_base_contact_list_true_func;

    g_type_class_add_private (klass, sizeof (FetionContactListPrivate));

}

    static void
mutable_contact_list_iface_init (TpMutableContactListInterface *iface)
{
    // TODO
    iface->can_change_contact_list = tp_base_contact_list_true_func;
    iface->get_request_uses_message = tp_base_contact_list_true_func;
    iface->request_subscription_async =
        fetion_contact_list_request_subscription_async;
    iface->authorize_publication_async =
        fetion_contact_list_authorize_publication_async;
    iface->store_contacts_async = fetion_contact_list_store_contacts_async;
    iface->remove_contacts_async = fetion_contact_list_remove_contacts_async;
    iface->unsubscribe_async = fetion_contact_list_unsubscribe_async;
    iface->unpublish_async = fetion_contact_list_unpublish_async;
}

    static void
contact_group_list_iface_init (TpContactGroupListInterface *iface)
{
    iface->dup_groups = fetion_contact_list_dup_groups;
    iface->dup_group_members = fetion_contact_list_dup_group_members;
    iface->dup_contact_groups = fetion_contact_list_dup_contact_groups;
    iface->has_disjoint_groups = tp_base_contact_list_true_func;
    iface->normalize_group = fetion_contact_list_normalize_group;
}

static void
mutable_contact_group_list_iface_init (
        TpMutableContactGroupListInterface *iface)
{
    iface->set_group_members_async =
        fetion_contact_list_set_group_members_async;
    iface->add_to_group_async = fetion_contact_list_add_to_group_async;
    iface->remove_from_group_async =
        fetion_contact_list_remove_from_group_async;
    iface->remove_group_async = fetion_contact_list_remove_group_async;
    iface->rename_group_async = fetion_contact_list_rename_group_async;
    iface->set_contact_groups_async =
        fetion_contact_list_set_contact_groups_async;
    iface->get_group_storage = fetion_contact_list_get_group_storage;
}

