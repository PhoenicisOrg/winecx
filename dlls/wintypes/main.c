/*
 * Copyright 2022-2024 Zhiyi Zhang for CodeWeavers
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

#define COBJMACROS
#include "initguid.h"
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "wine/debug.h"
#include "objbase.h"

#include "activation.h"
#include "rometadataresolution.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Metadata
#include "windows.foundation.metadata.h"
#include "wintypes_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintypes);

static const struct
{
    const WCHAR *name;
    unsigned int max_major;
}
present_contracts[] =
{
    { L"Windows.Foundation.UniversalApiContract", 10, },
};

static BOOLEAN is_api_contract_present( const HSTRING hname, unsigned int version )
{
    const WCHAR *name = WindowsGetStringRawBuffer( hname, NULL );
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(present_contracts); ++i)
        if (!wcsicmp( name, present_contracts[i].name )) return version <= present_contracts[i].max_major;

    return FALSE;
}

struct wintypes
{
    IActivationFactory IActivationFactory_iface;
    IApiInformationStatics IApiInformationStatics_iface;
    IPropertyValueStatics IPropertyValueStatics_iface;
    LONG ref;
};

static inline struct wintypes *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct wintypes, IActivationFactory_iface);
}

static HRESULT STDMETHODCALLTYPE wintypes_QueryInterface(IActivationFactory *iface, REFIID iid,
        void **out)
{
    struct wintypes *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IInspectable)
            || IsEqualGUID(iid, &IID_IAgileObject)
            || IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IApiInformationStatics))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IApiInformationStatics_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IPropertyValueStatics))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IPropertyValueStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE wintypes_AddRef(IActivationFactory *iface)
{
    struct wintypes *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE wintypes_Release(IActivationFactory *iface)
{
    struct wintypes *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE wintypes_GetIids(IActivationFactory *iface, ULONG *iid_count,
        IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE wintypes_GetRuntimeClassName(IActivationFactory *iface,
        HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE wintypes_GetTrustLevel(IActivationFactory *iface,
        TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE wintypes_ActivateInstance(IActivationFactory *iface,
        IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    wintypes_QueryInterface,
    wintypes_AddRef,
    wintypes_Release,
    /* IInspectable methods */
    wintypes_GetIids,
    wintypes_GetRuntimeClassName,
    wintypes_GetTrustLevel,
    /* IActivationFactory methods */
    wintypes_ActivateInstance,
};

DEFINE_IINSPECTABLE(api_information_statics, IApiInformationStatics, struct wintypes, IActivationFactory_iface)

