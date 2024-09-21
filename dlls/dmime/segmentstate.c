/*
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

static DWORD next_track_id;

struct track_entry
{
    struct list entry;

    IDirectMusicTrack *track;
    void *state_data;
    DWORD track_id;
};

static void track_entry_destroy(struct track_entry *entry)
{
    HRESULT hr;

    if (FAILED(hr = IDirectMusicTrack_EndPlay(entry->track, entry->state_data)))
        WARN("track %p EndPlay failed, hr %#lx\n", entry->track, hr);

    IDirectMusicTrack_Release(entry->track);
    free(entry);
}

struct segment_state
{
    IDirectMusicSegmentState8 IDirectMusicSegmentState8_iface;
    LONG ref;

    IDirectMusicSegment *segment;
    MUSIC_TIME start_time;
    MUSIC_TIME start_point;
    MUSIC_TIME end_point;
    MUSIC_TIME played;
    BOOL auto_download;
    DWORD repeats;

    struct list tracks;
};

static inline struct segment_state *impl_from_IDirectMusicSegmentState8(IDirectMusicSegmentState8 *iface)
{
    return CONTAINING_RECORD(iface, struct segment_state, IDirectMusicSegmentState8_iface);
}

static HRESULT WINAPI segment_state_QueryInterface(IDirectMusicSegmentState8 *iface, REFIID riid, void **ppobj)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

    if (!ppobj)
        return E_POINTER;

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectMusicSegmentState) ||
        IsEqualIID(riid, &IID_IDirectMusicSegmentState8))
    {
        IDirectMusicSegmentState8_AddRef(iface);
        *ppobj = &This->IDirectMusicSegmentState8_iface;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI segment_state_AddRef(IDirectMusicSegmentState8 *iface)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    return ref;
}

static ULONG WINAPI segment_state_Release(IDirectMusicSegmentState8 *iface)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    if (!ref)
    {
        segment_state_end_play((IDirectMusicSegmentState *)iface, NULL);
        if (This->segment) IDirectMusicSegment_Release(This->segment);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI segment_state_GetRepeats(IDirectMusicSegmentState8 *iface, DWORD *repeats)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %p): semi-stub\n", This, repeats);
    return S_OK;
}

static HRESULT WINAPI segment_state_GetSegment(IDirectMusicSegmentState8 *iface, IDirectMusicSegment **segment)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %p)\n", This, segment);

    if (!(*segment = This->segment)) return DMUS_E_NOT_FOUND;

    IDirectMusicSegment_AddRef(This->segment);
    return S_OK;
}

static HRESULT WINAPI segment_state_GetStartTime(IDirectMusicSegmentState8 *iface, MUSIC_TIME *start_time)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %p)\n", This, start_time);

    *start_time = This->start_time;
    return S_OK;
}

static HRESULT WINAPI segment_state_GetSeek(IDirectMusicSegmentState8 *iface, MUSIC_TIME *seek_time)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %p): semi-stub\n", This, seek_time);
    *seek_time = 0;
    return S_OK;
}

static HRESULT WINAPI segment_state_GetStartPoint(IDirectMusicSegmentState8 *iface, MUSIC_TIME *start_point)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %p)\n", This, start_point);

    *start_point = This->start_point;
    return S_OK;
}

static HRESULT WINAPI segment_state_SetTrackConfig(IDirectMusicSegmentState8 *iface,
        REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %s, %ld, %ld, %ld, %ld): stub\n", This, debugstr_dmguid(rguidTrackClassID), dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);
    return S_OK;
}

static HRESULT WINAPI segment_state_GetObjectInPath(IDirectMusicSegmentState8 *iface, DWORD dwPChannel,
        DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, void **ppObject)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %ld, %ld, %ld, %s, %ld, %s, %p): stub\n", This, dwPChannel, dwStage, dwBuffer, debugstr_dmguid(guidObject), dwIndex, debugstr_dmguid(iidInterface), ppObject);
    return S_OK;
}

static const IDirectMusicSegmentState8Vtbl segment_state_vtbl =
{
    segment_state_QueryInterface,
    segment_state_AddRef,
    segment_state_Release,
    segment_state_GetRepeats,
    segment_state_GetSegment,
    segment_state_GetStartTime,
    segment_state_GetSeek,
    segment_state_GetStartPoint,
    segment_state_SetTrackConfig,
    segment_state_GetObjectInPath,
};

/* for ClassFactory */
HRESULT create_dmsegmentstate(REFIID riid, void **ret_iface)
{
    struct segment_state *obj;
    HRESULT hr;

    *ret_iface = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicSegmentState8_iface.lpVtbl = &segment_state_vtbl;
    obj->ref = 1;
    obj->start_time = -1;
    list_init(&obj->tracks);

    TRACE("Created IDirectMusicSegmentState %p\n", obj);
    hr = IDirectMusicSegmentState8_QueryInterface(&obj->IDirectMusicSegmentState8_iface, riid, ret_iface);
    IDirectMusicSegmentState8_Release(&obj->IDirectMusicSegmentState8_iface);
    return hr;
}

