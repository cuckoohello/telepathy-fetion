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
#include "confirm.h"
#include <sys/types.h>
#include <sys/wait.h>

void hybrid_confirm_window_create(
        HybridAccount *account, guchar *pic_data, gint pic_len,
        HybridConfirmOkFunc ok_func, HybridConfirmCancelFunc cancel_func,
        gpointer user_data)
{
    pid_t   pid;
    g_file_set_contents("/tmp/fetionauthcode.jpg", (const gchar*)pic_data,pic_len,NULL);
    if ((pid = fork()) < 0) {
        hybrid_debug_error("authcode","fork error!");
    } else if (pid == 0) {  /* specify pathname, specify environment */

        if (execl("/usr/share/telepathy-fetion/bin/fetion_auth", "fetion_auth", (char *)0 ) < 0)
            hybrid_debug_error("authcode","execle error");
    }

    if (waitpid(pid, NULL, 0) < 0)
        hybrid_debug_error("authcode","wait error");

    const gchar          *auth_data;
    gsize            auth_len;

    g_file_get_contents("/tmp/fetionauthcode.txt",
            (gchar**)&auth_data, &auth_len, NULL);
    if(auth_len >0)
    {
        ok_func(account,auth_data,user_data);
    }else
        cancel_func(account,user_data);
}