static HRESULT STDMETHODCALLTYPE api_information_statics_IsTypePresent(
        IApiInformationStatics *iface, HSTRING type_name, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, value %p stub!\n", iface, debugstr_hstring(type_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsMethodPresent(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING method_name, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, method_name %s, value %p stub!\n", iface,
            debugstr_hstring(type_name), debugstr_hstring(method_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsMethodPresentWithArity(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING method_name,
        UINT32 input_parameter_count, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, method_name %s, input_parameter_count %u, value %p stub!\n",
            iface, debugstr_hstring(type_name), debugstr_hstring(method_name),
            input_parameter_count, value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsEventPresent(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING event_name, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, event_name %s, value %p stub!\n", iface,
            debugstr_hstring(type_name), debugstr_hstring(event_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsPropertyPresent(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING property_name, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, property_name %s, value %p stub!\n", iface,
            debugstr_hstring(type_name), debugstr_hstring(property_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsReadOnlyPropertyPresent(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING property_name,
        BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, property_name %s, value %p stub!\n", iface,
            debugstr_hstring(type_name), debugstr_hstring(property_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsWriteablePropertyPresent(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING property_name, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, property_name %s, value %p stub!\n", iface,
            debugstr_hstring(type_name), debugstr_hstring(property_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsEnumNamedValuePresent(
        IApiInformationStatics *iface, HSTRING enum_type_name, HSTRING value_name, BOOLEAN *value)
{
    FIXME("iface %p, enum_type_name %s, value_name %s, value %p stub!\n", iface,
            debugstr_hstring(enum_type_name), debugstr_hstring(value_name), value);

    if (!enum_type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsApiContractPresentByMajor(
        IApiInformationStatics *iface, HSTRING contract_name, UINT16 major_version, BOOLEAN *value)
{
    FIXME("iface %p, contract_name %s, major_version %u, value %p semi-stub.\n", iface,
            debugstr_hstring(contract_name), major_version, value);

    if (!contract_name)
        return E_INVALIDARG;

    *value = is_api_contract_present( contract_name, major_version );
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsApiContractPresentByMajorAndMinor(
        IApiInformationStatics *iface, HSTRING contract_name, UINT16 major_version,
        UINT16 minor_version, BOOLEAN *value)
{
    FIXME("iface %p, contract_name %s, major_version %u, minor_version %u, value %p stub!\n", iface,
            debugstr_hstring(contract_name), major_version, minor_version, value);

    if (!contract_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static const struct IApiInformationStaticsVtbl api_information_statics_vtbl =
{
    api_information_statics_QueryInterface,
    api_information_statics_AddRef,
    api_information_statics_Release,
    /* IInspectable methods */
    api_information_statics_GetIids,
    api_information_statics_GetRuntimeClassName,
    api_information_statics_GetTrustLevel,
    /* IApiInformationStatics methods */
    api_information_statics_IsTypePresent,
    api_information_statics_IsMethodPresent,
    api_information_statics_IsMethodPresentWithArity,
    api_information_statics_IsEventPresent,
    api_information_statics_IsPropertyPresent,
    api_information_statics_IsReadOnlyPropertyPresent,
    api_information_statics_IsWriteablePropertyPresent,
    api_information_statics_IsEnumNamedValuePresent,
    api_information_statics_IsApiContractPresentByMajor,
    api_information_statics_IsApiContractPresentByMajorAndMinor
};

struct property_value
{
    IPropertyValue IPropertyValue_iface;
    union
    {
        IReference_boolean boolean_iface;
        IReference_DOUBLE double_iface;
        IReference_HSTRING hstring_iface;
    } irefs;
    PropertyType type;
    unsigned int value_size;
    void *value;
    LONG ref;
};

static inline struct property_value *impl_from_IPropertyValue(IPropertyValue *iface)
{
    return CONTAINING_RECORD(iface, struct property_value, IPropertyValue_iface);
}

static inline struct property_value *impl_from_IInspectable(IInspectable *iface)
{
    return CONTAINING_RECORD(iface, struct property_value, IPropertyValue_iface);
}

#define property_value_get_primitive(type) _property_value_get_primitive(iface, type, value, sizeof(*value))
static HRESULT _property_value_get_primitive(IPropertyValue *iface, PropertyType type, void *value, size_t size)
{
    struct property_value *impl = impl_from_IPropertyValue(iface);

    if (!value)
        return E_POINTER;

    if (impl->type != type)
        return TYPE_E_TYPEMISMATCH;

    memcpy(value, impl->value, size);
    return S_OK;
}

#define property_value_get_primitive_array(type) _property_value_get_primitive_array(iface, type, value_size, (void **)value)
static HRESULT _property_value_get_primitive_array(IPropertyValue *iface, PropertyType type, unsigned int *value_size, void **value)
{
    struct property_value *impl = impl_from_IPropertyValue(iface);

    if (!value_size || !value)
        return E_POINTER;

    if (impl->type != type)
        return TYPE_E_TYPEMISMATCH;

    *value_size = impl->value_size;
    *value = impl->value;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE property_value_QueryInterface(IPropertyValue *iface, REFIID riid, void **out)
{
    struct property_value *impl = impl_from_IPropertyValue(iface);

    TRACE("%p, %s, %p\n", impl, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IInspectable)
        || IsEqualIID(riid, &IID_IPropertyValue))
    {
        IPropertyValue_AddRef(iface);
        *out = &impl->IPropertyValue_iface;
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IReference_boolean) && impl->type == PropertyType_Boolean)
    {
        IReference_boolean_AddRef(&impl->irefs.boolean_iface);
        *out = &impl->irefs.boolean_iface;
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IReference_DOUBLE) && impl->type == PropertyType_Double)
    {
        IReference_DOUBLE_AddRef(&impl->irefs.double_iface);
        *out = &impl->irefs.double_iface;
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IReference_HSTRING) && impl->type == PropertyType_String)
    {
        IReference_HSTRING_AddRef(&impl->irefs.hstring_iface);
        *out = &impl->irefs.hstring_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE property_value_AddRef(IPropertyValue *iface)
{
    struct property_value *impl = impl_from_IPropertyValue(iface);
    ULONG refcount = InterlockedIncrement(&impl->ref);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE property_value_Release(IPropertyValue *iface)
{
    struct property_value *impl = impl_from_IPropertyValue(iface);
    ULONG refcount = InterlockedDecrement(&impl->ref);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (impl->value)
            free(impl->value);
        free(impl);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE property_value_GetIids(IPropertyValue *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_GetRuntimeClassName(IPropertyValue *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_GetTrustLevel(IPropertyValue *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_get_Type(IPropertyValue *iface, enum PropertyType *type)
{
    struct property_value *impl = impl_from_IPropertyValue(iface);

    TRACE("iface %p, type %p.\n", iface, type);

    if (!type)
        return E_POINTER;

    *type = impl->type;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE property_value_get_IsNumericScalar(IPropertyValue *iface, boolean *value)
{
    TRACE("iface %p, value %p.\n", iface, value);

    *value = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE property_value_GetUInt8(IPropertyValue *iface, BYTE *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_UInt8);
}

static HRESULT STDMETHODCALLTYPE property_value_GetInt16(IPropertyValue *iface, INT16 *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_Int16);
}

static HRESULT STDMETHODCALLTYPE property_value_GetUInt16(IPropertyValue *iface, UINT16 *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_UInt16);
}

static HRESULT STDMETHODCALLTYPE property_value_GetInt32(IPropertyValue *iface, INT32 *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_Int32);
}

static HRESULT STDMETHODCALLTYPE property_value_GetUInt32(IPropertyValue *iface, UINT32 *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_UInt32);
}

static HRESULT STDMETHODCALLTYPE property_value_GetInt64(IPropertyValue *iface, INT64 *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_Int64);
}

static HRESULT STDMETHODCALLTYPE property_value_GetUInt64(IPropertyValue *iface, UINT64 *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_UInt64);
}

static HRESULT STDMETHODCALLTYPE property_value_GetSingle(IPropertyValue *iface, FLOAT *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_Single);
}

static HRESULT STDMETHODCALLTYPE property_value_GetDouble(IPropertyValue *iface, DOUBLE *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_Double);
}

static HRESULT STDMETHODCALLTYPE property_value_GetChar16(IPropertyValue *iface, WCHAR *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_Char16);
}

static HRESULT STDMETHODCALLTYPE property_value_GetBoolean(IPropertyValue *iface, boolean *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_Boolean);
}

static HRESULT STDMETHODCALLTYPE property_value_GetString(IPropertyValue *iface, HSTRING *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_String);
}

static HRESULT STDMETHODCALLTYPE property_value_GetGuid(IPropertyValue *iface, GUID *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_Guid);
}

static HRESULT STDMETHODCALLTYPE property_value_GetDateTime(IPropertyValue *iface,
                                                            struct DateTime *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_DateTime);
}

static HRESULT STDMETHODCALLTYPE property_value_GetTimeSpan(IPropertyValue *iface, struct TimeSpan *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_TimeSpan);
}

static HRESULT STDMETHODCALLTYPE property_value_GetPoint(IPropertyValue *iface, struct Point *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_Point);
}

static HRESULT STDMETHODCALLTYPE property_value_GetSize(IPropertyValue *iface, struct Size *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_Size);
}

static HRESULT STDMETHODCALLTYPE property_value_GetRect(IPropertyValue *iface, struct Rect *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    return property_value_get_primitive(PropertyType_Rect);
}

static HRESULT STDMETHODCALLTYPE property_value_GetUInt8Array(IPropertyValue *iface, UINT32 *value_size, BYTE **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_UInt8Array);
}

static HRESULT STDMETHODCALLTYPE property_value_GetInt16Array(IPropertyValue *iface, UINT32 *value_size, INT16 **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_Int16Array);
}

static HRESULT STDMETHODCALLTYPE property_value_GetUInt16Array(IPropertyValue *iface, UINT32 *value_size, UINT16 **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_UInt16Array);
}

static HRESULT STDMETHODCALLTYPE property_value_GetInt32Array(IPropertyValue *iface, UINT32 *value_size, INT32 **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_Int32Array);
}

static HRESULT STDMETHODCALLTYPE property_value_GetUInt32Array(IPropertyValue *iface, UINT32 *value_size, UINT32 **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_UInt32Array);
}

