/*
 * Copyright 2008 Hans Leidekker
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winhttp.h"

#include "wine/test.h"

static WCHAR empty[]    = {0};
static WCHAR ftp[]      = {'f','t','p',0};
static WCHAR http[]     = {'h','t','t','p',0};
static WCHAR winehq[]   = {'w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static WCHAR username[] = {'u','s','e','r','n','a','m','e',0};
static WCHAR password[] = {'p','a','s','s','w','o','r','d',0};
static WCHAR about[]    = {'/','s','i','t','e','/','a','b','o','u','t',0};
static WCHAR query[]    = {'?','q','u','e','r','y',0};
static WCHAR escape[]   = {' ','!','"','#','$','%','&','\'','(',')','*','+',',','-','.','/',':',';','<','=','>','?','@','[','\\',']','^','_','`','{','|','}','~',0};
static WCHAR escape2[]  = {'\r',0x1f,' ','\n',0x7f,'\r','\n',0};
static WCHAR escape3[]  = {'?','t','e','x','t','=',0xfb00,0};
static WCHAR escape4[]  = {'/','t','e','x','t','=',0xfb00,0};

static const WCHAR url1[]  =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url2[] = {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':',0};
static const WCHAR url3[] =
    {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url4[] = {'h','t','t','p',':','/','/',0};
static const WCHAR url5[] =
    {'f','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','8','0','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url6[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','4','2','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url7[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t',
     '%','2','0','!','%','2','2','%','2','3','$','%','2','5','&','\'','(',')','*','+',',','-','.','/',':',';','%','3','C','=','%','3','E','?','@','%',
     '5','B','%','5','C','%','5','D','%','5','E','_','%','6','0','%','7','B','%','7','C','%','7','D','%','7','E',0};
static const WCHAR url8[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','0','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url9[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','8','0','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url10[] =
    {'h','t','t','p','s',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','4','4','3','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url11[] =
    {'h','t','t','p',':','/','/','e','x','a','m','p','l','e','.','n','e','t','/','p','a','t','h','?','v','a','r','1','=','e','x','a','m','p','l','e','@','e','x','a','m','p','l','e','.','c','o','m','&','v','a','r','2','=','x','&','v','a','r','3','=','y', 0};
static const WCHAR url12[] =
    {'h','t','t','p','s',':','/','/','t','o','o','l','s','.','g','o','o','g','l','e','.','c','o','m','/','s','e','r','v','i','c','e','/','u','p','d','a','t','e','2','?','w','=','3',':','B','x','D','H','o','W','y','8','e','z','M',0};
static const WCHAR url13[] =
    {'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o',' ','g','/','p','a','t','h',' ','w','i','t','h',' ','s','p','a','c','e','s',0};
static const WCHAR url14[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','t','e','s','t',0};
static const WCHAR url15[] = {'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g',':','6','5','5','3','6',0};
static const WCHAR url16[] = {'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g',':','0',0};
static const WCHAR url17[] = {'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g',':',0};
static const WCHAR url18[] =
    {'h','t','t','p',':','/','/','%','0','D','%','1','F','%','2','0','%','0','A','%','7','F','%','0','D','%','0','A',0};
static const WCHAR url19[] =
    {'h','t','t','p',':','/','/','?','t','e','x','t','=',0xfb00,0};
static const WCHAR url20[] =
    {'h','t','t','p',':','/','/','/','t','e','x','t','=',0xfb00,0};
static const WCHAR url21[] =
    {'h','t','t','p','s',':','/','/','n','b','a','2','k','1','9','-','w','s','.','2','k','s','p','o','r','t','s','.','c','o','m',':','1','9','1','3','3',
     '/','n','b','a','/','v','4','/','A','c','c','o','u','n','t','s','/','g','e','t','_','a','c','c','o','u','n','t','?','x','=','3','7','8','9','5','2',
     '6','7','7','5','2','6','5','6','6','3','8','7','6',0};

static const WCHAR url_k1[]  =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t',0};
static const WCHAR url_k2[]  =
    {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR url_k3[]  =
    {'h','t','t','p','s',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','p','o','s','t','?',0};
static const WCHAR url_k4[]  =
    {'H','T','T','P',':','w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR url_k5[]  =
    {'h','t','t','p',':','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR url_k6[]  =
    {'w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR url_k7[]  =
    {'w','w','w',0};
static const WCHAR url_k8[]  =
    {'h','t','t','p',0};
static const WCHAR url_k9[]  =
    {'h','t','t','p',':','/','/','w','i','n','e','h','q','?',0};
static const WCHAR url_k10[]  =
    {'h','t','t','p',':','/','/','w','i','n','e','h','q','/','p','o','s','t',';','a',0};

static void fill_url_components( URL_COMPONENTS *uc )
{
    uc->dwStructSize = sizeof(URL_COMPONENTS);
    uc->lpszScheme = http;
    uc->dwSchemeLength = lstrlenW( uc->lpszScheme );
    uc->nScheme = INTERNET_SCHEME_HTTP;
    uc->lpszHostName = winehq;
    uc->dwHostNameLength = lstrlenW( uc->lpszHostName );
    uc->nPort = 80;
    uc->lpszUserName = username;
    uc->dwUserNameLength = lstrlenW( uc->lpszUserName );
    uc->lpszPassword = password;
    uc->dwPasswordLength = lstrlenW( uc->lpszPassword );
    uc->lpszUrlPath = about;
    uc->dwUrlPathLength = lstrlenW( uc->lpszUrlPath );
    uc->lpszExtraInfo = query;
    uc->dwExtraInfoLength = lstrlenW( uc->lpszExtraInfo );
}

static void WinHttpCreateUrl_test( void )
{
    URL_COMPONENTS uc;
    WCHAR *url;
    DWORD len, err;
    BOOL ret;

    /* NULL components */
    len = ~0u;
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( NULL, 0, NULL, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );
    ok( len == ~0u, "expected len ~0u got %u\n", len );

    /* zero'ed components */
    memset( &uc, 0, sizeof(URL_COMPONENTS) );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );
    ok( len == ~0u, "expected len ~0u got %u\n", len );

    /* valid components, NULL url, NULL length */
    fill_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, NULL );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );

    /* valid components, NULL url, insufficient length */
    len = 0;
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER got %u\n", GetLastError() );
    ok( len == 57, "expected len 57 got %u\n", len );

    /* valid components, NULL url, sufficient length */
    SetLastError( 0xdeadbeef );
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, NULL, &len );
    err = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( err == ERROR_INVALID_PARAMETER || broken(err == ERROR_INSUFFICIENT_BUFFER) /* < win7 */,
        "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );
    ok( len == 256 || broken(len == 57) /* < win7 */, "expected len 256 got %u\n", len );

    /* correct size, NULL url */
    fill_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, &len );
    err = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( err == ERROR_INVALID_PARAMETER || broken(err == ERROR_INSUFFICIENT_BUFFER) /* < win7 */,
        "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );
    ok( len == 256 || broken(len == 57) /* < win7 */, "expected len 256 got %u\n", len );

    /* valid components, allocated url, short length */
    SetLastError( 0xdeadbeef );
    url = HeapAlloc( GetProcessHeap(), 0, 256 * sizeof(WCHAR) );
    url[0] = 0;
    len = 2;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER got %u\n", GetLastError() );
    ok( len == 57, "expected len 57 got %u\n", len );

    /* allocated url, NULL scheme */
    SetLastError( 0xdeadbeef );
    uc.lpszScheme = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( GetLastError() == ERROR_SUCCESS || broken(GetLastError() == 0xdeadbeef) /* < win7 */,
        "expected ERROR_SUCCESS got %u\n", GetLastError() );
    ok( len == 56, "expected len 56 got %u\n", len );
    ok( !lstrcmpW( url, url1 ), "url doesn't match\n" );

    /* allocated url, 0 scheme */
    fill_url_components( &uc );
    uc.nScheme = 0;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %u\n", len );

    /* valid components, allocated url */
    fill_url_components( &uc );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %d\n", len );
    ok( !lstrcmpW( url, url1 ), "url doesn't match\n" );

    /* valid username, NULL password */
    fill_url_components( &uc );
    uc.lpszPassword = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );

    /* valid username, empty password */
    fill_url_components( &uc );
    uc.lpszPassword = empty;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %u\n", len );
    ok( !lstrcmpW( url, url2 ), "url doesn't match\n" );

    /* valid password, NULL username */
    fill_url_components( &uc );
    SetLastError( 0xdeadbeef );
    uc.lpszUserName = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );

    /* valid password, empty username */
    fill_url_components( &uc );
    uc.lpszUserName = empty;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n");

    /* NULL username, NULL password */
    fill_url_components( &uc );
    uc.lpszUserName = NULL;
    uc.lpszPassword = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 38, "expected len 38 got %u\n", len );
    ok( !lstrcmpW( url, url3 ), "url doesn't match\n" );

    /* empty username, empty password */
    fill_url_components( &uc );
    uc.lpszUserName = empty;
    uc.lpszPassword = empty;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %u\n", len );
    ok( !lstrcmpW( url, url4 ), "url doesn't match\n" );

    /* nScheme has lower precedence than lpszScheme */
    fill_url_components( &uc );
    uc.lpszScheme = ftp;
    uc.dwSchemeLength = lstrlenW( uc.lpszScheme );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == lstrlenW( url5 ), "expected len %d got %u\n", lstrlenW( url5 ) + 1, len );
    ok( !lstrcmpW( url, url5 ), "url doesn't match\n" );

    /* non-standard port */
    uc.lpszScheme = http;
    uc.dwSchemeLength = lstrlenW( uc.lpszScheme );
    uc.nPort = 42;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 59, "expected len 59 got %u\n", len );
    ok( !lstrcmpW( url, url6 ), "url doesn't match\n" );

    /* escape extra info */
    fill_url_components( &uc );
    uc.lpszExtraInfo = escape;
    uc.dwExtraInfoLength = lstrlenW( uc.lpszExtraInfo );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, ICU_ESCAPE, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 113, "expected len 113 got %u\n", len );
    ok( !lstrcmpW( url, url7 ), "url doesn't match %s\n", wine_dbgstr_w(url) );

    /* escape extra info */
    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.lpszExtraInfo = escape2;
    uc.dwExtraInfoLength = lstrlenW( uc.lpszExtraInfo );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, ICU_ESCAPE, url, &len );
    ok( ret, "expected success\n" );
    ok( len == lstrlenW(url18), "expected len %u got %u\n", lstrlenW(url18), len );
    ok( !lstrcmpW( url, url18 ), "url doesn't match\n" );

    /* extra info with Unicode characters */
    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.lpszExtraInfo = escape3;
    uc.dwExtraInfoLength = lstrlenW( uc.lpszExtraInfo );
    url[0] = 0;
    len = 256;
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, ICU_ESCAPE, url, &len );
    err = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( err == ERROR_INVALID_PARAMETER, "got %u\n", err );

    /* extra info with Unicode characters, no ICU_ESCAPE */
    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.lpszExtraInfo = escape3;
    uc.dwExtraInfoLength = lstrlenW( uc.lpszExtraInfo );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret || broken(!ret) /* < win7 */, "expected success\n" );
    if (ret)
    {
        ok( len == lstrlenW(url19), "expected len %u got %u\n", lstrlenW(url19), len );
        ok( !lstrcmpW( url, url19 ), "url doesn't match %s\n", wine_dbgstr_w(url) );
    }

    /* escape path */
    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.lpszUrlPath = escape2;
    uc.dwUrlPathLength = lstrlenW( uc.lpszUrlPath );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, ICU_ESCAPE, url, &len );
    ok( ret, "expected success\n" );
    ok( len == lstrlenW(url18), "expected len %u got %u\n", lstrlenW(url18), len );
    ok( !lstrcmpW( url, url18 ), "url doesn't match\n" );

    /* path with Unicode characters */
    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.lpszUrlPath = escape4;
    uc.dwUrlPathLength = lstrlenW( uc.lpszUrlPath );
    url[0] = 0;
    len = 256;
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, ICU_ESCAPE, url, &len );
    err = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( err == ERROR_INVALID_PARAMETER, "got %u\n", err );

    /* path with Unicode characters, no ICU_ESCAPE */
    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.lpszUrlPath = escape4;
    uc.dwUrlPathLength = lstrlenW( uc.lpszUrlPath );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret || broken(!ret) /* < win7 */, "expected success\n" );
    if (ret)
    {
        ok( len == lstrlenW(url20), "expected len %u got %u\n", lstrlenW(url20), len );
        ok( !lstrcmpW( url, url20 ), "url doesn't match %s\n", wine_dbgstr_w(url) );
    }

    /* NULL lpszScheme, 0 nScheme and nPort */
    fill_url_components( &uc );
    uc.lpszScheme = NULL;
    uc.dwSchemeLength = 0;
    uc.nScheme = 0;
    uc.nPort = 0;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 58, "expected len 58 got %u\n", len );
    ok( !lstrcmpW( url, url8 ), "url doesn't match\n" );

    HeapFree( GetProcessHeap(), 0, url );
}

