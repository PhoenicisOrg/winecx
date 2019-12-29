/*              DirectShow Editing Services (qedit.dll)
 *
 * Copyright 2008 Google (Lei Zhang)
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

#include "qedit_private.h"
#include "rpcproxy.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qedit);

static HINSTANCE instance;

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            instance = hInstDLL;
            DisableThreadLibraryCalls(hInstDLL);
            break;
    }
    return TRUE;
}

/******************************************************************************
 * DirectShow ClassFactory
 */
typedef struct {
    IClassFactory IClassFactory_iface;
    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_AMTimeline, AMTimeline_create },
    { &CLSID_MediaDet, MediaDet_create },
    { &CLSID_NullRenderer, NullRenderer_create },
    { &CLSID_SampleGrabber, SampleGrabber_create },
};

static HRESULT WINAPI DSCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppobj)
{
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *ppobj = iface;
        return S_OK;
    }

    *ppobj = NULL;
    WARN("(%p)->(%s,%p), not found\n", iface, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI DSCF_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DSCF_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        CoTaskMemFree(This);

    return ref;
}

static HRESULT WINAPI DSCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter, REFIID riid,
        void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    HRESULT hres;
    LPUNKNOWN punk;

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    if (pOuter && !IsEqualGUID(&IID_IUnknown, riid))
        return E_NOINTERFACE;

    hres = This->pfnCreateInstance(pOuter, (LPVOID *) &punk);
    if (SUCCEEDED(hres)) {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    return hres;
}

static HRESULT WINAPI DSCF_LockServer(IClassFactory *iface, BOOL dolock)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static const IClassFactoryVtbl DSCF_Vtbl =
{
    DSCF_QueryInterface,
    DSCF_AddRef,
    DSCF_Release,
    DSCF_CreateInstance,
    DSCF_LockServer
};


/***********************************************************************
 *              DllCanUnloadNow (QEDIT.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/*******************************************************************************
 * DllGetClassObject [QEDIT.@]
 * Retrieves class object from a DLL object
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED, E_NOINTERFACE
 */

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    unsigned int i;
    IClassFactoryImpl *factory;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if ( !IsEqualGUID( &IID_IClassFactory, riid )
         && ! IsEqualGUID( &IID_IUnknown, riid) )
        return E_NOINTERFACE;

    for (i = 0; i < ARRAY_SIZE(object_creation); i++)
    {
        if (IsEqualGUID(object_creation[i].clsid, rclsid))
            break;
    }

    if (i == ARRAY_SIZE(object_creation))
    {
        FIXME("%s: no class found.\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    factory = CoTaskMemAlloc(sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &DSCF_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = &factory->IClassFactory_iface;
    return S_OK;
}

/***********************************************************************
 *		DllRegisterServer (QEDIT.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (QEDIT.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
