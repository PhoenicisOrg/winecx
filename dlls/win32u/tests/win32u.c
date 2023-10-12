/*
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "wine/test.h"

#include "winbase.h"
#include "ntuser.h"


static void test_NtUserEnumDisplayDevices(void)
{
    NTSTATUS ret;
    DISPLAY_DEVICEW info = { sizeof(DISPLAY_DEVICEW) };

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 0, &info, 0 );
    ok( !ret && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %#lx %lu\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 12345, &info, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
                  "NtUserEnumDisplayDevices returned %#lx %lu\n", ret,
                  GetLastError() );

    info.cb = 0;

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 0, &info, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %#lx %lu\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 12345, &info, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %#lx %lu\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 0, NULL, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %#lx %lu\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 12345, NULL, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %#lx %lu\n", ret, GetLastError() );
}

static void test_NtUserCloseWindowStation(void)
{
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = NtUserCloseWindowStation( 0 );
    ok( !ret && GetLastError() == ERROR_INVALID_HANDLE,
        "NtUserCloseWindowStation returned %x %lu\n", ret, GetLastError() );
}

static void test_window_props(void)
{
    HANDLE prop;
    ATOM atom;
    HWND hwnd;
    BOOL ret;

    hwnd = CreateWindowExA( 0, "static", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL );

    atom = GlobalAddAtomW( L"test" );

    ret = NtUserSetProp( hwnd, UlongToPtr(atom), UlongToHandle(0xdeadbeef) );
    ok( ret, "NtUserSetProp failed: %lu\n", GetLastError() );

    prop = GetPropW( hwnd, L"test" );
    ok( prop == UlongToHandle(0xdeadbeef), "prop = %p\n", prop );

    prop = NtUserGetProp( hwnd, UlongToPtr(atom) );
    ok( prop == UlongToHandle(0xdeadbeef), "prop = %p\n", prop );

    prop = NtUserRemoveProp( hwnd, UlongToPtr(atom) );
    ok( prop == UlongToHandle(0xdeadbeef), "prop = %p\n", prop );

    prop = GetPropW(hwnd, L"test");
    ok(!prop, "prop = %p\n", prop);

    GlobalDeleteAtom( atom );
    DestroyWindow( hwnd );
}

static void test_class(void)
{
    UNICODE_STRING name;
    WCHAR buf[64];
    WNDCLASSW cls;
    ATOM class;
    HWND hwnd;
    ULONG ret;

    memset( &cls, 0, sizeof(cls) );
    cls.style         = CS_HREDRAW | CS_VREDRAW;
    cls.lpfnWndProc   = DefWindowProcW;
    cls.hInstance     = GetModuleHandleW( NULL );
    cls.hbrBackground = GetStockObject( WHITE_BRUSH );
    cls.lpszMenuName  = 0;
    cls.lpszClassName = L"test";

    class = RegisterClassW( &cls );
    ok( class, "RegisterClassW failed: %lu\n", GetLastError() );

    hwnd = CreateWindowW( L"test", L"test name", WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
                          CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, NULL, 0 );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.Length = 0xdead;
    name.MaximumLength = sizeof(buf);
    ret = NtUserGetAtomName( class, &name );
    ok( ret == 4, "NtUserGetAtomName returned %lu\n", ret );
    ok( name.Length == 0xdead, "Length = %u\n", name.Length );
    ok( name.MaximumLength == sizeof(buf), "MaximumLength = %u\n", name.MaximumLength );
    ok( !wcscmp( buf, L"test" ), "buf = %s\n", debugstr_w(buf) );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.Length = 0xdead;
    name.MaximumLength = 8;
    ret = NtUserGetAtomName( class, &name );
    ok( ret == 3, "NtUserGetAtomName returned %lu\n", ret );
    ok( name.Length == 0xdead, "Length = %u\n", name.Length );
    ok( name.MaximumLength == 8, "MaximumLength = %u\n", name.MaximumLength );
    ok( !wcscmp( buf, L"tes" ), "buf = %s\n", debugstr_w(buf) );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.MaximumLength = 1;
    SetLastError( 0xdeadbeef );
    ret = NtUserGetAtomName( class, &name );
    ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "NtUserGetAtomName returned %lx %lu\n", ret, GetLastError() );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.Length = 0xdead;
    name.MaximumLength = sizeof(buf);
    ret = NtUserGetClassName( hwnd, FALSE, &name );
    ok( ret == 4, "NtUserGetClassName returned %lu\n", ret );
    ok( name.Length == 0xdead, "Length = %u\n", name.Length );
    ok( name.MaximumLength == sizeof(buf), "MaximumLength = %u\n", name.MaximumLength );
    ok( !wcscmp( buf, L"test" ), "buf = %s\n", debugstr_w(buf) );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.Length = 0xdead;
    name.MaximumLength = 8;
    ret = NtUserGetClassName( hwnd, FALSE, &name );
    ok( ret == 3, "NtUserGetClassName returned %lu\n", ret );
    ok( name.Length == 0xdead, "Length = %u\n", name.Length );
    ok( name.MaximumLength == 8, "MaximumLength = %u\n", name.MaximumLength );
    ok( !wcscmp( buf, L"tes" ), "buf = %s\n", debugstr_w(buf) );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.MaximumLength = 1;
    SetLastError( 0xdeadbeef );
    ret = NtUserGetClassName( hwnd, FALSE, &name );
    ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "NtUserGetClassName returned %lx %lu\n", ret, GetLastError() );

    DestroyWindow( hwnd );

    ret = UnregisterClassW( L"test", GetModuleHandleW(NULL) );
    ok( ret, "UnregisterClassW failed: %lu\n", GetLastError() );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.MaximumLength = sizeof(buf);
    SetLastError( 0xdeadbeef );
    ret = NtUserGetAtomName( class, &name );
    ok( !ret && GetLastError() == ERROR_INVALID_HANDLE,
        "NtUserGetAtomName returned %lx %lu\n", ret, GetLastError() );
    ok( buf[0] == 0xcccc, "buf = %s\n", debugstr_w(buf) );

}

static BOOL WINAPI count_win( HWND hwnd, LPARAM lparam )
{
    ULONG *cnt = (ULONG *)lparam;
    (*cnt)++;
    return TRUE;
}

static void test_NtUserBuildHwndList(void)
{
    ULONG size, desktop_windows_cnt;
    HWND buf[512], hwnd;
    NTSTATUS status;

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 1, "size = %lu\n", size );
    ok( buf[0] == HWND_BOTTOM, "buf[0] = %p\n", buf[0] );

    hwnd = CreateWindowExA( 0, "static", NULL, WS_POPUP, 0,0,0,0,GetDesktopWindow(),0,0, NULL );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 3, "size = %lu\n", size );
    ok( buf[0] == hwnd, "buf[0] = %p\n", buf[0] );
    ok( buf[2] == HWND_BOTTOM, "buf[0] = %p\n", buf[2] );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), 3, buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 3, "size = %lu\n", size );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), 2, buf, &size );
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 3, "size = %lu\n", size );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), 1, buf, &size );
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 3, "size = %lu\n", size );

    desktop_windows_cnt = 0;
    EnumDesktopWindows( 0, count_win, (LPARAM)&desktop_windows_cnt );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 1, 0, ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == desktop_windows_cnt + 1, "size = %lu, expected %lu\n", size, desktop_windows_cnt + 1 );

    desktop_windows_cnt = 0;
    EnumDesktopWindows( GetThreadDesktop( GetCurrentThreadId() ), count_win, (LPARAM)&desktop_windows_cnt );

    size = 0;
    status = NtUserBuildHwndList( GetThreadDesktop(GetCurrentThreadId()), 0, 0, 1, 0,
                                  ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == desktop_windows_cnt + 1, "size = %lu, expected %lu\n", size, desktop_windows_cnt + 1 );

    size = 0;
    status = NtUserBuildHwndList( GetThreadDesktop(GetCurrentThreadId()), 0, 0, 0, 0,
                                  ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    todo_wine
    ok( size > desktop_windows_cnt + 1, "size = %lu, expected %lu\n", size, desktop_windows_cnt + 1 );

    size = 0xdeadbeef;
    status = NtUserBuildHwndList( UlongToHandle(0xdeadbeef), 0, 0, 0, 0,
                                  ARRAYSIZE(buf), buf, &size );
    ok( status == STATUS_INVALID_HANDLE, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 0xdeadbeef, "size = %lu\n", size );

    DestroyWindow( hwnd );
}

static void test_cursoricon(void)
{
    WCHAR module[MAX_PATH], res_buf[MAX_PATH];
    UNICODE_STRING module_str, res_str;
    BYTE bmp_bits[1024];
    LONG width, height;
    DWORD rate, steps;
    HCURSOR frame;
    HANDLE handle;
    ICONINFO info;
    unsigned int i;
    BOOL ret;

    for (i = 0; i < sizeof(bmp_bits); ++i)
        bmp_bits[i] = 111 * i;

    handle = CreateIcon( 0, 16, 16, 1, 1, bmp_bits, &bmp_bits[16 * 16 / 8] );
    ok(handle != 0, "CreateIcon failed\n");

    ret = NtUserGetIconSize( handle, 0, &width, &height );
    ok( ret, "NtUserGetIconSize failed: %lu\n", GetLastError() );
    ok( width == 16, "width = %ld\n", width );
    ok( height == 32, "height = %ld\n", height );

    ret = NtUserGetIconSize( handle, 6, &width, &height );
    ok( ret, "NtUserGetIconSize failed: %lu\n", GetLastError() );
    ok( width == 16, "width = %ld\n", width );
    ok( height == 32, "height = %ld\n", height );

    frame = NtUserGetCursorFrameInfo( handle, 0, &rate, &steps );
    ok( frame != NULL, "NtUserGetCursorFrameInfo failed: %lu\n", GetLastError() );
    ok( frame == handle, "frame != handle\n" );
    ok( rate == 0, "rate = %lu\n", rate );
    ok( steps == 1, "steps = %lu\n", steps );

    ret = NtUserGetIconInfo( handle, &info, NULL, NULL, NULL, 0 );
    ok( ret, "NtUserGetIconInfo failed: %lu\n", GetLastError() );
    ok( info.fIcon == TRUE, "fIcon = %x\n", info.fIcon );
    ok( info.xHotspot == 8, "xHotspot = %lx\n", info.xHotspot );
    ok( info.yHotspot == 8, "yHotspot = %lx\n", info.yHotspot );
    DeleteObject( info.hbmColor );
    DeleteObject( info.hbmMask );

    memset( module, 0xcc, sizeof(module) );
    module_str.Buffer = module;
    module_str.Length = 0xdead;
    module_str.MaximumLength = sizeof(module);
    memset( res_buf, 0xcc, sizeof(res_buf) );
    res_str.Buffer = res_buf;
    res_str.Length = 0xdead;
    res_str.MaximumLength = sizeof(res_buf);
    ret = NtUserGetIconInfo( handle, &info, &module_str, &res_str, NULL, 0 );
    ok( ret, "NtUserGetIconInfo failed: %lu\n", GetLastError() );
    ok( info.fIcon == TRUE, "fIcon = %x\n", info.fIcon );
    ok( !module_str.Length, "module_str.Length = %u\n", module_str.Length );
    ok( !res_str.Length, "res_str.Length = %u\n", res_str.Length );
    ok( module_str.Buffer == module, "module_str.Buffer = %p\n", module_str.Buffer );
    ok( !res_str.Buffer, "res_str.Buffer = %p\n", res_str.Buffer );
    ok( module[0] == 0xcccc, "module[0] = %x\n", module[0] );
    ok( res_buf[0] == 0xcccc, "res_buf[0] = %x\n", res_buf[0] );
    DeleteObject( info.hbmColor );
    DeleteObject( info.hbmMask );

    ret = NtUserDestroyCursor( handle, 0 );
    ok( ret, "NtUserDestroyIcon failed: %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserGetIconSize( handle, 0, &width, &height );
    ok( !ret && GetLastError() == ERROR_INVALID_CURSOR_HANDLE,
        "NtUserGetIconSize returned %x %lu\n", ret, GetLastError() );

    /* Test a system icon */
    handle = LoadIconA( 0, (LPCSTR)IDI_HAND );
    ok( handle != NULL, "LoadIcon icon failed, error %lu\n", GetLastError() );

    ret = NtUserGetIconSize( handle, 0, &width, &height );
    ok( width == 32, "width = %ld\n", width );
    ok( height == 64, "height = %ld\n", height );
    ok( ret, "NtUserGetIconSize failed: %lu\n", GetLastError() );

    memset( module, 0xcc, sizeof(module) );
    module_str.Buffer = module;
    module_str.Length = 0xdead;
    module_str.MaximumLength = sizeof(module);
    memset( res_buf, 0xcc, sizeof(res_buf) );
    res_str.Buffer = res_buf;
    res_str.Length = 0xdead;
    res_str.MaximumLength = sizeof(res_buf);
    ret = NtUserGetIconInfo( handle, &info, &module_str, &res_str, NULL, 0 );
    ok( ret, "NtUserGetIconInfo failed: %lu\n", GetLastError() );
    ok( info.fIcon == TRUE, "fIcon = %x\n", info.fIcon );
    ok( module_str.Length, "module_str.Length = 0\n" );
    ok( !res_str.Length, "res_str.Length = %u\n", res_str.Length );
    ok( module_str.Buffer == module, "module_str.Buffer = %p\n", module_str.Buffer );
    ok( res_str.Buffer == (WCHAR *)IDI_HAND, "res_str.Buffer = %p\n", res_str.Buffer );
    DeleteObject( info.hbmColor );
    DeleteObject( info.hbmMask );

    module[module_str.Length] = 0;
    ok( GetModuleHandleW(module) == GetModuleHandleW(L"user32.dll"),
        "GetIconInfoEx wrong module %s\n", wine_dbgstr_w(module) );

    ret = DestroyIcon(handle);
    ok(ret, "Destroy icon failed, error %lu.\n", GetLastError());
}

