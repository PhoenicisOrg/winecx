/*
 * USER private definitions
 *
 * Copyright 1993 Alexandre Julliard
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

#ifndef __WINE_USER_PRIVATE_H
#define __WINE_USER_PRIVATE_H

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "../win32u/ntuser_private.h"
#include "winreg.h"
#include "winternl.h"
#include "hidusage.h"
#include "wine/heap.h"

#define GET_WORD(ptr)  (*(const WORD *)(ptr))
#define GET_DWORD(ptr) (*(const DWORD *)(ptr))
#define GET_LONG(ptr) (*(const LONG *)(ptr))

#define WINE_MOUSE_HANDLE       ((HANDLE)1)
#define WINE_KEYBOARD_HANDLE    ((HANDLE)2)

struct received_message_info;

/* data to store state for A/W mappings of WM_CHAR */
struct wm_char_mapping_data
{
    BYTE lead_byte[WMCHAR_MAP_COUNT];
    MSG  get_msg;
};

/* on windows the buffer capacity is quite large as well, enough to */
/* hold up to 10s of 1kHz mouse rawinput events */
#define RAWINPUT_BUFFER_SIZE (512*1024)

struct rawinput_thread_data
{
    UINT     hw_id;     /* current rawinput message id */
    RAWINPUT buffer[1]; /* rawinput message data buffer */
};

extern BOOL (WINAPI *imm_register_window)(HWND) DECLSPEC_HIDDEN;
extern void (WINAPI *imm_unregister_window)(HWND) DECLSPEC_HIDDEN;

static inline struct user_thread_info *get_user_thread_info(void)
{
    return (struct user_thread_info *)NtCurrentTeb()->Win32ClientInfo;
}

extern HMODULE user32_module DECLSPEC_HIDDEN;

struct dce;
struct tagWND;

struct hardware_msg_data;
extern BOOL rawinput_from_hardware_message(RAWINPUT *rawinput, const struct hardware_msg_data *msg_data);
extern BOOL rawinput_device_get_usages(HANDLE handle, USAGE *usage_page, USAGE *usage);
extern struct rawinput_thread_data *rawinput_thread_data(void);
extern void rawinput_update_device_list(void);

extern BOOL post_dde_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, DWORD dest_tid,
                              DWORD type ) DECLSPEC_HIDDEN;
extern BOOL process_rawinput_message( MSG *msg, UINT hw_id,
                                      const struct hardware_msg_data *msg_data ) DECLSPEC_HIDDEN;
extern BOOL unpack_dde_message( HWND hwnd, UINT message, WPARAM *wparam, LPARAM *lparam,
                                void **buffer, size_t size ) DECLSPEC_HIDDEN;
extern void free_cached_data( UINT format, HANDLE handle ) DECLSPEC_HIDDEN;
extern HANDLE render_synthesized_format( UINT format, UINT from ) DECLSPEC_HIDDEN;

extern void CLIPBOARD_ReleaseOwner( HWND hwnd ) DECLSPEC_HIDDEN;
extern BOOL FOCUS_MouseActivate( HWND hwnd ) DECLSPEC_HIDDEN;
extern BOOL set_capture_window( HWND hwnd, UINT gui_flags, HWND *prev_ret ) DECLSPEC_HIDDEN;
extern HDC get_display_dc(void) DECLSPEC_HIDDEN;
extern void release_display_dc( HDC hdc ) DECLSPEC_HIDDEN;
extern void wait_graphics_driver_ready(void) DECLSPEC_HIDDEN;
extern void *get_hook_proc( void *proc, const WCHAR *module, HMODULE *free_module ) DECLSPEC_HIDDEN;
extern RECT get_virtual_screen_rect(void) DECLSPEC_HIDDEN;
extern RECT get_primary_monitor_rect(void) DECLSPEC_HIDDEN;
extern DWORD get_input_codepage( void ) DECLSPEC_HIDDEN;
extern BOOL map_wparam_AtoW( UINT message, WPARAM *wparam, enum wm_char_mapping mapping ) DECLSPEC_HIDDEN;
extern HPEN SYSCOLOR_GetPen( INT index ) DECLSPEC_HIDDEN;
extern HBRUSH SYSCOLOR_Get55AABrush(void) DECLSPEC_HIDDEN;
extern void SYSPARAMS_Init(void) DECLSPEC_HIDDEN;
extern void USER_CheckNotLock(void) DECLSPEC_HIDDEN;
extern BOOL USER_IsExitingThread( DWORD tid ) DECLSPEC_HIDDEN;
extern void CDECL toggle_caret( HWND hwnd ) DECLSPEC_HIDDEN;
extern void CDECL update_mouse_tracking_info( HWND hwnd ) DECLSPEC_HIDDEN;

