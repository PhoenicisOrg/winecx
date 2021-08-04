/*
 * Copyright (C) 2000 Alexandre Julliard
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

#include "wine/winheader_enter.h"

#ifndef GUID_DEFINED
#define GUID_DEFINED

#ifdef __WIDL__
typedef struct
{
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    byte           Data4[ 8 ];
} GUID;
#else
typedef struct _GUID
{
#ifdef _MSC_VER
    unsigned long  Data1;
#else
    unsigned int   Data1;
#endif
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;
#endif

/* Macros for __uuidof emulation */
#if defined(__cplusplus) && !defined(_MSC_VER)

extern "C++" {
    template<typename T> const GUID &__wine_uuidof();
}

#define __CRT_UUID_DECL(type,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)           \
    extern "C++" {                                                      \
    template<> inline const GUID &__wine_uuidof<type>() {               \
        static const IID __uuid_inst = {l,w1,w2, {b1,b2,b3,b4,b5,b6,b7,b8}}; \
        return __uuid_inst;                                             \
    }                                                                   \
    template<> inline const GUID &__wine_uuidof<type*>() {              \
        return __wine_uuidof<type>();                                   \
    }                                                                   \
    }

#define __uuidof(type) __wine_uuidof<typeof(type)>()

#else

#define __CRT_UUID_DECL(type,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#endif

#endif

#undef DEFINE_GUID

#ifdef INITGUID
#ifdef __cplusplus
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name DECLSPEC_HIDDEN; \
        EXTERN_C const GUID name = \
	{ l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const GUID name DECLSPEC_HIDDEN; \
        const GUID name = \
	{ l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name DECLSPEC_HIDDEN
#endif

#define DEFINE_OLEGUID(name, l, w1, w2) \
	DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

#ifndef _GUIDDEF_H_
#define _GUIDDEF_H_

#ifndef __LPGUID_DEFINED__
#define __LPGUID_DEFINED__
typedef GUID *LPGUID;
#endif

#ifndef __LPCGUID_DEFINED__
#define __LPCGUID_DEFINED__
typedef const GUID *LPCGUID;
#endif

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef GUID IID,*LPIID;
typedef GUID CLSID,*LPCLSID;
typedef GUID FMTID,*LPFMTID;
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)
#define IsEqualFMTID(rfmtid1, rfmtid2) IsEqualGUID(rfmtid1, rfmtid2)
#define IID_NULL   GUID_NULL
#define CLSID_NULL GUID_NULL
#define FMTID_NULL GUID_NULL

#ifdef __midl_proxy
#define __MIDL_CONST
#else
#define __MIDL_CONST const
#endif

#endif /* ndef __IID_DEFINED__ */

#ifdef __cplusplus
#define REFGUID             const GUID &
#define REFCLSID            const CLSID &
#define REFIID              const IID &
#define REFFMTID            const FMTID &
#else /* !defined(__cplusplus) */
#define REFGUID             const GUID* __MIDL_CONST
#define REFCLSID            const CLSID* __MIDL_CONST
#define REFIID              const IID* __MIDL_CONST
#define REFFMTID            const FMTID* __MIDL_CONST
#endif /* !defined(__cplusplus) */

#if defined(__cplusplus) && !defined(CINTERFACE)
#define IsEqualGUID(rguid1, rguid2) (!memcmp(&(rguid1), &(rguid2), sizeof(GUID)))
inline int InlineIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (((ULONG *)&rguid1)[0] == ((ULONG *)&rguid2)[0] &&
           ((ULONG *)&rguid1)[1] == ((ULONG *)&rguid2)[1] &&
           ((ULONG *)&rguid1)[2] == ((ULONG *)&rguid2)[2] &&
           ((ULONG *)&rguid1)[3] == ((ULONG *)&rguid2)[3]);
}
#else /* defined(__cplusplus) && !defined(CINTERFACE) */
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#define InlineIsEqualGUID(rguid1, rguid2)  \
        (((ULONG *)rguid1)[0] == ((ULONG *)rguid2)[0] && \
         ((ULONG *)rguid1)[1] == ((ULONG *)rguid2)[1] && \
         ((ULONG *)rguid1)[2] == ((ULONG *)rguid2)[2] && \
         ((ULONG *)rguid1)[3] == ((ULONG *)rguid2)[3])
#endif /* defined(__cplusplus) && !defined(CINTERFACE) */

#if defined(__cplusplus) && !defined(CINTERFACE)
#include <string.h>
inline bool operator==(const GUID& guidOne, const GUID& guidOther)
{
    return !memcmp(&guidOne,&guidOther,sizeof(GUID));
}
inline bool operator!=(const GUID& guidOne, const GUID& guidOther)
{
    return !(guidOne == guidOther);
}
#endif

extern const IID GUID_NULL;

#endif /* _GUIDDEF_H_ */

#include "wine/winheader_exit.h"
