/***************************************************************************
 *   Copyright (C) 2011 by levin                                           *
 *   levin108@gmail.com                                                    *
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

#include <glib.h>
#include "util.h"
#include "blist.h"
#include "connection.h"



static void hybrid_blist_set_group_name(HybridGroup *group, const gchar *name);
static void hybrid_blist_update_group(HybridGroup *group);
static void hybrid_blist_buddy_icon_save(HybridBuddy *buddy);
static void hybrid_blist_buddy_to_cache(HybridBuddy *buddy,
                                        HybridBlistCacheType type);
static void hybrid_blist_group_to_cache(HybridGroup *group);

static void hybrid_group_remove(HybridGroup *group);

static HybridAccount *current_choose_account = NULL;


HybridGroup*
hybrid_blist_add_group(HybridAccount *ac, const gchar *id, const gchar *name)
{
    g_return_val_if_fail(name != NULL , NULL);

    HybridGroup *group;

    g_return_val_if_fail(ac != NULL, NULL);
    g_return_val_if_fail(id != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);

    /*
     * Make sure we will add a group to an account only
     * when the account's connection status is CONNECTED
     */
    if (!HYBRID_IS_CONNECTED(ac)) {
        return NULL;
    }

    /*
     * If current account has a group exists with the specified ID,
     * then just return the existing group.
     */
    if ((group = hybrid_blist_find_group(ac, id))) {
        return group;
    }

    group = g_new0(HybridGroup, 1);
    group->renamable = 1;

    group->name    = g_strdup(name);
    group->id      = g_strdup(id);
    group->account = ac;

    g_hash_table_insert(ac->group_list, group->id, group);

    hybrid_blist_group_to_cache(group);

     FetionConnection *conn =
	         FETION_CONNECTION (ac->conn);


    tp_base_contact_list_groups_created((TpBaseContactList*)conn->priv->contact_list,&name,1); 

    return group;
}

void
hybrid_blist_remove_group(HybridGroup *group)
{
    HybridAccount *account;

    g_return_if_fail(group != NULL);

    account = group->account;


    /* remove from the cache file. */
    xmlnode_remove_node(group->cache_node);

    hybrid_blist_cache_flush();

    /*
     * remove from the account's group hashtable, and destroy it
     * with the DestroyNotify function registered before.
     */
    g_hash_table_remove(account->group_list, group->id);
}

void
hybrid_blist_group_destroy(HybridGroup *group)
{
    if (group) {
        g_free(group->id);
        g_free(group->name);

        g_free(group);
    }
}

HybridBuddy*
hybrid_blist_add_buddy(HybridAccount *ac, HybridGroup *parent, const gchar *id,
        const gchar *name)
{
    HybridBuddy  *buddy;
    TpHandle handle;


    g_return_val_if_fail(parent != NULL, NULL);
    g_return_val_if_fail(id != NULL, NULL);

    /*
     * Make sure we will add a buddy to an account only
     * when the account's connection status is CONNECTED
     */
    if (!HYBRID_IS_CONNECTED(ac)) {
        return NULL;
    }

    /*
     * If current account has a buddy exists with the specified ID,
     * then just return the existing buddy.
     */
    if ((buddy = hybrid_blist_find_buddy(ac, id))) {
        return buddy;
    }


    buddy = g_new0(HybridBuddy, 1);

    buddy->id      = g_strdup(id);
    buddy->name    = g_strdup(name);
    buddy->account = ac;
    buddy->parent  = parent;

    handle = tp_handle_ensure (ac->contact_repo, buddy->id,
		    NULL, NULL);

    g_hash_table_insert(ac->buddy_list, buddy->id, buddy);

    tp_handle_set_add(ac->contacts,handle);

     FetionConnection *conn =
	         FETION_CONNECTION (ac->conn);

    tp_base_contact_list_one_contact_changed((TpBaseContactList*)conn->priv->contact_list,handle);
    tp_base_contact_list_one_contact_groups_changed ((TpBaseContactList*)conn->priv->contact_list,handle,(const gchar *const *)&parent->name,1,NULL,0);

    tp_handle_unref(ac->contact_repo,handle);


    /*
     * Now we should update the buddy count displayed besides the gruop name.
     * We did add a new buddy, so we show add the buddy_count by one.
     */
    parent->buddy_count ++;

    hybrid_blist_update_group(parent);

    hybrid_blist_buddy_to_cache(buddy, HYBRID_BLIST_CACHE_ADD);

    return buddy;
}

