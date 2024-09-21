/*
 * Copyright 2020 Brendan Shanks for CodeWeavers
 * Copyright 2023 Maxim Karasev <mxkrsv@etersoft.ru>
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

#define WIN32_LEAN_AND_MEAN
#define SECURITY_WIN32
#include <windows.h>
#include <security.h>
#include <sddl.h>
#include <stdarg.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(whoami);

static int output_write(const WCHAR* str, int len)
{
    DWORD count;

    if (len < 0)
        len = wcslen(str);

    if (!WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str, len, &count, NULL))
    {
        DWORD lenA;
        char *strA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile() with OEM code page.
         */
        lenA = WideCharToMultiByte(GetOEMCP(), 0, str, len, NULL, 0, NULL, NULL);
        strA = malloc(lenA);
        if (!strA)
            return 0;

        WideCharToMultiByte(GetOEMCP(), 0, str, len, strA, lenA, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), strA, lenA, &count, FALSE);
        free(strA);
    }
    return count;
}

static WCHAR *get_user_name(EXTENDED_NAME_FORMAT name_format)
{
    ULONG size = 0;
    WCHAR *ret;

    if (GetUserNameExW(name_format, NULL, &size) || GetLastError() != ERROR_MORE_DATA)
        return NULL;

    ret = malloc(size * sizeof(WCHAR));
    if (!ret)
        return NULL;

    if (!GetUserNameExW(name_format, ret, &size))
    {
        free(ret);
        return NULL;
    }

    return ret;
}

static void *get_token(TOKEN_INFORMATION_CLASS token_type)
{
    HANDLE token_handle;
    void *ret;
    /* GetTokenInformation wants a dword-aligned buffer */
    ULONG ret_size = sizeof(DWORD) * 256;
    ULONG token_size;

    ret = malloc(ret_size);
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token_handle))
        return NULL;

    if (!GetTokenInformation(token_handle, token_type, ret, ret_size, &token_size))
    {
        CloseHandle(token_handle);
        return NULL;
    }

    CloseHandle(token_handle);
    return ret;
}

static SID *get_process_sid(void)
{
    ULONG user_sid_len;
    TOKEN_USER *token_user;
    SID *ret;

    token_user = get_token(TokenUser);
    if (!token_user)
        return NULL;

    user_sid_len = GetLengthSid(token_user->User.Sid);
    ret = malloc(user_sid_len);
    if (!ret)
    {
        free(token_user);
        return NULL;
    }

    if (!CopySid(user_sid_len, ret, token_user->User.Sid))
    {
        free(token_user);
        free(ret);
        return NULL;
    }

    free(token_user);
    return ret;
}

static SID *get_logon_sid(void)
{
    TOKEN_GROUPS *token_groups;
    DWORD group_token_len;
    DWORD i;
    SID *ret;

    token_groups = get_token(TokenGroups);
    if (!token_groups)
        return NULL;

    for (i = 0; i < token_groups->GroupCount; i++)
    {
        if ((token_groups->Groups[i].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID)
        {
            group_token_len = GetLengthSid(token_groups->Groups[i].Sid);
            ret = malloc(group_token_len);
            if (!ret)
            {
                free(token_groups);
                return NULL;
            }

            if (!CopySid(group_token_len, ret, token_groups->Groups[i].Sid))
            {
                free(token_groups);
                free(ret);
                return NULL;
            }

            free(token_groups);
            return ret;
        }
    }

    free(token_groups);
    return NULL;
}

static int simple(EXTENDED_NAME_FORMAT name_format)
{
    WCHAR *name;

    name = get_user_name(name_format);
    if (!name)
    {
        ERR("get_user_name failed\n");
        return 1;
    }

    output_write(name, -1);
    output_write(L"\n", 1);

    free(name);

    return 0;
}

static int logon_id(void)
{
    PSID sid;
    WCHAR *sid_string;

    sid = get_logon_sid();
    if (!sid)
    {
        ERR("get_logon_sid failed\n");
        return 1;
    }

    if (!ConvertSidToStringSidW(sid, &sid_string))
    {
        ERR("ConvertSidToStringSidW failed, error %ld\n", GetLastError());
        return 1;
    }

    output_write(sid_string, -1);
    output_write(L"\n", 1);

    free(sid);
    LocalFree(sid_string);

    return 0;
}

static int user(void)
{
    PSID sid;
    WCHAR *name;
    WCHAR *sid_string;
    ULONG i;

    name = get_user_name(NameSamCompatible);
    if (!name)
    {
        ERR("get_user_name failed\n");
        return 1;
    }

    sid = get_process_sid();
    if (!sid)
    {
        ERR("get_process_sid failed\n");
        return 1;
    }

    if (!ConvertSidToStringSidW(sid, &sid_string))
    {
        ERR("ConvertSidToStringSidW failed, error %ld\n", GetLastError());
        return 1;
    }

    output_write(L"\nUSER INFORMATION\n----------------\n\n", -1);
    output_write(L"User Name", -1);
    for (i = 0; i <= max(wcslen(name), wcslen(L"User Name")) - wcslen(L"User Name"); i++)
        output_write(L" ", 1);
    output_write(L"SID\n", -1);

    for (i = 0; i < wcslen(name); i++)
        output_write(L"=", 1);
    output_write(L" ", 1);
    for (i = 0; i < wcslen(sid_string); i++)
        output_write(L"=", 1);
    output_write(L"\n", 1);

    output_write(name, -1);
    output_write(L" ", 1);
    output_write(sid_string, -1);
    output_write(L"\n", 1);

    free(name);
    free(sid);
    LocalFree(sid_string);

    return 0;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    if (argv[1] == NULL)
    {

        return simple(NameSamCompatible);
    }
    else
    {
        wcslwr(argv[1]);

        if (!wcscmp(argv[1], L"/upn"))
        {
            /* Not implemented as of now, therefore fails */
            return simple(NameUserPrincipal);
        }
        else if (!wcscmp(argv[1], L"/fqdn"))
        {
            /* Not implemented as of now, therefore fails */
            return simple(NameFullyQualifiedDN);
        }
        else if (!wcscmp(argv[1], L"/logonid"))
        {
            return logon_id();
        }
        else if (!wcscmp(argv[1], L"/user"))
        {
            return user();
        }
        else
        {
            FIXME("stub\n");
            return 0;
        }
    }
}
