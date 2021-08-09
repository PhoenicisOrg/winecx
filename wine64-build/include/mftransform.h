/*** Autogenerated by WIDL 5.0 from ../../include/mftransform.idl - Do not edit ***/

#ifdef _WIN32
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif
#include <rpc.h>
#include <rpcndr.h>
#endif

#ifndef COM_NO_WINDOWS_H
#include <windows.h>
#include <ole2.h>
#endif

#ifndef __mftransform_h__
#define __mftransform_h__

#ifdef __i386_on_x86_64__
#pragma clang default_addr_space(push, ptr32)
#pragma clang storage_addr_space(push, ptr32)
#endif
/* Forward declarations */

#ifndef __IMFTransform_FWD_DEFINED__
#define __IMFTransform_FWD_DEFINED__
typedef interface IMFTransform IMFTransform;
#ifdef __cplusplus
interface IMFTransform;
#endif /* __cplusplus */
#endif

/* Headers for imported files */

#include <mfobjects.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MFT_INPUT_STREAM_INFO {
    LONGLONG hnsMaxLatency;
    DWORD dwFlags;
    DWORD cbSize;
    DWORD cbMaxLookahead;
    DWORD cbAlignment;
} MFT_INPUT_STREAM_INFO;
typedef struct _MFT_OUTPUT_STREAM_INFO {
    DWORD dwFlags;
    DWORD cbSize;
    DWORD cbAlignment;
} MFT_OUTPUT_STREAM_INFO;
typedef struct _MFT_OUTPUT_DATA_BUFFER {
    DWORD dwStreamID;
    IMFSample *pSample;
    DWORD dwStatus;
    IMFCollection *pEvents;
} MFT_OUTPUT_DATA_BUFFER;
typedef struct _MFT_OUTPUT_DATA_BUFFER *PMFT_OUTPUT_DATA_BUFFER;
typedef enum _MFT_MESSAGE_TYPE {
    MFT_MESSAGE_COMMAND_FLUSH = 0x0,
    MFT_MESSAGE_COMMAND_DRAIN = 0x1,
    MFT_MESSAGE_SET_D3D_MANAGER = 0x2,
    MFT_MESSAGE_DROP_SAMPLES = 0x3,
    MFT_MESSAGE_COMMAND_TICK = 0x4,
    MFT_MESSAGE_NOTIFY_BEGIN_STREAMING = 0x10000000,
    MFT_MESSAGE_NOTIFY_END_STREAMING = 0x10000001,
    MFT_MESSAGE_NOTIFY_END_OF_STREAM = 0x10000002,
    MFT_MESSAGE_NOTIFY_START_OF_STREAM = 0x10000003,
    MFT_MESSAGE_COMMAND_MARKER = 0x20000000
} MFT_MESSAGE_TYPE;
enum _MFT_SET_TYPE_FLAGS {
    MFT_SET_TYPE_TEST_ONLY = 0x1
};
enum _MFT_INPUT_STATUS_FLAGS {
    MFT_INPUT_STATUS_ACCEPT_DATA = 0x1
};
/*****************************************************************************
 * IMFTransform interface
 */
#ifndef __IMFTransform_INTERFACE_DEFINED__
#define __IMFTransform_INTERFACE_DEFINED__

