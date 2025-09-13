/*
 * Unix interface definitions
 *
 * Copyright 2024 Vibhav Pant
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINEBTH_UNIXLIB_H
#define __WINE_WINEBTH_UNIXLIB_H

#include <stdlib.h>
#include <stdarg.h>

#include <windef.h>
#include <winbase.h>

#include <bthsdpdef.h>
#include <bluetoothapis.h>

#include <wine/unixlib.h>
#include <wine/debug.h>

#include "winebth_priv.h"

#ifdef WINE_UNIX_LIB
typedef struct unix_name *unix_name_t;
typedef void *unix_handle_t;
#else
typedef UINT_PTR unix_name_t;
typedef UINT_PTR unix_handle_t;
#endif

struct bluetooth_adapter_free_params
{
    unix_name_t adapter;
};

struct bluetooth_adapter_get_unique_name_params
{
    unix_name_t adapter;

    char *buf;
    SIZE_T buf_size;
};

struct bluetooth_get_event_params
{
    struct winebluetooth_event result;
};

enum bluetoothapis_funcs
{
    unix_bluetooth_init,
    unix_bluetooth_shutdown,

    unix_bluetooth_adapter_get_unique_name,
    unix_bluetooth_adapter_free,

    unix_bluetooth_get_event,

    unix_funcs_count
};

#define UNIX_BLUETOOTH_CALL( func, params ) WINE_UNIX_CALL( unix_##func, params )

#endif /* __WINE_WINEBTH_UNIXLIB_H */