static HRESULT STDMETHODCALLTYPE property_value_GetInt64Array(IPropertyValue *iface, UINT32 *value_size, INT64 **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_Int64Array);
}

static HRESULT STDMETHODCALLTYPE property_value_GetUInt64Array(IPropertyValue *iface, UINT32 *value_size, UINT64 **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_UInt64Array);
}

static HRESULT STDMETHODCALLTYPE property_value_GetSingleArray(IPropertyValue *iface, UINT32 *value_size, FLOAT **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_SingleArray);
}

static HRESULT STDMETHODCALLTYPE property_value_GetDoubleArray(IPropertyValue *iface, UINT32 *value_size, DOUBLE **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_DoubleArray);
}

static HRESULT STDMETHODCALLTYPE property_value_GetChar16Array(IPropertyValue *iface, UINT32 *value_size, WCHAR **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_Char16Array);
}

static HRESULT STDMETHODCALLTYPE property_value_GetBooleanArray(IPropertyValue *iface, UINT32 *value_size, boolean **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_BooleanArray);
}

static HRESULT STDMETHODCALLTYPE property_value_GetStringArray(IPropertyValue *iface, UINT32 *value_size, HSTRING **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_StringArray);
}

static HRESULT STDMETHODCALLTYPE property_value_GetInspectableArray(IPropertyValue *iface, UINT32 *value_size, IInspectable ***value)
{
    FIXME("iface %p, value_size %p, value %p stub!\n", iface, value_size, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_GetGuidArray(IPropertyValue *iface, UINT32 *value_size, GUID **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_GuidArray);
}

static HRESULT STDMETHODCALLTYPE property_value_GetDateTimeArray(IPropertyValue *iface, UINT32 *value_size, struct DateTime **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_DateTimeArray);
}

