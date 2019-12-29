/*
 * Generic Implementation of IPin Interface
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

static const IMemInputPinVtbl MemInputPin_Vtbl;

typedef HRESULT (*SendPinFunc)( IPin *to, LPVOID arg );

static inline BasePin *impl_from_IPin( IPin *iface )
{
    return CONTAINING_RECORD(iface, BasePin, IPin_iface);
}

/** Helper function, there are a lot of places where the error code is inherited
 * The following rules apply:
 *
 * Return the first received error code (E_NOTIMPL is ignored)
 * If no errors occur: return the first received non-error-code that isn't S_OK
 */
static HRESULT updatehres( HRESULT original, HRESULT new )
{
    if (FAILED( original ) || new == E_NOTIMPL)
        return original;

    if (FAILED( new ) || original == S_OK)
        return new;

    return original;
}

/** Sends a message from a pin further to other, similar pins
 * fnMiddle is called on each pin found further on the stream.
 * fnEnd (can be NULL) is called when the message can't be sent any further (this is a renderer or source)
 *
 * If the pin given is an input pin, the message will be sent downstream to other input pins
 * If the pin given is an output pin, the message will be sent upstream to other output pins
 */
static HRESULT SendFurther( IPin *from, SendPinFunc fnMiddle, LPVOID arg, SendPinFunc fnEnd )
{
    PIN_INFO pin_info;
    ULONG amount = 0;
    HRESULT hr = S_OK;
    HRESULT hr_return = S_OK;
    IEnumPins *enumpins = NULL;
    BOOL foundend = TRUE;
    PIN_DIRECTION from_dir;

    IPin_QueryDirection( from, &from_dir );

    hr = IPin_QueryInternalConnections( from, NULL, &amount );
    if (hr != E_NOTIMPL && amount)
        FIXME("Use QueryInternalConnections!\n");

    pin_info.pFilter = NULL;
    hr = IPin_QueryPinInfo( from, &pin_info );
    if (FAILED(hr))
        goto out;

    hr = IBaseFilter_EnumPins( pin_info.pFilter, &enumpins );
    if (FAILED(hr))
        goto out;

    hr = IEnumPins_Reset( enumpins );
    while (hr == S_OK) {
        IPin *pin = NULL;
        hr = IEnumPins_Next( enumpins, 1, &pin, NULL );
        if (hr == VFW_E_ENUM_OUT_OF_SYNC)
        {
            hr = IEnumPins_Reset( enumpins );
            continue;
        }
        if (pin)
        {
            PIN_DIRECTION dir;

            IPin_QueryDirection( pin, &dir );
            if (dir != from_dir)
            {
                IPin *connected = NULL;

                foundend = FALSE;
                IPin_ConnectedTo( pin, &connected );
                if (connected)
                {
                    HRESULT hr_local;

                    hr_local = fnMiddle( connected, arg );
                    hr_return = updatehres( hr_return, hr_local );
                    IPin_Release(connected);
                }
            }
            IPin_Release( pin );
        }
        else
        {
            hr = S_OK;
            break;
        }
    }

    if (!foundend)
        hr = hr_return;
    else if (fnEnd) {
        HRESULT hr_local;

        hr_local = fnEnd( from, arg );
        hr_return = updatehres( hr_return, hr_local );
    }
    IEnumPins_Release(enumpins);

out:
    if (pin_info.pFilter)
        IBaseFilter_Release( pin_info.pFilter );
    return hr;
}

static void Copy_PinInfo(PIN_INFO * pDest, const PIN_INFO * pSrc)
{
    /* avoid copying uninitialized data */
    lstrcpyW(pDest->achName, pSrc->achName);
    pDest->dir = pSrc->dir;
    pDest->pFilter = pSrc->pFilter;
}

static void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt)
{
    if (!pmt)
        return;
    TRACE("\t%s\n\t%s\n\t...\n\t%s\n", debugstr_guid(&pmt->majortype), debugstr_guid(&pmt->subtype), debugstr_guid(&pmt->formattype));
}