DEFINE_GUID(IID_IMFTransform, 0xbf94c121, 0x5b05, 0x4e6f, 0x80,0x00, 0xba,0x59,0x89,0x61,0x41,0x4d);
#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("bf94c121-5b05-4e6f-8000-ba598961414d")
IMFTransform : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetStreamLimits(
        DWORD *input_minimum,
        DWORD *input_maximum,
        DWORD *output_minimum,
        DWORD *output_maximum) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetStreamCount(
        DWORD *inputs,
        DWORD *outputs) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetStreamIDs(
        DWORD input_size,
        DWORD *inputs,
        DWORD output_size,
        DWORD *outputs) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetInputStreamInfo(
        DWORD id,
        MFT_INPUT_STREAM_INFO *info) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetOutputStreamInfo(
        DWORD id,
        MFT_OUTPUT_STREAM_INFO *info) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetAttributes(
        IMFAttributes **attributes) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetInputStreamAttributes(
        DWORD id,
        IMFAttributes **attributes) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetOutputStreamAttributes(
        DWORD id,
        IMFAttributes **attributes) = 0;

    virtual HRESULT STDMETHODCALLTYPE DeleteInputStream(
        DWORD id) = 0;

    virtual HRESULT STDMETHODCALLTYPE AddInputStreams(
        DWORD streams,
        DWORD *ids) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetInputAvailableType(
        DWORD id,
        DWORD index,
        IMFMediaType **type) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetOutputAvailableType(
        DWORD id,
        DWORD index,
        IMFMediaType **type) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetInputType(
        DWORD id,
        IMFMediaType *type,
        DWORD flags) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetOutputType(
        DWORD id,
        IMFMediaType *type,
        DWORD flags) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetInputCurrentType(
        DWORD id,
        IMFMediaType **type) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetOutputCurrentType(
        DWORD id,
        IMFMediaType **type) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetInputStatus(
        DWORD id,
        DWORD *flags) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetOutputStatus(
        DWORD *flags) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetOutputBounds(
        LONGLONG lower,
        LONGLONG upper) = 0;

    virtual HRESULT STDMETHODCALLTYPE ProcessEvent(
        DWORD id,
        IMFMediaEvent *event) = 0;

    virtual HRESULT STDMETHODCALLTYPE ProcessMessage(
        MFT_MESSAGE_TYPE message,
        ULONG_PTR param) = 0;

    virtual HRESULT STDMETHODCALLTYPE ProcessInput(
        DWORD id,
        IMFSample *sample,
        DWORD flags) = 0;

    virtual HRESULT STDMETHODCALLTYPE ProcessOutput(
        DWORD flags,
        DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples,
        DWORD *status) = 0;

};
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IMFTransform, 0xbf94c121, 0x5b05, 0x4e6f, 0x80,0x00, 0xba,0x59,0x89,0x61,0x41,0x4d)
#endif
#else
typedef struct IMFTransformVtbl {
    BEGIN_INTERFACE

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IMFTransform *This,
        REFIID riid,
        void **ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IMFTransform *This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IMFTransform *This);

    /*** IMFTransform methods ***/
    HRESULT (STDMETHODCALLTYPE *GetStreamLimits)(
        IMFTransform *This,
        DWORD *input_minimum,
        DWORD *input_maximum,
        DWORD *output_minimum,
        DWORD *output_maximum);

    HRESULT (STDMETHODCALLTYPE *GetStreamCount)(
        IMFTransform *This,
        DWORD *inputs,
        DWORD *outputs);

    HRESULT (STDMETHODCALLTYPE *GetStreamIDs)(
        IMFTransform *This,
        DWORD input_size,
        DWORD *inputs,
        DWORD output_size,
        DWORD *outputs);

    HRESULT (STDMETHODCALLTYPE *GetInputStreamInfo)(
        IMFTransform *This,
        DWORD id,
        MFT_INPUT_STREAM_INFO *info);

    HRESULT (STDMETHODCALLTYPE *GetOutputStreamInfo)(
        IMFTransform *This,
        DWORD id,
        MFT_OUTPUT_STREAM_INFO *info);

    HRESULT (STDMETHODCALLTYPE *GetAttributes)(
        IMFTransform *This,
        IMFAttributes **attributes);

    HRESULT (STDMETHODCALLTYPE *GetInputStreamAttributes)(
        IMFTransform *This,
        DWORD id,
        IMFAttributes **attributes);

    HRESULT (STDMETHODCALLTYPE *GetOutputStreamAttributes)(
        IMFTransform *This,
        DWORD id,
        IMFAttributes **attributes);

    HRESULT (STDMETHODCALLTYPE *DeleteInputStream)(
        IMFTransform *This,
        DWORD id);

    HRESULT (STDMETHODCALLTYPE *AddInputStreams)(
        IMFTransform *This,
        DWORD streams,
        DWORD *ids);

    HRESULT (STDMETHODCALLTYPE *GetInputAvailableType)(
        IMFTransform *This,
        DWORD id,
        DWORD index,
        IMFMediaType **type);

    HRESULT (STDMETHODCALLTYPE *GetOutputAvailableType)(
        IMFTransform *This,
        DWORD id,
        DWORD index,
        IMFMediaType **type);

    HRESULT (STDMETHODCALLTYPE *SetInputType)(
        IMFTransform *This,
        DWORD id,
        IMFMediaType *type,
        DWORD flags);

    HRESULT (STDMETHODCALLTYPE *SetOutputType)(
        IMFTransform *This,
        DWORD id,
        IMFMediaType *type,
        DWORD flags);

    HRESULT (STDMETHODCALLTYPE *GetInputCurrentType)(
        IMFTransform *This,
        DWORD id,
        IMFMediaType **type);

    HRESULT (STDMETHODCALLTYPE *GetOutputCurrentType)(
        IMFTransform *This,
        DWORD id,
        IMFMediaType **type);

    HRESULT (STDMETHODCALLTYPE *GetInputStatus)(
        IMFTransform *This,
        DWORD id,
        DWORD *flags);

    HRESULT (STDMETHODCALLTYPE *GetOutputStatus)(
        IMFTransform *This,
        DWORD *flags);

    HRESULT (STDMETHODCALLTYPE *SetOutputBounds)(
        IMFTransform *This,
        LONGLONG lower,
        LONGLONG upper);

    HRESULT (STDMETHODCALLTYPE *ProcessEvent)(
        IMFTransform *This,
        DWORD id,
        IMFMediaEvent *event);

    HRESULT (STDMETHODCALLTYPE *ProcessMessage)(
        IMFTransform *This,
        MFT_MESSAGE_TYPE message,
        ULONG_PTR param);

    HRESULT (STDMETHODCALLTYPE *ProcessInput)(
        IMFTransform *This,
        DWORD id,
        IMFSample *sample,
        DWORD flags);

    HRESULT (STDMETHODCALLTYPE *ProcessOutput)(
        IMFTransform *This,
        DWORD flags,
        DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples,
        DWORD *status);

    END_INTERFACE
} IMFTransformVtbl;

