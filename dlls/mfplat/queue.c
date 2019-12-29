/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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
#define NONAMELESSUNION

#include "wine/debug.h"
#include "wine/list.h"

#include "mfplat_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

#define FIRST_USER_QUEUE_HANDLE 5
#define MAX_USER_QUEUE_HANDLES 124

#define WAIT_ITEM_KEY_MASK      (0x82000000)
#define SCHEDULED_ITEM_KEY_MASK (0x80000000)

static LONG next_item_key;

static MFWORKITEM_KEY get_item_key(DWORD mask, DWORD key)
{
    return ((MFWORKITEM_KEY)mask << 32) | key;
}

static MFWORKITEM_KEY generate_item_key(DWORD mask)
{
    return get_item_key(mask, InterlockedIncrement(&next_item_key));
}

struct work_item
{
    struct list entry;
    LONG refcount;
    IMFAsyncResult *result;
    struct queue *queue;
    MFWORKITEM_KEY key;
    union
    {
        TP_WAIT *wait_object;
        TP_TIMER *timer_object;
    } u;
};

static const TP_CALLBACK_PRIORITY priorities[] =
{
    TP_CALLBACK_PRIORITY_HIGH,
    TP_CALLBACK_PRIORITY_NORMAL,
    TP_CALLBACK_PRIORITY_LOW,
};

struct queue
{
    TP_POOL *pool;
    TP_CALLBACK_ENVIRON_V3 envs[ARRAY_SIZE(priorities)];
    CRITICAL_SECTION cs;
    struct list pending_items;
};

struct queue_handle
{
    void *obj;
    LONG refcount;
    WORD generation;
};

static struct queue_handle user_queues[MAX_USER_QUEUE_HANDLES];
static struct queue_handle *next_free_user_queue;
static struct queue_handle *next_unused_user_queue = user_queues;
static WORD queue_generation;

static CRITICAL_SECTION queues_section;
static CRITICAL_SECTION_DEBUG queues_critsect_debug =
{
    0, 0, &queues_section,
    { &queues_critsect_debug.ProcessLocksList, &queues_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": queues_section") }
};
static CRITICAL_SECTION queues_section = { &queues_critsect_debug, -1, 0, 0, 0, 0 };

static struct queue_handle *get_queue_obj(DWORD handle)
{
    unsigned int idx = HIWORD(handle) - FIRST_USER_QUEUE_HANDLE;

    if (idx < MAX_USER_QUEUE_HANDLES && user_queues[idx].refcount)
    {
        if (LOWORD(handle) == user_queues[idx].generation)
            return &user_queues[idx];
    }

    return NULL;
}

enum system_queue_index
{
    SYS_QUEUE_STANDARD = 0,
    SYS_QUEUE_RT,
    SYS_QUEUE_IO,
    SYS_QUEUE_TIMER,
    SYS_QUEUE_MULTITHREADED,
    SYS_QUEUE_DO_NOT_USE,
    SYS_QUEUE_LONG_FUNCTION,
    SYS_QUEUE_COUNT,
};

static struct queue system_queues[SYS_QUEUE_COUNT];

static struct queue *get_system_queue(DWORD queue_id)
{
    switch (queue_id)
    {
        case MFASYNC_CALLBACK_QUEUE_STANDARD:
        case MFASYNC_CALLBACK_QUEUE_RT:
        case MFASYNC_CALLBACK_QUEUE_IO:
        case MFASYNC_CALLBACK_QUEUE_TIMER:
        case MFASYNC_CALLBACK_QUEUE_MULTITHREADED:
        case MFASYNC_CALLBACK_QUEUE_LONG_FUNCTION:
            return &system_queues[queue_id - 1];
        default:
            return NULL;
    }
}

static void CALLBACK standard_queue_cleanup_callback(void *object_data, void *group_data)
{
}

static struct work_item * alloc_work_item(struct queue *queue, IMFAsyncResult *result)
{
    struct work_item *item;

