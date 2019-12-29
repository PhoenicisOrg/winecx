/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include <assert.h>

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

static const WCHAR createW[] = {'c','r','e','a','t','e',0};
static const WCHAR getOwnPropertyDescriptorW[] =
    {'g','e','t','O','w','n','P','r','o','p','e','r','t','y','D','e','s','c','r','i','p','t','o','r',0};
static const WCHAR getPrototypeOfW[] =
    {'g','e','t','P','r','o','t','o','t','y','p','e','O','f',0};
static const WCHAR definePropertyW[] = {'d','e','f','i','n','e','P','r','o','p','e','r','t','y',0};

static const WCHAR definePropertiesW[] = {'d','e','f','i','n','e','P','r','o','p','e','r','t','i','e','s',0};

static const WCHAR default_valueW[] = {'[','o','b','j','e','c','t',' ','O','b','j','e','c','t',']',0};

static const WCHAR configurableW[] = {'c','o','n','f','i','g','u','r','a','b','l','e',0};
static const WCHAR enumerableW[] = {'e','n','u','m','e','r','a','b','l','e',0};
static const WCHAR valueW[] = {'v','a','l','u','e',0};
static const WCHAR writableW[] = {'w','r','i','t','a','b','l','e',0};
static const WCHAR getW[] = {'g','e','t',0};
static const WCHAR setW[] = {'s','e','t',0};

static HRESULT Object_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsdisp;
    const WCHAR *str;

    static const WCHAR formatW[] = {'[','o','b','j','e','c','t',' ','%','s',']',0};

    static const WCHAR arrayW[] = {'A','r','r','a','y',0};
    static const WCHAR booleanW[] = {'B','o','o','l','e','a','n',0};
    static const WCHAR dateW[] = {'D','a','t','e',0};
    static const WCHAR errorW[] = {'E','r','r','o','r',0};
    static const WCHAR functionW[] = {'F','u','n','c','t','i','o','n',0};
    static const WCHAR mathW[] = {'M','a','t','h',0};
    static const WCHAR numberW[] = {'N','u','m','b','e','r',0};
    static const WCHAR objectW[] = {'O','b','j','e','c','t',0};
    static const WCHAR regexpW[] = {'R','e','g','E','x','p',0};
    static const WCHAR stringW[] = {'S','t','r','i','n','g',0};
    /* Keep in sync with jsclass_t enum */
    static const WCHAR *names[] = {NULL, arrayW, booleanW, dateW, objectW, errorW,
        functionW, NULL, mathW, numberW, objectW, regexpW, stringW, objectW, objectW, objectW};

    TRACE("\n");

    jsdisp = get_jsdisp(jsthis);
    if(!jsdisp) {
        str = objectW;
    }else if(names[jsdisp->builtin_info->class]) {
        str = names[jsdisp->builtin_info->class];
    }else {
        assert(jsdisp->builtin_info->class != JSCLASS_NONE);
        FIXME("jdisp->builtin_info->class = %d\n", jsdisp->builtin_info->class);
        return E_FAIL;
    }

    if(r) {
        jsstr_t *ret;
        WCHAR *ptr;

        ret = jsstr_alloc_buf(9+lstrlenW(str), &ptr);
        if(!ret)
            return E_OUTOFMEMORY;

        swprintf(ptr, 9 + lstrlenW(str), formatW, str);
        *r = jsval_string(ret);
    }

    return S_OK;
}

static HRESULT Object_toLocaleString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    if(!is_jsdisp(jsthis)) {
        FIXME("Host object this\n");
        return E_FAIL;
    }

    return jsdisp_call_name(jsthis->u.jsdisp, toStringW, DISPATCH_METHOD, 0, NULL, r);
}

static HRESULT Object_valueOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    if(r) {
        IDispatch_AddRef(jsthis->u.disp);
        *r = jsval_disp(jsthis->u.disp);
    }
    return S_OK;
}