static LRESULT WINAPI test_message_call_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch (msg)
    {
    case WM_SETTEXT:
        ok( !wcscmp( (const WCHAR *)lparam, L"test" ),
            "lparam = %s\n", wine_dbgstr_w( (const WCHAR *)lparam ));
        return 6;
    case WM_USER:
        ok( wparam == 1, "wparam = %Iu\n", wparam );
        ok( lparam == 2, "lparam = %Iu\n", lparam );
        return 3;
    case WM_USER + 1:
        return lparam;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void WINAPI test_message_callback( HWND hwnd, UINT msg, ULONG_PTR data, LRESULT result )
{
    ok( msg == WM_USER, "msg = %u\n", msg );
    ok( data == 10, "data = %Iu\n", data );
    ok( result == 3, "result = %Iu\n", result );
}

static void test_message_call(void)
{
    const LPARAM large_lparam = (LPARAM)(3 + ((ULONGLONG)1 << 60));
    struct send_message_callback_params callback_params = {
        .callback = test_message_callback,
        .data = 10,
    };
    struct send_message_timeout_params smp;
    WNDCLASSW cls = { 0 };
    LRESULT res;
    HWND hwnd;

    cls.lpfnWndProc = test_message_call_proc;
    cls.lpszClassName = L"TestClass";
    RegisterClassW( &cls );

    hwnd = CreateWindowExW( 0, L"TestClass", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL );

    res = NtUserMessageCall( hwnd, WM_USER, 1, 2, NULL, NtUserSendMessage, FALSE );
    ok( res == 3, "res = %Iu\n", res );

    res = NtUserMessageCall( hwnd, WM_USER, 1, 2, NULL, NtUserSendMessage, TRUE );
    ok( res == 3, "res = %Iu\n", res );

    res = NtUserMessageCall( hwnd, WM_SETTEXT, 0, (LPARAM)L"test", NULL, NtUserSendMessage, FALSE );
    ok( res == 6, "res = %Iu\n", res );

    res = NtUserMessageCall( hwnd, WM_SETTEXT, 0, (LPARAM)"test", NULL, NtUserSendMessage, TRUE );
    ok( res == 6, "res = %Iu\n", res );

    SetLastError( 0xdeadbeef );
    res = NtUserMessageCall( UlongToHandle(0xdeadbeef), WM_USER, 1, 2, 0, NtUserSendMessage, TRUE );
    ok( !res, "res = %Iu\n", res );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "GetLastError() = %lu\n", GetLastError());

    res = NtUserMessageCall( hwnd, WM_USER + 1, 0, large_lparam, 0, NtUserSendMessage, FALSE );
    ok( res == large_lparam, "res = %Iu\n", res );

    smp.flags = 0;
    smp.timeout = 10;
    smp.result = 0xdeadbeef;
    res = NtUserMessageCall( hwnd, WM_USER, 1, 2, &smp, NtUserSendMessageTimeout, FALSE );
    ok( res == 3, "res = %Iu\n", res );
    ok( smp.result == 1, "smp.result = %Iu\n", smp.result );

    smp.flags = 0;
    smp.timeout = 10;
    smp.result = 0xdeadbeef;
    res = NtUserMessageCall( hwnd, WM_USER + 1, 0, large_lparam,
                             &smp, NtUserSendMessageTimeout, FALSE );
    ok( res == large_lparam, "res = %Iu\n", res );
    ok( smp.result == 1, "smp.result = %Iu\n", smp.result );

    res = NtUserMessageCall( hwnd, WM_USER, 1, 2, (void *)0xdeadbeef,
                             NtUserSendNotifyMessage, FALSE );
    ok( res == 1, "res = %Iu\n", res );

    res = NtUserMessageCall( hwnd, WM_USER, 1, 2, &callback_params,
                             NtUserSendMessageCallback, FALSE );
    ok( res == 1, "res = %Iu\n", res );

    DestroyWindow( hwnd );
    UnregisterClassW( L"TestClass", NULL );
}