    item = heap_alloc_zero(sizeof(*item));
    item->result = result;
    IMFAsyncResult_AddRef(item->result);
    item->refcount = 1;
    item->queue = queue;
    list_init(&item->entry);

    return item;
}

static void release_work_item(struct work_item *item)
{
    if (InterlockedDecrement(&item->refcount) == 0)
    {
         IMFAsyncResult_Release(item->result);
         heap_free(item);
    }
}

static void init_work_queue(MFASYNC_WORKQUEUE_TYPE queue_type, struct queue *queue)
{
    TP_CALLBACK_ENVIRON_V3 env;
    unsigned int max_thread, i;

    queue->pool = CreateThreadpool(NULL);

    memset(&env, 0, sizeof(env));
    env.Version = 3;
    env.Size = sizeof(env);
    env.Pool = queue->pool;
    env.CleanupGroup = CreateThreadpoolCleanupGroup();
    env.CleanupGroupCancelCallback = standard_queue_cleanup_callback;
    env.CallbackPriority = TP_CALLBACK_PRIORITY_NORMAL;
    for (i = 0; i < ARRAY_SIZE(queue->envs); ++i)
    {
        queue->envs[i] = env;
        queue->envs[i].CallbackPriority = priorities[i];
    }
    list_init(&queue->pending_items);
    InitializeCriticalSection(&queue->cs);

    max_thread = (queue_type == MF_STANDARD_WORKQUEUE || queue_type == MF_WINDOW_WORKQUEUE) ? 1 : 4;

    SetThreadpoolThreadMinimum(queue->pool, 1);
    SetThreadpoolThreadMaximum(queue->pool, max_thread);

    if (queue_type == MF_WINDOW_WORKQUEUE)
        FIXME("MF_WINDOW_WORKQUEUE is not supported.\n");
}

static HRESULT grab_queue(DWORD queue_id, struct queue **ret)
{
    struct queue *queue = get_system_queue(queue_id);
    MFASYNC_WORKQUEUE_TYPE queue_type;
    struct queue_handle *entry;

    *ret = NULL;

    if (!system_queues[SYS_QUEUE_STANDARD].pool)
        return MF_E_SHUTDOWN;

    if (queue && queue->pool)
    {
        *ret = queue;
        return S_OK;
    }
    else if (queue)
    {
        EnterCriticalSection(&queues_section);
        switch (queue_id)
        {
            case MFASYNC_CALLBACK_QUEUE_IO:
            case MFASYNC_CALLBACK_QUEUE_MULTITHREADED:
            case MFASYNC_CALLBACK_QUEUE_LONG_FUNCTION:
                queue_type = MF_MULTITHREADED_WORKQUEUE;
                break;
            default:
                queue_type = MF_STANDARD_WORKQUEUE;
        }
        init_work_queue(queue_type, queue);
        LeaveCriticalSection(&queues_section);
        *ret = queue;
        return S_OK;
    }

    /* Handles user queues. */
    if ((entry = get_queue_obj(queue_id)))
        *ret = entry->obj;

    return *ret ? S_OK : MF_E_INVALID_WORKQUEUE;
}

void init_system_queues(void)
{
    /* Always initialize standard queue, keep the rest lazy. */

    EnterCriticalSection(&queues_section);

    if (system_queues[SYS_QUEUE_STANDARD].pool)
    {
        LeaveCriticalSection(&queues_section);
        return;
    }

    init_work_queue(MF_STANDARD_WORKQUEUE, &system_queues[SYS_QUEUE_STANDARD]);

    LeaveCriticalSection(&queues_section);
}

static HRESULT lock_user_queue(DWORD queue)
{
    HRESULT hr = MF_E_INVALID_WORKQUEUE;
    struct queue_handle *entry;

    if (!(queue & MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK))
        return S_OK;

    EnterCriticalSection(&queues_section);
    entry = get_queue_obj(queue);
    if (entry && entry->refcount)
    {
        entry->refcount++;
        hr = S_OK;
    }
    LeaveCriticalSection(&queues_section);
    return hr;
}