static HRESULT Object_hasOwnProperty(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *name;
    DISPID id;
    BSTR bstr;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_bool(FALSE);
        return S_OK;
    }

    hres = to_string(ctx, argv[0], &name);
    if(FAILED(hres))
        return hres;

    if(is_jsdisp(jsthis)) {
        property_desc_t prop_desc;
        const WCHAR *name_str;

        name_str = jsstr_flatten(name);
        if(!name_str) {
            jsstr_release(name);
            return E_OUTOFMEMORY;
        }

        hres = jsdisp_get_own_property(jsthis->u.jsdisp, name_str, TRUE, &prop_desc);
        jsstr_release(name);
        if(FAILED(hres) && hres != DISP_E_UNKNOWNNAME)
            return hres;

        if(r) *r = jsval_bool(hres == S_OK);
        return S_OK;
    }


    bstr = SysAllocStringLen(NULL, jsstr_length(name));
    if(bstr)
        jsstr_flush(name, bstr);
    jsstr_release(name);
    if(!bstr)
        return E_OUTOFMEMORY;

    if(is_dispex(jsthis))
        hres = IDispatchEx_GetDispID(jsthis->u.dispex, bstr, make_grfdex(ctx, fdexNameCaseSensitive), &id);
    else
        hres = IDispatch_GetIDsOfNames(jsthis->u.disp, &IID_NULL, &bstr, 1, ctx->lcid, &id);

    SysFreeString(bstr);
    if(r)
        *r = jsval_bool(SUCCEEDED(hres));
    return S_OK;
}

static HRESULT Object_propertyIsEnumerable(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    property_desc_t prop_desc;
    const WCHAR *name;
    jsstr_t *name_str;
    HRESULT hres;

    TRACE("\n");

    if(argc != 1) {
        FIXME("argc %d not supported\n", argc);
        return E_NOTIMPL;
    }

    if(!is_jsdisp(jsthis)) {
        FIXME("Host object this\n");
        return E_FAIL;
    }

    hres = to_flat_string(ctx, argv[0], &name_str, &name);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_get_own_property(jsthis->u.jsdisp, name, TRUE, &prop_desc);
    jsstr_release(name_str);
    if(FAILED(hres) && hres != DISP_E_UNKNOWNNAME)
        return hres;

    if(r)
        *r = jsval_bool(hres == S_OK && (prop_desc.flags & PROPF_ENUMERABLE) != 0);
    return S_OK;
}

static HRESULT Object_isPrototypeOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Object_get_value(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    jsstr_t *ret;

    TRACE("\n");

    ret = jsstr_alloc(default_valueW);
    if(!ret)
        return E_OUTOFMEMORY;

    *r = jsval_string(ret);
    return S_OK;
}

static void Object_destructor(jsdisp_t *dispex)
{
    heap_free(dispex);
}

static const builtin_prop_t Object_props[] = {
    {hasOwnPropertyW,        Object_hasOwnProperty,        PROPF_METHOD|1},
    {isPrototypeOfW,         Object_isPrototypeOf,         PROPF_METHOD|1},
    {propertyIsEnumerableW,  Object_propertyIsEnumerable,  PROPF_METHOD|1},
    {toLocaleStringW,        Object_toLocaleString,        PROPF_METHOD},
    {toStringW,              Object_toString,              PROPF_METHOD},
    {valueOfW,               Object_valueOf,               PROPF_METHOD}
};

static const builtin_info_t Object_info = {
    JSCLASS_OBJECT,
    {NULL, NULL,0, Object_get_value},
    ARRAY_SIZE(Object_props),
    Object_props,
    Object_destructor,
    NULL
};

static const builtin_info_t ObjectInst_info = {
    JSCLASS_OBJECT,
    {NULL, NULL,0, Object_get_value},
    0, NULL,
    Object_destructor,
    NULL
};

static void release_property_descriptor(property_desc_t *desc)
{
    if(desc->explicit_value)
        jsval_release(desc->value);
    if(desc->getter)
        jsdisp_release(desc->getter);
    if(desc->setter)
        jsdisp_release(desc->setter);
}

