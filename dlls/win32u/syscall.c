/*
 * Unix interface for Win32 syscalls
 *
 * Copyright (C) 2021 Alexandre Julliard
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "ntuser.h"
#include "wine/unixlib.h"

ULONG_PTR zero_bits = 0;

static void * const syscalls[] =
{
    NtGdiAbortDoc,
    NtGdiAbortPath,
    NtGdiAddFontMemResourceEx,
    NtGdiAddFontResourceW,
    NtGdiAlphaBlend,
    NtGdiAngleArc,
    NtGdiArcInternal,
    NtGdiBeginPath,
    NtGdiBitBlt,
    NtGdiCloseFigure,
    NtGdiCombineRgn,
    NtGdiComputeXformCoefficients,
    NtGdiCreateBitmap,
    NtGdiCreateClientObj,
    NtGdiCreateCompatibleBitmap,
    NtGdiCreateCompatibleDC,
    NtGdiCreateDIBBrush,
    NtGdiCreateDIBSection,
    NtGdiCreateDIBitmapInternal,
    NtGdiCreateEllipticRgn,
    NtGdiCreateHalftonePalette,
    NtGdiCreateHatchBrushInternal,
    NtGdiCreateMetafileDC,
    NtGdiCreatePaletteInternal,
    NtGdiCreatePatternBrushInternal,
    NtGdiCreatePen,
    NtGdiCreateRectRgn,
    NtGdiCreateRoundRectRgn,
    NtGdiCreateSolidBrush,
    NtGdiDdDDICheckVidPnExclusiveOwnership,
    NtGdiDdDDICloseAdapter,
    NtGdiDdDDICreateDCFromMemory,
    NtGdiDdDDICreateDevice,
    NtGdiDdDDIDestroyDCFromMemory,
    NtGdiDdDDIDestroyDevice,
    NtGdiDdDDIEscape,
    NtGdiDdDDIOpenAdapterFromDeviceName,
    NtGdiDdDDIOpenAdapterFromHdc,
    NtGdiDdDDIOpenAdapterFromLuid,
    NtGdiDdDDIQueryStatistics,
    NtGdiDdDDIQueryVideoMemoryInfo,
    NtGdiDdDDISetQueuedLimit,
    NtGdiDdDDISetVidPnSourceOwner,
    NtGdiDeleteClientObj,
    NtGdiDeleteObjectApp,
    NtGdiDescribePixelFormat,
    NtGdiDoPalette,
    NtGdiDrawStream,
    NtGdiEllipse,
    NtGdiEndDoc,
    NtGdiEndPage,
    NtGdiEndPath,
    NtGdiEnumFonts,
    NtGdiEqualRgn,
    NtGdiExcludeClipRect,
    NtGdiExtCreatePen,
    NtGdiExtCreateRegion,
    NtGdiExtEscape,
    NtGdiExtFloodFill,
    NtGdiExtGetObjectW,
    NtGdiExtSelectClipRgn,
    NtGdiExtTextOutW,
    NtGdiFillPath,
    NtGdiFillRgn,
    NtGdiFlattenPath,
    NtGdiFlush,
    NtGdiFontIsLinked,
    NtGdiFrameRgn,
    NtGdiGetAndSetDCDword,
    NtGdiGetAppClipBox,
    NtGdiGetBitmapBits,
    NtGdiGetBitmapDimension,
    NtGdiGetBoundsRect,
    NtGdiGetCharABCWidthsW,
    NtGdiGetCharWidthInfo,
    NtGdiGetCharWidthW,
    NtGdiGetColorAdjustment,
    NtGdiGetDCDword,
    NtGdiGetDCObject,
    NtGdiGetDCPoint,
    NtGdiGetDIBitsInternal,
    NtGdiGetDeviceCaps,
    NtGdiGetDeviceGammaRamp,
    NtGdiGetFontData,
    NtGdiGetFontFileData,
    NtGdiGetFontFileInfo,
    NtGdiGetFontUnicodeRanges,
    NtGdiGetGlyphIndicesW,
    NtGdiGetGlyphOutline,
    NtGdiGetKerningPairs,
    NtGdiGetNearestColor,
    NtGdiGetNearestPaletteIndex,
    NtGdiGetOutlineTextMetricsInternalW,
    NtGdiGetPath,
    NtGdiGetPixel,
    NtGdiGetRandomRgn,
    NtGdiGetRasterizerCaps,
    NtGdiGetRealizationInfo,
    NtGdiGetRegionData,
    NtGdiGetRgnBox,
    NtGdiGetSpoolMessage,
    NtGdiGetSystemPaletteUse,
    NtGdiGetTextCharsetInfo,
    NtGdiGetTextExtentExW,
    NtGdiGetTextFaceW,
    NtGdiGetTextMetricsW,
    NtGdiGetTransform,
    NtGdiGradientFill,
    NtGdiHfontCreate,
    NtGdiInitSpool,
    NtGdiIntersectClipRect,
    NtGdiInvertRgn,
    NtGdiLineTo,
    NtGdiMaskBlt,
    NtGdiModifyWorldTransform,
    NtGdiMoveTo,
    NtGdiOffsetClipRgn,
    NtGdiOffsetRgn,
    NtGdiOpenDCW,
    NtGdiPatBlt,
    NtGdiPathToRegion,
    NtGdiPlgBlt,
    NtGdiPolyDraw,
    NtGdiPolyPolyDraw,
    NtGdiPtInRegion,
    NtGdiPtVisible,
    NtGdiRectInRegion,
    NtGdiRectVisible,
    NtGdiRectangle,
    NtGdiRemoveFontMemResourceEx,
    NtGdiRemoveFontResourceW,
    NtGdiResetDC,
    NtGdiResizePalette,
    NtGdiRestoreDC,
    NtGdiRoundRect,
    NtGdiSaveDC,
    NtGdiScaleViewportExtEx,
    NtGdiScaleWindowExtEx,
    NtGdiSelectBitmap,
    NtGdiSelectBrush,
    NtGdiSelectClipPath,
    NtGdiSelectFont,
    NtGdiSelectPen,
    NtGdiSetBitmapBits,
    NtGdiSetBitmapDimension,
    NtGdiSetBoundsRect,
    NtGdiSetBrushOrg,
    NtGdiSetColorAdjustment,
    SetDIBits,
    NtGdiSetDIBitsToDeviceInternal,
    NtGdiSetDeviceGammaRamp,
    NtGdiSetLayout,
    NtGdiSetMagicColors,
    NtGdiSetMetaRgn,
    NtGdiSetPixel,
    NtGdiSetPixelFormat,
    NtGdiSetRectRgn,
    NtGdiSetSystemPaletteUse,
    NtGdiSetTextJustification,
    NtGdiSetVirtualResolution,
    NtGdiStartDoc,
    NtGdiStartPage,
    NtGdiStretchBlt,
    NtGdiStretchDIBitsInternal,
    NtGdiStrokeAndFillPath,
    NtGdiStrokePath,
    NtGdiSwapBuffers,
    NtGdiTransformPoints,
    NtGdiTransparentBlt,
    NtGdiUnrealizeObject,
    NtGdiUpdateColors,
    NtGdiWidenPath,
    NtUserActivateKeyboardLayout,
    NtUserAddClipboardFormatListener,
    NtUserAssociateInputContext,
    NtUserAttachThreadInput,
    NtUserBeginPaint,
    NtUserBuildHwndList,
    NtUserCallHwnd,
    NtUserCallHwndParam,
    NtUserCallMsgFilter,
    NtUserCallNextHookEx,
    NtUserCallNoParam,
    NtUserCallOneParam,
    NtUserCallTwoParam,
    NtUserChangeClipboardChain,
    NtUserChangeDisplaySettings,
    NtUserCheckMenuItem,
    NtUserChildWindowFromPointEx,
    NtUserClipCursor,
    NtUserCloseClipboard,
    NtUserCloseDesktop,
    NtUserCloseWindowStation,
    NtUserCopyAcceleratorTable,
    NtUserCountClipboardFormats,
    NtUserCreateAcceleratorTable,
    NtUserCreateCaret,
    NtUserCreateDesktopEx,
    NtUserCreateInputContext,
    NtUserCreateWindowEx,
    NtUserCreateWindowStation,
    NtUserDeferWindowPosAndBand,
    NtUserDeleteMenu,
    NtUserDestroyAcceleratorTable,
    NtUserDestroyCursor,
    NtUserDestroyInputContext,
    NtUserDestroyMenu,
    NtUserDestroyWindow,
    NtUserDisableThreadIme,
    NtUserDispatchMessage,
    NtUserDisplayConfigGetDeviceInfo,
    NtUserDragDetect,
    NtUserDragObject,
    NtUserDrawCaptionTemp,
    NtUserDrawIconEx,
    NtUserDrawMenuBarTemp,
    NtUserEmptyClipboard,
    NtUserEnableMenuItem,
    NtUserEnableScrollBar,
    NtUserEndDeferWindowPosEx,
    NtUserEndMenu,
    NtUserEndPaint,
    NtUserEnumDisplayDevices,
    NtUserEnumDisplayMonitors,
    NtUserEnumDisplaySettings,
    NtUserExcludeUpdateRgn,
    NtUserFindExistingCursorIcon,
    NtUserFindWindowEx,
    NtUserFlashWindowEx,
    NtUserGetAncestor,
    NtUserGetAsyncKeyState,
    NtUserGetAtomName,
    NtUserGetCaretBlinkTime,
    NtUserGetCaretPos,
    NtUserGetClassInfoEx,
    NtUserGetClassName,
    NtUserGetClipboardData,
    NtUserGetClipboardFormatName,
    NtUserGetClipboardOwner,
    NtUserGetClipboardSequenceNumber,
    NtUserGetClipboardViewer,
    NtUserGetCursor,
    NtUserGetCursorFrameInfo,
    NtUserGetCursorInfo,
    NtUserGetDC,
    NtUserGetDCEx,
    NtUserGetDisplayConfigBufferSizes,
    NtUserGetDoubleClickTime,
    NtUserGetDpiForMonitor,
    NtUserGetForegroundWindow,
    NtUserGetGUIThreadInfo,
    NtUserGetIconInfo,
    NtUserGetIconSize,
    NtUserGetInternalWindowPos,
    NtUserGetKeyNameText,
    NtUserGetKeyState,
    NtUserGetKeyboardLayout,
    NtUserGetKeyboardLayoutList,
    NtUserGetKeyboardLayoutName,
    NtUserGetKeyboardState,
    NtUserGetLayeredWindowAttributes,
    NtUserGetMenuBarInfo,
    NtUserGetMenuItemRect,
    NtUserGetMessage,
    NtUserGetMouseMovePointsEx,
    NtUserGetObjectInformation,
    NtUserGetOpenClipboardWindow,
    NtUserGetPriorityClipboardFormat,
    NtUserGetProcessDpiAwarenessContext,
    NtUserGetProcessWindowStation,
    NtUserGetProp,
    NtUserGetQueueStatus,
    NtUserGetRawInputBuffer,
    NtUserGetRawInputData,
    NtUserGetRawInputDeviceInfo,
    NtUserGetRawInputDeviceList,
    NtUserGetRegisteredRawInputDevices,
    NtUserGetScrollBarInfo,
    NtUserGetSystemDpiForProcess,
    NtUserGetSystemMenu,
    NtUserGetThreadDesktop,
    NtUserGetTitleBarInfo,
    NtUserGetUpdateRect,
    NtUserGetUpdateRgn,
    NtUserGetUpdatedClipboardFormats,
    NtUserGetWindowDC,
    NtUserGetWindowPlacement,
    NtUserGetWindowRgnEx,
    NtUserHideCaret,
    NtUserHiliteMenuItem,
    NtUserInitializeClientPfnArrays,
    NtUserInternalGetWindowIcon,
    NtUserInternalGetWindowText,
    NtUserInvalidateRect,
    NtUserInvalidateRgn,
    NtUserIsClipboardFormatAvailable,
    NtUserKillTimer,
    NtUserLockWindowUpdate,
    NtUserLogicalToPerMonitorDPIPhysicalPoint,
    NtUserMapVirtualKeyEx,
    NtUserMenuItemFromPoint,
    NtUserMessageCall,
    NtUserMoveWindow,
    NtUserMsgWaitForMultipleObjectsEx,
    NtUserNotifyWinEvent,
    NtUserOpenClipboard,
    NtUserOpenDesktop,
    NtUserOpenInputDesktop,
    NtUserOpenWindowStation,
    NtUserPeekMessage,
    NtUserPerMonitorDPIPhysicalToLogicalPoint,
    NtUserPostMessage,
    NtUserPostThreadMessage,
    NtUserPrintWindow,
    NtUserQueryInputContext,
    NtUserRealChildWindowFromPoint,
    NtUserRedrawWindow,
    NtUserRegisterClassExWOW,
    NtUserRegisterHotKey,
    NtUserRegisterRawInputDevices,
    NtUserReleaseDC,
    NtUserRemoveClipboardFormatListener,
    NtUserRemoveMenu,
    NtUserRemoveProp,
    NtUserScrollDC,
    NtUserScrollWindowEx,
    NtUserSelectPalette,
    NtUserSendInput,
    NtUserSetActiveWindow,
    NtUserSetCapture,
    NtUserSetClassLong,
    NtUserSetClassLongPtr,
    NtUserSetClassWord,
    NtUserSetClipboardData,
    NtUserSetClipboardViewer,
    NtUserSetCursor,
    NtUserSetCursorIconData,
    NtUserSetCursorPos,
    NtUserSetFocus,
    NtUserSetInternalWindowPos,
    NtUserSetKeyboardState,
    NtUserSetLayeredWindowAttributes,
    NtUserSetMenu,
    NtUserSetMenuContextHelpId,
    NtUserSetMenuDefaultItem,
    NtUserSetObjectInformation,
    NtUserSetParent,
    NtUserSetProcessDpiAwarenessContext,
    NtUserSetProcessWindowStation,
    NtUserSetProp,
    NtUserSetScrollInfo,
    NtUserSetShellWindowEx,
    NtUserSetSysColors,
    NtUserSetSystemMenu,
    NtUserSetSystemTimer,
    NtUserSetThreadDesktop,
    NtUserSetTimer,
    NtUserSetWinEventHook,
    NtUserSetWindowLong,
    NtUserSetWindowLongPtr,
    NtUserSetWindowPlacement,
    NtUserSetWindowPos,
    NtUserSetWindowRgn,
    NtUserSetWindowWord,
    NtUserSetWindowsHookEx,
    NtUserShowCaret,
    NtUserShowCursor,
    NtUserShowScrollBar,
    NtUserShowWindow,
    NtUserShowWindowAsync,
    NtUserSystemParametersInfo,
    NtUserSystemParametersInfoForDpi,
    NtUserThunkedMenuInfo,
    NtUserThunkedMenuItemInfo,
    NtUserToUnicodeEx,
    NtUserTrackMouseEvent,
    NtUserTrackPopupMenuEx,
    NtUserTranslateAccelerator,
    NtUserTranslateMessage,
    NtUserUnhookWinEvent,
    NtUserUnhookWindowsHookEx,
    NtUserUnregisterClass,
    NtUserUnregisterHotKey,
    NtUserUpdateInputContext,
    NtUserUpdateLayeredWindow,
    NtUserValidateRect,
    NtUserVkKeyScanEx,
    NtUserWaitForInputIdle,
    NtUserWaitMessage,
    NtUserWindowFromDC,
    NtUserWindowFromPoint,
    __wine_get_brush_bitmap_info,
    __wine_get_current_process_explicit_app_user_model_id,
    __wine_get_file_outline_text_metric,
    __wine_get_icm_profile,
    __wine_send_input,
    __wine_set_current_process_explicit_app_user_model_id,
};

static BYTE arguments[ARRAY_SIZE(syscalls)];

static SYSTEM_SERVICE_TABLE syscall_table =
{
    (ULONG_PTR *)syscalls,
    0,
    ARRAY_SIZE(syscalls),
    arguments
};

static NTSTATUS init( void *dispatcher )
{
#ifdef _WIN64
    if (NtCurrentTeb()->WowTebOffset)
    {
        SYSTEM_BASIC_INFORMATION info;

        NtQuerySystemInformation(SystemEmulationBasicInformation, &info, sizeof(info), NULL);
        zero_bits = (ULONG_PTR)info.HighestUserAddress | 0x7fffffff;
    }
#endif

    return ntdll_init_syscalls( 1, &syscall_table, dispatcher );
}

unixlib_entry_t __wine_unix_call_funcs[] =
{
    init,
};

#ifdef _WIN64

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    init,
};

#endif /* _WIN64 */