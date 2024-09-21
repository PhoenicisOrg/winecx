/*
 * nsiproxy.sys
 *
 * Copyright 2021 Huw Davies
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

#include "config.h"
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <limits.h>
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "ifdef.h"
#define __WINE_INIT_NPI_MODULEID
#define USE_WS_PREFIX
#include "netiodef.h"
#include "wine/nsi.h"
#include "wine/debug.h"
#include "wine/unixlib.h"
#include "unix_private.h"
#include "nsiproxy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(nsi);

static const struct module *modules[] =
{
    &ndis_module,
    &ipv4_module,
    &ipv6_module,
    &tcp_module,
    &udp_module,
};

static const struct module_table *get_module_table( const NPI_MODULEID *id, UINT table )
{
    const struct module_table *entry;
    int i;

    for (i = 0; i < ARRAY_SIZE(modules); i++)
        if (NmrIsEqualNpiModuleId( modules[i]->module, id ))
            for (entry = modules[i]->tables; entry->table != ~0u; entry++)
                if (entry->table == table) return entry;

    return NULL;
}

NTSTATUS nsi_enumerate_all_ex( struct nsi_enumerate_all_ex *params )
{
    const struct module_table *entry = get_module_table( params->module, params->table );
    UINT sizes[4] = { params->key_size, params->rw_size, params->dynamic_size, params->static_size };
    void *data[4] = { params->key_data, params->rw_data, params->dynamic_data, params->static_data };
    int i;

    if (!entry || !entry->enumerate_all)
    {
        WARN( "table not found\n" );
        return STATUS_INVALID_PARAMETER;
    }

    for (i = 0; i < ARRAY_SIZE(sizes); i++)
    {
        if (!sizes[i]) data[i] = NULL;
        else if (sizes[i] != entry->sizes[i]) return STATUS_INVALID_PARAMETER;
    }

    return entry->enumerate_all( data[0], sizes[0], data[1], sizes[1], data[2], sizes[2], data[3], sizes[3], &params->count );
}

NTSTATUS nsi_get_all_parameters_ex( struct nsi_get_all_parameters_ex *params )
{
    const struct module_table *entry = get_module_table( params->module, params->table );
    void *rw = params->rw_data;
    void *dyn = params->dynamic_data;
    void *stat = params->static_data;

    if (!entry || !entry->get_all_parameters)
    {
        WARN( "table not found\n" );
        return STATUS_INVALID_PARAMETER;
    }

    if (params->key_size != entry->sizes[0]) return STATUS_INVALID_PARAMETER;
    if (!params->rw_size) rw = NULL;
    else if (params->rw_size != entry->sizes[1]) return STATUS_INVALID_PARAMETER;
    if (!params->dynamic_size) dyn = NULL;
    else if (params->dynamic_size != entry->sizes[2]) return STATUS_INVALID_PARAMETER;
    if (!params->static_size) stat = NULL;
    else if (params->static_size != entry->sizes[3]) return STATUS_INVALID_PARAMETER;

    return entry->get_all_parameters( params->key, params->key_size, rw, params->rw_size,
                                      dyn, params->dynamic_size, stat, params->static_size );
}

NTSTATUS nsi_get_parameter_ex( struct nsi_get_parameter_ex *params )
{
    const struct module_table *entry = get_module_table( params->module, params->table );

    if (!entry || !entry->get_parameter)
    {
        WARN( "table not found\n" );
        return STATUS_INVALID_PARAMETER;
    }

    if (params->param_type > 2) return STATUS_INVALID_PARAMETER;
    if (params->key_size != entry->sizes[0]) return STATUS_INVALID_PARAMETER;
    if (params->data_offset + params->data_size > entry->sizes[params->param_type + 1])
        return STATUS_INVALID_PARAMETER;
    return entry->get_parameter( params->key, params->key_size, params->param_type,
                                 params->data, params->data_size, params->data_offset );
}

static NTSTATUS unix_nsi_enumerate_all_ex( void *args )
{
    struct nsi_enumerate_all_ex *params = (struct nsi_enumerate_all_ex *)args;
    return nsi_enumerate_all_ex( params );
}

static NTSTATUS unix_nsi_get_all_parameters_ex( void *args )
{
    struct nsi_get_all_parameters_ex *params = (struct nsi_get_all_parameters_ex *)args;
    return nsi_get_all_parameters_ex( params );
}

static NTSTATUS unix_nsi_get_parameter_ex( void *args )
{
    struct nsi_get_parameter_ex *params = (struct nsi_get_parameter_ex *)args;
    return nsi_get_parameter_ex( params );
}

#ifdef HAVE_LINUX_RTNETLINK_H
static struct
{
    const NPI_MODULEID *module;
    UINT32 table;
}
queued_notifications[256];
static unsigned int queued_notification_count;

static NTSTATUS add_notification( const NPI_MODULEID *module, UINT32 table )
{
    unsigned int i;

    for (i = 0; i < queued_notification_count; ++i)
        if (queued_notifications[i].module == module && queued_notifications[i].table == table) return STATUS_SUCCESS;
    if (queued_notification_count == ARRAY_SIZE(queued_notifications))
    {
        ERR( "Notification queue full.\n" );
        return STATUS_NO_MEMORY;
    }
    queued_notifications[i].module = module;
    queued_notifications[i].table = table;
    ++queued_notification_count;
    return STATUS_SUCCESS;
}

static NTSTATUS poll_netlink(void)
{
    static int netlink_fd = -1;
    char buffer[PIPE_BUF];
    struct nlmsghdr *nlh;
    NTSTATUS status;
    int len;

    if (netlink_fd == -1)
    {
        struct sockaddr_nl addr;

        if ((netlink_fd = socket( PF_NETLINK, SOCK_RAW, NETLINK_ROUTE )) == -1)
        {
            ERR( "netlink socket creation failed, errno %d.\n", errno );
            return STATUS_NOT_IMPLEMENTED;
        }

        memset( &addr, 0, sizeof(addr) );
        addr.nl_family = AF_NETLINK;
        addr.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
        if (bind( netlink_fd, (struct sockaddr *)&addr, sizeof(addr) ) == -1)
        {
            close( netlink_fd );
            netlink_fd = -1;
            ERR( "bind failed, errno %d.\n", errno );
            return STATUS_NOT_IMPLEMENTED;
        }
    }

    while (1)
    {
        len = recv( netlink_fd, buffer, sizeof(buffer), 0 );
        if (len <= 0)
        {
            if (errno == EINTR) continue;
            ERR( "error receivng, len %d, errno %d.\n", len, errno );
            return STATUS_UNSUCCESSFUL;
        }
        for (nlh = (struct nlmsghdr *)buffer; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len))
        {
            if (nlh->nlmsg_type == NLMSG_DONE) break;
            if (nlh->nlmsg_type == RTM_NEWADDR || nlh->nlmsg_type == RTM_DELADDR)
            {
                struct ifaddrmsg *addrmsg = (struct ifaddrmsg *)(nlh + 1);
                const NPI_MODULEID *module;

                if (addrmsg->ifa_family == AF_INET)       module = &NPI_MS_IPV4_MODULEID;
                else if (addrmsg->ifa_family == AF_INET6) module = &NPI_MS_IPV6_MODULEID;
                else
                {
                    WARN( "Unknown addrmsg->ifa_family %d.\n", addrmsg->ifa_family );
                    continue;
                }
                if ((status = add_notification( module, NSI_IP_UNICAST_TABLE))) return status;
            }
        }
        if (queued_notification_count) break;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS unix_nsi_get_notification( void *args )
{
    struct nsi_get_notification_params *params = (struct nsi_get_notification_params *)args;
    NTSTATUS status;

    if (!queued_notification_count && (status = poll_netlink())) return status;
    assert( queued_notification_count );
    params->module = *queued_notifications[0].module;
    params->table = queued_notifications[0].table;
    --queued_notification_count;
    memmove( queued_notifications, queued_notifications + 1, sizeof(*queued_notifications) * queued_notification_count );
    return STATUS_SUCCESS;
}
#else
static NTSTATUS unix_nsi_get_notification( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}
#endif

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    icmp_cancel_listen,
    icmp_close,
    icmp_listen,
    icmp_send_echo,
    unix_nsi_enumerate_all_ex,
    unix_nsi_get_all_parameters_ex,
    unix_nsi_get_parameter_ex,
    unix_nsi_get_notification,
};

#ifdef _WIN64

typedef UINT PTR32;

static NTSTATUS wow64_nsi_enumerate_all_ex( void *args )
{
    struct
    {
        PTR32 unknown[2];
        PTR32 module;
        UINT table;
        UINT first_arg;
        UINT second_arg;
        PTR32 key_data;
        UINT key_size;
        PTR32 rw_data;
        UINT rw_size;
        PTR32 dynamic_data;
        UINT dynamic_size;
        PTR32 static_data;
        UINT static_size;
        UINT count;
    } *params32 = args;
    struct nsi_enumerate_all_ex params =
    {
        .unknown[0] = ULongToPtr( params32->unknown[0] ),
        .unknown[1] = ULongToPtr( params32->unknown[1] ),
        .module = ULongToPtr( params32->module ),
        .table = params32->table,
        .first_arg = params32->first_arg,
        .second_arg = params32->second_arg,
        .key_data = ULongToPtr( params32->key_data ),
        .key_size = params32->key_size,
        .rw_data = ULongToPtr( params32->rw_data ),
        .rw_size = params32->rw_size,
        .dynamic_data = ULongToPtr( params32->dynamic_data ),
        .dynamic_size = params32->dynamic_size,
        .static_data = ULongToPtr( params32->static_data ),
        .static_size = params32->static_size,
        .count = params32->count
    };
    NTSTATUS status = nsi_enumerate_all_ex( &params );
    params32->count = params.count;
    return status;
}

static NTSTATUS wow64_nsi_get_all_parameters_ex( void *args )
{
    struct
    {
        PTR32 unknown[2];
        PTR32 module;
        UINT table;
        UINT first_arg;
        UINT unknown2;
        PTR32 key;
        UINT key_size;
        PTR32 rw_data;
        UINT rw_size;
        PTR32 dynamic_data;
        UINT dynamic_size;
        PTR32 static_data;
        UINT static_size;
    } const *params32 = args;
    struct nsi_get_all_parameters_ex params =
    {
        .unknown[0] = ULongToPtr( params32->unknown[0] ),
        .unknown[1] = ULongToPtr( params32->unknown[1] ),
        .module = ULongToPtr( params32->module ),
        .table = params32->table,
        .first_arg = params32->first_arg,
        .unknown2 = params32->unknown2,
        .key = ULongToPtr( params32->key ),
        .key_size = params32->key_size,
        .rw_data = ULongToPtr( params32->rw_data ),
        .rw_size = params32->rw_size,
        .dynamic_data = ULongToPtr( params32->dynamic_data ),
        .dynamic_size = params32->dynamic_size,
        .static_data = ULongToPtr( params32->static_data ),
        .static_size = params32->static_size
    };
    return nsi_get_all_parameters_ex( &params );
}

static NTSTATUS wow64_nsi_get_parameter_ex( void *args )
{
    struct
    {
        PTR32 unknown[2];
        PTR32 module;
        UINT table;
        UINT first_arg;
        UINT unknown2;
        PTR32 key;
        UINT key_size;
        UINT param_type;
        PTR32 data;
        UINT data_size;
        UINT data_offset;
    } const *params32 = args;
    struct nsi_get_parameter_ex params =
    {
        .unknown[0] = ULongToPtr( params32->unknown[0] ),
        .unknown[1] = ULongToPtr( params32->unknown[1] ),
        .module = ULongToPtr( params32->module ),
        .table = params32->table,
        .first_arg = params32->first_arg,
        .unknown2 = params32->unknown2,
        .key = ULongToPtr( params32->key ),
        .key_size = params32->key_size,
        .param_type = params32->param_type,
        .data = ULongToPtr( params32->data ),
        .data_size = params32->data_size,
        .data_offset = params32->data_offset
    };
    return nsi_get_parameter_ex( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    icmp_cancel_listen,
    icmp_close,
    wow64_icmp_listen,
    wow64_icmp_send_echo,
    wow64_nsi_enumerate_all_ex,
    wow64_nsi_get_all_parameters_ex,
    wow64_nsi_get_parameter_ex,
    unix_nsi_get_notification,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == ARRAYSIZE(__wine_unix_call_funcs) );

#endif /* _WIN64 */