static HRESULT to_property_descriptor(script_ctx_t *ctx, jsdisp_t *attr_obj, property_desc_t *desc)
{
    DISPID id;
    jsval_t v;
    BOOL b;
    HRESULT hres;

    memset(desc, 0, sizeof(*desc));
    desc->value = jsval_undefined();

    hres = jsdisp_get_id(attr_obj, enumerableW, 0, &id);
    if(SUCCEEDED(hres)) {
        desc->mask |= PROPF_ENUMERABLE;
        hres = jsdisp_propget(attr_obj, id, &v);
        if(FAILED(hres))
            return hres;
        hres = to_boolean(v, &b);
        jsval_release(v);
        if(FAILED(hres))
            return hres;
        if(b)
            desc->flags |= PROPF_ENUMERABLE;
    }else if(hres != DISP_E_UNKNOWNNAME) {
        return hres;
    }

    hres = jsdisp_get_id(attr_obj, configurableW, 0, &id);
    if(SUCCEEDED(hres)) {
        desc->mask |= PROPF_CONFIGURABLE;
        hres = jsdisp_propget(attr_obj, id, &v);
        if(FAILED(hres))
            return hres;
        hres = to_boolean(v, &b);
        jsval_release(v);
        if(FAILED(hres))
            return hres;
        if(b)
            desc->flags |= PROPF_CONFIGURABLE;
    }else if(hres != DISP_E_UNKNOWNNAME) {
        return hres;
    }

    hres = jsdisp_get_id(attr_obj, valueW, 0, &id);
    if(SUCCEEDED(hres)) {
        hres = jsdisp_propget(attr_obj, id, &desc->value);
        if(FAILED(hres))
            return hres;
        desc->explicit_value = TRUE;
    }else if(hres != DISP_E_UNKNOWNNAME) {
        return hres;
    }

    hres = jsdisp_get_id(attr_obj, writableW, 0, &id);
    if(SUCCEEDED(hres)) {
        desc->mask |= PROPF_WRITABLE;
        hres = jsdisp_propget(attr_obj, id, &v);
        if(SUCCEEDED(hres)) {
            hres = to_boolean(v, &b);
            jsval_release(v);
            if(SUCCEEDED(hres) && b)
                desc->flags |= PROPF_WRITABLE;
        }
    }else if(hres == DISP_E_UNKNOWNNAME) {
        hres = S_OK;
    }
    if(FAILED(hres)) {
        release_property_descriptor(desc);
        return hres;
    }

    hres = jsdisp_get_id(attr_obj, getW, 0, &id);
    if(SUCCEEDED(hres)) {
        desc->explicit_getter = TRUE;
        hres = jsdisp_propget(attr_obj, id, &v);
        if(SUCCEEDED(hres) && !is_undefined(v)) {
            if(!is_object_instance(v)) {
                FIXME("getter is not an object\n");
                jsval_release(v);
                hres = E_FAIL;
            }else {
                /* FIXME: Check IsCallable */
                desc->getter = to_jsdisp(get_object(v));
                if(!desc->getter)
                    FIXME("getter is not JS object\n");
            }
        }
    }else if(hres == DISP_E_UNKNOWNNAME) {
        hres = S_OK;
    }
    if(FAILED(hres)) {
        release_property_descriptor(desc);
        return hres;
    }

    hres = jsdisp_get_id(attr_obj, setW, 0, &id);
    if(SUCCEEDED(hres)) {
        desc->explicit_setter = TRUE;
        hres = jsdisp_propget(attr_obj, id, &v);
        if(SUCCEEDED(hres) && !is_undefined(v)) {
            if(!is_object_instance(v)) {
                FIXME("setter is not an object\n");
                jsval_release(v);
                hres = E_FAIL;
            }else {
                /* FIXME: Check IsCallable */
                desc->setter = to_jsdisp(get_object(v));
                if(!desc->setter)
                    FIXME("setter is not JS object\n");
            }
        }
    }else if(hres == DISP_E_UNKNOWNNAME) {
        hres = S_OK;
    }
    if(FAILED(hres)) {
        release_property_descriptor(desc);
        return hres;
    }

    if(desc->explicit_getter || desc->explicit_setter) {
        if(desc->explicit_value)
            hres = throw_type_error(ctx, JS_E_PROP_DESC_MISMATCH, NULL);
        else if(desc->mask & PROPF_WRITABLE)
            hres = throw_type_error(ctx, JS_E_INVALID_WRITABLE_PROP_DESC, NULL);
    }

    if(FAILED(hres))
        release_property_descriptor(desc);
    return hres;
}