static void shutdown_queue(struct queue *queue)
{
    struct work_item *item, *item2;

    if (!queue->pool)
        return;

    CloseThreadpoolCleanupGroupMembers(queue->envs[0].CleanupGroup, TRUE, NULL);
    CloseThreadpool(queue->pool);
    queue->pool = NULL;

    EnterCriticalSection(&queue->cs);
    LIST_FOR_EACH_ENTRY_SAFE(item, item2, &queue->pending_items, struct work_item, entry)
    {
        list_remove(&item->entry);
        release_work_item(item);
    }
    LeaveCriticalSection(&queue->cs);

    DeleteCriticalSection(&queue->cs);
}

static HRESULT unlock_user_queue(DWORD queue)
{
    HRESULT hr = MF_E_INVALID_WORKQUEUE;
    struct queue_handle *entry;

    if (!(queue & MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK))
        return S_OK;

    EnterCriticalSection(&queues_section);
    entry = get_queue_obj(queue);
    if (entry && entry->refcount)
    {
        if (--entry->refcount == 0)
        {
            shutdown_queue((struct queue *)entry->obj);
            heap_free(entry->obj);
            entry->obj = next_free_user_queue;
            next_free_user_queue = entry;
        }
        hr = S_OK;
    }
    LeaveCriticalSection(&queues_section);
    return hr;
}

void shutdown_system_queues(void)
{
    unsigned int i;

    EnterCriticalSection(&queues_section);

    for (i = 0; i < ARRAY_SIZE(system_queues); ++i)
    {
        shutdown_queue(&system_queues[i]);
    }

    LeaveCriticalSection(&queues_section);
}

static struct work_item *grab_work_item(struct work_item *item)
{
    InterlockedIncrement(&item->refcount);
    return item;
}

static void CALLBACK standard_queue_worker(TP_CALLBACK_INSTANCE *instance, void *context, TP_WORK *work)
{
    struct work_item *item = context;
    MFASYNCRESULT *result = (MFASYNCRESULT *)item->result;

    TRACE("result object %p.\n", result);

    IMFAsyncCallback_Invoke(result->pCallback, item->result);

    release_work_item(item);
}

static HRESULT queue_submit_item(struct queue *queue, LONG priority, IMFAsyncResult *result)
{
    TP_CALLBACK_PRIORITY callback_priority;
    struct work_item *item;
    TP_WORK *work_object;

    if (!(item = alloc_work_item(queue, result)))
        return E_OUTOFMEMORY;

    if (priority == 0)
        callback_priority = TP_CALLBACK_PRIORITY_NORMAL;
    else if (priority < 0)
        callback_priority = TP_CALLBACK_PRIORITY_LOW;
    else
        callback_priority = TP_CALLBACK_PRIORITY_HIGH;
    work_object = CreateThreadpoolWork(standard_queue_worker, item,
            (TP_CALLBACK_ENVIRON *)&queue->envs[callback_priority]);
    SubmitThreadpoolWork(work_object);

    TRACE("dispatched %p.\n", result);

    return S_OK;
}

static HRESULT queue_put_work_item(DWORD queue_id, LONG priority, IMFAsyncResult *result)
{
    struct queue *queue;
    HRESULT hr;

    if (FAILED(hr = grab_queue(queue_id, &queue)))
        return hr;

    hr = queue_submit_item(queue, priority, result);

    return hr;
}

static HRESULT invoke_async_callback(IMFAsyncResult *result)
{
    MFASYNCRESULT *result_data = (MFASYNCRESULT *)result;
    DWORD queue = MFASYNC_CALLBACK_QUEUE_STANDARD, flags;
    HRESULT hr;

    if (FAILED(IMFAsyncCallback_GetParameters(result_data->pCallback, &flags, &queue)))
        queue = MFASYNC_CALLBACK_QUEUE_STANDARD;

    if (FAILED(lock_user_queue(queue)))
        queue = MFASYNC_CALLBACK_QUEUE_STANDARD;

    hr = queue_put_work_item(queue, 0, result);

    unlock_user_queue(queue);

    return hr;
}