HRESULT segment_state_create(IDirectMusicSegment *segment, MUSIC_TIME start_time,
        IDirectMusicPerformance8 *performance, IDirectMusicSegmentState **ret_iface)
{
    IDirectMusicSegmentState *iface;
    struct segment_state *This;
    IDirectMusicTrack *track;
    HRESULT hr;
    UINT i;

    TRACE("(%p, %lu, %p)\n", segment, start_time, ret_iface);

    if (FAILED(hr = create_dmsegmentstate(&IID_IDirectMusicSegmentState, (void **)&iface))) return hr;
    This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);

    This->segment = segment;
    IDirectMusicSegment_AddRef(This->segment);

    if (SUCCEEDED(hr = IDirectMusicPerformance8_GetGlobalParam(performance, &GUID_PerfAutoDownload,
            &This->auto_download, sizeof(This->auto_download))) && This->auto_download)
        hr = IDirectMusicSegment_SetParam(segment, &GUID_DownloadToAudioPath, -1,
                    DMUS_SEG_ALLTRACKS, 0, performance);

    This->start_time = start_time;
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment_GetStartPoint(segment, &This->start_point);
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment_GetLength(segment, &This->end_point);
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment_GetRepeats(segment, &This->repeats);

    for (i = 0; SUCCEEDED(hr); i++)
    {
        DWORD track_id = ++next_track_id;
        struct track_entry *entry;

        if ((hr = IDirectMusicSegment_GetTrack(segment, &GUID_NULL, -1, i, &track)) != S_OK)
        {
            if (hr == DMUS_E_NOT_FOUND) hr = S_OK;
            break;
        }

        if (!(entry = malloc(sizeof(*entry))))
            hr = E_OUTOFMEMORY;
        else if (SUCCEEDED(hr = IDirectMusicTrack_InitPlay(track, iface, (IDirectMusicPerformance *)performance,
                &entry->state_data, track_id, 0)))
        {
            entry->track = track;
            entry->track_id = track_id;
            list_add_tail(&This->tracks, &entry->entry);
        }

        if (FAILED(hr))
        {
            WARN("Failed to initialize track %p, hr %#lx\n", track, hr);
            IDirectMusicTrack_Release(track);
            free(entry);
        }
    }

    if (SUCCEEDED(hr)) *ret_iface = iface;
    else IDirectMusicSegmentState_Release(iface);
    return hr;
}

static HRESULT segment_state_play_until(struct segment_state *This, IDirectMusicPerformance8 *performance,
        MUSIC_TIME end_time, DWORD track_flags)
{
    IDirectMusicSegmentState *iface = (IDirectMusicSegmentState *)&This->IDirectMusicSegmentState8_iface;
    struct track_entry *entry;
    HRESULT hr = S_OK;
    MUSIC_TIME played;

    played = min(end_time - This->start_time, This->end_point - This->start_point);

    LIST_FOR_EACH_ENTRY(entry, &This->tracks, struct track_entry, entry)
    {
        if (FAILED(hr = IDirectMusicTrack_Play(entry->track, entry->state_data,
                This->start_point + This->played, This->start_point + played,
                This->start_time, track_flags, (IDirectMusicPerformance *)performance,
                iface, entry->track_id)))
        {
            WARN("Failed to play track %p, hr %#lx\n", entry->track, hr);
            break;
        }
    }

    This->played = played;
    if (This->start_point + This->played >= This->end_point) return S_FALSE;
    return S_OK;
}