static HRESULT STDMETHODCALLTYPE property_value_GetTimeSpanArray(IPropertyValue *iface, UINT32 *value_size, struct TimeSpan **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_TimeSpanArray);
}

static HRESULT STDMETHODCALLTYPE property_value_GetPointArray(IPropertyValue *iface, UINT32 *value_size, struct Point **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_PointArray);
}

static HRESULT STDMETHODCALLTYPE property_value_GetSizeArray(IPropertyValue *iface, UINT32 *value_size, struct Size **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_SizeArray);
}

static HRESULT STDMETHODCALLTYPE property_value_GetRectArray(IPropertyValue *iface, UINT32 *value_size, struct Rect **value)
{
    TRACE("iface %p, value_size %p, value %p.\n", iface, value_size, value);
    return property_value_get_primitive_array(PropertyType_RectArray);
}

static const struct IPropertyValueVtbl property_value_vtbl =
{
    property_value_QueryInterface,
    property_value_AddRef,
    property_value_Release,
    /* IInspectable methods */
    property_value_GetIids,
    property_value_GetRuntimeClassName,
    property_value_GetTrustLevel,
    /* IPropertyValue methods */
    property_value_get_Type,
    property_value_get_IsNumericScalar,
    property_value_GetUInt8,
    property_value_GetInt16,
    property_value_GetUInt16,
    property_value_GetInt32,
    property_value_GetUInt32,
    property_value_GetInt64,
    property_value_GetUInt64,
    property_value_GetSingle,
    property_value_GetDouble,
    property_value_GetChar16,
    property_value_GetBoolean,
    property_value_GetString,
    property_value_GetGuid,
    property_value_GetDateTime,
    property_value_GetTimeSpan,
    property_value_GetPoint,
    property_value_GetSize,
    property_value_GetRect,
    property_value_GetUInt8Array,
    property_value_GetInt16Array,
    property_value_GetUInt16Array,
    property_value_GetInt32Array,
    property_value_GetUInt32Array,
    property_value_GetInt64Array,
    property_value_GetUInt64Array,
    property_value_GetSingleArray,
    property_value_GetDoubleArray,
    property_value_GetChar16Array,
    property_value_GetBooleanArray,
    property_value_GetStringArray,
    property_value_GetInspectableArray,
    property_value_GetGuidArray,
    property_value_GetDateTimeArray,
    property_value_GetTimeSpanArray,
    property_value_GetPointArray,
    property_value_GetSizeArray,
    property_value_GetRectArray,
};

#define create_primitive_property_value(type) _create_primitive_property_value(type, &value, 1, sizeof(value), property_value)
#define create_primitive_property_value_array(type) _create_primitive_property_value(type, value, value_size, sizeof(*value), property_value)
static HRESULT _create_primitive_property_value(PropertyType type, void *value,
        unsigned int count, size_t unit_size, IInspectable **property_value)
{
    struct property_value *impl;

    if (!value || !property_value)
        return E_POINTER;

    impl = calloc(1, sizeof(*impl));
    if (!impl)
        return E_OUTOFMEMORY;

    impl->IPropertyValue_iface.lpVtbl = &property_value_vtbl;
    impl->type = type;
    impl->ref = 1;
    if (type != PropertyType_Empty)
    {
        impl->value = malloc(unit_size * count);
        if (!impl->value)
        {
            free(impl);
            return E_OUTOFMEMORY;
        }
        memcpy(impl->value, value, unit_size * count);
        impl->value_size = count;
    }

    *property_value = (IInspectable *)&impl->IPropertyValue_iface;
    return S_OK;
}

#define create_primitive_property_value_iref(type, iref_vtbl, vtbl)                      \
    do                                                                                   \
    {                                                                                    \
        HRESULT hr = create_primitive_property_value(type);                              \
        if (hr == S_OK)                                                                  \
        {                                                                                \
            struct property_value *value_impl = impl_from_IInspectable(*property_value); \
            value_impl->iref_vtbl = &vtbl;                                               \
        }                                                                                \
        return hr;                                                                       \
    } while (0)

DEFINE_IINSPECTABLE_(iref_boolean, IReference_boolean, struct property_value,
                     impl_from_IReference_boolean, irefs.boolean_iface, &impl->IPropertyValue_iface);