static void queue_release_pending_item(struct work_item *item)
{
    EnterCriticalSection(&item->queue->cs);
    if (item->key)
    {
        list_remove(&item->entry);
        item->key = 0;
        release_work_item(item);
    }
    LeaveCriticalSection(&item->queue->cs);
}

static void CALLBACK waiting_item_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_WAIT *wait,
        TP_WAIT_RESULT wait_result)
{
    struct work_item *item = context;

    TRACE("result object %p.\n", item->result);

    invoke_async_callback(item->result);

    release_work_item(item);
}

static void CALLBACK waiting_item_cancelable_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_WAIT *wait,
        TP_WAIT_RESULT wait_result)
{
    struct work_item *item = context;

    TRACE("result object %p.\n", item->result);

    queue_release_pending_item(item);

    invoke_async_callback(item->result);

    release_work_item(item);
}

static void CALLBACK scheduled_item_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_TIMER *timer)
{
    struct work_item *item = context;

    TRACE("result object %p.\n", item->result);

    invoke_async_callback(item->result);

    release_work_item(item);
}

static void CALLBACK scheduled_item_cancelable_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_TIMER *timer)
{
    struct work_item *item = context;

    TRACE("result object %p.\n", item->result);

    queue_release_pending_item(item);

    invoke_async_callback(item->result);

    release_work_item(item);
}

static void CALLBACK periodic_item_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_TIMER *timer)
{
    struct work_item *item = grab_work_item(context);

    invoke_async_callback(item->result);

    release_work_item(item);
}

static void queue_mark_item_pending(DWORD mask, struct work_item *item, MFWORKITEM_KEY *key)
{
    *key = generate_item_key(mask);
    item->key = *key;

    EnterCriticalSection(&item->queue->cs);
    list_add_tail(&item->queue->pending_items, &item->entry);
    grab_work_item(item);
    LeaveCriticalSection(&item->queue->cs);
}

static HRESULT queue_submit_wait(struct queue *queue, HANDLE event, LONG priority, IMFAsyncResult *result,
        MFWORKITEM_KEY *key)
{
    PTP_WAIT_CALLBACK callback;
    struct work_item *item;

    if (!(item = alloc_work_item(queue, result)))
        return E_OUTOFMEMORY;

    if (key)
    {
        queue_mark_item_pending(WAIT_ITEM_KEY_MASK, item, key);
        callback = waiting_item_cancelable_callback;
    }
    else
        callback = waiting_item_callback;

    item->u.wait_object = CreateThreadpoolWait(callback, item,
            (TP_CALLBACK_ENVIRON *)&queue->envs[TP_CALLBACK_PRIORITY_NORMAL]);
    SetThreadpoolWait(item->u.wait_object, event, NULL);

    TRACE("dispatched %p.\n", result);

    return S_OK;
}

static HRESULT queue_submit_timer(struct queue *queue, IMFAsyncResult *result, INT64 timeout, DWORD period,
        MFWORKITEM_KEY *key)
{
    PTP_TIMER_CALLBACK callback;
    struct work_item *item;
    FILETIME filetime;
    LARGE_INTEGER t;

    if (!(item = alloc_work_item(queue, result)))
        return E_OUTOFMEMORY;

    if (key)
    {
        queue_mark_item_pending(SCHEDULED_ITEM_KEY_MASK, item, key);
    }

    if (period)
        callback = periodic_item_callback;
    else
        callback = key ? scheduled_item_cancelable_callback : scheduled_item_callback;

    t.QuadPart = timeout * 1000 * 10;
    filetime.dwLowDateTime = t.u.LowPart;
    filetime.dwHighDateTime = t.u.HighPart;

    item->u.timer_object = CreateThreadpoolTimer(callback, item,
            (TP_CALLBACK_ENVIRON *)&queue->envs[TP_CALLBACK_PRIORITY_NORMAL]);
    SetThreadpoolTimer(item->u.timer_object, &filetime, period, 0);

    TRACE("dispatched %p.\n", result);

    return S_OK;
}