void
hybrid_blist_remove_buddy(HybridBuddy *buddy)
{
    HybridAccount *account;
    HybridGroup   *group;
    TpHandle handle;

    g_return_if_fail(buddy != NULL);

    account = buddy->account;


    /* Remove the buddy from the xml cache context. */
    xmlnode_remove_node(buddy->cache_node);

    group = buddy->parent;
    group->buddy_count --;

    if (!BUDDY_IS_OFFLINE(buddy) && !BUDDY_IS_INVISIBLE(buddy)) {
        group->online_count --;
    }

    hybrid_blist_update_group(group);

    /* Synchronize the xml cache with the local xml file. */
    hybrid_blist_cache_flush();

    /* Remove the buddy from account's buddy_list hashtable. */


    handle =  tp_handle_lookup(account->contact_repo, buddy->id, NULL,NULL);

    tp_handle_set_remove(account->contacts,handle);
    g_hash_table_remove(account->buddy_list, buddy->id);
}

void
hybrid_blist_set_group_renamable(HybridGroup *group, gboolean renamable)
{
    g_return_if_fail(group != NULL);

    group->renamable = renamable ? 1 : 0;

    hybrid_blist_group_to_cache(group);
}

gboolean
hybrid_blist_get_group_renamable(HybridGroup *group)
{
    g_return_val_if_fail(group != NULL, TRUE);

    return group->renamable == 1;
}

const gchar*
hybrid_blist_get_buddy_checksum(HybridBuddy *buddy)
{
    g_return_val_if_fail(buddy != NULL, NULL);

    return buddy->icon_crc;
}


void
hybrid_blist_buddy_destroy(HybridBuddy *buddy)
{
    if (buddy) {
        g_free(buddy->id);
        g_free(buddy->name);
        g_free(buddy->mood);
        g_free(buddy->icon_name);
        g_free(buddy->icon_data);
        g_free(buddy->icon_crc);
        g_free(buddy);
    }
}

/**
 * Set the buddy's name field.
 */
static void
hybrid_blist_set_name_field(HybridBuddy *buddy)
{
    g_return_if_fail(buddy != NULL);

    TpHandle contact =  tp_handle_lookup(buddy->account->contact_repo, buddy->id, NULL,NULL);
    g_signal_emit (buddy->account->conn, signals[ALIAS_UPDATED], 0, contact);
}

/**
 * Update the group's display name, used when name or the number of account
 * changed, it won't synchronize with the local cache file.
 */
static void
hybrid_blist_update_group(HybridGroup *group)
{
    gchar        *name_str;

    g_return_if_fail(group != NULL);


    name_str = g_strdup_printf("<b>%s</b> (%d/%d)", group->name,
                    group->online_count, group->buddy_count);


    g_free(name_str);
}

/**
 * Set the group's name.
 */
static void
hybrid_blist_set_group_name(HybridGroup *group, const gchar *name)
{

    g_return_if_fail(group != NULL);
    g_return_if_fail(name != NULL);

    g_free(group->name);
    group->name = g_strdup(name);

    hybrid_blist_update_group(group);

    hybrid_blist_group_to_cache(group);
}

/**
 * Set the buddy state.
 */

void
hybrid_blist_set_buddy_name(HybridBuddy *buddy, const gchar *name)
{
    g_return_if_fail(buddy != NULL);

    /* if the name is unchanged, we do nothing but return. */
    if (g_strcmp0(buddy->name, name) == 0) {
        return;
    }

    hybrid_blist_set_buddy_name_priv(buddy, name);

    hybrid_blist_buddy_to_cache(buddy, HYBRID_BLIST_CACHE_UPDATE_NAME);
}

void
hybrid_blist_set_buddy_name_priv(HybridBuddy *buddy, const gchar *name)
{
    g_return_if_fail(buddy != NULL);

    g_free(buddy->name);
    buddy->name = g_strdup(name);

    hybrid_blist_set_name_field(buddy);
}

void
hybrid_blist_set_buddy_mood(HybridBuddy *buddy, const gchar *mood)
{
    g_return_if_fail(buddy != NULL);

    /* if the mood phrase is unchanged, we do nothing but return. */
    if (g_strcmp0(buddy->mood, mood) == 0) {
        return;
    }

    hybrid_blist_set_buddy_mood_priv(buddy, mood);

    hybrid_blist_buddy_to_cache(buddy, HYBRID_BLIST_CACHE_UPDATE_MOOD);
}