interface IMFTransform {
    CONST_VTBL IMFTransformVtbl* lpVtbl;
};

#ifdef COBJMACROS
#ifndef WIDL_C_INLINE_WRAPPERS
/*** IUnknown methods ***/
#define IMFTransform_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IMFTransform_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IMFTransform_Release(This) (This)->lpVtbl->Release(This)
/*** IMFTransform methods ***/
#define IMFTransform_GetStreamLimits(This,input_minimum,input_maximum,output_minimum,output_maximum) (This)->lpVtbl->GetStreamLimits(This,input_minimum,input_maximum,output_minimum,output_maximum)
#define IMFTransform_GetStreamCount(This,inputs,outputs) (This)->lpVtbl->GetStreamCount(This,inputs,outputs)
#define IMFTransform_GetStreamIDs(This,input_size,inputs,output_size,outputs) (This)->lpVtbl->GetStreamIDs(This,input_size,inputs,output_size,outputs)
#define IMFTransform_GetInputStreamInfo(This,id,info) (This)->lpVtbl->GetInputStreamInfo(This,id,info)
#define IMFTransform_GetOutputStreamInfo(This,id,info) (This)->lpVtbl->GetOutputStreamInfo(This,id,info)
#define IMFTransform_GetAttributes(This,attributes) (This)->lpVtbl->GetAttributes(This,attributes)
#define IMFTransform_GetInputStreamAttributes(This,id,attributes) (This)->lpVtbl->GetInputStreamAttributes(This,id,attributes)
#define IMFTransform_GetOutputStreamAttributes(This,id,attributes) (This)->lpVtbl->GetOutputStreamAttributes(This,id,attributes)
#define IMFTransform_DeleteInputStream(This,id) (This)->lpVtbl->DeleteInputStream(This,id)
#define IMFTransform_AddInputStreams(This,streams,ids) (This)->lpVtbl->AddInputStreams(This,streams,ids)
#define IMFTransform_GetInputAvailableType(This,id,index,type) (This)->lpVtbl->GetInputAvailableType(This,id,index,type)
#define IMFTransform_GetOutputAvailableType(This,id,index,type) (This)->lpVtbl->GetOutputAvailableType(This,id,index,type)
#define IMFTransform_SetInputType(This,id,type,flags) (This)->lpVtbl->SetInputType(This,id,type,flags)
#define IMFTransform_SetOutputType(This,id,type,flags) (This)->lpVtbl->SetOutputType(This,id,type,flags)
#define IMFTransform_GetInputCurrentType(This,id,type) (This)->lpVtbl->GetInputCurrentType(This,id,type)
#define IMFTransform_GetOutputCurrentType(This,id,type) (This)->lpVtbl->GetOutputCurrentType(This,id,type)
#define IMFTransform_GetInputStatus(This,id,flags) (This)->lpVtbl->GetInputStatus(This,id,flags)
#define IMFTransform_GetOutputStatus(This,flags) (This)->lpVtbl->GetOutputStatus(This,flags)
#define IMFTransform_SetOutputBounds(This,lower,upper) (This)->lpVtbl->SetOutputBounds(This,lower,upper)
#define IMFTransform_ProcessEvent(This,id,event) (This)->lpVtbl->ProcessEvent(This,id,event)
#define IMFTransform_ProcessMessage(This,message,param) (This)->lpVtbl->ProcessMessage(This,message,param)
#define IMFTransform_ProcessInput(This,id,sample,flags) (This)->lpVtbl->ProcessInput(This,id,sample,flags)
#define IMFTransform_ProcessOutput(This,flags,count,samples,status) (This)->lpVtbl->ProcessOutput(This,flags,count,samples,status)
#else
/*** IUnknown methods ***/
static FORCEINLINE HRESULT IMFTransform_QueryInterface(IMFTransform* This,REFIID riid,void **ppvObject) {
    return This->lpVtbl->QueryInterface(This,riid,ppvObject);
}
static FORCEINLINE ULONG IMFTransform_AddRef(IMFTransform* This) {
    return This->lpVtbl->AddRef(This);
}
static FORCEINLINE ULONG IMFTransform_Release(IMFTransform* This) {
    return This->lpVtbl->Release(This);
}
/*** IMFTransform methods ***/
static FORCEINLINE HRESULT IMFTransform_GetStreamLimits(IMFTransform* This,DWORD *input_minimum,DWORD *input_maximum,DWORD *output_minimum,DWORD *output_maximum) {
    return This->lpVtbl->GetStreamLimits(This,input_minimum,input_maximum,output_minimum,output_maximum);
}
static FORCEINLINE HRESULT IMFTransform_GetStreamCount(IMFTransform* This,DWORD *inputs,DWORD *outputs) {
    return This->lpVtbl->GetStreamCount(This,inputs,outputs);
}
static FORCEINLINE HRESULT IMFTransform_GetStreamIDs(IMFTransform* This,DWORD input_size,DWORD *inputs,DWORD output_size,DWORD *outputs) {
    return This->lpVtbl->GetStreamIDs(This,input_size,inputs,output_size,outputs);
}
static FORCEINLINE HRESULT IMFTransform_GetInputStreamInfo(IMFTransform* This,DWORD id,MFT_INPUT_STREAM_INFO *info) {
    return This->lpVtbl->GetInputStreamInfo(This,id,info);
}
static FORCEINLINE HRESULT IMFTransform_GetOutputStreamInfo(IMFTransform* This,DWORD id,MFT_OUTPUT_STREAM_INFO *info) {
    return This->lpVtbl->GetOutputStreamInfo(This,id,info);
}
static FORCEINLINE HRESULT IMFTransform_GetAttributes(IMFTransform* This,IMFAttributes **attributes) {
    return This->lpVtbl->GetAttributes(This,attributes);
}
static FORCEINLINE HRESULT IMFTransform_GetInputStreamAttributes(IMFTransform* This,DWORD id,IMFAttributes **attributes) {
    return This->lpVtbl->GetInputStreamAttributes(This,id,attributes);
}
static FORCEINLINE HRESULT IMFTransform_GetOutputStreamAttributes(IMFTransform* This,DWORD id,IMFAttributes **attributes) {
    return This->lpVtbl->GetOutputStreamAttributes(This,id,attributes);
}
static FORCEINLINE HRESULT IMFTransform_DeleteInputStream(IMFTransform* This,DWORD id) {
    return This->lpVtbl->DeleteInputStream(This,id);
}
static FORCEINLINE HRESULT IMFTransform_AddInputStreams(IMFTransform* This,DWORD streams,DWORD *ids) {
    return This->lpVtbl->AddInputStreams(This,streams,ids);
}
static FORCEINLINE HRESULT IMFTransform_GetInputAvailableType(IMFTransform* This,DWORD id,DWORD index,IMFMediaType **type) {
    return This->lpVtbl->GetInputAvailableType(This,id,index,type);
}
static FORCEINLINE HRESULT IMFTransform_GetOutputAvailableType(IMFTransform* This,DWORD id,DWORD index,IMFMediaType **type) {
    return This->lpVtbl->GetOutputAvailableType(This,id,index,type);
}
static FORCEINLINE HRESULT IMFTransform_SetInputType(IMFTransform* This,DWORD id,IMFMediaType *type,DWORD flags) {
    return This->lpVtbl->SetInputType(This,id,type,flags);
}
static FORCEINLINE HRESULT IMFTransform_SetOutputType(IMFTransform* This,DWORD id,IMFMediaType *type,DWORD flags) {
    return This->lpVtbl->SetOutputType(This,id,type,flags);
}
static FORCEINLINE HRESULT IMFTransform_GetInputCurrentType(IMFTransform* This,DWORD id,IMFMediaType **type) {
    return This->lpVtbl->GetInputCurrentType(This,id,type);
}
static FORCEINLINE HRESULT IMFTransform_GetOutputCurrentType(IMFTransform* This,DWORD id,IMFMediaType **type) {
    return This->lpVtbl->GetOutputCurrentType(This,id,type);
}
static FORCEINLINE HRESULT IMFTransform_GetInputStatus(IMFTransform* This,DWORD id,DWORD *flags) {
    return This->lpVtbl->GetInputStatus(This,id,flags);
}
static FORCEINLINE HRESULT IMFTransform_GetOutputStatus(IMFTransform* This,DWORD *flags) {
    return This->lpVtbl->GetOutputStatus(This,flags);
}
static FORCEINLINE HRESULT IMFTransform_SetOutputBounds(IMFTransform* This,LONGLONG lower,LONGLONG upper) {
    return This->lpVtbl->SetOutputBounds(This,lower,upper);
}
static FORCEINLINE HRESULT IMFTransform_ProcessEvent(IMFTransform* This,DWORD id,IMFMediaEvent *event) {
    return This->lpVtbl->ProcessEvent(This,id,event);
}
static FORCEINLINE HRESULT IMFTransform_ProcessMessage(IMFTransform* This,MFT_MESSAGE_TYPE message,ULONG_PTR param) {
    return This->lpVtbl->ProcessMessage(This,message,param);
}
static FORCEINLINE HRESULT IMFTransform_ProcessInput(IMFTransform* This,DWORD id,IMFSample *sample,DWORD flags) {
    return This->lpVtbl->ProcessInput(This,id,sample,flags);
}
static FORCEINLINE HRESULT IMFTransform_ProcessOutput(IMFTransform* This,DWORD flags,DWORD count,MFT_OUTPUT_DATA_BUFFER *samples,DWORD *status) {
    return This->lpVtbl->ProcessOutput(This,flags,count,samples,status);
}
#endif
#endif

#endif


#endif  /* __IMFTransform_INTERFACE_DEFINED__ */

/* Begin additional prototypes for all interfaces */


/* End additional prototypes */

#ifdef __cplusplus
}
#endif

#ifdef __i386_on_x86_64__
#pragma clang default_addr_space(pop)
#pragma clang storage_addr_space(pop)
#endif
#endif /* __mftransform_h__ */