typedef LRESULT (*winproc_callback_t)( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                       LRESULT *result, void *arg );

extern LRESULT WINPROC_CallProcAtoW( winproc_callback_t callback, HWND hwnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam, LRESULT *result, void *arg,
                                     enum wm_char_mapping mapping ) DECLSPEC_HIDDEN;

extern INT_PTR WINPROC_CallDlgProcA( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern INT_PTR WINPROC_CallDlgProcW( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern void winproc_init(void) DECLSPEC_HIDDEN;
extern void get_winproc_params( struct win_proc_params *params ) DECLSPEC_HIDDEN;

extern ATOM get_class_info( HINSTANCE instance, const WCHAR *name, WNDCLASSEXW *info,
                            UNICODE_STRING *name_str, BOOL ansi ) DECLSPEC_HIDDEN;

/* kernel callbacks */

BOOL WINAPI User32CallEnumDisplayMonitor( struct enum_display_monitor_params *params, ULONG size );
BOOL WINAPI User32CallSendAsyncCallback( const struct send_async_params *params, ULONG size );
BOOL WINAPI User32CallWinEventHook( const struct win_event_hook_params *params, ULONG size );
BOOL WINAPI User32CallWindowProc( struct win_proc_params *params, ULONG size );
BOOL WINAPI User32CallWindowsHook( const struct win_hook_params *params, ULONG size );

/* message spy definitions */

extern const char *SPY_GetMsgName( UINT msg, HWND hWnd ) DECLSPEC_HIDDEN;
extern const char *SPY_GetVKeyName(WPARAM wParam) DECLSPEC_HIDDEN;
extern void SPY_EnterMessage( INT iFlag, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern void SPY_ExitMessage( INT iFlag, HWND hwnd, UINT msg,
                             LRESULT lReturn, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;

#include "pshpack1.h"

typedef struct
{
    BYTE   bWidth;
    BYTE   bHeight;
    BYTE   bColorCount;
    BYTE   bReserved;
} ICONRESDIR;

typedef struct
{
    WORD   wWidth;
    WORD   wHeight;
} CURSORDIR;

typedef struct
{   union
    { ICONRESDIR icon;
      CURSORDIR  cursor;
    } ResInfo;
    WORD   wPlanes;
    WORD   wBitCount;
    DWORD  dwBytesInRes;
    WORD   wResId;
} CURSORICONDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONDIRENTRY  idEntries[1];
} CURSORICONDIR;

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} CURSORICONFILEDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONFILEDIRENTRY  idEntries[1];
} CURSORICONFILEDIR;

#include "poppack.h"

extern int bitmap_info_size( const BITMAPINFO * info, WORD coloruse ) DECLSPEC_HIDDEN;
extern BOOL get_icon_size( HICON handle, SIZE *size ) DECLSPEC_HIDDEN;

extern BOOL MENU_send_window_menubar_to_macapp( HWND hwnd );

extern struct user_api_hook *user_api DECLSPEC_HIDDEN;
LRESULT WINAPI USER_DefDlgProc(HWND, UINT, WPARAM, LPARAM, BOOL) DECLSPEC_HIDDEN;
LRESULT WINAPI USER_ScrollBarProc(HWND, UINT, WPARAM, LPARAM, BOOL) DECLSPEC_HIDDEN;
void WINAPI USER_ScrollBarDraw(HWND, HDC, INT, enum SCROLL_HITTEST,
                               const struct SCROLL_TRACKING_INFO *, BOOL, BOOL, RECT *, INT, INT,
                               INT, BOOL) DECLSPEC_HIDDEN;
void WINAPI SCROLL_SetStandardScrollPainted(HWND hwnd, INT bar, BOOL visible);

/*  CrossOver Hack for bug 6727. */
struct xml_buffer
{
    int len, alloc;
    char *data;
};

#endif /* __WINE_USER_PRIVATE_H */
