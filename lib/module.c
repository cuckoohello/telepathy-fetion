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

#include <dirent.h>
#include "util.h"
#include "module.h"
#include "fetion.h"

/* module chain stores the modules registered */
GSList *modules = NULL;

typedef gboolean (*ModuleInitFunc)(HybridModule *module);

HybridModule*
hybrid_module_create(const gchar *path)
{
    HybridModule *module = g_new0(HybridModule, 1);

    module->path = g_strdup(path);

    return module;
}

void
hybrid_module_destroy(HybridModule *module)
{
    g_return_if_fail(module != NULL);

    hybrid_module_deregister(module);

    if (module) {
        g_free(module->path);
    }

    g_free(module);
}

gint
hybrid_module_load(HybridModule *module)
{
    GModule        *gm;
    ModuleInitFunc  module_init;

    g_return_val_if_fail(module != NULL, HYBRID_ERROR);

    if (!(gm = g_module_open(module->path, G_MODULE_BIND_LOCAL))) {
        hybrid_debug_error("module", "%s", g_module_error());
        return HYBRID_ERROR;
    }

    if (!g_module_symbol(gm, "proto_module_init",
                         (gpointer*)&module_init)) {
        hybrid_debug_error("module", "%s", g_module_error());
        return HYBRID_ERROR;
    }

    module_init(module);

    /* load the option list. */
    if (module->info->options) {
        module->option_list = module->info->options();
    }

    return HYBRID_OK;
}

void
hybrid_module_register(HybridModule *module)
{
    GSList *iter;

    for (iter = modules; iter; iter = iter->next) {
        if (iter->data == module) {
            return;
        }
    }

    modules = g_slist_append(modules, module);
}

void
hybrid_module_deregister(HybridModule *module)
{
    g_return_if_fail(module != NULL);

    modules = g_slist_remove(modules, module);
}

HybridModule*
hybrid_module_find(const gchar *name)
{
    GSList       *iter;
    HybridModule *temp;

    for (iter = modules; iter; iter = iter->next) {
        temp = (HybridModule*)iter->data;

        if (g_strcmp0(temp->info->name, name) == 0) {
            return temp;
        }
    }

    return NULL;
}

gint
hybrid_module_init()
{
    HybridModule *module;

    hybrid_debug_info("module", "initialize module");

    module = hybrid_module_create("");
    fetion_module_register(module);

    return HYBRID_OK;
}