static void test_window_text(void)
{
    WCHAR buf[512];
    LRESULT res;
    int len;
    HWND hwnd;

    hwnd = CreateWindowExW( 0, L"static", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL );

    memset( buf, 0xcc, sizeof(buf) );
    len = NtUserInternalGetWindowText( hwnd, buf, ARRAYSIZE(buf) );
    ok( len == 0, "len = %d\n", len );
    ok( !buf[0], "buf = %s\n", wine_dbgstr_w(buf) );

    res = NtUserMessageCall( hwnd, WM_SETTEXT, 0, (LPARAM)L"test", 0, NtUserDefWindowProc, FALSE );
    ok( res == 1, "res = %Id\n", res );

    memset( buf, 0xcc, sizeof(buf) );
    len = NtUserInternalGetWindowText( hwnd, buf, ARRAYSIZE(buf) );
    ok( len == 4, "len = %d\n", len );
    ok( !lstrcmpW( buf, L"test" ), "buf = %s\n", wine_dbgstr_w(buf) );

    res = NtUserMessageCall( hwnd, WM_GETTEXTLENGTH, 0, 0, 0, NtUserDefWindowProc, TRUE );
    ok( res == 4, "res = %Id\n", res );

    res = NtUserMessageCall( hwnd, WM_GETTEXTLENGTH, 0, 0, 0, NtUserDefWindowProc, FALSE );
    ok( res == 4, "res = %Id\n", res );

    res = NtUserMessageCall( hwnd, WM_SETTEXT, 0, (LPARAM)"TestA", 0, NtUserDefWindowProc, TRUE );
    ok( res == 1, "res = %Id\n", res );

    memset( buf, 0xcc, sizeof(buf) );
    len = NtUserInternalGetWindowText( hwnd, buf, ARRAYSIZE(buf) );
    ok( len == 5, "len = %d\n", len );
    ok( !lstrcmpW( buf, L"TestA" ), "buf = %s\n", wine_dbgstr_w(buf) );

    res = NtUserMessageCall( hwnd, WM_GETTEXTLENGTH, 0, 0, 0, NtUserDefWindowProc, TRUE );
    ok( res == 5, "res = %Id\n", res );

    DestroyWindow( hwnd );
}

#define test_menu_item_id(a, b, c) test_menu_item_id_(a, b, c, __LINE__)
static void test_menu_item_id_( HMENU menu, int pos, int expect, int line )
{
    MENUITEMINFOW item;
    BOOL ret;

    item.cbSize = sizeof(item);
    item.fMask = MIIM_ID;
    ret = GetMenuItemInfoW( menu, pos, TRUE, &item );
    ok_(__FILE__,line)( ret, "GetMenuItemInfoW failed: %lu\n", GetLastError() );
    ok_(__FILE__,line)( item.wID == expect, "got if %d, expected %d\n", item.wID, expect );
}

static void test_menu(void)
{
    MENUITEMINFOW item;
    HMENU menu;
    int count;
    BOOL ret;

    menu = CreateMenu();

    memset( &item, 0, sizeof(item) );
    item.cbSize = sizeof(item);
    item.fMask = MIIM_ID;
    item.wID = 10;
    ret = NtUserThunkedMenuItemInfo( menu, 0, MF_BYPOSITION, NtUserInsertMenuItem, &item, NULL );
    ok( ret, "InsertMenuItemW failed: %lu\n", GetLastError() );

    count = GetMenuItemCount( menu );
    ok( count == 1, "count = %d\n", count );

    item.wID = 20;
    ret = NtUserThunkedMenuItemInfo( menu, 1, MF_BYPOSITION, NtUserInsertMenuItem, &item, NULL );
    ok( ret, "InsertMenuItemW failed: %lu\n", GetLastError() );

    count = GetMenuItemCount( menu );
    ok( count == 2, "count = %d\n", count );
    test_menu_item_id( menu, 0, 10 );
    test_menu_item_id( menu, 1, 20 );

    item.wID = 30;
    ret = NtUserThunkedMenuItemInfo( menu, 1, MF_BYPOSITION, NtUserInsertMenuItem, &item, NULL );
    ok( ret, "InsertMenuItemW failed: %lu\n", GetLastError() );

    count = GetMenuItemCount( menu );
    ok( count == 3, "count = %d\n", count );
    test_menu_item_id( menu, 0, 10 );
    test_menu_item_id( menu, 1, 30 );
    test_menu_item_id( menu, 2, 20 );

    item.wID = 50;
    ret = NtUserThunkedMenuItemInfo( menu, 10, 0, NtUserInsertMenuItem, &item, NULL );
    ok( ret, "InsertMenuItemW failed: %lu\n", GetLastError() );

    count = GetMenuItemCount( menu );
    ok( count == 4, "count = %d\n", count );
    test_menu_item_id( menu, 0, 50 );
    test_menu_item_id( menu, 1, 10 );
    test_menu_item_id( menu, 2, 30 );
    test_menu_item_id( menu, 3, 20 );

    item.wID = 60;
    ret = NtUserThunkedMenuItemInfo( menu, 1, MF_BYPOSITION, NtUserSetMenuItemInfo, &item, NULL );
    ok( ret, "InsertMenuItemW failed: %lu\n", GetLastError() );

    count = GetMenuItemCount( menu );
    ok( count == 4, "count = %d\n", count );
    test_menu_item_id( menu, 1, 60 );

    ret = NtUserDestroyMenu( menu );
    ok( ret, "NtUserDestroyMenu failed: %lu\n", GetLastError() );
}

static MSG *msg_ptr;

static LRESULT WINAPI hook_proc( INT code, WPARAM wparam, LPARAM lparam )
{
    msg_ptr = (MSG *)lparam;
    ok( code == 100, "code = %d\n", code );
    ok( msg_ptr->time == 1, "time = %lx\n", msg_ptr->time );
    ok( msg_ptr->wParam == 10, "wParam = %Ix\n", msg_ptr->wParam );
    ok( msg_ptr->lParam == 20, "lParam = %Ix\n", msg_ptr->lParam );
    msg_ptr->time = 3;
    msg_ptr->wParam = 1;
    msg_ptr->lParam = 2;
    return CallNextHookEx( NULL, code, wparam, lparam );
}

static void test_message_filter(void)
{
    HHOOK hook;
    MSG msg;
    BOOL ret;

    hook = SetWindowsHookExW( WH_MSGFILTER, hook_proc, NULL, GetCurrentThreadId() );
    ok( hook != NULL, "SetWindowsHookExW failed\n");

    memset( &msg, 0, sizeof(msg) );
    msg.time = 1;
    msg.wParam = 10;
    msg.lParam = 20;
    ret = NtUserCallMsgFilter( &msg, 100 );
    ok( !ret, "CallMsgFilterW returned: %x\n", ret );
    ok( msg_ptr != &msg, "our ptr was passed directly to hook\n" );

    if (sizeof(void *) == 8) /* on some Windows versions, msg is not modified on wow64 */
    {
        ok( msg.time == 3, "time = %lx\n", msg.time );
        ok( msg.wParam == 1, "wParam = %Ix\n", msg.wParam );
        ok( msg.lParam == 2, "lParam = %Ix\n", msg.lParam );
    }

    ret = NtUserUnhookWindowsHookEx( hook );
    ok( ret, "NtUserUnhookWindowsHook failed: %lu\n", GetLastError() );
}