static HRESULT Object_defineProperty(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
                                     unsigned argc, jsval_t *argv, jsval_t *r)
{
    property_desc_t prop_desc;
    jsdisp_t *obj, *attr_obj;
    const WCHAR *name;
    jsstr_t *name_str;
    HRESULT hres;

    TRACE("\n");

    if(argc < 1 || !is_object_instance(argv[0]))
        return throw_type_error(ctx, JS_E_OBJECT_EXPECTED, NULL);
    obj = to_jsdisp(get_object(argv[0]));
    if(!obj) {
        FIXME("not implemented non-JS object\n");
        return E_NOTIMPL;
    }

    hres = to_flat_string(ctx, argc >= 2 ? argv[1] : jsval_undefined(), &name_str, &name);
    if(FAILED(hres))
        return hres;

    if(argc >= 3 && is_object_instance(argv[2])) {
        attr_obj = to_jsdisp(get_object(argv[2]));
        if(attr_obj) {
            hres = to_property_descriptor(ctx, attr_obj, &prop_desc);
        }else {
            FIXME("not implemented non-JS object\n");
            hres = E_NOTIMPL;
        }
    }else {
        hres = throw_type_error(ctx, JS_E_OBJECT_EXPECTED, NULL);
    }
    jsstr_release(name_str);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_property(obj, name, &prop_desc);
    release_property_descriptor(&prop_desc);
    return hres;
}

static HRESULT Object_defineProperties(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
                                     unsigned argc, jsval_t *argv, jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Object_getOwnPropertyDescriptor(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
                                               unsigned argc, jsval_t *argv, jsval_t *r)
{
    property_desc_t prop_desc;
    jsdisp_t *obj, *desc_obj;
    const WCHAR *name;
    jsstr_t *name_str;
    HRESULT hres;

    TRACE("\n");

    if(argc < 1 || !is_object_instance(argv[0]))
        return throw_type_error(ctx, JS_E_OBJECT_EXPECTED, NULL);
    obj = to_jsdisp(get_object(argv[0]));
    if(!obj) {
        FIXME("not implemented non-JS object\n");
        return E_NOTIMPL;
    }

    hres = to_flat_string(ctx, argc >= 2 ? argv[1] : jsval_undefined(), &name_str, &name);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_get_own_property(obj, name, FALSE, &prop_desc);
    jsstr_release(name_str);
    if(hres == DISP_E_UNKNOWNNAME) {
        if(r) *r = jsval_undefined();
        return S_OK;
    }
    if(FAILED(hres))
        return hres;

    hres = create_object(ctx, NULL, &desc_obj);
    if(FAILED(hres))
        return hres;

    if(prop_desc.explicit_getter || prop_desc.explicit_setter) {
        hres = jsdisp_define_data_property(desc_obj, getW, PROPF_ALL,
                prop_desc.getter ? jsval_obj(prop_desc.getter) : jsval_undefined());
        if(SUCCEEDED(hres))
            hres = jsdisp_define_data_property(desc_obj, setW, PROPF_ALL,
                    prop_desc.setter ? jsval_obj(prop_desc.setter) : jsval_undefined());
    }else {
        hres = jsdisp_propput_name(desc_obj, valueW, prop_desc.value);
        if(SUCCEEDED(hres))
            hres = jsdisp_define_data_property(desc_obj, writableW, PROPF_ALL,
                    jsval_bool(!!(prop_desc.flags & PROPF_WRITABLE)));
    }
    if(SUCCEEDED(hres))
        hres = jsdisp_define_data_property(desc_obj, enumerableW, PROPF_ALL,
                jsval_bool(!!(prop_desc.flags & PROPF_ENUMERABLE)));
    if(SUCCEEDED(hres))
        hres = jsdisp_define_data_property(desc_obj, configurableW, PROPF_ALL,
                jsval_bool(!!(prop_desc.flags & PROPF_CONFIGURABLE)));

    release_property_descriptor(&prop_desc);
    if(SUCCEEDED(hres) && r)
        *r = jsval_obj(desc_obj);
    else
        jsdisp_release(desc_obj);
    return hres;
}

static HRESULT Object_create(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
                             unsigned argc, jsval_t *argv, jsval_t *r)
{
    jsdisp_t *proto = NULL, *obj;
    HRESULT hres;

    if(!argc || (!is_object_instance(argv[0]) && !is_null(argv[0]))) {
        FIXME("Invalid arg\n");
        return E_INVALIDARG;
    }

    TRACE("(%s)\n", debugstr_jsval(argv[0]));

    if(argc > 1) {
        FIXME("Unsupported properties argument %s\n", debugstr_jsval(argv[1]));
        return E_NOTIMPL;
    }

    if(argc && is_object_instance(argv[0])) {
        if(get_object(argv[0]))
            proto = to_jsdisp(get_object(argv[0]));
        if(!proto) {
            FIXME("Non-JS prototype\n");
            return E_NOTIMPL;
        }
    }else if(!is_null(argv[0])) {
        FIXME("Invalid arg %s\n", debugstr_jsval(argc ? argv[0] : jsval_undefined()));
        return E_INVALIDARG;
    }

    if(r) {
        hres = create_dispex(ctx, NULL, proto, &obj);
        if(FAILED(hres))
            return hres;
        *r = jsval_obj(obj);
    }
    return S_OK;
}

static HRESULT Object_getPrototypeOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
                                     unsigned argc, jsval_t *argv, jsval_t *r)
{
    jsdisp_t *obj;

    if(!argc || !is_object_instance(argv[0])) {
        FIXME("invalid arguments\n");
        return E_NOTIMPL;
    }

    TRACE("(%s)\n", debugstr_jsval(argv[1]));

    obj = to_jsdisp(get_object(argv[0]));
    if(!obj) {
        FIXME("Non-JS object\n");
        return E_NOTIMPL;
    }

    if(r)
        *r = obj->prototype
            ? jsval_obj(jsdisp_addref(obj->prototype))
            : jsval_null();
    return S_OK;
}