static void 
hybrid_blist_set_state_field(HybridBuddy *buddy)
{
  TpHandle contact =  tp_handle_lookup(buddy->account->contact_repo, buddy->id, NULL,NULL);
  g_signal_emit (buddy->account->conn, signals[PRESENCE_UPDATED], 0, contact);
}

void
hybrid_blist_set_buddy_mood_priv(HybridBuddy *buddy, const gchar *mood)
{
    g_return_if_fail(buddy != NULL);

    g_free(buddy->mood);
    buddy->mood = g_strdup(mood);

    hybrid_blist_set_state_field(buddy);
}

void
hybrid_blist_set_buddy_icon(HybridBuddy *buddy, const guchar *icon_data,
        gsize len, const gchar *crc)
{
    g_return_if_fail(buddy != NULL);

    g_free(buddy->icon_crc);
    g_free(buddy->icon_data);
    buddy->icon_data        = NULL;
    buddy->icon_data_length = len;
    buddy->icon_crc         = g_strdup(crc);

    if (icon_data != NULL) {
        buddy->icon_data = g_memdup(icon_data, len);
    }

    //hybrid_blist_set_state_field(buddy);
    hybrid_blist_buddy_icon_save(buddy);
    hybrid_blist_buddy_to_cache(buddy, HYBRID_BLIST_CACHE_UPDATE_ICON);
}

void
hybrid_blist_set_buddy_state(HybridBuddy *buddy, gint state)
{
    HybridGroup  *group;
    gint          online;

    g_return_if_fail(buddy != NULL);

    /*
     * Here we update the group's online count first by checking
     * whether state has changed
     */
    group = buddy->parent;

    online = group->online_count;

    if (buddy->state > 0 && state == 0) {
        online = group->online_count - 1;

    } else if (buddy->state == 0 && state > 0) {
        online = group->online_count + 1;
    }
    group->online_count = online;

    if (buddy->state != state) {
        buddy->state = state;
	hybrid_blist_set_state_field(buddy);
    }
}

void
hybrid_blist_set_buddy_status(HybridBuddy *buddy, gboolean authorized)
{
    g_return_if_fail(buddy != NULL);

    hybrid_blist_set_buddy_status_priv(buddy, authorized);

    hybrid_blist_buddy_to_cache(buddy, HYBRID_BLIST_CACHE_UPDATE_STATUS);
}

void
hybrid_blist_set_buddy_status_priv(HybridBuddy *buddy, gboolean authorized)
{
    gint          status;

    g_return_if_fail(buddy != NULL);


    status = authorized ? 0 : 1;

    /* we will do nothing if the status hasn't been changed. */
    if (status == buddy->status) {
        return;
    }

    buddy->status = status;
}

gboolean
hybrid_blist_get_buddy_authorized(HybridBuddy *buddy)
{
    g_return_val_if_fail(buddy != NULL, FALSE);

    return buddy->status == 0;
}

HybridGroup*
hybrid_blist_find_group(HybridAccount *account, const gchar *id)
{
    HybridGroup *group;

    g_return_val_if_fail(account != NULL, NULL);
    g_return_val_if_fail(id != NULL, NULL);

    if ((group = g_hash_table_lookup(account->group_list, id))) {
        return group;
    }

    return NULL;
}

HybridGroup*
hybrid_blist_find_group_by_name(HybridAccount *account, const gchar *name)
{
    GHashTableIter  hash_iter;
    gpointer        key;
    HybridGroup    *group;

    g_return_val_if_fail(account != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);

    g_hash_table_iter_init(&hash_iter, account->group_list);

    while (g_hash_table_iter_next(&hash_iter, &key, (gpointer*)&group)) {

        if (g_strcmp0(group->name, name) == 0) {
            return group;
        }
    }

    return NULL;
}

HybridBuddy*
hybrid_blist_find_buddy(HybridAccount *account, const gchar *id)
{
    HybridBuddy *buddy;


    g_return_val_if_fail(account != NULL, NULL);
    g_return_val_if_fail(id != NULL, NULL);


    if ((buddy = g_hash_table_lookup(account->buddy_list, id))) {
	    return buddy;
    }
    return NULL;
}


HybridBuddy*
hybrid_blist_find_buddy_by_handle(HybridAccount *account, TpHandle handle)
{
    HybridBuddy *buddy;


    g_return_val_if_fail(account != NULL, NULL);

    if ((buddy = g_hash_table_lookup(account->buddy_list, tp_handle_inspect(account->contact_repo,handle)))) {
	    return buddy;
    }
    return NULL;
}
void
hybrid_blist_select_first_item(HybridAccount *account)
{
    GHashTableIter    hash_iter;
    HybridGroup      *group;
    gpointer          key;

    g_return_if_fail(account != NULL);



#if 0
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        return;
    }
