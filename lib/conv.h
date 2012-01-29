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
#ifndef _CONV_H_
#define _CONV_H_

#include "blist.h"

#ifdef __cplusplus
extern "C" {
#endif

    void hybrid_conv_got_message(HybridAccount *account,
            const gchar *buddy_id, const gchar *message,
            time_t time);
    void hybrid_conv_send_message(HybridAccount *account,HybridBuddy *buddy,const gchar *message);


#ifdef _cplusplus
}
#endif

#endif /* HYBRID_BLIST_H */