static const builtin_prop_t ObjectConstr_props[] = {
    {createW,                   Object_create,                      PROPF_ES5|PROPF_METHOD|2},
    {definePropertiesW,         Object_defineProperties,            PROPF_ES5|PROPF_METHOD|2},
    {definePropertyW,           Object_defineProperty,              PROPF_ES5|PROPF_METHOD|2},
    {getOwnPropertyDescriptorW, Object_getOwnPropertyDescriptor,    PROPF_ES5|PROPF_METHOD|2},
    {getPrototypeOfW,           Object_getPrototypeOf,              PROPF_ES5|PROPF_METHOD|1}
};

static const builtin_info_t ObjectConstr_info = {
    JSCLASS_FUNCTION,
    DEFAULT_FUNCTION_VALUE,
    ARRAY_SIZE(ObjectConstr_props),
    ObjectConstr_props,
    NULL,
    NULL
};

static HRESULT ObjectConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
    case DISPATCH_CONSTRUCT: {
        jsdisp_t *obj;

        if(argc) {
            if(!is_undefined(argv[0]) && !is_null(argv[0]) && (!is_object_instance(argv[0]) || get_object(argv[0]))) {
                IDispatch *disp;

                hres = to_object(ctx, argv[0], &disp);
                if(FAILED(hres))
                    return hres;

                if(r)
                    *r = jsval_disp(disp);
                else
                    IDispatch_Release(disp);
                return S_OK;
            }
        }

        hres = create_object(ctx, NULL, &obj);
        if(FAILED(hres))
            return hres;

        if(r)
            *r = jsval_obj(obj);
        else
            jsdisp_release(obj);
        break;
    }

    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT create_object_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    static const WCHAR ObjectW[] = {'O','b','j','e','c','t',0};

    return create_builtin_constructor(ctx, ObjectConstr_value, ObjectW, &ObjectConstr_info, PROPF_CONSTR,
            object_prototype, ret);
}

HRESULT create_object_prototype(script_ctx_t *ctx, jsdisp_t **ret)
{
    return create_dispex(ctx, &Object_info, NULL, ret);
}

HRESULT create_object(script_ctx_t *ctx, jsdisp_t *constr, jsdisp_t **ret)
{
    jsdisp_t *object;
    HRESULT hres;

    object = heap_alloc_zero(sizeof(jsdisp_t));
    if(!object)
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(object, ctx, &ObjectInst_info, constr ? constr : ctx->object_constr);
    if(FAILED(hres)) {
        heap_free(object);
        return hres;
    }

    *ret = object;
    return S_OK;
}