static HRESULT STDMETHODCALLTYPE iref_boolean_get_Value(IReference_boolean *iface, boolean *value)
{
    struct property_value *impl = impl_from_IReference_boolean(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return property_value_GetBoolean(&impl->IPropertyValue_iface, value);
}

static const struct IReference_booleanVtbl iref_boolean_vtbl =
{
    iref_boolean_QueryInterface,
    iref_boolean_AddRef,
    iref_boolean_Release,
    /* IInspectable methods */
    iref_boolean_GetIids,
    iref_boolean_GetRuntimeClassName,
    iref_boolean_GetTrustLevel,
    /* IReference<boolean> methods */
    iref_boolean_get_Value,
};

DEFINE_IINSPECTABLE_(iref_hstring, IReference_HSTRING, struct property_value,
                     impl_from_IReference_HSTRING, irefs.hstring_iface, &impl->IPropertyValue_iface);

static HRESULT STDMETHODCALLTYPE iref_hstring_get_Value(IReference_HSTRING *iface, HSTRING *value)
{
    struct property_value *impl = impl_from_IReference_HSTRING(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return property_value_GetString(&impl->IPropertyValue_iface, value);
}

static const struct IReference_HSTRINGVtbl iref_hstring_vtbl =
{
    iref_hstring_QueryInterface,
    iref_hstring_AddRef,
    iref_hstring_Release,
    /* IInspectable methods */
    iref_hstring_GetIids,
    iref_hstring_GetRuntimeClassName,
    iref_hstring_GetTrustLevel,
    /* IReference<HSTRING> methods */
    iref_hstring_get_Value,
};

DEFINE_IINSPECTABLE_(iref_double, IReference_DOUBLE, struct property_value,
                     impl_from_IReference_DOUBLE, irefs.double_iface, &impl->IPropertyValue_iface);

static HRESULT STDMETHODCALLTYPE iref_double_get_Value(IReference_DOUBLE *iface, DOUBLE *value)
{
    struct property_value *impl = impl_from_IReference_DOUBLE(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return property_value_GetDouble(&impl->IPropertyValue_iface, value);
}

static const struct IReference_DOUBLEVtbl iref_double_vtbl =
{
    iref_double_QueryInterface,
    iref_double_AddRef,
    iref_double_Release,
    /* IInspectable methods */
    iref_double_GetIids,
    iref_double_GetRuntimeClassName,
    iref_double_GetTrustLevel,
    /* IReference<DOUBLE> methods */
    iref_double_get_Value,
};

DEFINE_IINSPECTABLE(property_value_statics, IPropertyValueStatics, struct wintypes, IActivationFactory_iface)

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateEmpty(IPropertyValueStatics *iface,
        IInspectable **property_value)
{
    TRACE("iface %p, property_value %p.\n", iface, property_value);

    if (!property_value)
        return E_POINTER;

    *property_value = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt8(IPropertyValueStatics *iface,
        BYTE value, IInspectable **property_value)
{
    TRACE("iface %p, value %#x, property_value %p.\n", iface, value, property_value);
    return create_primitive_property_value(PropertyType_UInt8);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt16(IPropertyValueStatics *iface,
        INT16 value, IInspectable **property_value)
{
    TRACE("iface %p, value %d, property_value %p.\n", iface, value, property_value);
    return create_primitive_property_value(PropertyType_Int16);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt16(IPropertyValueStatics *iface,
        UINT16 value, IInspectable **property_value)
{
    TRACE("iface %p, value %u, property_value %p.\n", iface, value, property_value);
    return create_primitive_property_value(PropertyType_UInt16);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt32(IPropertyValueStatics *iface,
        INT32 value, IInspectable **property_value)
{
    TRACE("iface %p, value %d, property_value %p.\n", iface, value, property_value);
    return create_primitive_property_value(PropertyType_Int32);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt32(IPropertyValueStatics *iface,
        UINT32 value, IInspectable **property_value)
{
    TRACE("iface %p, value %u, property_value %p.\n", iface, value, property_value);
    return create_primitive_property_value(PropertyType_UInt32);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt64(IPropertyValueStatics *iface,
        INT64 value, IInspectable **property_value)
{
    TRACE("iface %p, value %I64d, property_value %p.\n", iface, value, property_value);
    return create_primitive_property_value(PropertyType_Int64);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt64(IPropertyValueStatics *iface,
        UINT64 value, IInspectable **property_value)
{
    TRACE("iface %p, value %I64u, property_value %p.\n", iface, value, property_value);
    return create_primitive_property_value(PropertyType_UInt64);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateSingle(IPropertyValueStatics *iface,
        FLOAT value, IInspectable **property_value)
{
    TRACE("iface %p, value %f, property_value %p.\n", iface, value, property_value);
    return create_primitive_property_value(PropertyType_Single);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateDouble(IPropertyValueStatics *iface,
        DOUBLE value, IInspectable **property_value)
{
    TRACE("iface %p, value %f, property_value %p.\n", iface, value, property_value);
    create_primitive_property_value_iref(PropertyType_Double, irefs.double_iface.lpVtbl, iref_double_vtbl);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateChar16(IPropertyValueStatics *iface,
        WCHAR value, IInspectable **property_value)
{
    TRACE("iface %p, value %s, property_value %p.\n", iface, wine_dbgstr_wn(&value, 1), property_value);
    return create_primitive_property_value(PropertyType_Char16);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateBoolean(IPropertyValueStatics *iface,
        boolean value, IInspectable **property_value)
{
    TRACE("iface %p, value %d, property_value %p.\n", iface, value, property_value);
    create_primitive_property_value_iref(PropertyType_Boolean, irefs.boolean_iface.lpVtbl, iref_boolean_vtbl);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateString(IPropertyValueStatics *iface,
        HSTRING value, IInspectable **property_value)
{
    TRACE("iface %p, value %s, property_value %p.\n", iface, debugstr_hstring(value), property_value);
    create_primitive_property_value_iref(PropertyType_String, irefs.hstring_iface.lpVtbl, iref_hstring_vtbl);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInspectable(IPropertyValueStatics *iface,
        IInspectable *value, IInspectable **property_value)
{
    FIXME("iface %p, value %p, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateGuid(IPropertyValueStatics *iface,
        GUID value, IInspectable **property_value)
{
    TRACE("iface %p, value %s, property_value %p.\n", iface, wine_dbgstr_guid(&value), property_value);
    return create_primitive_property_value(PropertyType_Guid);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateDateTime(IPropertyValueStatics *iface,
        DateTime value, IInspectable **property_value)
{
    TRACE("iface %p, value %I64d, property_value %p.\n", iface, value.UniversalTime, property_value);
    return create_primitive_property_value(PropertyType_DateTime);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateTimeSpan(IPropertyValueStatics *iface,
        TimeSpan value, IInspectable **property_value)
{
    TRACE("iface %p, value %I64d, property_value %p.\n", iface, value.Duration, property_value);
    return create_primitive_property_value(PropertyType_TimeSpan);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreatePoint(IPropertyValueStatics *iface,
        Point value, IInspectable **property_value)
{
    TRACE("iface %p, value (%f, %f), property_value %p.\n", iface, value.X, value.Y, property_value);
    return create_primitive_property_value(PropertyType_Point);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateSize(IPropertyValueStatics *iface,
        Size value, IInspectable **property_value)
{
    TRACE("iface %p, value (%fx%f), property_value %p.\n", iface, value.Width, value.Height, property_value);
    return create_primitive_property_value(PropertyType_Size);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateRect(IPropertyValueStatics *iface,
        Rect value, IInspectable **property_value)
{
    TRACE("iface %p, value (%f, %f %fx%f), property_value %p.\n", iface, value.X, value.Y,
            value.Width, value.Height, property_value);
    return create_primitive_property_value(PropertyType_Rect);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt8Array(IPropertyValueStatics *iface,
        UINT32 value_size, BYTE *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_UInt8Array);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt16Array(IPropertyValueStatics *iface,
        UINT32 value_size, INT16 *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_Int16Array);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt16Array(IPropertyValueStatics *iface,
        UINT32 value_size, UINT16 *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_UInt16Array);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt32Array(IPropertyValueStatics *iface,
        UINT32 value_size, INT32 *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_Int32Array);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt32Array(IPropertyValueStatics *iface,
        UINT32 value_size, UINT32 *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_UInt32Array);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt64Array(IPropertyValueStatics *iface,
        UINT32 value_size, INT64 *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_Int64Array);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt64Array(IPropertyValueStatics *iface,
        UINT32 value_size, UINT64 *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_UInt64Array);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateSingleArray(IPropertyValueStatics *iface,
        UINT32 value_size, FLOAT *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_SingleArray);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateDoubleArray(IPropertyValueStatics *iface,
        UINT32 value_size, DOUBLE *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_DoubleArray);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateChar16Array(IPropertyValueStatics *iface,
        UINT32 value_size, WCHAR *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_Char16Array);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateBooleanArray(IPropertyValueStatics *iface,
        UINT32 value_size, boolean *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_BooleanArray);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateStringArray(IPropertyValueStatics *iface,
        UINT32 value_size, HSTRING *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_StringArray);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInspectableArray(IPropertyValueStatics *iface,
        UINT32 value_size, IInspectable **value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateGuidArray(IPropertyValueStatics *iface,
        UINT32 value_size, GUID *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_GuidArray);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateDateTimeArray(IPropertyValueStatics *iface,
        UINT32 value_size, DateTime *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_DateTimeArray);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateTimeSpanArray(IPropertyValueStatics *iface,
        UINT32 value_size, TimeSpan *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_TimeSpanArray);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreatePointArray(IPropertyValueStatics *iface,
        UINT32 value_size, Point *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_PointArray);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateSizeArray(IPropertyValueStatics *iface,
        UINT32 value_size, Size *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_SizeArray);
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateRectArray(IPropertyValueStatics *iface,
        UINT32 value_size, Rect *value, IInspectable **property_value)
{
    TRACE("iface %p, value_size %u, value %p, property_value %p.\n", iface, value_size, value, property_value);
    return create_primitive_property_value_array(PropertyType_RectArray);
}

static const struct IPropertyValueStaticsVtbl property_value_statics_vtbl =
{
    property_value_statics_QueryInterface,
    property_value_statics_AddRef,
    property_value_statics_Release,
    /* IInspectable methods */
    property_value_statics_GetIids,
    property_value_statics_GetRuntimeClassName,
    property_value_statics_GetTrustLevel,
    /* IPropertyValueStatics methods */
    property_value_statics_CreateEmpty,
    property_value_statics_CreateUInt8,
    property_value_statics_CreateInt16,
    property_value_statics_CreateUInt16,
    property_value_statics_CreateInt32,
    property_value_statics_CreateUInt32,
    property_value_statics_CreateInt64,
    property_value_statics_CreateUInt64,
    property_value_statics_CreateSingle,
    property_value_statics_CreateDouble,
    property_value_statics_CreateChar16,
    property_value_statics_CreateBoolean,
    property_value_statics_CreateString,
    property_value_statics_CreateInspectable,
    property_value_statics_CreateGuid,
    property_value_statics_CreateDateTime,
    property_value_statics_CreateTimeSpan,
    property_value_statics_CreatePoint,
    property_value_statics_CreateSize,
    property_value_statics_CreateRect,
    property_value_statics_CreateUInt8Array,
    property_value_statics_CreateInt16Array,
    property_value_statics_CreateUInt16Array,
    property_value_statics_CreateInt32Array,
    property_value_statics_CreateUInt32Array,
    property_value_statics_CreateInt64Array,
    property_value_statics_CreateUInt64Array,
    property_value_statics_CreateSingleArray,
    property_value_statics_CreateDoubleArray,
    property_value_statics_CreateChar16Array,
    property_value_statics_CreateBooleanArray,
    property_value_statics_CreateStringArray,
    property_value_statics_CreateInspectableArray,
    property_value_statics_CreateGuidArray,
    property_value_statics_CreateDateTimeArray,
    property_value_statics_CreateTimeSpanArray,
    property_value_statics_CreatePointArray,
    property_value_statics_CreateSizeArray,
    property_value_statics_CreateRectArray,
};

static struct wintypes wintypes =
{
    {&activation_factory_vtbl},
    {&api_information_statics_vtbl},
    {&property_value_statics_vtbl},
    1
};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);
    *factory = &wintypes.IActivationFactory_iface;
    IUnknown_AddRef(*factory);
    return S_OK;
}

HRESULT WINAPI RoIsApiContractMajorVersionPresent(const WCHAR *name, UINT16 major, BOOL *result)
{
    FIXME("name %s, major %u, result %p\n", debugstr_w(name), major, result);
    *result = FALSE;
    return S_OK;
}

HRESULT WINAPI RoResolveNamespace(HSTRING name, HSTRING windowsMetaDataDir,
                                  DWORD packageGraphDirsCount, const HSTRING *packageGraphDirs,
                                  DWORD *metaDataFilePathsCount, HSTRING **metaDataFilePaths,
                                  DWORD *subNamespacesCount, HSTRING **subNamespaces)
{
    FIXME("name %s, windowsMetaDataDir %s, metaDataFilePaths %p, subNamespaces %p stub!\n",
            debugstr_hstring(name), debugstr_hstring(windowsMetaDataDir),
            metaDataFilePaths, subNamespaces);

    if (!metaDataFilePaths && !subNamespaces)
        return E_INVALIDARG;

    return RO_E_METADATA_NAME_NOT_FOUND;
}

struct parse_type_context
{
    DWORD allocated_parts_count;
    DWORD parts_count;
    HSTRING *parts;
};

static HRESULT add_part(struct parse_type_context *context, const WCHAR *part, size_t length)
{
    DWORD new_parts_count;
    HSTRING *new_parts;
    HRESULT hr;

    if (context->parts_count == context->allocated_parts_count)
    {
        new_parts_count = context->allocated_parts_count ? context->allocated_parts_count * 2 : 4;
        new_parts = CoTaskMemRealloc(context->parts, new_parts_count * sizeof(*context->parts));
        if (!new_parts)
            return E_OUTOFMEMORY;

        context->allocated_parts_count = new_parts_count;
        context->parts = new_parts;
    }

    if (FAILED(hr = WindowsCreateString(part, length, &context->parts[context->parts_count])))
        return hr;

    context->parts_count++;
    return S_OK;
}

static HRESULT parse_part(struct parse_type_context *context, const WCHAR *input, unsigned int length)
{
    const WCHAR *start, *end, *ptr;

    start = input;
    end = start + length;

    /* Remove leading spaces */
    while (start < end && *start == ' ')
        start++;

    /* Remove trailing spaces */
    while (end - 1 >= start && end[-1] == ' ')
        end--;

    /* Only contains spaces */
    if (start == end)
        return RO_E_METADATA_INVALID_TYPE_FORMAT;

    /* Has spaces in the middle */
    for (ptr = start; ptr < end; ptr++)
    {
        if (*ptr == ' ')
            return RO_E_METADATA_INVALID_TYPE_FORMAT;
    }

    return add_part(context, start, end - start);
}

static HRESULT parse_type(struct parse_type_context *context, const WCHAR *input, unsigned int length)
{
    unsigned int i, parameter_count, nested_level;
    const WCHAR *start, *end, *part_start, *ptr;
    HRESULT hr;

    start = input;
    end = start + length;
    part_start = start;
    ptr = start;

    /* Read until the end of input or '`' or '<' or '>' or ',' */
    while (ptr < end && *ptr != '`' && *ptr != '<' && *ptr != '>' && *ptr != ',')
        ptr++;

    /* If the type name has '`' and there are characters before '`' */
    if (ptr > start && ptr < end && *ptr == '`')
    {
        /* Move past the '`' */
        ptr++;

        /* Read the number of type parameters, expecting '1' to '9' */
        if (!(ptr < end && *ptr >= '1' && *ptr <= '9'))
            return RO_E_METADATA_INVALID_TYPE_FORMAT;
        parameter_count = *ptr - '0';

        /* Move past the number of type parameters, expecting '<' */
        ptr++;
        if (!(ptr < end && *ptr == '<'))
            return RO_E_METADATA_INVALID_TYPE_FORMAT;

        /* Add the name of parameterized interface, e.g., the "interface`1" in "interface`1<parameter>" */
        if (FAILED(hr = parse_part(context, part_start, ptr - part_start)))
            return hr;

        /* Move past the '<' */
        ptr++;
        nested_level = 1;

        /* Read parameters inside brackets, e.g., the "p1" and "p2" in "interface`2<p1, p2>" */
        for (i = 0; i < parameter_count; i++)
        {
            /* Read a new parameter */
            part_start = ptr;

            /* Read until ','. The comma must be at the same nested bracket level */
            while (ptr < end)
            {
                if (*ptr == '<')
                {
                    nested_level++;
                    ptr++;
                }
                else if (*ptr == '>')
                {
                    /* The last parameter before '>' */
                    if (i == parameter_count - 1 && nested_level == 1)
                    {
                        if (FAILED(hr = parse_type(context, part_start, ptr - part_start)))
                            return hr;

                        nested_level--;
                        ptr++;

                        /* Finish reading all parameters */
                        break;
                    }

                    nested_level--;
                    ptr++;
                }
                else if (*ptr == ',' && nested_level == 1)
                {
                    /* Parse the parameter, which can be another parameterized type */
                    if (FAILED(hr = parse_type(context, part_start, ptr - part_start)))
                        return hr;

                    /* Move past the ',' */
                    ptr++;

                    /* Finish reading one parameter */
                    break;
                }
                else
                {
                    ptr++;
                }
            }
        }

        /* Mismatching brackets or not enough parameters */
        if (nested_level != 0 || i != parameter_count)
            return RO_E_METADATA_INVALID_TYPE_FORMAT;

        /* The remaining characters must be spaces */
        while (ptr < end)
        {
            if (*ptr++ != ' ')
                return RO_E_METADATA_INVALID_TYPE_FORMAT;
        }

        return S_OK;
    }
    /* Contain invalid '`', '<', '>' or ',' */
    else if (ptr != end)
    {
        return RO_E_METADATA_INVALID_TYPE_FORMAT;
    }
    /* Non-parameterized */
    else
    {
        return parse_part(context, part_start, ptr - part_start);
    }
}

HRESULT WINAPI RoParseTypeName(HSTRING type_name, DWORD *parts_count, HSTRING **parts)
{
    struct parse_type_context context = {0};
    const WCHAR *input;
    unsigned int i;
    HRESULT hr;

    TRACE("%s %p %p.\n", debugstr_hstring(type_name), parts_count, parts);

    /* Empty string */
    if (!WindowsGetStringLen(type_name))
        return E_INVALIDARG;

    input = WindowsGetStringRawBuffer(type_name, NULL);
    /* The string has a leading space */
    if (input[0] == ' ')
        return RO_E_METADATA_INVALID_TYPE_FORMAT;

    *parts_count = 0;
    *parts = NULL;
    if (FAILED(hr = parse_type(&context, input, wcslen(input))))
    {
        for (i = 0; i < context.parts_count; i++)
            WindowsDeleteString(context.parts[i]);
        CoTaskMemFree(context.parts);
        return hr;
    }

    *parts_count = context.parts_count;
    *parts = context.parts;
    return S_OK;
}