static HRESULT queue_cancel_item(struct queue *queue, MFWORKITEM_KEY key)
{
    HRESULT hr = MF_E_NOT_FOUND;
    struct work_item *item;

    EnterCriticalSection(&queue->cs);
    LIST_FOR_EACH_ENTRY(item, &queue->pending_items, struct work_item, entry)
    {
        if (item->key == key)
        {
            key >>= 32;
            if ((key & WAIT_ITEM_KEY_MASK) == WAIT_ITEM_KEY_MASK)
                CloseThreadpoolWait(item->u.wait_object);
            else if ((key & SCHEDULED_ITEM_KEY_MASK) == SCHEDULED_ITEM_KEY_MASK)
                CloseThreadpoolTimer(item->u.timer_object);
            else
                WARN("Unknown item key mask %#x.\n", (DWORD)key);
            queue_release_pending_item(item);
            hr = S_OK;
            break;
        }
    }
    LeaveCriticalSection(&queue->cs);

    return hr;
}

static HRESULT alloc_user_queue(MFASYNC_WORKQUEUE_TYPE queue_type, DWORD *queue_id)
{
    struct queue_handle *entry;
    struct queue *queue;
    unsigned int idx;

    *queue_id = MFASYNC_CALLBACK_QUEUE_UNDEFINED;

    if (!is_platform_locked())
        return MF_E_SHUTDOWN;

    queue = heap_alloc_zero(sizeof(*queue));
    if (!queue)
        return E_OUTOFMEMORY;
    init_work_queue(queue_type, queue);

    EnterCriticalSection(&queues_section);

    entry = next_free_user_queue;
    if (entry)
        next_free_user_queue = entry->obj;
    else if (next_unused_user_queue < user_queues + MAX_USER_QUEUE_HANDLES)
        entry = next_unused_user_queue++;
    else
    {
        LeaveCriticalSection(&queues_section);
        heap_free(queue);
        WARN("Out of user queue handles.\n");
        return E_OUTOFMEMORY;
    }

    entry->refcount = 1;
    entry->obj = queue;
    if (++queue_generation == 0xffff) queue_generation = 1;
    entry->generation = queue_generation;
    idx = entry - user_queues + FIRST_USER_QUEUE_HANDLE;
    *queue_id = (idx << 16) | entry->generation;

    LeaveCriticalSection(&queues_section);

    return S_OK;
}

struct async_result
{
    MFASYNCRESULT result;
    LONG refcount;
    IUnknown *object;
    IUnknown *state;
};

static struct async_result *impl_from_IMFAsyncResult(IMFAsyncResult *iface)
{
    return CONTAINING_RECORD(iface, struct async_result, result.AsyncResult);
}

static HRESULT WINAPI async_result_QueryInterface(IMFAsyncResult *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFAsyncResult) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncResult_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI async_result_AddRef(IMFAsyncResult *iface)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);
    ULONG refcount = InterlockedIncrement(&result->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI async_result_Release(IMFAsyncResult *iface)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);
    ULONG refcount = InterlockedDecrement(&result->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    if (!refcount)
    {
        if (result->result.pCallback)
            IMFAsyncCallback_Release(result->result.pCallback);
        if (result->object)
            IUnknown_Release(result->object);
        if (result->state)
            IUnknown_Release(result->state);
        if (result->result.hEvent)
            CloseHandle(result->result.hEvent);
        heap_free(result);

        MFUnlockPlatform();
    }

    return refcount;
}