#endif

    g_hash_table_iter_init(&hash_iter, account->group_list);

    if (!g_hash_table_iter_next(&hash_iter, &key, (gpointer*)&group)) {
        return;
    }

    current_choose_account = group->account;

}

static void
hybrid_blist_group_to_cache(HybridGroup *group)
{
    HybridAccount    *ac;
    HybridConfig     *config;
    HybridBlistCache *cache;
    xmlnode          *root;
    xmlnode          *node;
    xmlnode          *temp;
    gchar            *username;
    gchar            *protoname;
    gchar            *id;
    gchar            *name;
    gchar            *renamable;

    g_return_if_fail(group != NULL);

    if (group->cache_node) {
        node = group->cache_node;
        goto group_exist;
    }

    ac     = group->account;
    config = ac->config;
    cache  = config->blist_cache;
    root   = cache->root;

    if (!(node = xmlnode_find(root, "accounts"))) {
        hybrid_debug_error("blist", "can't find accounts node in blist cache");
        return;
    }

    /* find node named 'account' */
    if ((temp = xmlnode_child(node))) {

        while (temp) {
            if (!xmlnode_has_prop(temp, "username") ||
                !xmlnode_has_prop(temp, "proto")) {
                hybrid_debug_error("blist", "invalid blist cache node found");
                temp = xmlnode_next(temp);
                continue;
            }

            username = xmlnode_prop(temp, "username");
            protoname = xmlnode_prop(temp, "proto");

            if (g_strcmp0(ac->username, username) == 0 &&
                g_strcmp0(ac->proto->info->name, protoname) == 0) {
                g_free(username);
                g_free(protoname);
                node = temp;
                goto account_exist;
            }

            temp = xmlnode_next(temp);

            g_free(username);
            g_free(protoname);
        }
    }

    node = xmlnode_new_child(node, "account");
    xmlnode_new_prop(node, "username", ac->username);
    xmlnode_new_prop(node, "proto", ac->proto->info->name);

account_exist:
    if (!(temp = xmlnode_find(node, "buddies"))) {
        node = xmlnode_new_child(node, "buddies");

    } else {
        node = temp;
    }

    /* Now node point to node named 'buddies'', make it
     * point to group node.
     *
     * check whether there was a group node that we need.
     */
    if ((temp = xmlnode_child(node))) {

        while (temp) {

            if (!xmlnode_has_prop(temp, "id") ||
                !xmlnode_has_prop(temp, "name")) {
                hybrid_debug_error("blist", "invalid blist cache group node found");
                temp = xmlnode_next(temp);
                continue;
            }

            id = xmlnode_prop(temp, "id");
            name = xmlnode_prop(temp, "name");

            if (g_strcmp0(id, group->id) == 0 &&
                g_strcmp0(name, group->name) == 0) {
                g_free(id);
                g_free(name);
                node = temp;
                goto group_exist;
            }

            temp = xmlnode_next(temp);

            g_free(id);
            g_free(name);
        }
    }

    node = xmlnode_new_child(node, "group");

group_exist:
    if (xmlnode_has_prop(node, "id")) {
        xmlnode_set_prop(node, "id", group->id);

    } else {
        xmlnode_new_prop(node, "id", group->id);
    }

    if (xmlnode_has_prop(node, "name")) {
        xmlnode_set_prop(node, "name", group->name);

    } else {
        xmlnode_new_prop(node, "name", group->name);
    }


    renamable = g_strdup_printf("%d", group->renamable);
    if (xmlnode_has_prop(node, "renamable")) {
        xmlnode_set_prop(node, "renamable", renamable);

    } else {
        xmlnode_new_prop(node, "renamable", renamable);
    }

    g_free(renamable);

    group->cache_node = node;

    hybrid_blist_cache_flush();
}


/**
 * Write the buddy information to the cache which in fact is
 * a XML tree in the memory, if you want to synchronize the cache
 * with the cache file, use hybrid_blist_cache_flush().
 *
 * @param buddy The buddy to write to cache.
 * @param type  The action of writing to cache.
 */
