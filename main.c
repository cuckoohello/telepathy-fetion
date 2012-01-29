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

#include <telepathy-glib/debug.h>
#include <telepathy-glib/run.h>
#ifndef CONFIG_ARM
#include <gtk/gtk.h>
#endif

#include "connection-manager.h"
#include "logs.h"
#include "config.h"

    static TpBaseConnectionManager *
construct_cm (void)
{
    return (TpBaseConnectionManager *) g_object_new (
            FETION_TYPE_CONNECTION_MANAGER,
            NULL);
}

    int
main (int argc, char **argv)
{
#ifdef ENABLE_DEBUG
    tp_debug_divert_messages (g_getenv ("FETION_CM_LOGFILE"));
    tp_debug_set_flags (g_getenv ("FETION_DEBUG"));

    if (g_getenv ("FETION_TIMING") != NULL)
        g_log_set_default_handler (tp_debug_timestamped_log_handler, NULL);

    if (g_getenv ("FETION_PERSIST") != NULL)
        tp_debug_set_persistent (TRUE);
#endif

    if(!g_thread_supported())
        g_thread_init(NULL);

#ifndef CONFIG_ARM
    gdk_threads_init();
    gtk_init(&argc, &argv);
#endif

    hybrid_config_init();

    hybrid_logs_init();

    hybrid_module_init();

    hybrid_account_init();

    return tp_run_connection_manager ("telepathy-fetion",
            PACKAGE_VERSION, construct_cm, argc, argv);
}