static BOOL CompareMediaTypes(const AM_MEDIA_TYPE * pmt1, const AM_MEDIA_TYPE * pmt2, BOOL bWildcards)
{
    TRACE("pmt1: ");
    dump_AM_MEDIA_TYPE(pmt1);
    TRACE("pmt2: ");
    dump_AM_MEDIA_TYPE(pmt2);
    return (((bWildcards && (IsEqualGUID(&pmt1->majortype, &GUID_NULL) || IsEqualGUID(&pmt2->majortype, &GUID_NULL))) || IsEqualGUID(&pmt1->majortype, &pmt2->majortype)) &&
            ((bWildcards && (IsEqualGUID(&pmt1->subtype, &GUID_NULL)   || IsEqualGUID(&pmt2->subtype, &GUID_NULL)))   || IsEqualGUID(&pmt1->subtype, &pmt2->subtype)));
}

/*** Common Base Pin function */
HRESULT WINAPI BasePinImpl_GetMediaType(BasePin *iface, int iPosition, AM_MEDIA_TYPE *pmt)
{
    if (iPosition < 0)
        return E_INVALIDARG;
    return VFW_S_NO_MORE_ITEMS;
}

LONG WINAPI BasePinImpl_GetMediaTypeVersion(BasePin *iface)
{
    return 1;
}

ULONG WINAPI BasePinImpl_AddRef(IPin *iface)
{
    BasePin *pin = impl_from_IPin(iface);
    return IBaseFilter_AddRef(pin->pinInfo.pFilter);
}

ULONG WINAPI BasePinImpl_Release(IPin *iface)
{
    BasePin *pin = impl_from_IPin(iface);
    return IBaseFilter_Release(pin->pinInfo.pFilter);
}

HRESULT WINAPI BasePinImpl_Disconnect(IPin * iface)
{
    HRESULT hr;
    BasePin *This = impl_from_IPin(iface);

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            IPin_Release(This->pConnectedTo);
            This->pConnectedTo = NULL;
            FreeMediaType(&This->mtCurrent);
            ZeroMemory(&This->mtCurrent, sizeof(This->mtCurrent));
            hr = S_OK;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

HRESULT WINAPI BasePinImpl_ConnectedTo(IPin * iface, IPin ** ppPin)
{
    HRESULT hr;
    BasePin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p)\n", This, ppPin);

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            *ppPin = This->pConnectedTo;
            IPin_AddRef(*ppPin);
            hr = S_OK;
        }
        else
        {
            hr = VFW_E_NOT_CONNECTED;
            *ppPin = NULL;
        }
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

HRESULT WINAPI BasePinImpl_ConnectionMediaType(IPin * iface, AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    BasePin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p)\n", This, pmt);

    EnterCriticalSection(This->pCritSec);
    {
        if (This->pConnectedTo)
        {
            CopyMediaType(pmt, &This->mtCurrent);
            hr = S_OK;
        }
        else
        {
            ZeroMemory(pmt, sizeof(*pmt));
            hr = VFW_E_NOT_CONNECTED;
        }
    }
    LeaveCriticalSection(This->pCritSec);

    return hr;
}

HRESULT WINAPI BasePinImpl_QueryPinInfo(IPin * iface, PIN_INFO * pInfo)
{
    BasePin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p)\n", This, pInfo);

    Copy_PinInfo(pInfo, &This->pinInfo);
    IBaseFilter_AddRef(pInfo->pFilter);

    return S_OK;
}

HRESULT WINAPI BasePinImpl_QueryDirection(IPin * iface, PIN_DIRECTION * pPinDir)
{
    BasePin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p)\n", This, pPinDir);

    *pPinDir = This->pinInfo.dir;

    return S_OK;
}

HRESULT WINAPI BasePinImpl_QueryId(IPin * iface, LPWSTR * Id)
{
    BasePin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p)\n", This, Id);

    *Id = CoTaskMemAlloc((lstrlenW(This->pinInfo.achName) + 1) * sizeof(WCHAR));
    if (!*Id)
        return E_OUTOFMEMORY;

    lstrcpyW(*Id, This->pinInfo.achName);

    return S_OK;
}

HRESULT WINAPI BasePinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt)
{
    BasePin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p)\n", iface, pmt);

    return (This->pFuncsTable->pfnCheckMediaType(This, pmt) == S_OK ? S_OK : S_FALSE);
}

HRESULT WINAPI BasePinImpl_EnumMediaTypes(IPin *iface, IEnumMediaTypes **enum_media_types)
{
    BasePin *pin = impl_from_IPin(iface);

    TRACE("iface %p, enum_media_types %p.\n", iface, enum_media_types);

    return EnumMediaTypes_Construct(pin, pin->pFuncsTable->pfnGetMediaType,
            BasePinImpl_GetMediaTypeVersion, enum_media_types);
}