static char calls[128];

static void WINAPI timer_func( HWND hwnd, UINT msg, UINT_PTR id, DWORD time )
{
    sprintf( calls + strlen( calls ), "timer%Iu,", id );
    NtUserKillTimer( hwnd, id );

    if (!id)
    {
        MSG msg;
        NtUserSetTimer( hwnd, 1, 1, timer_func, TIMERV_DEFAULT_COALESCING );
        while (GetMessageW( &msg, NULL, 0, 0 ))
        {
            TranslateMessage( &msg );
            NtUserDispatchMessage( &msg );
            if (msg.message == WM_TIMER) break;
        }
    }

    strcat( calls, "crash," );
    *(volatile int *)0 = 0;
}

static void test_timer(void)
{
    HWND hwnd;
    MSG msg;

    hwnd = CreateWindowExA( 0, "static", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL );

    NtUserSetTimer( hwnd, 0, 1, timer_func, TIMERV_DEFAULT_COALESCING );

    calls[0] = 0;
    while (GetMessageW( &msg, NULL, 0, 0 ))
    {
        TranslateMessage( &msg );
        NtUserDispatchMessage( &msg );
        if (msg.message == WM_TIMER) break;
    }

    ok( !strcmp( calls, "timer0,timer1,crash,crash," ), "calls = %s\n", calls );
    DestroyWindow( hwnd );
}

static void test_NtUserDisplayConfigGetDeviceInfo(void)
{
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source_name;
    NTSTATUS status;

    source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source_name.header.size = sizeof(source_name.header);
    status = NtUserDisplayConfigGetDeviceInfo(&source_name.header);
    ok(status == STATUS_INVALID_PARAMETER, "got %#lx.\n", status);

    source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source_name.header.size = sizeof(source_name);
    source_name.header.adapterId.LowPart = 0xFFFF;
    source_name.header.adapterId.HighPart = 0xFFFF;
    source_name.header.id = 0;
    status = NtUserDisplayConfigGetDeviceInfo(&source_name.header);
    ok(status == STATUS_UNSUCCESSFUL || status == STATUS_NOT_SUPPORTED, "got %#lx.\n", status);
}