static HRESULT WINAPI async_result_GetState(IMFAsyncResult *iface, IUnknown **state)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p, %p.\n", iface, state);

    if (!result->state)
        return E_POINTER;

    *state = result->state;
    IUnknown_AddRef(*state);

    return S_OK;
}

static HRESULT WINAPI async_result_GetStatus(IMFAsyncResult *iface)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p.\n", iface);

    return result->result.hrStatusResult;
}

static HRESULT WINAPI async_result_SetStatus(IMFAsyncResult *iface, HRESULT status)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p, %#x.\n", iface, status);

    result->result.hrStatusResult = status;

    return S_OK;
}

static HRESULT WINAPI async_result_GetObject(IMFAsyncResult *iface, IUnknown **object)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p, %p.\n", iface, object);

    if (!result->object)
        return E_POINTER;

    *object = result->object;
    IUnknown_AddRef(*object);

    return S_OK;
}

static IUnknown * WINAPI async_result_GetStateNoAddRef(IMFAsyncResult *iface)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p.\n", iface);

    return result->state;
}

static const IMFAsyncResultVtbl async_result_vtbl =
{
    async_result_QueryInterface,
    async_result_AddRef,
    async_result_Release,
    async_result_GetState,
    async_result_GetStatus,
    async_result_SetStatus,
    async_result_GetObject,
    async_result_GetStateNoAddRef,
};

static HRESULT create_async_result(IUnknown *object, IMFAsyncCallback *callback, IUnknown *state, IMFAsyncResult **out)
{
    struct async_result *result;

    if (!out)
        return E_INVALIDARG;

    result = heap_alloc_zero(sizeof(*result));
    if (!result)
        return E_OUTOFMEMORY;

    MFLockPlatform();

    result->result.AsyncResult.lpVtbl = &async_result_vtbl;
    result->refcount = 1;
    result->object = object;
    if (result->object)
        IUnknown_AddRef(result->object);
    result->result.pCallback = callback;
    if (result->result.pCallback)
        IMFAsyncCallback_AddRef(result->result.pCallback);
    result->state = state;
    if (result->state)
        IUnknown_AddRef(result->state);

    *out = &result->result.AsyncResult;

    TRACE("Created async result object %p.\n", *out);

    return S_OK;
}

/***********************************************************************
 *      MFCreateAsyncResult (mfplat.@)
 */
HRESULT WINAPI MFCreateAsyncResult(IUnknown *object, IMFAsyncCallback *callback, IUnknown *state, IMFAsyncResult **out)
{
    TRACE("%p, %p, %p, %p.\n", object, callback, state, out);

    return create_async_result(object, callback, state, out);
}

/***********************************************************************
 *      MFAllocateWorkQueue (mfplat.@)
 */
HRESULT WINAPI MFAllocateWorkQueue(DWORD *queue)
{
    TRACE("%p.\n", queue);

    return alloc_user_queue(MF_STANDARD_WORKQUEUE, queue);
}

/***********************************************************************
 *      MFAllocateWorkQueueEx (mfplat.@)
 */
HRESULT WINAPI MFAllocateWorkQueueEx(MFASYNC_WORKQUEUE_TYPE queue_type, DWORD *queue)
{
    TRACE("%d, %p.\n", queue_type, queue);

    return alloc_user_queue(queue_type, queue);
}

/***********************************************************************
 *      MFLockWorkQueue (mfplat.@)
 */
HRESULT WINAPI MFLockWorkQueue(DWORD queue)
{
    TRACE("%#x.\n", queue);

    return lock_user_queue(queue);
}

/***********************************************************************
 *      MFUnlockWorkQueue (mfplat.@)
 */
HRESULT WINAPI MFUnlockWorkQueue(DWORD queue)
{
    TRACE("%#x.\n", queue);

    return unlock_user_queue(queue);
}