HRESULT WINAPI BasePinImpl_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin)
{
    BasePin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p, %p)\n", This, apPin, cPin);

    return E_NOTIMPL; /* to tell caller that all input pins connected to all output pins */
}

HRESULT WINAPI BasePinImpl_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    BasePin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%s, %s, %e)\n", This, wine_dbgstr_longlong(tStart), wine_dbgstr_longlong(tStop), dRate);

    This->tStart = tStart;
    This->tStop = tStop;
    This->dRate = dRate;

    return S_OK;
}

/*** OutputPin implementation ***/

static inline BaseOutputPin *impl_BaseOutputPin_from_IPin( IPin *iface )
{
    return CONTAINING_RECORD(iface, BaseOutputPin, pin.IPin_iface);
}

HRESULT WINAPI BaseOutputPinImpl_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    BaseOutputPin *This = impl_BaseOutputPin_from_IPin(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMediaSeeking) ||
             IsEqualIID(riid, &IID_IQualityControl))
    {
        return IBaseFilter_QueryInterface(This->pin.pinInfo.pFilter, riid, ppv);
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

HRESULT WINAPI BaseOutputPinImpl_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    BaseOutputPin *This = impl_BaseOutputPin_from_IPin(iface);

    TRACE("(%p)->(%p, %p)\n", This, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    if (!pReceivePin)
        return E_POINTER;

    /* If we try to connect to ourselves, we will definitely deadlock.
     * There are other cases where we could deadlock too, but this
     * catches the obvious case */
    assert(pReceivePin != iface);

    EnterCriticalSection(This->pin.pCritSec);
    {
        /* if we have been a specific type to connect with, then we can either connect
         * with that or fail. We cannot choose different AM_MEDIA_TYPE */
        if (pmt && !IsEqualGUID(&pmt->majortype, &GUID_NULL) && !IsEqualGUID(&pmt->subtype, &GUID_NULL))
            hr = This->pFuncsTable->pfnAttemptConnection(This, pReceivePin, pmt);
        else
        {
            /* negotiate media type */

            IEnumMediaTypes * pEnumCandidates;
            AM_MEDIA_TYPE * pmtCandidate = NULL; /* Candidate media type */

            if (SUCCEEDED(hr = IPin_EnumMediaTypes(iface, &pEnumCandidates)))
            {
                hr = VFW_E_NO_ACCEPTABLE_TYPES; /* Assume the worst, but set to S_OK if connected successfully */

                /* try this filter's media types first */
                while (S_OK == IEnumMediaTypes_Next(pEnumCandidates, 1, &pmtCandidate, NULL))
                {
                    assert(pmtCandidate);
                    dump_AM_MEDIA_TYPE(pmtCandidate);
                    if (!IsEqualGUID(&FORMAT_None, &pmtCandidate->formattype)
                        && !IsEqualGUID(&GUID_NULL, &pmtCandidate->formattype))
                        assert(pmtCandidate->pbFormat);
                    if ((!pmt || CompareMediaTypes(pmt, pmtCandidate, TRUE))
                            && This->pFuncsTable->pfnAttemptConnection(This, pReceivePin, pmtCandidate) == S_OK)
                    {
                        hr = S_OK;
                        DeleteMediaType(pmtCandidate);
                        break;
                    }
                    DeleteMediaType(pmtCandidate);
                    pmtCandidate = NULL;
                }
                IEnumMediaTypes_Release(pEnumCandidates);
            }

            /* then try receiver filter's media types */
            if (hr != S_OK && SUCCEEDED(hr = IPin_EnumMediaTypes(pReceivePin, &pEnumCandidates))) /* if we haven't already connected successfully */
            {
                ULONG fetched;

                hr = VFW_E_NO_ACCEPTABLE_TYPES; /* Assume the worst, but set to S_OK if connected successfully */

                while (S_OK == IEnumMediaTypes_Next(pEnumCandidates, 1, &pmtCandidate, &fetched))
                {
                    assert(pmtCandidate);
                    dump_AM_MEDIA_TYPE(pmtCandidate);
                    if ((!pmt || CompareMediaTypes(pmt, pmtCandidate, TRUE))
                            && This->pFuncsTable->pfnAttemptConnection(This, pReceivePin, pmtCandidate) == S_OK)
                    {
                        hr = S_OK;
                        DeleteMediaType(pmtCandidate);
                        break;
                    }
                    DeleteMediaType(pmtCandidate);
                    pmtCandidate = NULL;
                } /* while */
                IEnumMediaTypes_Release(pEnumCandidates);
            } /* if not found */
        } /* if negotiate media type */
    } /* if succeeded */
    LeaveCriticalSection(This->pin.pCritSec);

    TRACE(" -- %x\n", hr);
    return hr;
}

HRESULT WINAPI BaseOutputPinImpl_ReceiveConnection(IPin *iface, IPin *pin, const AM_MEDIA_TYPE *pmt)
{
    ERR("(%p)->(%p, %p) incoming connection on an output pin!\n", iface, pin, pmt);
    return E_UNEXPECTED;
}

HRESULT WINAPI BaseOutputPinImpl_Disconnect(IPin * iface)
{
    HRESULT hr;
    BaseOutputPin *This = impl_BaseOutputPin_from_IPin(iface);

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (This->pMemInputPin)
        {
            IMemInputPin_Release(This->pMemInputPin);
            This->pMemInputPin = NULL;
        }
        if (This->pin.pConnectedTo)
        {
            IPin_Release(This->pin.pConnectedTo);
            This->pin.pConnectedTo = NULL;
            FreeMediaType(&This->pin.mtCurrent);
            ZeroMemory(&This->pin.mtCurrent, sizeof(This->pin.mtCurrent));
            hr = S_OK;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

HRESULT WINAPI BaseOutputPinImpl_EndOfStream(IPin * iface)
{
    TRACE("(%p)->()\n", iface);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

HRESULT WINAPI BaseOutputPinImpl_BeginFlush(IPin * iface)
{
    TRACE("(%p)->()\n", iface);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

HRESULT WINAPI BaseOutputPinImpl_EndFlush(IPin * iface)
{
    TRACE("(%p)->()\n", iface);

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

HRESULT WINAPI BaseOutputPinImpl_GetDeliveryBuffer(BaseOutputPin *This, IMediaSample ** ppSample, REFERENCE_TIME * tStart, REFERENCE_TIME * tStop, DWORD dwFlags)
{
    HRESULT hr;

    TRACE("(%p)->(%p, %p, %p, %x)\n", This, ppSample, tStart, tStop, dwFlags);

    if (!This->pin.pConnectedTo)
        hr = VFW_E_NOT_CONNECTED;
    else
    {
        hr = IMemAllocator_GetBuffer(This->pAllocator, ppSample, tStart, tStop, dwFlags);

        if (SUCCEEDED(hr))
            hr = IMediaSample_SetTime(*ppSample, tStart, tStop);
    }

    return hr;
}

/* replaces OutputPin_SendSample */
HRESULT WINAPI BaseOutputPinImpl_Deliver(BaseOutputPin *This, IMediaSample * pSample)
{
    IMemInputPin * pMemConnected = NULL;
    PIN_INFO pinInfo;
    HRESULT hr;

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
        {
            /* we don't have the lock held when using This->pMemInputPin,
             * so we need to AddRef it to stop it being deleted while we are
             * using it. Same with its filter. */
            pMemConnected = This->pMemInputPin;
            IMemInputPin_AddRef(pMemConnected);
            hr = IPin_QueryPinInfo(This->pin.pConnectedTo, &pinInfo);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    if (SUCCEEDED(hr))
    {
        /* NOTE: if we are in a critical section when Receive is called
         * then it causes some problems (most notably with the native Video
         * Renderer) if we are re-entered for whatever reason */
        hr = IMemInputPin_Receive(pMemConnected, pSample);

        /* If the filter's destroyed, tell upstream to stop sending data */
        if(IBaseFilter_Release(pinInfo.pFilter) == 0 && SUCCEEDED(hr))
            hr = S_FALSE;
    }
    if (pMemConnected)
        IMemInputPin_Release(pMemConnected);

    return hr;
}

/* replaces OutputPin_CommitAllocator */
HRESULT WINAPI BaseOutputPinImpl_Active(BaseOutputPin *This)
{
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
            hr = IMemAllocator_Commit(This->pAllocator);
    }
    LeaveCriticalSection(This->pin.pCritSec);

    TRACE("--> %08x\n", hr);
    return hr;
}

/* replaces OutputPin_DecommitAllocator */
HRESULT WINAPI BaseOutputPinImpl_Inactive(BaseOutputPin *This)
{
    HRESULT hr;

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (!This->pin.pConnectedTo || !This->pMemInputPin)
            hr = VFW_E_NOT_CONNECTED;
        else
            hr = IMemAllocator_Decommit(This->pAllocator);
    }
    LeaveCriticalSection(This->pin.pCritSec);

    TRACE("--> %08x\n", hr);
    return hr;
}

HRESULT WINAPI BaseOutputPinImpl_InitAllocator(BaseOutputPin *This, IMemAllocator **pMemAlloc)
{
    return CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER, &IID_IMemAllocator, (LPVOID*)pMemAlloc);
}

HRESULT WINAPI BaseOutputPinImpl_DecideAllocator(BaseOutputPin *This, IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    HRESULT hr;

    hr = IMemInputPin_GetAllocator(pPin, pAlloc);

    if (hr == VFW_E_NO_ALLOCATOR)
        /* Input pin provides no allocator, use standard memory allocator */
        hr = BaseOutputPinImpl_InitAllocator(This, pAlloc);

    if (SUCCEEDED(hr))
    {
        ALLOCATOR_PROPERTIES rProps;
        ZeroMemory(&rProps, sizeof(ALLOCATOR_PROPERTIES));

        IMemInputPin_GetAllocatorRequirements(pPin, &rProps);
        hr = This->pFuncsTable->pfnDecideBufferSize(This, *pAlloc, &rProps);
    }

    if (SUCCEEDED(hr))
        hr = IMemInputPin_NotifyAllocator(pPin, *pAlloc, FALSE);

    return hr;
}

/*** The Construct functions ***/

/* Function called as a helper to IPin_Connect */
/* specific AM_MEDIA_TYPE - it cannot be NULL */
HRESULT WINAPI BaseOutputPinImpl_AttemptConnection(BaseOutputPin *This, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;
    IMemAllocator * pMemAlloc = NULL;

    TRACE("(%p)->(%p, %p)\n", This, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    if ((hr = This->pFuncsTable->base.pfnCheckMediaType(&This->pin, pmt)) != S_OK)
        return hr;

    This->pin.pConnectedTo = pReceivePin;
    IPin_AddRef(pReceivePin);
    CopyMediaType(&This->pin.mtCurrent, pmt);

    hr = IPin_ReceiveConnection(pReceivePin, &This->pin.IPin_iface, pmt);

    /* get the IMemInputPin interface we will use to deliver samples to the
     * connected pin */
    if (SUCCEEDED(hr))
    {
        This->pMemInputPin = NULL;
        hr = IPin_QueryInterface(pReceivePin, &IID_IMemInputPin, (LPVOID)&This->pMemInputPin);

        if (SUCCEEDED(hr))
        {
            hr = This->pFuncsTable->pfnDecideAllocator(This, This->pMemInputPin, &pMemAlloc);
            if (SUCCEEDED(hr))
                This->pAllocator = pMemAlloc;
            else if (pMemAlloc)
                IMemAllocator_Release(pMemAlloc);
        }

        /* break connection if we couldn't get the allocator */
        if (FAILED(hr))
        {
            if (This->pMemInputPin)
                IMemInputPin_Release(This->pMemInputPin);
            This->pMemInputPin = NULL;

            IPin_Disconnect(pReceivePin);
        }
    }

    if (FAILED(hr))
    {
        IPin_Release(This->pin.pConnectedTo);
        This->pin.pConnectedTo = NULL;
        FreeMediaType(&This->pin.mtCurrent);
    }

    TRACE(" -- %x\n", hr);
    return hr;
}

static void strmbase_pin_init(BasePin *pin, const IPinVtbl *vtbl,
        const BasePinFuncTable *func_table, const PIN_INFO *info, CRITICAL_SECTION *cs)
{
    memset(pin, 0, sizeof(*pin));
    pin->IPin_iface.lpVtbl = vtbl;
    pin->pCritSec = cs;
    pin->dRate = 1.0;
    Copy_PinInfo(&pin->pinInfo, info);
    pin->pFuncsTable = func_table;
}

void strmbase_source_init(BaseOutputPin *pin, const IPinVtbl *vtbl,
        const PIN_INFO *info, const BaseOutputPinFuncTable *func_table, CRITICAL_SECTION *cs)
{
    memset(pin, 0, sizeof(*pin));
    strmbase_pin_init(&pin->pin, vtbl, &func_table->base, info, cs);
    pin->pFuncsTable = func_table;
}

HRESULT WINAPI BaseOutputPin_Construct(const IPinVtbl *OutputPin_Vtbl, LONG outputpin_size, const PIN_INFO * pPinInfo, const BaseOutputPinFuncTable* vtbl, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    BaseOutputPin * pPinImpl;

    *ppPin = NULL;

    if (pPinInfo->dir != PINDIR_OUTPUT)
    {
        ERR("Pin direction(%x) != PINDIR_OUTPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    assert(outputpin_size >= sizeof(BaseOutputPin));
    assert(vtbl->pfnAttemptConnection);

    pPinImpl = CoTaskMemAlloc(outputpin_size);

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    strmbase_source_init(pPinImpl, OutputPin_Vtbl, pPinInfo, vtbl, pCritSec);
    *ppPin = &pPinImpl->pin.IPin_iface;
    return S_OK;
}

void strmbase_source_cleanup(BaseOutputPin *pin)
{
    FreeMediaType(&pin->pin.mtCurrent);
    if (pin->pAllocator)
        IMemAllocator_Release(pin->pAllocator);
    pin->pAllocator = NULL;
}

HRESULT WINAPI BaseOutputPin_Destroy(BaseOutputPin *This)
{
    strmbase_source_cleanup(This);
    CoTaskMemFree(This);
    return S_OK;
}

/*** Input Pin implementation ***/

static inline BaseInputPin *impl_BaseInputPin_from_IPin( IPin *iface )
{
    return CONTAINING_RECORD(iface, BaseInputPin, pin.IPin_iface);
}

HRESULT WINAPI BaseInputPinImpl_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    BaseInputPin *This = impl_BaseInputPin_from_IPin(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMemInputPin))
        *ppv = &This->IMemInputPin_iface;
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
    {
        return IBaseFilter_QueryInterface(This->pin.pinInfo.pFilter, &IID_IMediaSeeking, ppv);
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

HRESULT WINAPI BaseInputPinImpl_Connect(IPin *iface, IPin *pin, const AM_MEDIA_TYPE *pmt)
{
    ERR("(%p)->(%p, %p) outgoing connection on an input pin!\n", iface, pin, pmt);
    return E_UNEXPECTED;
}


HRESULT WINAPI BaseInputPinImpl_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    BaseInputPin *This = impl_BaseInputPin_from_IPin(iface);
    PIN_DIRECTION pindirReceive;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p, %p)\n", This, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    EnterCriticalSection(This->pin.pCritSec);
    {
        if (This->pin.pConnectedTo)
            hr = VFW_E_ALREADY_CONNECTED;

        if (SUCCEEDED(hr) && This->pin.pFuncsTable->pfnCheckMediaType(&This->pin, pmt) != S_OK)
            hr = VFW_E_TYPE_NOT_ACCEPTED; /* FIXME: shouldn't we just map common errors onto
                                           * VFW_E_TYPE_NOT_ACCEPTED and pass the value on otherwise? */

        if (SUCCEEDED(hr))
        {
            IPin_QueryDirection(pReceivePin, &pindirReceive);

            if (pindirReceive != PINDIR_OUTPUT)
            {
                ERR("Can't connect from non-output pin\n");
                hr = VFW_E_INVALID_DIRECTION;
            }
        }

        if (SUCCEEDED(hr))
        {
            CopyMediaType(&This->pin.mtCurrent, pmt);
            This->pin.pConnectedTo = pReceivePin;
            IPin_AddRef(pReceivePin);
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

static HRESULT deliver_endofstream(IPin* pin, LPVOID unused)
{
    return IPin_EndOfStream( pin );
}

HRESULT WINAPI BaseInputPinImpl_EndOfStream(IPin * iface)
{
    HRESULT hr = S_OK;
    BaseInputPin *This = impl_BaseInputPin_from_IPin(iface);

    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    if (This->flushing)
        hr = S_FALSE;
    else
        This->end_of_stream = TRUE;
    LeaveCriticalSection(This->pin.pCritSec);

    if (hr == S_OK)
        hr = SendFurther( iface, deliver_endofstream, NULL, NULL );
    return hr;
}

static HRESULT deliver_beginflush(IPin* pin, LPVOID unused)
{
    return IPin_BeginFlush( pin );
}

HRESULT WINAPI BaseInputPinImpl_BeginFlush(IPin * iface)
{
    BaseInputPin *This = impl_BaseInputPin_from_IPin(iface);
    HRESULT hr;
    TRACE("(%p) semi-stub\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    This->flushing = TRUE;

    hr = SendFurther( iface, deliver_beginflush, NULL, NULL );
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

static HRESULT deliver_endflush(IPin* pin, LPVOID unused)
{
    return IPin_EndFlush( pin );
}

HRESULT WINAPI BaseInputPinImpl_EndFlush(IPin * iface)
{
    BaseInputPin *This = impl_BaseInputPin_from_IPin(iface);
    HRESULT hr;
    TRACE("(%p)->()\n", This);

    EnterCriticalSection(This->pin.pCritSec);
    This->flushing = This->end_of_stream = FALSE;

    hr = SendFurther( iface, deliver_endflush, NULL, NULL );
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

typedef struct newsegmentargs
{
    REFERENCE_TIME tStart, tStop;
    double rate;
} newsegmentargs;

static HRESULT deliver_newsegment(IPin *pin, LPVOID data)
{
    newsegmentargs *args = data;
    return IPin_NewSegment(pin, args->tStart, args->tStop, args->rate);
}

HRESULT WINAPI BaseInputPinImpl_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    BaseInputPin *This = impl_BaseInputPin_from_IPin(iface);
    newsegmentargs args;

    TRACE("(%p)->(%s, %s, %e)\n", This, wine_dbgstr_longlong(tStart), wine_dbgstr_longlong(tStop), dRate);

    args.tStart = This->pin.tStart = tStart;
    args.tStop = This->pin.tStop = tStop;
    args.rate = This->pin.dRate = dRate;

    return SendFurther( iface, deliver_newsegment, &args, NULL );
}

/*** IMemInputPin implementation ***/

static inline BaseInputPin *impl_from_IMemInputPin( IMemInputPin *iface )
{
    return CONTAINING_RECORD(iface, BaseInputPin, IMemInputPin_iface);
}

static HRESULT WINAPI MemInputPin_QueryInterface(IMemInputPin * iface, REFIID riid, LPVOID * ppv)
{
    BaseInputPin *This = impl_from_IMemInputPin(iface);

    return IPin_QueryInterface(&This->pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI MemInputPin_AddRef(IMemInputPin * iface)
{
    BaseInputPin *This = impl_from_IMemInputPin(iface);

    return IPin_AddRef(&This->pin.IPin_iface);
}

static ULONG WINAPI MemInputPin_Release(IMemInputPin * iface)
{
    BaseInputPin *This = impl_from_IMemInputPin(iface);

    return IPin_Release(&This->pin.IPin_iface);
}

static HRESULT WINAPI MemInputPin_GetAllocator(IMemInputPin * iface, IMemAllocator ** ppAllocator)
{
    BaseInputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppAllocator);

    *ppAllocator = This->pAllocator;
    if (*ppAllocator)
        IMemAllocator_AddRef(*ppAllocator);

    return *ppAllocator ? S_OK : VFW_E_NO_ALLOCATOR;
}

static HRESULT WINAPI MemInputPin_NotifyAllocator(IMemInputPin * iface, IMemAllocator * pAllocator, BOOL bReadOnly)
{
    BaseInputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p, %d)\n", This, iface, pAllocator, bReadOnly);

    if (bReadOnly)
        FIXME("Read only flag not handled yet!\n");

    /* FIXME: Should we release the allocator on disconnection? */
    if (!pAllocator)
    {
        WARN("Null allocator\n");
        return E_POINTER;
    }

    if (This->preferred_allocator && pAllocator != This->preferred_allocator)
        return E_FAIL;

    if (This->pAllocator)
        IMemAllocator_Release(This->pAllocator);
    This->pAllocator = pAllocator;
    if (This->pAllocator)
        IMemAllocator_AddRef(This->pAllocator);

    return S_OK;
}

static HRESULT WINAPI MemInputPin_GetAllocatorRequirements(IMemInputPin * iface, ALLOCATOR_PROPERTIES * pProps)
{
    BaseInputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pProps);

    /* override this method if you have any specific requirements */

    return E_NOTIMPL;
}

static HRESULT WINAPI MemInputPin_Receive(IMemInputPin * iface, IMediaSample * pSample)
{
    BaseInputPin *This = impl_from_IMemInputPin(iface);
    HRESULT hr = S_FALSE;

    /* this trace commented out for performance reasons */
    /*TRACE("(%p/%p)->(%p)\n", This, iface, pSample);*/
    if (This->pFuncsTable->pfnReceive)
        hr = This->pFuncsTable->pfnReceive(This, pSample);
    return hr;
}

static HRESULT WINAPI MemInputPin_ReceiveMultiple(IMemInputPin * iface, IMediaSample ** pSamples, LONG nSamples, LONG *nSamplesProcessed)
{
    HRESULT hr = S_OK;
    BaseInputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->(%p, %d, %p)\n", This, iface, pSamples, nSamples, nSamplesProcessed);

    for (*nSamplesProcessed = 0; *nSamplesProcessed < nSamples; (*nSamplesProcessed)++)
    {
        hr = IMemInputPin_Receive(iface, pSamples[*nSamplesProcessed]);
        if (hr != S_OK)
            break;
    }

    return hr;
}

static HRESULT WINAPI MemInputPin_ReceiveCanBlock(IMemInputPin * iface)
{
    BaseInputPin *This = impl_from_IMemInputPin(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return S_OK;
}

static const IMemInputPinVtbl MemInputPin_Vtbl =
{
    MemInputPin_QueryInterface,
    MemInputPin_AddRef,
    MemInputPin_Release,
    MemInputPin_GetAllocator,
    MemInputPin_NotifyAllocator,
    MemInputPin_GetAllocatorRequirements,
    MemInputPin_Receive,
    MemInputPin_ReceiveMultiple,
    MemInputPin_ReceiveCanBlock
};

void strmbase_sink_init(BaseInputPin *pin, const IPinVtbl *vtbl,
        const PIN_INFO *info, const BaseInputPinFuncTable *func_table, CRITICAL_SECTION *cs,
        IMemAllocator *allocator)
{
    memset(pin, 0, sizeof(*pin));
    strmbase_pin_init(&pin->pin, vtbl, &func_table->base, info, cs);
    pin->pFuncsTable = func_table;
    pin->pAllocator = pin->preferred_allocator = allocator;
    if (pin->preferred_allocator)
        IMemAllocator_AddRef(pin->preferred_allocator);
    pin->IMemInputPin_iface.lpVtbl = &MemInputPin_Vtbl;
}

HRESULT BaseInputPin_Construct(const IPinVtbl *InputPin_Vtbl, LONG inputpin_size, const PIN_INFO * pPinInfo,
                               const BaseInputPinFuncTable* vtbl,
                               LPCRITICAL_SECTION pCritSec, IMemAllocator *allocator, IPin ** ppPin)
{
    BaseInputPin * pPinImpl;

    *ppPin = NULL;

    assert(inputpin_size >= sizeof(BaseInputPin));
    assert(vtbl->base.pfnCheckMediaType);

    if (pPinInfo->dir != PINDIR_INPUT)
    {
        ERR("Pin direction(%x) != PINDIR_INPUT\n", pPinInfo->dir);
        return E_INVALIDARG;
    }

    pPinImpl = CoTaskMemAlloc(inputpin_size);

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    strmbase_sink_init(pPinImpl, InputPin_Vtbl, pPinInfo, vtbl, pCritSec, allocator);

    *ppPin = &pPinImpl->pin.IPin_iface;
    return S_OK;
}

void strmbase_sink_cleanup(BaseInputPin *pin)
{
    FreeMediaType(&pin->pin.mtCurrent);
    if (pin->pAllocator)
        IMemAllocator_Release(pin->pAllocator);
    pin->pAllocator = NULL;
    pin->pin.IPin_iface.lpVtbl = NULL;
}

HRESULT WINAPI BaseInputPin_Destroy(BaseInputPin *This)
{
    strmbase_sink_cleanup(This);
    CoTaskMemFree(This);
    return S_OK;
}