static void reset_url_components( URL_COMPONENTS *uc )
{
    memset( uc, 0, sizeof(URL_COMPONENTS) );
    uc->dwStructSize = sizeof(URL_COMPONENTS);
    uc->dwSchemeLength    = ~0u;
    uc->dwHostNameLength  = 1;
    uc->nPort             =  0;
    uc->dwUserNameLength  = ~0u;
    uc->dwPasswordLength  = ~0u;
    uc->dwUrlPathLength   = ~0u;
    uc->dwExtraInfoLength = ~0u;
}

static void WinHttpCrackUrl_test( void )
{
    static const WCHAR hostnameW[] =
        {'w','i','n','e','h','q','.','o',' ','g',0};
    static const WCHAR pathW[] =
        {'/','p','a','t','h','%','2','0','w','i','t','h','%','2','0','s','p','a','c','e','s',0};
    URL_COMPONENTSW uc;
    WCHAR scheme[20], user[20], pass[20], host[40], path[80], extra[40];
    DWORD error;
    BOOL ret;

    /* buffers of sufficient length */
    scheme[0] = user[0] = pass[0] = host[0] = path[0] = extra[0] = 0;

    uc.dwStructSize = sizeof(URL_COMPONENTS);
    uc.nScheme = 0;
    uc.lpszScheme = scheme;
    uc.dwSchemeLength = 20;
    uc.lpszUserName = user;
    uc.dwUserNameLength = 20;
    uc.lpszPassword = pass;
    uc.dwPasswordLength = 20;
    uc.lpszHostName = host;
    uc.dwHostNameLength = 20;
    uc.nPort = 0;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 40;
    uc.lpszExtraInfo = extra;
    uc.dwExtraInfoLength = 20;

    ret = WinHttpCrackUrl( url1, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( uc.nScheme == INTERNET_SCHEME_HTTP, "unexpected scheme: %u\n", uc.nScheme );
    ok( !memcmp( uc.lpszScheme, http, sizeof(http) ), "unexpected scheme: %s\n", wine_dbgstr_w(uc.lpszScheme) );
    ok( uc.dwSchemeLength == 4, "unexpected scheme length: %u\n", uc.dwSchemeLength );
    ok( !memcmp( uc.lpszUserName, username, sizeof(username) ), "unexpected username: %s\n", wine_dbgstr_w(uc.lpszUserName) );
    ok( uc.dwUserNameLength == 8, "unexpected username length: %u\n", uc.dwUserNameLength );
    ok( !memcmp( uc.lpszPassword, password, sizeof(password) ), "unexpected password: %s\n", wine_dbgstr_w(uc.lpszPassword) );
    ok( uc.dwPasswordLength == 8, "unexpected password length: %u\n", uc.dwPasswordLength );
    ok( !memcmp( uc.lpszHostName, winehq, sizeof(winehq) ), "unexpected hostname: %s\n", wine_dbgstr_w(uc.lpszHostName) );
    ok( uc.dwHostNameLength == 14, "unexpected hostname length: %u\n", uc.dwHostNameLength );
    ok( uc.nPort == 80, "unexpected port: %u\n", uc.nPort );
    ok( !memcmp( uc.lpszUrlPath, about, sizeof(about) ), "unexpected path: %s\n", wine_dbgstr_w(uc.lpszUrlPath) );
    ok( uc.dwUrlPathLength == 11, "unexpected path length: %u\n", uc.dwUrlPathLength );
    ok( !memcmp( uc.lpszExtraInfo, query, sizeof(query) ), "unexpected extra info: %s\n", wine_dbgstr_w(uc.lpszExtraInfo) );
    ok( uc.dwExtraInfoLength == 6, "unexpected extra info length: %u\n", uc.dwExtraInfoLength );

    /* buffers of insufficient length */
    uc.dwSchemeLength   = 1;
    uc.dwHostNameLength = 1;
    uc.dwUrlPathLength  = 40; /* sufficient */
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url1, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_INSUFFICIENT_BUFFER, "got %u, expected ERROR_INSUFFICIENT_BUFFER\n", error );
    ok( uc.dwSchemeLength == 5, "unexpected scheme length: %u\n", uc.dwSchemeLength );
    ok( uc.dwHostNameLength == 15, "unexpected hostname length: %u\n", uc.dwHostNameLength );
    ok( uc.dwUrlPathLength == 11, "unexpected path length: %u\n", uc.dwUrlPathLength );

    /* no buffers */
    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url_k1, 0, 0, &uc);
    error = GetLastError();
    ok( ret, "WinHttpCrackUrl failed le=%u\n", error );
    ok( error == ERROR_SUCCESS || broken(error == ERROR_INVALID_PARAMETER) /* < win7 */,
        "got %u, expected ERROR_SUCCESS\n", error );
    ok( uc.nScheme == INTERNET_SCHEME_HTTP, "unexpected scheme\n" );
    ok( uc.lpszScheme == url_k1,"unexpected scheme\n" );
    ok( uc.dwSchemeLength == 4, "unexpected scheme length\n" );
    ok( uc.lpszUserName == url_k1 + 7, "unexpected username\n" );
    ok( uc.dwUserNameLength == 8, "unexpected username length\n" );
    ok( uc.lpszPassword == url_k1 + 16, "unexpected password\n" );
    ok( uc.dwPasswordLength == 8, "unexpected password length\n" );
    ok( uc.lpszHostName == url_k1 + 25, "unexpected hostname\n" );
    ok( uc.dwHostNameLength == 14, "unexpected hostname length\n" );
    ok( uc.nPort == 80, "unexpected port: %u\n", uc.nPort );
    ok( uc.lpszUrlPath == url_k1 + 39, "unexpected path\n" );
    ok( uc.dwUrlPathLength == 11, "unexpected path length\n" );
    ok( uc.lpszExtraInfo == url_k1 + 50, "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 0, "unexpected extra info length\n" );

    reset_url_components( &uc );
    uc.dwSchemeLength = uc.dwHostNameLength = uc.dwUserNameLength = 1;
    uc.dwPasswordLength = uc.dwUrlPathLength = uc.dwExtraInfoLength = 1;
    ret = WinHttpCrackUrl( url_k2, 0, 0,&uc);
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( uc.nScheme == INTERNET_SCHEME_HTTP, "unexpected scheme\n" );
    ok( uc.lpszScheme == url_k2, "unexpected scheme\n" );
    ok( uc.dwSchemeLength == 4, "unexpected scheme length\n" );
    ok( uc.lpszUserName == NULL ,"unexpected username\n" );
    ok( uc.dwUserNameLength == 0, "unexpected username length\n" );
    ok( uc.lpszPassword == NULL, "unexpected password\n" );
    ok( uc.dwPasswordLength == 0, "unexpected password length\n" );
    ok( uc.lpszHostName == url_k2 + 7, "unexpected hostname\n" );
    ok( uc.dwHostNameLength == 14, "unexpected hostname length\n" );
    ok( uc.nPort == 80, "unexpected port: %u\n", uc.nPort );
    ok( uc.lpszUrlPath == url_k2 + 21, "unexpected path\n" );
    ok( uc.dwUrlPathLength == 0, "unexpected path length\n" );
    ok( uc.lpszExtraInfo == url_k2 + 21, "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 0, "unexpected extra info length\n" );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k3, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( uc.nScheme == INTERNET_SCHEME_HTTPS, "unexpected scheme\n" );
    ok( uc.lpszScheme == url_k3, "unexpected scheme\n" );
    ok( uc.dwSchemeLength == 5, "unexpected scheme length\n" );
    ok( uc.lpszUserName == NULL, "unexpected username\n" );
    ok( uc.dwUserNameLength == 0, "unexpected username length\n" );
    ok( uc.lpszPassword == NULL, "unexpected password\n" );
    ok( uc.dwPasswordLength == 0, "unexpected password length\n" );
    ok( uc.lpszHostName == url_k3 + 8, "unexpected hostname\n" );
    ok( uc.dwHostNameLength == 14, "unexpected hostname length\n" );
    ok( uc.nPort == 443, "unexpected port: %u\n", uc.nPort );
    ok( uc.lpszUrlPath == url_k3 + 22, "unexpected path\n" );
    ok( uc.dwUrlPathLength == 5, "unexpected path length\n" );
    ok( uc.lpszExtraInfo == url_k3 + 27, "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 1, "unexpected extra info length\n" );

    /* bad parameters */
    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url_k4, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    error = GetLastError();
    ok( error == ERROR_WINHTTP_INVALID_URL, "got %u\n", error );

    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url_k5, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    error = GetLastError();
    ok( error == ERROR_WINHTTP_INVALID_URL, "got %u\n", error );

    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url_k6, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    error = GetLastError();
    ok( error == ERROR_WINHTTP_UNRECOGNIZED_SCHEME, "got %u\n", error );

    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url_k7, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    error = GetLastError();
    ok( error == ERROR_WINHTTP_UNRECOGNIZED_SCHEME, "got %u\n", error );

    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url_k8, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_WINHTTP_UNRECOGNIZED_SCHEME, "got %u\n", error );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k9, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( uc.lpszUrlPath == url_k9 + 14 || broken(uc.lpszUrlPath == url_k9 + 13) /* win8 */,
        "unexpected path: %s\n", wine_dbgstr_w(uc.lpszUrlPath) );
    ok( uc.dwUrlPathLength == 0, "unexpected path length: %u\n", uc.dwUrlPathLength );
    ok( uc.lpszExtraInfo == url_k9 + 14 || broken(uc.lpszExtraInfo == url_k9 + 13) /* win8 */,
        "unexpected extra info: %s\n", wine_dbgstr_w(uc.lpszExtraInfo) );
    ok( uc.dwExtraInfoLength == 0 || broken(uc.dwExtraInfoLength == 1) /* win8 */,
        "unexpected extra info length: %u\n", uc.dwExtraInfoLength );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k10, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( uc.lpszUrlPath == url_k10 + 13, "unexpected path: %s\n", wine_dbgstr_w(uc.lpszUrlPath) );
    ok( uc.dwUrlPathLength == 7, "unexpected path length: %u\n", uc.dwUrlPathLength );
    ok( uc.lpszExtraInfo == url_k10 + 20, "unexpected extra info: %s\n", wine_dbgstr_w(uc.lpszExtraInfo) );
    ok( uc.dwExtraInfoLength == 0, "unexpected extra info length: %u\n", uc.dwExtraInfoLength );

    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url4, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_WINHTTP_INVALID_URL, "got %u\n", error );

    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( empty, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_WINHTTP_UNRECOGNIZED_SCHEME, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url1, 0, 0, NULL );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( NULL, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    /* decoding without buffers */
    reset_url_components( &uc );
    SetLastError(0xdeadbeef);
    ret = WinHttpCrackUrl( url7, 0, ICU_DECODE, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u, expected ERROR_INVALID_PARAMETER\n", error );

    /* decoding with buffers */
    uc.lpszScheme = scheme;
    uc.dwSchemeLength = 20;
    uc.lpszUserName = user;
    uc.dwUserNameLength = 20;
    uc.lpszPassword = pass;
    uc.dwPasswordLength = 20;
    uc.lpszHostName = host;
    uc.dwHostNameLength = 20;
    uc.nPort = 0;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 80;
    uc.lpszExtraInfo = extra;
    uc.dwExtraInfoLength = 40;
    path[0] = 0;

    ret = WinHttpCrackUrl( url7, 0, ICU_DECODE, &uc );
    ok( ret, "WinHttpCrackUrl failed %u\n", GetLastError() );
    ok( !memcmp( uc.lpszUrlPath + 11, escape, 21 * sizeof(WCHAR) ), "unexpected path\n" );
    ok( uc.dwUrlPathLength == 32, "unexpected path length %u\n", uc.dwUrlPathLength );
    ok( !memcmp( uc.lpszExtraInfo, escape + 21, 12 * sizeof(WCHAR) ), "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 12, "unexpected extra info length %u\n", uc.dwExtraInfoLength );

    /* Urls with specified port numbers */
    /* decoding with buffers */
    uc.lpszScheme = scheme;
    uc.dwSchemeLength = 20;
    uc.lpszUserName = user;
    uc.dwUserNameLength = 20;
    uc.lpszPassword = pass;
    uc.dwPasswordLength = 20;
    uc.lpszHostName = host;
    uc.dwHostNameLength = 20;
    uc.nPort = 0;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 40;
    uc.lpszExtraInfo = extra;
    uc.dwExtraInfoLength = 20;
    path[0] = 0;

    ret = WinHttpCrackUrl( url6, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( !memcmp( uc.lpszHostName, winehq, sizeof(winehq) ), "unexpected host name: %s\n", wine_dbgstr_w(uc.lpszHostName) );
    ok( uc.dwHostNameLength == 14, "unexpected host name length: %d\n", uc.dwHostNameLength );
    ok( uc.nPort == 42, "unexpected port: %u\n", uc.nPort );

    /* decoding without buffers */
    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url8, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( uc.nPort == 0, "unexpected port: %u\n", uc.nPort );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url9, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( uc.nPort == 80, "unexpected port: %u\n", uc.nPort );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url10, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( uc.nPort == 443, "unexpected port: %u\n", uc.nPort );

    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( empty, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_WINHTTP_UNRECOGNIZED_SCHEME, "got %u, expected ERROR_WINHTTP_UNRECOGNIZED_SCHEME\n", error );

    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( http, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_WINHTTP_UNRECOGNIZED_SCHEME, "got %u, expected ERROR_WINHTTP_UNRECOGNIZED_SCHEME\n", error );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url11, 0, 0, &uc);
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( uc.nScheme == INTERNET_SCHEME_HTTP, "unexpected scheme\n" );
    ok( uc.lpszScheme == url11,"unexpected scheme\n" );
    ok( uc.dwSchemeLength == 4, "unexpected scheme length\n" );
    ok( uc.lpszUserName == NULL, "unexpected username\n" );
    ok( uc.lpszPassword == NULL, "unexpected password\n" );
    ok( uc.lpszHostName == url11 + 7, "unexpected hostname\n" );
    ok( uc.dwHostNameLength == 11, "unexpected hostname length\n" );
    ok( uc.nPort == 80, "unexpected port: %u\n", uc.nPort );
    ok( uc.lpszUrlPath == url11 + 18, "unexpected path\n" );
    ok( uc.dwUrlPathLength == 5, "unexpected path length\n" );
    ok( uc.lpszExtraInfo == url11 + 23, "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 39, "unexpected extra info length\n" );

    uc.lpszScheme = scheme;
    uc.dwSchemeLength = 20;
    uc.lpszHostName = host;
    uc.dwHostNameLength = 20;
    uc.lpszUserName = NULL;
    uc.dwUserNameLength = 0;
    uc.lpszPassword = NULL;
    uc.dwPasswordLength = 0;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 40;
    uc.lpszExtraInfo = NULL;
    uc.dwExtraInfoLength = 0;
    uc.nPort = 0;
    ret = WinHttpCrackUrl( url12, 0, ICU_DECODE, &uc );
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );

    uc.lpszScheme = scheme;
    uc.dwSchemeLength = 20;
    uc.lpszHostName = host;
    uc.dwHostNameLength = 20;
    uc.lpszUserName = NULL;
    uc.dwUserNameLength = 0;
    uc.lpszPassword = NULL;
    uc.dwPasswordLength = 0;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 40;
    uc.lpszExtraInfo = NULL;
    uc.dwExtraInfoLength = 0;
    uc.nPort = 0;
    ret = WinHttpCrackUrl( url13, 0, ICU_ESCAPE|ICU_DECODE, &uc );
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( !lstrcmpW( uc.lpszHostName, hostnameW ), "unexpected host name\n" );
    ok( !lstrcmpW( uc.lpszUrlPath, pathW ), "unexpected path\n" );
    ok( uc.dwUrlPathLength == lstrlenW(pathW), "got %u\n", uc.dwUrlPathLength );

    uc.dwStructSize = sizeof(uc);
    uc.lpszScheme = NULL;
    uc.dwSchemeLength = 0;
    uc.nScheme = 0;
    uc.lpszHostName = NULL;
    uc.dwHostNameLength = ~0u;
    uc.nPort = 0;
    uc.lpszUserName = NULL;
    uc.dwUserNameLength = ~0u;
    uc.lpszPassword = NULL;
    uc.dwPasswordLength = ~0u;
    uc.lpszUrlPath = NULL;
    uc.dwUrlPathLength = ~0u;
    uc.lpszExtraInfo = NULL;
    uc.dwExtraInfoLength = ~0u;
    ret = WinHttpCrackUrl( url14, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed le=%u\n", GetLastError() );
    ok( !uc.lpszScheme, "unexpected scheme %s\n", wine_dbgstr_w(uc.lpszScheme) );
    ok( !uc.dwSchemeLength, "unexpected length %u\n", uc.dwSchemeLength );
    ok( uc.nScheme == INTERNET_SCHEME_HTTP, "unexpected scheme %u\n", uc.nScheme );
    ok( !lstrcmpW( uc.lpszHostName, url14 + 7 ), "unexpected hostname %s\n", wine_dbgstr_w(uc.lpszHostName) );
    ok( uc.dwHostNameLength == 14, "unexpected length %u\n", uc.dwHostNameLength );
    ok( uc.nPort == 80, "unexpected port %u\n", uc.nPort );
    ok( !uc.lpszUserName, "unexpected username\n" );
    ok( !uc.dwUserNameLength, "unexpected length %u\n", uc.dwUserNameLength );
    ok( !uc.lpszPassword, "unexpected password\n" );
    ok( !uc.dwPasswordLength, "unexpected length %u\n", uc.dwPasswordLength );
    ok( !lstrcmpW( uc.lpszUrlPath, url14 + 21 ), "unexpected path %s\n", wine_dbgstr_w(uc.lpszUrlPath) );
    ok( uc.dwUrlPathLength == 5, "unexpected length %u\n", uc.dwUrlPathLength );
    ok( !uc.lpszExtraInfo[0], "unexpected extra info %s\n", wine_dbgstr_w(uc.lpszExtraInfo) );
    ok( uc.dwExtraInfoLength == 0, "unexpected length %u\n", uc.dwExtraInfoLength );

    uc.dwStructSize = sizeof(uc);
    uc.lpszScheme = scheme;
    uc.dwSchemeLength = 0;
    uc.nScheme = 0;
    uc.lpszHostName = NULL;
    uc.dwHostNameLength = 0;
    uc.nPort = 0;
    uc.lpszUserName = NULL;
    uc.dwUserNameLength = ~0u;
    uc.lpszPassword = NULL;
    uc.dwPasswordLength = ~0u;
    uc.lpszUrlPath = NULL;
    uc.dwUrlPathLength = 0;
    uc.lpszExtraInfo = NULL;
    uc.dwExtraInfoLength = 0;
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url14, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );
    ok( !lstrcmpW( uc.lpszScheme, http ), "unexpected scheme %s\n", wine_dbgstr_w(uc.lpszScheme) );
    ok( !uc.dwSchemeLength, "unexpected length %u\n", uc.dwSchemeLength );
    ok( uc.nScheme == 0, "unexpected scheme %u\n", uc.nScheme );
    ok( !uc.lpszHostName, "unexpected hostname %s\n", wine_dbgstr_w(uc.lpszHostName) );
    ok( uc.dwHostNameLength == 0, "unexpected length %u\n", uc.dwHostNameLength );
    ok( uc.nPort == 0, "unexpected port %u\n", uc.nPort );
    ok( !uc.lpszUserName, "unexpected username\n" );
    ok( uc.dwUserNameLength == ~0u, "unexpected length %u\n", uc.dwUserNameLength );
    ok( !uc.lpszPassword, "unexpected password\n" );
    ok( uc.dwPasswordLength == ~0u, "unexpected length %u\n", uc.dwPasswordLength );
    ok( !uc.lpszUrlPath, "unexpected path %s\n", wine_dbgstr_w(uc.lpszUrlPath) );
    ok( uc.dwUrlPathLength == 0, "unexpected length %u\n", uc.dwUrlPathLength );
    ok( !uc.lpszExtraInfo, "unexpected extra info %s\n", wine_dbgstr_w(uc.lpszExtraInfo) );
    ok( uc.dwExtraInfoLength == 0, "unexpected length %u\n", uc.dwExtraInfoLength );

    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url15, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_WINHTTP_INVALID_URL, "got %u\n", error );

    reset_url_components( &uc );
    uc.nPort = 1;
    ret = WinHttpCrackUrl( url16, 0, 0, &uc );
    ok( ret, "got %u\n", GetLastError() );
    ok( !uc.nPort, "got %u\n", uc.nPort );

    reset_url_components( &uc );
    uc.nPort = 1;
    ret = WinHttpCrackUrl( url17, 0, 0, &uc );
    ok( ret, "got %u\n", GetLastError() );
    todo_wine ok( uc.nPort == 80, "got %u\n", uc.nPort );

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize      = sizeof(uc);
    uc.lpszScheme        = scheme;
    uc.dwSchemeLength    = ARRAY_SIZE(scheme);
    uc.lpszHostName      = host;
    uc.dwHostNameLength  = ARRAY_SIZE(host);
    uc.lpszUrlPath       = path;
    uc.dwUrlPathLength   = ARRAY_SIZE(path);
    ret = WinHttpCrackUrl( url21, 0, 0, &uc );
    ok( ret, "got %u\n", GetLastError() );
    ok( !lstrcmpW( uc.lpszUrlPath, url21 + 37 ), "unexpected path %s\n", wine_dbgstr_w(uc.lpszUrlPath) );
    ok( uc.dwUrlPathLength == 50, "unexpected length %u\n", uc.dwUrlPathLength );
}

START_TEST(url)
{
    WinHttpCreateUrl_test();
    WinHttpCrackUrl_test();
}