/***********************************************************************
 *      MFPutWorkItem (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItem(DWORD queue, IMFAsyncCallback *callback, IUnknown *state)
{
    IMFAsyncResult *result;
    HRESULT hr;

    TRACE("%#x, %p, %p.\n", queue, callback, state);

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &result)))
        return hr;

    hr = queue_put_work_item(queue, 0, result);

    IMFAsyncResult_Release(result);

    return hr;
}

/***********************************************************************
 *      MFPutWorkItem2 (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItem2(DWORD queue, LONG priority, IMFAsyncCallback *callback, IUnknown *state)
{
    IMFAsyncResult *result;
    HRESULT hr;

    TRACE("%#x, %d, %p, %p.\n", queue, priority, callback, state);

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &result)))
        return hr;

    hr = queue_put_work_item(queue, priority, result);

    IMFAsyncResult_Release(result);

    return hr;
}

/***********************************************************************
 *      MFPutWorkItemEx (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItemEx(DWORD queue, IMFAsyncResult *result)
{
    TRACE("%#x, %p\n", queue, result);

    return queue_put_work_item(queue, 0, result);
}

/***********************************************************************
 *      MFPutWorkItemEx2 (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItemEx2(DWORD queue, LONG priority, IMFAsyncResult *result)
{
    TRACE("%#x, %d, %p\n", queue, priority, result);

    return queue_put_work_item(queue, priority, result);
}

/***********************************************************************
 *      MFInvokeCallback (mfplat.@)
 */
HRESULT WINAPI MFInvokeCallback(IMFAsyncResult *result)
{
    TRACE("%p.\n", result);

    return invoke_async_callback(result);
}

static HRESULT schedule_work_item(IMFAsyncResult *result, INT64 timeout, MFWORKITEM_KEY *key)
{
    struct queue *queue;
    HRESULT hr;

    if (FAILED(hr = grab_queue(MFASYNC_CALLBACK_QUEUE_TIMER, &queue)))
        return hr;

    TRACE("%p, %s, %p.\n", result, wine_dbgstr_longlong(timeout), key);

    hr = queue_submit_timer(queue, result, timeout, 0, key);

    return hr;
}

/***********************************************************************
 *      MFScheduleWorkItemEx (mfplat.@)
 */
HRESULT WINAPI MFScheduleWorkItemEx(IMFAsyncResult *result, INT64 timeout, MFWORKITEM_KEY *key)
{
    TRACE("%p, %s, %p.\n", result, wine_dbgstr_longlong(timeout), key);

    return schedule_work_item(result, timeout, key);
}

/***********************************************************************
 *      MFScheduleWorkItemEx (mfplat.@)
 */
HRESULT WINAPI MFScheduleWorkItem(IMFAsyncCallback *callback, IUnknown *state, INT64 timeout, MFWORKITEM_KEY *key)
{
    IMFAsyncResult *result;
    HRESULT hr;

    TRACE("%p, %p, %s, %p.\n", callback, state, wine_dbgstr_longlong(timeout), key);

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &result)))
        return hr;

    hr = schedule_work_item(result, timeout, key);

    IMFAsyncResult_Release(result);

    return hr;
}

/***********************************************************************
 *      MFPutWaitingWorkItem (mfplat.@)
 */
HRESULT WINAPI MFPutWaitingWorkItem(HANDLE event, LONG priority, IMFAsyncResult *result, MFWORKITEM_KEY *key)
{
    struct queue *queue;
    HRESULT hr;

    TRACE("%p, %d, %p, %p.\n", event, priority, result, key);

    if (FAILED(hr = grab_queue(MFASYNC_CALLBACK_QUEUE_TIMER, &queue)))
        return hr;

    hr = queue_submit_wait(queue, event, priority, result, key);

    return hr;
}

/***********************************************************************
 *      MFCancelWorkItem (mfplat.@)
 */
HRESULT WINAPI MFCancelWorkItem(MFWORKITEM_KEY key)
{
    struct queue *queue;
    HRESULT hr;

    TRACE("%s.\n", wine_dbgstr_longlong(key));

    if (FAILED(hr = grab_queue(MFASYNC_CALLBACK_QUEUE_TIMER, &queue)))
        return hr;

    hr = queue_cancel_item(queue, key);

    return hr;
}