static void
hybrid_blist_buddy_to_cache(HybridBuddy *buddy, HybridBlistCacheType type)
{
    HybridAccount    *ac;
    HybridConfig     *config;
    HybridBlistCache *cache;
    xmlnode          *root;
    xmlnode          *node;
    xmlnode          *temp;
    gchar            *value;
    gchar            *id;

    g_return_if_fail(buddy != NULL);

    if (buddy->cache_node) {
        node = buddy->cache_node;
        goto buddy_exist;
    }

    ac     = buddy->account;
    config = ac->config;
    cache  = config->blist_cache;
    root   = cache->root;

    if (!(node = buddy->parent->cache_node)) {
        hybrid_debug_error("blist",
                "group node isn't cached, cache buddy failed");
        return;
    }

    /* whether there's a buddy node */
    if ((temp = xmlnode_child(node))) {

        while (temp) {

            if (!xmlnode_has_prop(temp, "id")) {
                hybrid_debug_error("blist",
                        "invalid blist cache buddy node found");
                temp = xmlnode_next(temp);
                continue;
            }

            id = xmlnode_prop(temp, "id");

            if (g_strcmp0(id, buddy->id) == 0) {
                g_free(id);
                node = temp;
                goto buddy_exist;
            }

            temp = xmlnode_next(temp);

            g_free(id);
        }
    }

    node = xmlnode_new_child(node, "buddy");

buddy_exist:

    if (type == HYBRID_BLIST_CACHE_ADD) {

        if (xmlnode_has_prop(node, "id")) {
            xmlnode_set_prop(node, "id", buddy->id);

        } else {
            xmlnode_new_prop(node, "id", buddy->id);
        }
    }

    if (type == HYBRID_BLIST_CACHE_ADD ||
        type == HYBRID_BLIST_CACHE_UPDATE_NAME) {

        if (xmlnode_has_prop(node, "name")) {
            xmlnode_set_prop(node, "name", buddy->name);

        } else {
            xmlnode_new_prop(node, "name", buddy->name);
        }
    }

    if (type == HYBRID_BLIST_CACHE_UPDATE_MOOD) {

        if (xmlnode_has_prop(node, "mood")) {
            xmlnode_set_prop(node, "mood", buddy->mood);

        } else {
            xmlnode_new_prop(node, "mood", buddy->mood);
        }
    }

    if (type == HYBRID_BLIST_CACHE_UPDATE_ICON) {

        if (xmlnode_has_prop(node, "crc")) {
            xmlnode_set_prop(node, "crc", buddy->icon_crc);

        } else {
            xmlnode_new_prop(node, "crc", buddy->icon_crc);
        }

        if (xmlnode_has_prop(node, "icon")) {
            xmlnode_set_prop(node, "icon", buddy->icon_name);

        } else {
            xmlnode_new_prop(node, "icon", buddy->icon_name);
        }
    }

    if (type == HYBRID_BLIST_CACHE_UPDATE_STATUS) {

        value = g_strdup_printf("%d", buddy->status);

        if (xmlnode_has_prop(node, "status")) {
            xmlnode_set_prop(node, "status", value);

        } else {
            xmlnode_new_prop(node, "status", value);
        }

        g_free(value);
    }

    /* Set the buddy's xml cache node property. */
    buddy->cache_node = node;

    hybrid_blist_cache_flush();
}


/**
 * Save the buddy's icon to the local file.The naming method is:
 * SHA1(buddy_id + '_' + proto_name).type
 */
static void
hybrid_blist_buddy_icon_save(HybridBuddy *buddy)
{
    gchar         *name;
    gchar         *hashed_name;
    HybridAccount *account;
    HybridModule  *module;
    HybridConfig  *config;

    g_return_if_fail(buddy != NULL);

    if (buddy->icon_data == NULL || buddy->icon_data_length == 0) {
        return;
    }

    account = buddy->account;
    module  = account->proto;
    config  = account->config;

    if (!buddy->icon_name) {
        name = g_strdup_printf("%s_%s", module->info->name, buddy->id);
        hashed_name = hybrid_sha1(name, strlen(name));
        g_free(name);

        buddy->icon_name = g_strdup_printf("%s.jpg", hashed_name);

        name = g_strdup_printf("%s/%s.jpg", config->icon_path, hashed_name);
        g_free(hashed_name);

    } else {
        name = g_strdup_printf("%s/%s", config->icon_path, buddy->icon_name);
    }

    g_file_set_contents(name, (gchar*)buddy->icon_data,
            buddy->icon_data_length, NULL);

    TpHandle contact =  tp_handle_lookup(account->contact_repo, buddy->id, NULL,NULL);
    g_signal_emit (buddy->account->conn, signals[AVATAR_UPDATED], 0, contact,buddy->icon_name);

    g_free(name);
}