static HRESULT segment_state_play_chunk(struct segment_state *This, IDirectMusicPerformance8 *performance,
        REFERENCE_TIME duration, DWORD track_flags)
{
    IDirectMusicSegmentState *iface = (IDirectMusicSegmentState *)&This->IDirectMusicSegmentState8_iface;
    MUSIC_TIME next_time;
    REFERENCE_TIME time;
    HRESULT hr;

    if (FAILED(hr = IDirectMusicPerformance8_MusicToReferenceTime(performance,
            This->start_time + This->played, &time)))
        return hr;
    if (FAILED(hr = IDirectMusicPerformance8_ReferenceToMusicTime(performance,
            time + duration, &next_time)))
        return hr;

    while ((hr = segment_state_play_until(This, performance, next_time, track_flags)) == S_FALSE)
    {
        if (!This->repeats)
        {
            MUSIC_TIME end_time = This->start_time + This->played;

            if (FAILED(hr = performance_send_segment_end(performance, end_time, iface, FALSE)))
            {
                ERR("Failed to send segment end, hr %#lx\n", hr);
                return hr;
            }

            return S_FALSE;
        }

        if (FAILED(hr = IDirectMusicSegment_GetLoopPoints(This->segment, &This->played,
                &This->end_point)))
            break;
        This->start_time += This->end_point - This->start_point;
        This->repeats--;

        if (next_time <= This->start_time || This->end_point <= This->start_point) break;
    }

    return S_OK;
}

HRESULT segment_state_play(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);
    HRESULT hr;

    TRACE("%p %p\n", iface, performance);

    if (FAILED(hr = performance_send_segment_start(performance, This->start_time, iface)))
    {
        ERR("Failed to send segment start, hr %#lx\n", hr);
        return hr;
    }

    if (FAILED(hr = segment_state_play_chunk(This, performance, 10000000,
            DMUS_TRACKF_START | DMUS_TRACKF_SEEK | DMUS_TRACKF_DIRTY)))
        return hr;

    if (hr == S_FALSE) return S_OK;
    return performance_send_segment_tick(performance, This->start_time, iface);
}

HRESULT segment_state_tick(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);

    TRACE("%p %p\n", iface, performance);

    return segment_state_play_chunk(This, performance, 10000000, 0);
}

HRESULT segment_state_stop(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);

    TRACE("%p %p\n", iface, performance);

    This->played = This->end_point - This->start_point;
    return performance_send_segment_end(performance, This->start_time + This->played, iface, TRUE);
}

HRESULT segment_state_end_play(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);
    struct track_entry *entry, *next;
    HRESULT hr;

    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &This->tracks, struct track_entry, entry)
    {
        list_remove(&entry->entry);
        track_entry_destroy(entry);
    }

    if (performance && This->auto_download && FAILED(hr = IDirectMusicSegment_SetParam(This->segment,
            &GUID_UnloadFromAudioPath, -1, DMUS_SEG_ALLTRACKS, 0, performance)))
        ERR("Failed to unload segment from performance, hr %#lx\n", hr);

    return S_OK;
}

BOOL segment_state_has_segment(IDirectMusicSegmentState *iface, IDirectMusicSegment *segment)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);
    return !segment || This->segment == segment;
}

BOOL segment_state_has_track(IDirectMusicSegmentState *iface, DWORD track_id)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);
    struct track_entry *entry;

    LIST_FOR_EACH_ENTRY(entry, &This->tracks, struct track_entry, entry)
        if (entry->track_id == track_id) return TRUE;

    return FALSE;
}