static DWORD get_timer_period(void)
{
    return 10;
}

/***********************************************************************
 *      MFGetTimerPeriodicity (mfplat.@)
 */
HRESULT WINAPI MFGetTimerPeriodicity(DWORD *period)
{
    TRACE("%p.\n", period);

    *period = get_timer_period();

    return S_OK;
}

struct periodic_callback
{
    IMFAsyncCallback IMFAsyncCallback_iface;
    LONG refcount;
    MFPERIODICCALLBACK callback;
};

static struct periodic_callback *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct periodic_callback, IMFAsyncCallback_iface);
}

static HRESULT WINAPI periodic_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI periodic_callback_AddRef(IMFAsyncCallback *iface)
{
    struct periodic_callback *callback = impl_from_IMFAsyncCallback(iface);
    ULONG refcount = InterlockedIncrement(&callback->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI periodic_callback_Release(IMFAsyncCallback *iface)
{
    struct periodic_callback *callback = impl_from_IMFAsyncCallback(iface);
    ULONG refcount = InterlockedDecrement(&callback->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    if (!refcount)
        heap_free(callback);

    return refcount;
}

static HRESULT WINAPI periodic_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI periodic_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct periodic_callback *callback = impl_from_IMFAsyncCallback(iface);
    IUnknown *context = NULL;

    if (FAILED(IMFAsyncResult_GetObject(result, &context)))
        WARN("Expected object to be set for result object.\n");

    callback->callback(context);

    if (context)
        IUnknown_Release(context);

    return S_OK;
}

static const IMFAsyncCallbackVtbl periodic_callback_vtbl =
{
    periodic_callback_QueryInterface,
    periodic_callback_AddRef,
    periodic_callback_Release,
    periodic_callback_GetParameters,
    periodic_callback_Invoke,
};

static HRESULT create_periodic_callback_obj(MFPERIODICCALLBACK callback, IMFAsyncCallback **out)
{
    struct periodic_callback *object;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFAsyncCallback_iface.lpVtbl = &periodic_callback_vtbl;
    object->refcount = 1;
    object->callback = callback;

    *out = &object->IMFAsyncCallback_iface;

    return S_OK;
}

/***********************************************************************
 *      MFAddPeriodicCallback (mfplat.@)
 */
HRESULT WINAPI MFAddPeriodicCallback(MFPERIODICCALLBACK callback, IUnknown *context, DWORD *key)
{
    IMFAsyncCallback *periodic_callback;
    MFWORKITEM_KEY workitem_key;
    IMFAsyncResult *result;
    struct queue *queue;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", callback, context, key);

    if (FAILED(hr = grab_queue(MFASYNC_CALLBACK_QUEUE_TIMER, &queue)))
        return hr;

    if (FAILED(hr = create_periodic_callback_obj(callback, &periodic_callback)))
        return hr;

    hr = create_async_result(context, periodic_callback, NULL, &result);
    IMFAsyncCallback_Release(periodic_callback);
    if (FAILED(hr))
        return hr;

    hr = queue_submit_timer(queue, result, 0, get_timer_period(), key ? &workitem_key : NULL);

    IMFAsyncResult_Release(result);

    if (key)
        *key = workitem_key;

    return S_OK;
}

/***********************************************************************
 *      MFRemovePeriodicCallback (mfplat.@)
 */
HRESULT WINAPI MFRemovePeriodicCallback(DWORD key)
{
    struct queue *queue;
    HRESULT hr;

    TRACE("%#x.\n", key);

    if (FAILED(hr = grab_queue(MFASYNC_CALLBACK_QUEUE_TIMER, &queue)))
        return hr;

    return queue_cancel_item(queue, get_item_key(SCHEDULED_ITEM_KEY_MASK, key));
}