static LRESULT WINAPI test_ipc_message_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch (msg)
    {
    case WM_SETTEXT:
    case CB_FINDSTRING:
        ok( !wcscmp( (const WCHAR *)lparam, L"test" ),
            "lparam = %s\n", wine_dbgstr_w( (const WCHAR *)lparam ));
        return 6;

    case WM_GETTEXT:
    case EM_GETLINE:
        ok( wparam == 100, "wparam = %Iu\n", wparam );
        wcscpy( (void *)lparam, L"Test" );
        return 4;

    case CB_GETLBTEXT:
        ok( wparam == 100, "wparam = %Iu\n", wparam );
        wcscpy( (void *)lparam, L"Te" );
        return 4;

    case WM_GETTEXTLENGTH:
        return 99;

    case WM_MDICREATE:
        {
            MDICREATESTRUCTW *mdi = (MDICREATESTRUCTW *)lparam;
            ok( !wcscmp( mdi->szClass, L"TestClass" ), "szClass = %s\n", wine_dbgstr_w( mdi->szClass ));
            ok( !wcscmp( mdi->szTitle, L"TestTitle" ), "szTitle = %s\n", wine_dbgstr_w( mdi->szTitle ));
            return 0xdeadbeef;
        }

    case WM_GETDLGCODE:
        return !lparam;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void test_inter_process_messages( const char *argv0 )
{
    WNDCLASSW cls = { 0 };
    char path[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA startup;
    MSG msg;
    HWND hwnd;
    BOOL ret;

    cls.lpfnWndProc = test_ipc_message_proc;
    cls.lpszClassName = L"TestIPCClass";
    RegisterClassW( &cls );

    hwnd = CreateWindowExW( 0, L"TestIPCClass", NULL, WS_POPUP | CBS_HASSTRINGS, 0,0,0,0,0,0,0, NULL );

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    sprintf( path, "%s win32u ipcmsg %Ix", argv0, (INT_PTR)hwnd );
    ret = CreateProcessA( NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &pi );
    ok( ret, "CreateProcess '%s' failed err %lu.\n", path, GetLastError() );

    do
    {
        GetMessageW( &msg, NULL, 0, 0 );
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    } while (msg.message != WM_USER);

    wait_child_process( pi.hProcess );

    CloseHandle( pi.hThread );
    CloseHandle( pi.hProcess );

    DestroyWindow( hwnd );
    UnregisterClassW( L"TestIPCClass", NULL );
}

static void test_inter_process_child( HWND hwnd )
{
    MDICREATESTRUCTA mdi;
    WCHAR bufW[100];
    char buf[100];
    int res;

    res = SendMessageA( hwnd, WM_SETTEXT, 0, (LPARAM)"test" );
    ok( res == 6, "WM_SETTEXT returned %d\n", res );

    res = NtUserMessageCall( hwnd, WM_SETTEXT, 0, (LPARAM)"test", NULL, NtUserSendMessage, TRUE );
    ok( res == 6, "res = %d\n", res );

    res = NtUserMessageCall( hwnd, CB_FINDSTRING, 0, (LPARAM)"test", NULL, NtUserSendMessage, TRUE );
    ok( res == 6, "res = %d\n", res );

    memset( buf, 0xcc, sizeof(buf) );
    res = NtUserMessageCall( hwnd, WM_GETTEXT, sizeof(buf), (LPARAM)buf, NULL, NtUserSendMessage, TRUE );
    ok( res == 4, "res = %d\n", res );
    ok( !strcmp( buf, "Test" ), "buf = %s\n", buf );

    memset( bufW, 0xcc, sizeof(bufW) );
    res = NtUserMessageCall( hwnd, WM_GETTEXT, ARRAYSIZE(bufW), (LPARAM)bufW, NULL, NtUserSendMessage, FALSE );
    ok( res == 4, "res = %d\n", res );
    ok( !wcscmp( bufW, L"Test" ), "buf = %s\n", buf );

    memset( buf, 0xcc, sizeof(buf) );
    res = NtUserMessageCall( hwnd, CB_GETLBTEXT, 100, (LPARAM)buf, NULL, NtUserSendMessage, TRUE );
    todo_wine
    ok( res == 1, "res = %d\n", res );
    ok( buf[0] == 'T', "buf[0] = %c\n", buf[0] );
    todo_wine
    ok( buf[1] == (char)0xcc, "buf[1] = %x\n", buf[1] );

    memset( bufW, 0xcc, sizeof(bufW) );
    res = NtUserMessageCall( hwnd, CB_GETLBTEXT, 100, (LPARAM)bufW, NULL, NtUserSendMessage, FALSE );
    todo_wine
    ok( res == 1, "res = %d\n", res );
    ok( bufW[0] == 'T', "bufW[0] = %c\n", buf[0] );
    ok( bufW[1] && buf[1] != 'e', "bufW[1] = %x\n", buf[1] );

    memset( buf, 0xcc, sizeof(buf) );
    *(DWORD *)buf = ARRAYSIZE(buf);
    res = NtUserMessageCall( hwnd, EM_GETLINE, sizeof(buf), (LPARAM)buf, NULL, NtUserSendMessage, TRUE );
    ok( res == 4, "res = %d\n", res );
    ok( !strcmp( buf, "Test" ), "buf = %s\n", buf );

    res = NtUserMessageCall( hwnd, WM_GETTEXTLENGTH, 0, 0, NULL, NtUserSendMessage, TRUE );
    ok( res == 4, "res = %d\n", res );

    res = NtUserMessageCall( hwnd, WM_GETDLGCODE, 0, 0, NULL, NtUserSendMessage, TRUE );
    ok( res == 1, "res = %d\n", res );

    mdi.szClass = "TestClass";
    mdi.szTitle = "TestTitle";
    mdi.hOwner = 0;
    mdi.x = mdi.y = 10;
    mdi.cx = mdi.cy = 100;
    mdi.style = 0;
    mdi.lParam = 0xdeadbeef;
    res = NtUserMessageCall( hwnd, WM_MDICREATE, 0, (LPARAM)&mdi, NULL, NtUserSendMessage, TRUE );
    ok( res == 0xdeadbeef, "res = %d\n", res );

    PostMessageA( hwnd, WM_USER, 0, 0 );
}

struct lparam_hook_test
{
    const char *name;
    UINT message;
    WPARAM wparam;
    BOOL no_wparam_check;
    LRESULT msg_result;
    LRESULT check_result;
    BOOL todo_result;
    const void *lparam;
    const void *change_lparam;
    const void *check_lparam;
    const void *in_lparam;
    size_t lparam_size;
    size_t lparam_init_size;
    size_t check_size;
    BOOL poison_lparam;
    BOOL not_allowed;
};

static const struct lparam_hook_test *current_hook_test;

static LPARAM callwnd_hook_lparam, callwnd_hook_lparam2, retwnd_hook_lparam, retwnd_hook_lparam2;
static LPARAM wndproc_lparam;
static char lparam_buffer[521];

static void check_params( const struct lparam_hook_test *test, UINT message,
                         WPARAM wparam, LPARAM lparam, BOOL is_ret )
{
    if (!test->no_wparam_check)
        ok( wparam == test->wparam, "got wparam %Ix, expected %Ix\n", wparam, test->wparam );
    if (lparam == (LPARAM)lparam_buffer)
        return;

    if (sizeof(void *) != 4 || (test->message != EM_SETTABSTOPS && test->message != LB_SETTABSTOPS))
        ok( (LPARAM)&message < lparam && lparam < (LPARAM)NtCurrentTeb()->Tib.StackBase,
            "lparam is not on the stack\n");

    switch (test->message)
    {
    case WM_COPYDATA:
        {
            const COPYDATASTRUCT *cds = (const COPYDATASTRUCT *)lparam;
            const COPYDATASTRUCT *cds_in = (const COPYDATASTRUCT *)lparam_buffer;
            ok( cds->dwData == cds_in->dwData, "cds->dwData != cds_in->dwData\n");
            ok( cds->cbData == cds_in->cbData, "cds->dwData != cds_in->dwData\n");
            if (cds_in->lpData)
            {
                ok( cds->lpData != cds_in->lpData, "cds->lpData == cds_in->lpData\n" );
                if (cds->cbData)
                    ok( !memcmp( cds->lpData, cds_in->lpData, cds->cbData ), "unexpected pvData %s\n",
                        wine_dbgstr_an( cds->lpData, cds->cbData ));
            }
            else
                ok( !cds->lpData, "cds->lpData = %p\n", cds->lpData );
        }
        break;

    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        ok( wparam, "wparam = 0\n" );
        break;

    case EM_GETLINE:
        if (!is_ret)
        {
            WCHAR *buf = (WCHAR *)lparam;
            ok(buf[0] == 8, "buf[0] = %x\n", buf[0]);
        }
        break;

    default:
        if (test->check_size) {
            const void *expected;
            if (is_ret && test->check_lparam)
                expected = test->check_lparam;
            else if (is_ret && test->change_lparam)
                expected = test->change_lparam;
            else if (test->in_lparam)
                expected = test->in_lparam;
            else
                expected = test->lparam;
            ok( !memcmp( (const void *)lparam, expected, test->check_size ), "unexpected lparam content\n" );
        }
    }
}

static void poison_lparam( const struct lparam_hook_test *test, LPARAM lparam )
{
    /* message copy is never transfered back in hooks */
    if (test->lparam_size && lparam != (LPARAM)lparam_buffer)
        memset( (void *)lparam, 0xc0, test->lparam_size );
}

static LRESULT WINAPI callwnd_hook_proc( INT code, WPARAM wparam, LPARAM lparam )
{
    const struct lparam_hook_test *test = current_hook_test;
    CWPSTRUCT *cwp = (CWPSTRUCT *)lparam;
    LRESULT result;

    if (cwp->message != test->message) return CallNextHookEx( NULL, code, wparam, lparam );

    callwnd_hook_lparam = cwp->lParam;
    winetest_push_context( "call_hook" );
    check_params( test, cwp->message, cwp->wParam, cwp->lParam, FALSE );
    winetest_pop_context();

    result = CallNextHookEx( NULL, code, wparam, lparam );

    poison_lparam( test, cwp->lParam );
    return result;
}

static LRESULT WINAPI callwnd_hook_proc2( INT code, WPARAM wparam, LPARAM lparam )
{
    const struct lparam_hook_test *test = current_hook_test;
    CWPSTRUCT *cwp = (CWPSTRUCT *)lparam;
    LRESULT result;

    if (cwp->message != test->message) return CallNextHookEx( NULL, code, wparam, lparam );

    callwnd_hook_lparam2 = cwp->lParam;
    winetest_push_context( "call_hook2" );
    check_params( test, cwp->message, cwp->wParam, cwp->lParam, FALSE );
    winetest_pop_context();

    result = CallNextHookEx( NULL, code, wparam, lparam );

    poison_lparam( test, cwp->lParam );
    return result;
}

static LRESULT WINAPI retwnd_hook_proc( INT code, WPARAM wparam, LPARAM lparam )
{
    const struct lparam_hook_test *test = current_hook_test;
    CWPRETSTRUCT *cwpret = (CWPRETSTRUCT *)lparam;
    LRESULT result;

    if (cwpret->message != test->message) return CallNextHookEx( NULL, code, wparam, lparam );

    retwnd_hook_lparam = cwpret->lParam;
    winetest_push_context( "ret_hook" );
    check_params( test, cwpret->message, cwpret->wParam, cwpret->lParam, TRUE );
    winetest_pop_context();

    result = CallNextHookEx( NULL, code, wparam, lparam );

    poison_lparam( test, cwpret->lParam );
    return result;
}

static LRESULT WINAPI retwnd_hook_proc2( INT code, WPARAM wparam, LPARAM lparam )
{
    const struct lparam_hook_test *test = current_hook_test;
    CWPRETSTRUCT *cwpret = (CWPRETSTRUCT *)lparam;
    LRESULT result;

    if (cwpret->message != test->message) return CallNextHookEx( NULL, code, wparam, lparam );

    retwnd_hook_lparam2 = cwpret->lParam;
    winetest_push_context( "ret_hook2" );
    check_params( test, cwpret->message, cwpret->wParam, cwpret->lParam, TRUE );
    winetest_pop_context();

    result = CallNextHookEx( NULL, code, wparam, lparam );

    poison_lparam( test, cwpret->lParam );
    return result;
}

static LRESULT WINAPI lparam_test_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    const struct lparam_hook_test *test = current_hook_test;

    if (test && msg == test->message)
    {
        wndproc_lparam = lparam;

        winetest_push_context( "wndproc" );
        check_params( test, msg, wparam, lparam, FALSE );
        winetest_pop_context();

        if (test->change_lparam) memcpy( (void *)lparam, test->change_lparam, test->lparam_size );
        else if(test->poison_lparam) memset( (void *)lparam, 0xcc, test->lparam_size );
        return test->msg_result;
    }

    switch (msg)
    {
    case CB_GETLBTEXTLEN:
    case LB_GETTEXTLEN:
        return 7;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void test_msg_output( const struct lparam_hook_test *test, LRESULT result, BOOL hooks_called )
{
    const LPARAM orig = (LPARAM)lparam_buffer;
    const void *expected;

    /* Some messages are not allowed with NtUserMessageCall, they seem to be reserved
     * for system. Unhooked SendMessage still works for them. */
    if (test->not_allowed)
    {
        todo_wine ok( !wndproc_lparam, "wndproc_lparam called\n" );
        return;
    }

    ok( wndproc_lparam, "wndproc_lparam not called\n" );

    if (test->check_result)
        todo_wine_if(test->todo_result)
        ok( result == test->check_result, "unexpected result %Ix\n", result );
    else
        todo_wine_if(test->todo_result)
        ok( result == test->msg_result, "unexpected result %Ix\n", result );

    if (!test->lparam_size)
    {
        ok( wndproc_lparam == orig, "wndproc_lparam modified\n" );
        if (hooks_called)
        {
            ok( callwnd_hook_lparam == orig, "callwnd_hook_lparam modified\n" );
            ok( callwnd_hook_lparam2 == orig, "callwnd_hook_lparam2 modified\n" );
            ok( retwnd_hook_lparam == orig, "retwnd_hook_lparam modified\n" );
            ok( retwnd_hook_lparam2 == orig, "retwnd_hook_lparam2 modified\n" );
        }
        return;
    }

    expected = test->change_lparam ? test->change_lparam : test->lparam;
    if (test->check_lparam)
        expected = test->check_lparam;
    else if(test->change_lparam)
        expected = test->change_lparam;
    else
        expected = test->lparam;
    if (expected)
        todo_wine_if((test->message == CB_GETLBTEXT && test->msg_result == 7) ||
                     (test->message == LB_GETTEXT && test->msg_result == 7))
        ok( !memcmp( lparam_buffer, expected, test->lparam_size ), "unexpected lparam content\n" );

    ok( wndproc_lparam != orig, "wndproc_lparam unmodified\n" );
    if (!hooks_called)
        return;

    ok( callwnd_hook_lparam, "callwnd_hook_lparam not called\n" );
    ok( callwnd_hook_lparam2, "callwnd_hook_lparam2 not called\n" );
    ok( retwnd_hook_lparam, "retwnd_hook_lparam not called\n" );
    ok( retwnd_hook_lparam2, "retwnd_hook_lparam2 not called\n" );

    ok( orig != callwnd_hook_lparam, "callwnd_hook_lparam not modified\n" );
    ok( orig != callwnd_hook_lparam2, "callwnd_hook_lparam2 not modified\n" );
    ok( orig != retwnd_hook_lparam, "retwnd_hook_lparam not modified\n" );
    ok( orig != retwnd_hook_lparam2, "retwnd_hook_lparam2 not modified\n" );

    /*
     * Only the first hook's lparam matches window proc, following hook
     * calls copy the message again. Even when lparam values match, they
     * are copied separatelly for each proc invocation. Poisoning their
     * content in hook procs has no effect on other calls.
     */
    ok( wndproc_lparam == callwnd_hook_lparam, "wndproc_lparam %Ix != callwnd_hook_lparam %Ix\n",
        wndproc_lparam, callwnd_hook_lparam);
    todo_wine
    ok( callwnd_hook_lparam != callwnd_hook_lparam2, "wndproc_lparam == callwnd_hook_lparam2\n" );
    ok( wndproc_lparam == retwnd_hook_lparam, "wndproc_lparam %Ix != retwnd_hook_lparam %Ix\n",
        wndproc_lparam, retwnd_hook_lparam);
    todo_wine
    ok( retwnd_hook_lparam != retwnd_hook_lparam2, "wndproc_lparam == retwnd_hook_lparam2\n"  );
}

static void init_hook_test( const struct lparam_hook_test *test )
{
    wndproc_lparam = 0;
    callwnd_hook_lparam = 0;
    callwnd_hook_lparam2 = 0;
    retwnd_hook_lparam = 0;
    retwnd_hook_lparam2 = 0;

    if (test->lparam_size)
    {
        if (test->lparam_init_size)
            memcpy( lparam_buffer, test->lparam, test->lparam_init_size );
        else if (test->lparam)
            memcpy( lparam_buffer, test->lparam, test->lparam_size );
        else
            memset( lparam_buffer, 0xcc, test->lparam_size );
    }
}

static void test_wndproc_hook(void)
{
    const struct lparam_hook_test *test;
    HHOOK call_hook, call_hook2, ret_hook, ret_hook2;
    WNDCLASSW cls = { 0 };
    LRESULT res;
    HWND hwnd;
    BOOL ret;

    static const BOOL false_lparam = FALSE;
    static const WCHAR strbufW[8] = L"abc\0defg";
    static const WCHAR strbuf2W[8] = L"\0\xcccc\xcccc\xcccc\xcccc\xcccc\xcccc\xcccc";
    static const WCHAR strbuf3W[8] = L"abcdefgh";
    static const WCHAR strbuf4W[8] = L"abc\0\xcccc\xcccc\xcccc\xcccc";
    static const RECT rect_in = { 1, 2, 100, 200 };
    static const RECT rect_out = { 3, 4, 110, 220 };
    static const MINMAXINFO minmax_in = { .ptMinTrackSize.x = 1 };
    static const MINMAXINFO minmax_out = { .ptMinTrackSize.x = 2 };
    static const DRAWITEMSTRUCT drawitem_in = { .itemID = 1 };
    static const MEASUREITEMSTRUCT mis_in = { .itemID = 1 };
    static const MEASUREITEMSTRUCT mis_out = { .itemID = 2, .CtlType = 3, .CtlID = 4, .itemData = 5 };
    static const DELETEITEMSTRUCT dis_in = { .itemID = 1 };
    static const COMPAREITEMSTRUCT cis_in = { .itemID1 = 1 };
    static const WINDOWPOS winpos_in = { .x = 1, .cy = 2 };
    static const WINDOWPOS winpos_out = { .x = 10, .cy = 22 };
    static const COPYDATASTRUCT cds_in = { .dwData = 1 };
    static WORD data_word = 3;
    static const COPYDATASTRUCT cds2_in = { .cbData = 2, .lpData = &data_word };
    static const COPYDATASTRUCT cds3_in = { .dwData = 2, .lpData = (void *)0xdeadbeef };
    static const COPYDATASTRUCT cds4_in = { .cbData = 2 };
    static const COPYDATASTRUCT cds5_in = { .lpData = (void *)0xdeadbeef };
    static const STYLESTRUCT style_in = { .styleOld = 1, .styleNew = 2 };
    static const STYLESTRUCT style_out = { .styleOld = 10, .styleNew = 20 };
    static const MSG msg_in = { .wParam = 1, .lParam = 2 };
    static const SCROLLINFO si_in = { .cbSize = sizeof(si_in), .nPos = 6 };
    static const SCROLLINFO si_out = { .cbSize = sizeof(si_in), .nPos = 60 };
    static const SCROLLBARINFO sbi_in = { .xyThumbTop = 6 };
    static const SCROLLBARINFO sbi_out = { .xyThumbTop = 60 };
    static const DWORD dw_in = 1, dw_out = 2;
    static const UINT32 tabstops_in[2] = { 3, 4 };
    static const UINT32 items_out[2] = { 1, 2 };
    static const MDINEXTMENU nm_in = { .hmenuIn = (HMENU)0xdeadbeef };
    static const MDINEXTMENU nm_out = { .hmenuIn = (HMENU)1 };
    static const MDICREATESTRUCTW mcs_in = { .x = 1, .y = 2 };
    static const COMBOBOXINFO cbi_in = { .cbSize = 1, .hwndList = HWND_MESSAGE };
    static const COMBOBOXINFO cbi_check =
        { .cbSize = sizeof(void *) == 4 ? sizeof(cbi_in) : 1, .hwndList = HWND_MESSAGE };
    static const COMBOBOXINFO cbi_out = { .hwndList = (HWND)2 };
    static const COMBOBOXINFO cbi_ret = { .hwndList = (HWND)2,
        .cbSize = sizeof(void *) == 4 ? sizeof(cbi_in) : 0 };

    static const struct lparam_hook_test lparam_hook_tests[] =
    {
        {
            "WM_NCCALCSIZE", WM_NCCALCSIZE,
            .lparam = &rect_in, .lparam_size = sizeof(RECT), .change_lparam = &rect_out,
            .check_size = sizeof(RECT),
        },
        {
            "WM_MOVING", WM_MOVING,
            .lparam = &rect_in, .lparam_size = sizeof(RECT), .change_lparam = &rect_out,
            .check_size = sizeof(RECT),
        },
        {
            "WM_SIZING", WM_SIZING,
            .lparam = &rect_in, .lparam_size = sizeof(RECT), .change_lparam = &rect_out,
            .check_size = sizeof(RECT),
        },
        {
            "EM_GETRECT", EM_GETRECT,
            .lparam = &rect_in, .lparam_size = sizeof(RECT), .change_lparam = &rect_out,
        },
        {
            "EM_SETRECT", EM_SETRECT,
            .lparam = &rect_in, .lparam_size = sizeof(RECT), .change_lparam = &rect_out,
            .check_size = sizeof(RECT),
        },
        {
            "EM_SETRECTNP", EM_SETRECTNP,
            .lparam = &rect_in, .lparam_size = sizeof(RECT), .change_lparam = &rect_out,
            .check_size = sizeof(RECT),
        },
        {
            "LB_GETITEMRECT", LB_GETITEMRECT,
            .lparam = &rect_in, .lparam_size = sizeof(RECT), .change_lparam = &rect_out,
            .check_size = sizeof(RECT),
        },
        {
            "CB_GETDROPPEDCONTROLRECT", CB_GETDROPPEDCONTROLRECT,
            .lparam = &rect_in, .lparam_size = sizeof(RECT), .change_lparam = &rect_out,
        },
        {
            "WM_GETTEXT", WM_GETTEXT, .wparam = 8,
            .lparam_size = sizeof(strbufW), .change_lparam = strbufW, .check_lparam = strbuf2W,
        },
        {
            "WM_GETTEXT2", WM_GETTEXT, .wparam = 8, .msg_result = 1,
            .lparam_size = sizeof(strbufW), .change_lparam = strbufW, .check_lparam = strbuf4W,
        },
        {
            "WM_GETTEXT3", WM_GETTEXT, .wparam = 8, .msg_result = 9,
            .lparam_size = sizeof(strbufW), .change_lparam = strbufW, .check_lparam = strbuf4W,
        },
        {
            "WM_ASKCBFORMATNAME", WM_ASKCBFORMATNAME, .wparam = 8,
            .lparam_size = sizeof(strbufW), .change_lparam = strbufW, .check_lparam = strbuf4W,
        },
        {
            "WM_ASKCBFORMATNAME2", WM_ASKCBFORMATNAME, .wparam = 8, .msg_result = 1,
            .lparam_size = sizeof(strbufW), .change_lparam = strbufW, .check_lparam = strbuf4W,
        },
        {
            "WM_ASKCBFORMATNAME3", WM_ASKCBFORMATNAME, .wparam = 8, .msg_result = 9,
            .lparam_size = sizeof(strbufW), .change_lparam = strbufW, .check_lparam = strbuf4W,
        },
        {
            "CB_GETLBTEXT", CB_GETLBTEXT, .msg_result = 7, .check_result = 4, .todo_result = TRUE,
            .lparam_size = sizeof(strbufW), .change_lparam = strbufW, .check_lparam = strbuf4W,
        },
        {
            "CB_GETLBTEXT2", CB_GETLBTEXT, .msg_result = 9, .check_result = 8, .todo_result = TRUE,
            .lparam_size = sizeof(strbufW), .change_lparam = strbuf3W, .check_lparam = strbuf3W,
        },
        {
            "CB_GETLBTEXT3", CB_GETLBTEXT,
            .lparam_size = sizeof(strbufW), .change_lparam = strbuf3W, .check_lparam = strbuf3W,
        },
        {
            "LB_GETTEXT", LB_GETTEXT, .msg_result = 7, .check_result = 4, .todo_result = TRUE,
            .lparam_size = sizeof(strbufW), .change_lparam = strbufW, .check_lparam = strbuf4W,
        },
        {
            "LB_GETTEXT2", LB_GETTEXT, .msg_result = 9, .check_result = 8, .todo_result = TRUE,
            .lparam_size = sizeof(strbufW), .change_lparam = strbuf3W, .check_lparam = strbuf3W,
        },
        {
            "LB_GETTEXT3", LB_GETTEXT,
            .lparam_size = sizeof(strbufW), .change_lparam = strbuf3W, .check_lparam = strbuf3W,
        },
        {
            "WM_MDIGETACTIVE", WM_MDIGETACTIVE, .no_wparam_check = TRUE,
            .lparam_size = sizeof(BOOL), .change_lparam = &false_lparam,
        },
        {
            "WM_GETMINMAXINFO", WM_GETMINMAXINFO,
            .lparam_size = sizeof(minmax_in), .lparam = &minmax_in, .change_lparam = &minmax_out,
            .check_size = sizeof(minmax_in)
        },
        {
            "WM_MEASUREITEM", WM_MEASUREITEM, .wparam = 10,
            .lparam_size = sizeof(mis_in), .lparam = &mis_in, .change_lparam = &mis_out,
            .check_size = sizeof(mis_in),
        },
        {
            "WM_DELETEITEM", WM_DELETEITEM, .wparam = 10,
            .lparam_size = sizeof(dis_in), .lparam = &dis_in, .poison_lparam = TRUE,
            .check_size = sizeof(dis_in),
        },
        {
            "WM_COMPAREITEM", WM_COMPAREITEM, .wparam = 10,
            .lparam_size = sizeof(cis_in), .lparam = &cis_in, .poison_lparam = TRUE,
            .check_size = sizeof(cis_in),
        },
        {
            "WM_WINDOWPOSCHANGING", WM_WINDOWPOSCHANGING,
            .lparam_size = sizeof(WINDOWPOS), .lparam = &winpos_in, .change_lparam = &winpos_out,
            .check_size = sizeof(WINDOWPOS)
        },
        {
            "WM_WINDOWPOSCHANGED", WM_WINDOWPOSCHANGED,
            .lparam_size = sizeof(WINDOWPOS), .lparam = &winpos_in, .poison_lparam = TRUE,
            .check_size = sizeof(WINDOWPOS),
        },
        {
            "WM_COPYDATA", WM_COPYDATA, .wparam = 0xdeadbeef,
            .lparam_size = sizeof(cds_in), .lparam = &cds_in, .poison_lparam = TRUE,
            .check_size = sizeof(cds_in),
        },
        {
            "WM_COPYDATA-2", WM_COPYDATA, .wparam = 0xdeadbeef,
            .lparam_size = sizeof(cds2_in), .lparam = &cds2_in, .poison_lparam = TRUE,
            .check_size = sizeof(cds2_in),
        },
        {
            "WM_COPYDATA-3", WM_COPYDATA, .wparam = 0xdeadbeef,
            .lparam_size = sizeof(cds3_in), .lparam = &cds3_in, .poison_lparam = TRUE,
            .check_size = sizeof(cds3_in),
        },
        {
            "WM_COPYDATA-4", WM_COPYDATA, .wparam = 0xdeadbeef,
            .lparam_size = sizeof(cds4_in), .lparam = &cds4_in, .poison_lparam = TRUE,
            .check_size = sizeof(cds4_in),
        },
        {
            "WM_COPYDATA-5", WM_COPYDATA, .wparam = 0xdeadbeef,
            .lparam_size = sizeof(cds5_in), .lparam = &cds5_in, .poison_lparam = TRUE,
            .check_size = sizeof(cds5_in),
        },
        {
            "WM_STYLECHANGING", WM_STYLECHANGING,
            .lparam_size = sizeof(style_in), .lparam = &style_in, .change_lparam = &style_out,
            .check_size = sizeof(style_in)
        },
        {
            "WM_STYLECHANGED", WM_STYLECHANGED,
            .lparam_size = sizeof(style_in), .lparam = &style_in, .poison_lparam = TRUE,
            .check_size = sizeof(style_in),
        },
        {
            "WM_GETDLGCODE", WM_GETDLGCODE,
            .lparam_size = sizeof(msg_in), .lparam = &msg_in, .poison_lparam = TRUE,
            .check_size = sizeof(msg_in),
        },
        {
            "SBM_SETSCROLLINFO", SBM_SETSCROLLINFO,
            .lparam_size = sizeof(si_in), .lparam = &si_in, .change_lparam = &si_out,
            .check_size = sizeof(si_in),
        },
        {
            "SBM_GETSCROLLINFO", SBM_GETSCROLLINFO,
            .lparam_size = sizeof(si_in), .lparam = &si_in, .change_lparam = &si_out,
            .check_size = sizeof(si_in),
        },
        {
            "SBM_GETSCROLLBARINFO", SBM_GETSCROLLBARINFO,
            .lparam_size = sizeof(sbi_in), .lparam = &sbi_in, .change_lparam = &sbi_out,
            .check_size = sizeof(sbi_in),
        },
        {
            "EM_GETSEL", EM_GETSEL, .no_wparam_check = TRUE,
            .lparam_size = sizeof(DWORD), .lparam = &dw_in, .change_lparam = &dw_out,
            .check_size = sizeof(DWORD),
        },
        {
            "SBM_GETRANGE", SBM_GETRANGE, .no_wparam_check = TRUE,
            .lparam_size = sizeof(DWORD), .lparam = &dw_in, .change_lparam = &dw_out,
            .check_size = sizeof(DWORD),
        },
        {
            "CB_GETEDITSEL", CB_GETEDITSEL, .no_wparam_check = TRUE,
            .lparam_size = sizeof(DWORD), .lparam = &dw_in, .change_lparam = &dw_out,
            .check_size = sizeof(DWORD),
        },
        {
            "EM_GETLINE", EM_GETLINE, .msg_result = 5,
            .lparam = L"\x8""2345678", .lparam_size = sizeof(strbufW), .change_lparam = L"abc\0defg",
            .check_size = sizeof(WCHAR), .check_lparam = L"abc\0""5678",
        },
        {
            "EM_GETLINE-2", EM_GETLINE, .msg_result = 1,
            .lparam = L"\x8""2345678", .lparam_size = sizeof(strbufW), .change_lparam = L"abc\0defg",
            .check_size = sizeof(WCHAR), .check_lparam = L"abc\0""5678",
        },
        {
            "EM_SETTABSTOPS", EM_SETTABSTOPS, .wparam = ARRAYSIZE(tabstops_in),
            .lparam_size = sizeof(tabstops_in), .lparam = &tabstops_in, .poison_lparam = TRUE,
            .check_size = sizeof(tabstops_in),
        },
        {
            "LB_SETTABSTOPS", LB_SETTABSTOPS, .wparam = ARRAYSIZE(tabstops_in),
            .lparam_size = sizeof(tabstops_in), .lparam = &tabstops_in, .poison_lparam = TRUE,
            .check_size = sizeof(tabstops_in),
        },
        {
            "LB_GETSELITEMS", LB_GETSELITEMS,
            .wparam = ARRAYSIZE(items_out), .msg_result = ARRAYSIZE(items_out),
            .lparam_size = sizeof(items_out), .change_lparam = items_out,
        },
        {
            "WM_NEXTMENU", WM_NEXTMENU,
            .lparam_size = sizeof(nm_in), .lparam = &nm_in, .change_lparam = &nm_out,
            .check_size = sizeof(nm_in)
        },
        {
            "WM_MDICREATE", WM_MDICREATE,
            .lparam_size = sizeof(mcs_in), .lparam = &mcs_in, .poison_lparam = TRUE,
            .check_size = sizeof(mcs_in),
        },
        {
            "CB_GETCOMBOBOXINFO", CB_GETCOMBOBOXINFO,
            .lparam_size = sizeof(cbi_in), .change_lparam = &cbi_out, .lparam = &cbi_in,
            .check_lparam = &cbi_ret, .check_size = sizeof(cbi_in), .in_lparam = &cbi_check,
        },
        /* messages that don't change lparam */
        { "WM_USER", WM_USER },
        { "WM_NOTIFY", WM_NOTIFY },
        { "WM_SETTEXT", WM_SETTEXT, .lparam = strbufW, .lparam_init_size = sizeof(strbufW) },
        { "WM_DEVMODECHANGE", WM_DEVMODECHANGE, .lparam = strbufW, .lparam_init_size = sizeof(strbufW) },
        { "CB_DIR", CB_DIR, .lparam = strbufW, .lparam_init_size = sizeof(strbufW) },
        { "LB_DIR", LB_DIR, .lparam = strbufW, .lparam_init_size = sizeof(strbufW) },
        { "LB_ADDFILE", LB_ADDFILE, .lparam = strbufW, .lparam_init_size = sizeof(strbufW) },
        { "EM_REPLACESEL", EM_REPLACESEL, .lparam = strbufW, .lparam_init_size = sizeof(strbufW) },
        { "WM_WININICHANGE", WM_WININICHANGE, .lparam = strbufW, .lparam_init_size = sizeof(strbufW) },
        { "CB_ADDSTRING", CB_ADDSTRING },
        { "CB_INSERTSTRING", CB_INSERTSTRING },
        { "LB_ADDSTRING", LB_ADDSTRING },
        /* messages not allowed to be sent by NtUserMessageCall */
        {
            "WM_DRAWITEM", WM_DRAWITEM, .wparam = 10,
            .lparam_size = sizeof(drawitem_in), .lparam = &drawitem_in, .not_allowed = TRUE,
        },
    };

    cls.lpfnWndProc = lparam_test_proc;
    cls.lpszClassName = L"TestLParamClass";
    RegisterClassW( &cls );

    hwnd = CreateWindowExA( 0, "TestLParamClass", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL );

    for (test = lparam_hook_tests; test < lparam_hook_tests + ARRAYSIZE(lparam_hook_tests); test++)
    {
        current_hook_test = test;
        winetest_push_context("%s", test->name);

        /* Simple unhooked SendMessage just passes unmodified lparam. */
        winetest_push_context( "sendmsg" );
        init_hook_test( test );
        res = SendMessageW( hwnd, test->message, test->wparam, (LPARAM)lparam_buffer );
        ok( res == test->msg_result, "NtUserMessageCall returned %Ix\n", res );
        ok( wndproc_lparam == (LPARAM)lparam_buffer, "unexpected wndproc_lparam %Ix, expected %p\n",
            wndproc_lparam, lparam_buffer );
        winetest_pop_context();

        /* NtUserMessageCall uses a copy of lparam even when not hooked. */
        wndproc_lparam = 0;
        winetest_push_context( "ntsendmsg" );
        init_hook_test( test );
        res = NtUserMessageCall( hwnd, test->message, test->wparam, (LPARAM)lparam_buffer, NULL,
                                 NtUserSendMessage, FALSE );
        test_msg_output( test, res, FALSE );
        winetest_pop_context();

        call_hook2 = SetWindowsHookExW( WH_CALLWNDPROC, callwnd_hook_proc2, NULL, GetCurrentThreadId() );
        ok( call_hook2 != NULL, "SetWindowsHookExW failed\n");
        call_hook = SetWindowsHookExW( WH_CALLWNDPROC, callwnd_hook_proc, NULL, GetCurrentThreadId() );
        ok( call_hook != NULL, "SetWindowsHookExW failed\n");
        ret_hook2 = SetWindowsHookExW( WH_CALLWNDPROCRET, retwnd_hook_proc2,
                                       NULL, GetCurrentThreadId() );
        ok( ret_hook2 != NULL, "SetWindowsHookExW failed\n");
        ret_hook = SetWindowsHookExW( WH_CALLWNDPROCRET, retwnd_hook_proc,
                                      NULL, GetCurrentThreadId() );
        ok( ret_hook != NULL, "SetWindowsHookExW failed\n");

        /* Hooked SendMessage behaves just like NtUserMessageCall. */
        winetest_push_context( "hooked_sendmsg" );
        init_hook_test( test );
        res = SendMessageW( hwnd, test->message, test->wparam, (LPARAM)lparam_buffer );
        test_msg_output( test, res, TRUE );
        winetest_pop_context();

        winetest_push_context( "hooked_ntsendmsg" );
        init_hook_test( test );
        res = NtUserMessageCall( hwnd, test->message, test->wparam, (LPARAM)lparam_buffer,
                                 NULL, NtUserSendMessage, FALSE );
        test_msg_output( test, res, TRUE );
        winetest_pop_context();

        ret = NtUserUnhookWindowsHookEx( call_hook );
        ok( ret, "NtUserUnhookWindowsHook failed: %lu\n", GetLastError() );
        ret = NtUserUnhookWindowsHookEx( call_hook2 );
        ok( ret, "NtUserUnhookWindowsHook failed: %lu\n", GetLastError() );
        ret = NtUserUnhookWindowsHookEx( ret_hook );
        ok( ret, "NtUserUnhookWindowsHook failed: %lu\n", GetLastError() );
        ret = NtUserUnhookWindowsHookEx( ret_hook2 );
        ok( ret, "NtUserUnhookWindowsHook failed: %lu\n", GetLastError() );

        winetest_pop_context();
    }

    DestroyWindow( hwnd );
    UnregisterClassW( L"TestLParamClass", NULL );
}

START_TEST(win32u)
{
    char **argv;
    int argc;

    /* native win32u.dll fails if user32 is not loaded, so make sure it's fully initialized */
    GetDesktopWindow();

    argc = winetest_get_mainargs( &argv );
    if (argc > 3 && !strcmp( argv[2], "ipcmsg" ))
    {
        test_inter_process_child( LongToHandle( strtol( argv[3], NULL, 16 )));
        return;
    }

    test_NtUserEnumDisplayDevices();
    test_window_props();
    test_class();
    test_NtUserBuildHwndList();
    test_cursoricon();
    test_message_call();
    test_window_text();
    test_menu();
    test_message_filter();
    test_timer();
    test_inter_process_messages( argv[0] );
    test_wndproc_hook();

    test_NtUserCloseWindowStation();
    test_NtUserDisplayConfigGetDeviceInfo();
}
