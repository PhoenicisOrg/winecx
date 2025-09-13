/*
 * winebluetooth Unix interface
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

#if 0
#pragma makedep unix
#endif

#include <config.h>

#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <pthread.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <winternl.h>
#include <winbase.h>
#include <windef.h>
#include <wine/list.h>
#include <wine/rbtree.h>
#include <wine/debug.h>

#include "unixlib.h"
#include "unixlib_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL( winebth );

static int compare_string( const void *key, const struct wine_rb_entry *entry )
{
    struct unix_name *str = WINE_RB_ENTRY_VALUE( entry, struct unix_name, entry );
    return strcmp( key, str->str );
}

static struct rb_tree names = { .compare = compare_string };
static pthread_mutex_t names_mutex = PTHREAD_MUTEX_INITIALIZER;

struct unix_name *unix_name_get_or_create( const char *str )
{
    struct rb_entry *entry;
    struct unix_name *s;

    pthread_mutex_lock( &names_mutex );
    entry = rb_get( &names, str );
    if (!entry)
    {
        struct unix_name *s = malloc( sizeof( struct unix_name ) );
        if (!s)
        {
            pthread_mutex_unlock( &names_mutex );
            return NULL;
        }
        s->str = strdup( str );
        s->refcnt = 0;
        rb_put( &names, str, &s->entry );
        entry = &s->entry;
    }
    s = RB_ENTRY_VALUE( entry, struct unix_name, entry );
    s->refcnt++;
    pthread_mutex_unlock( &names_mutex );
    return s;
}

void unix_name_free( struct unix_name *name )
{
    pthread_mutex_lock( &names_mutex );
    name->refcnt--;
    if (name->refcnt == 0)
    {
        rb_remove( &names, &name->entry );
        free( name );
    }
    pthread_mutex_unlock( &names_mutex );
}

static void *dbus_connection;
static void *bluetooth_watcher;

static NTSTATUS bluetooth_init ( void *params )
{
    NTSTATUS status;

    dbus_connection = bluez_dbus_init();
    if (!dbus_connection)
        return STATUS_INTERNAL_ERROR;

    status = bluez_watcher_init( dbus_connection, &bluetooth_watcher );
    if (status)
        bluez_dbus_close( dbus_connection );
    else
        TRACE( "dbus_connection=%p bluetooth_watcher=%p\n", dbus_connection, bluetooth_watcher );
    return status;
}

static NTSTATUS bluetooth_shutdown( void *params )
{
    if (!dbus_connection) return STATUS_NOT_SUPPORTED;

    bluez_dbus_close( dbus_connection );
    bluez_dbus_free( dbus_connection );
    return STATUS_SUCCESS;
}

static NTSTATUS get_unique_name( const struct unix_name *name, char *buf, SIZE_T *buf_size )
{
    SIZE_T path_len, i;

    path_len = strlen( name->str );
    if (*buf_size <= (path_len * sizeof(char)))
    {
        *buf_size = (path_len + 1) * sizeof(char);
        return STATUS_BUFFER_TOO_SMALL;
    }

    for (i = 0; i < path_len; i++)
    {
        if (name->str[i] == '/') buf[i] = '_';
        else
            buf[i] = name->str[i];
    }
    buf[path_len] = '\0';
    return STATUS_SUCCESS;
}

static NTSTATUS bluetooth_adapter_get_unique_name( void *args )
{
    struct bluetooth_adapter_get_unique_name_params *params = args;
    if (!dbus_connection) return STATUS_NOT_SUPPORTED;

    return get_unique_name( params->adapter, params->buf, &params->buf_size );
}

static NTSTATUS bluetooth_adapter_free( void *args )
{
    struct bluetooth_adapter_free_params *params = args;
    unix_name_free( params->adapter );
    return STATUS_SUCCESS;
}

static NTSTATUS bluetooth_get_event( void *args )
{
    struct bluetooth_get_event_params *params = args;

    if (!dbus_connection) return STATUS_NOT_SUPPORTED;
    memset( &params->result, 0, sizeof( params->result ) );
    return bluez_dbus_loop( dbus_connection, bluetooth_watcher, &params->result );
}

const unixlib_entry_t __wine_unix_call_funcs[] = {
    bluetooth_init,
    bluetooth_shutdown,

    bluetooth_adapter_get_unique_name,
    bluetooth_adapter_free,

    bluetooth_get_event,
};

C_ASSERT( ARRAYSIZE( __wine_unix_call_funcs ) == unix_funcs_count );
