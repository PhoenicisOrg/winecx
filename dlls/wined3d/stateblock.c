/*
 * state block implementation
 *
 * Copyright 2002 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
 * Copyright 2005 Oliver Stieber
 * Copyright 2007 Stefan Dösinger for CodeWeavers
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2019,2020,2022 Zebediah Figura for CodeWeavers
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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

struct wined3d_saved_states
{
    uint32_t vs_consts_f[WINED3D_BITMAP_SIZE(WINED3D_MAX_VS_CONSTS_F)];
    uint16_t vertexShaderConstantsI;                        /* WINED3D_MAX_CONSTS_I, 16 */
    uint16_t vertexShaderConstantsB;                        /* WINED3D_MAX_CONSTS_B, 16 */
    uint32_t ps_consts_f[WINED3D_BITMAP_SIZE(WINED3D_MAX_PS_CONSTS_F)];
    uint16_t pixelShaderConstantsI;                         /* WINED3D_MAX_CONSTS_I, 16 */
    uint16_t pixelShaderConstantsB;                         /* WINED3D_MAX_CONSTS_B, 16 */
    uint32_t transform[WINED3D_BITMAP_SIZE(WINED3D_HIGHEST_TRANSFORM_STATE + 1)];
    uint16_t streamSource;                                  /* WINED3D_MAX_STREAMS, 16 */
    uint16_t streamFreq;                                    /* WINED3D_MAX_STREAMS, 16 */
    uint32_t renderState[WINED3D_BITMAP_SIZE(WINEHIGHEST_RENDER_STATE + 1)];
    uint32_t textureState[WINED3D_MAX_FFP_TEXTURES];        /* WINED3D_HIGHEST_TEXTURE_STATE + 1, 18 */
    uint16_t samplerState[WINED3D_MAX_COMBINED_SAMPLERS];   /* WINED3D_HIGHEST_SAMPLER_STATE + 1, 14 */
    uint32_t clipplane;                                     /* WINED3D_MAX_CLIP_DISTANCES, 8 */
    uint32_t textures : 20;                                 /* WINED3D_MAX_COMBINED_SAMPLERS, 20 */
    uint32_t indices : 1;
    uint32_t material : 1;
    uint32_t viewport : 1;
    uint32_t vertexDecl : 1;
    uint32_t pixelShader : 1;
    uint32_t vertexShader : 1;
    uint32_t scissorRect : 1;
    uint32_t store_stream_offset : 1;
    uint32_t alpha_to_coverage : 1;
    uint32_t lights : 1;
    uint32_t transforms : 1;
    uint32_t padding : 1;

    struct list changed_lights;
};

struct stage_state
{
    unsigned int stage, state;
};

struct wined3d_stateblock
{
    LONG ref;
    struct wined3d_device *device;

    struct wined3d_saved_states changed;

    struct wined3d_stateblock_state stateblock_state;
    struct wined3d_light_state light_state;

    unsigned int contained_render_states[WINEHIGHEST_RENDER_STATE + 1];
    unsigned int num_contained_render_states;
    unsigned int contained_transform_states[WINED3D_HIGHEST_TRANSFORM_STATE + 1];
    unsigned int num_contained_transform_states;
    struct stage_state contained_tss_states[WINED3D_MAX_FFP_TEXTURES * (WINED3D_HIGHEST_TEXTURE_STATE + 1)];
    unsigned int num_contained_tss_states;
    struct stage_state contained_sampler_states[WINED3D_MAX_COMBINED_SAMPLERS * WINED3D_HIGHEST_SAMPLER_STATE];
    unsigned int num_contained_sampler_states;
};

static const DWORD pixel_states_render[] =
{
    WINED3D_RS_ALPHABLENDENABLE,
    WINED3D_RS_ALPHAFUNC,
    WINED3D_RS_ALPHAREF,
    WINED3D_RS_ALPHATESTENABLE,
    WINED3D_RS_ANTIALIASEDLINEENABLE,
    WINED3D_RS_BLENDFACTOR,
    WINED3D_RS_BLENDOP,
    WINED3D_RS_BLENDOPALPHA,
    WINED3D_RS_BACK_STENCILFAIL,
    WINED3D_RS_BACK_STENCILPASS,
    WINED3D_RS_BACK_STENCILZFAIL,
    WINED3D_RS_COLORWRITEENABLE,
    WINED3D_RS_COLORWRITEENABLE1,
    WINED3D_RS_COLORWRITEENABLE2,
    WINED3D_RS_COLORWRITEENABLE3,
    WINED3D_RS_DEPTHBIAS,
    WINED3D_RS_DESTBLEND,
    WINED3D_RS_DESTBLENDALPHA,
    WINED3D_RS_DITHERENABLE,
    WINED3D_RS_FILLMODE,
    WINED3D_RS_FOGDENSITY,
    WINED3D_RS_FOGEND,
    WINED3D_RS_FOGSTART,
    WINED3D_RS_LASTPIXEL,
    WINED3D_RS_SCISSORTESTENABLE,
    WINED3D_RS_SEPARATEALPHABLENDENABLE,
    WINED3D_RS_SHADEMODE,
    WINED3D_RS_SLOPESCALEDEPTHBIAS,
    WINED3D_RS_SRCBLEND,
    WINED3D_RS_SRCBLENDALPHA,
    WINED3D_RS_SRGBWRITEENABLE,
    WINED3D_RS_STENCILENABLE,
    WINED3D_RS_STENCILFAIL,
    WINED3D_RS_STENCILFUNC,
    WINED3D_RS_STENCILMASK,
    WINED3D_RS_STENCILPASS,
    WINED3D_RS_STENCILREF,
    WINED3D_RS_STENCILWRITEMASK,
    WINED3D_RS_STENCILZFAIL,
    WINED3D_RS_TEXTUREFACTOR,
    WINED3D_RS_TWOSIDEDSTENCILMODE,
    WINED3D_RS_WRAP0,
    WINED3D_RS_WRAP1,
    WINED3D_RS_WRAP10,
    WINED3D_RS_WRAP11,
    WINED3D_RS_WRAP12,
    WINED3D_RS_WRAP13,
    WINED3D_RS_WRAP14,
    WINED3D_RS_WRAP15,
    WINED3D_RS_WRAP2,
    WINED3D_RS_WRAP3,
    WINED3D_RS_WRAP4,
    WINED3D_RS_WRAP5,
    WINED3D_RS_WRAP6,
    WINED3D_RS_WRAP7,
    WINED3D_RS_WRAP8,
    WINED3D_RS_WRAP9,
    WINED3D_RS_ZENABLE,
    WINED3D_RS_ZFUNC,
    WINED3D_RS_ZWRITEENABLE,
};

static const DWORD pixel_states_texture[] =
{
    WINED3D_TSS_ALPHA_ARG0,
    WINED3D_TSS_ALPHA_ARG1,
    WINED3D_TSS_ALPHA_ARG2,
    WINED3D_TSS_ALPHA_OP,
    WINED3D_TSS_BUMPENV_LOFFSET,
    WINED3D_TSS_BUMPENV_LSCALE,
    WINED3D_TSS_BUMPENV_MAT00,
    WINED3D_TSS_BUMPENV_MAT01,
    WINED3D_TSS_BUMPENV_MAT10,
    WINED3D_TSS_BUMPENV_MAT11,
    WINED3D_TSS_COLOR_ARG0,
    WINED3D_TSS_COLOR_ARG1,
    WINED3D_TSS_COLOR_ARG2,
    WINED3D_TSS_COLOR_OP,
    WINED3D_TSS_RESULT_ARG,
    WINED3D_TSS_TEXCOORD_INDEX,
    WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS,
};

static const DWORD pixel_states_sampler[] =
{
    WINED3D_SAMP_ADDRESS_U,
    WINED3D_SAMP_ADDRESS_V,
    WINED3D_SAMP_ADDRESS_W,
    WINED3D_SAMP_BORDER_COLOR,
    WINED3D_SAMP_MAG_FILTER,
    WINED3D_SAMP_MIN_FILTER,
    WINED3D_SAMP_MIP_FILTER,
    WINED3D_SAMP_MIPMAP_LOD_BIAS,
    WINED3D_SAMP_MAX_MIP_LEVEL,
    WINED3D_SAMP_MAX_ANISOTROPY,
    WINED3D_SAMP_SRGB_TEXTURE,
    WINED3D_SAMP_ELEMENT_INDEX,
};

static const DWORD vertex_states_render[] =
{
    WINED3D_RS_ADAPTIVETESS_W,
    WINED3D_RS_ADAPTIVETESS_X,
    WINED3D_RS_ADAPTIVETESS_Y,
    WINED3D_RS_ADAPTIVETESS_Z,
    WINED3D_RS_AMBIENT,
    WINED3D_RS_AMBIENTMATERIALSOURCE,
    WINED3D_RS_CLIPPING,
    WINED3D_RS_CLIPPLANEENABLE,
    WINED3D_RS_COLORVERTEX,
    WINED3D_RS_CULLMODE,
    WINED3D_RS_DIFFUSEMATERIALSOURCE,
    WINED3D_RS_EMISSIVEMATERIALSOURCE,
    WINED3D_RS_ENABLEADAPTIVETESSELLATION,
    WINED3D_RS_FOGCOLOR,
    WINED3D_RS_FOGDENSITY,
    WINED3D_RS_FOGENABLE,
    WINED3D_RS_FOGEND,
    WINED3D_RS_FOGSTART,
    WINED3D_RS_FOGTABLEMODE,
    WINED3D_RS_FOGVERTEXMODE,
    WINED3D_RS_INDEXEDVERTEXBLENDENABLE,
    WINED3D_RS_LIGHTING,
    WINED3D_RS_LOCALVIEWER,
    WINED3D_RS_MAXTESSELLATIONLEVEL,
    WINED3D_RS_MINTESSELLATIONLEVEL,
    WINED3D_RS_MULTISAMPLEANTIALIAS,
    WINED3D_RS_MULTISAMPLEMASK,
    WINED3D_RS_NORMALDEGREE,
    WINED3D_RS_NORMALIZENORMALS,
    WINED3D_RS_PATCHEDGESTYLE,
    WINED3D_RS_POINTSCALE_A,
    WINED3D_RS_POINTSCALE_B,
    WINED3D_RS_POINTSCALE_C,
    WINED3D_RS_POINTSCALEENABLE,
    WINED3D_RS_POINTSIZE,
    WINED3D_RS_POINTSIZE_MAX,
    WINED3D_RS_POINTSIZE_MIN,
    WINED3D_RS_POINTSPRITEENABLE,
    WINED3D_RS_POSITIONDEGREE,
    WINED3D_RS_RANGEFOGENABLE,
    WINED3D_RS_SHADEMODE,
    WINED3D_RS_SPECULARENABLE,
    WINED3D_RS_SPECULARMATERIALSOURCE,
    WINED3D_RS_TWEENFACTOR,
    WINED3D_RS_VERTEXBLEND,
};

static const DWORD vertex_states_texture[] =
{
    WINED3D_TSS_TEXCOORD_INDEX,
    WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS,
};

static const DWORD vertex_states_sampler[] =
{
    WINED3D_SAMP_DMAP_OFFSET,
};

static inline void stateblock_set_all_bits(uint32_t *map, UINT map_size)
{
    DWORD mask = (1u << (map_size & 0x1f)) - 1;
    memset(map, 0xff, (map_size >> 5) * sizeof(*map));
    if (mask) map[map_size >> 5] = mask;
}

/* Set all members of a stateblock savedstate to the given value */
static void stateblock_savedstates_set_all(struct wined3d_saved_states *states, DWORD vs_consts, DWORD ps_consts)
{
    unsigned int i;

    states->indices = 1;
    states->material = 1;
    states->viewport = 1;
    states->vertexDecl = 1;
    states->pixelShader = 1;
    states->vertexShader = 1;
    states->scissorRect = 1;
    states->alpha_to_coverage = 1;
    states->lights = 1;
    states->transforms = 1;

    states->streamSource = 0xffff;
    states->streamFreq = 0xffff;
    states->textures = 0xfffff;
    stateblock_set_all_bits(states->transform, WINED3D_HIGHEST_TRANSFORM_STATE + 1);
    stateblock_set_all_bits(states->renderState, WINEHIGHEST_RENDER_STATE + 1);
    for (i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i) states->textureState[i] = 0x3ffff;
    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i) states->samplerState[i] = 0x3ffe;
    states->clipplane = wined3d_mask_from_size(WINED3D_MAX_CLIP_DISTANCES);
    states->pixelShaderConstantsB = 0xffff;
    states->pixelShaderConstantsI = 0xffff;
    states->vertexShaderConstantsB = 0xffff;
    states->vertexShaderConstantsI = 0xffff;

    memset(states->ps_consts_f, 0xffu, sizeof(states->ps_consts_f));
    memset(states->vs_consts_f, 0xffu, sizeof(states->vs_consts_f));
}

static void stateblock_savedstates_set_pixel(struct wined3d_saved_states *states, const DWORD num_constants)
{
    DWORD texture_mask = 0;
    WORD sampler_mask = 0;
    unsigned int i;

    states->pixelShader = 1;

    for (i = 0; i < ARRAY_SIZE(pixel_states_render); ++i)
    {
        DWORD rs = pixel_states_render[i];
        states->renderState[rs >> 5] |= 1u << (rs & 0x1f);
    }

    for (i = 0; i < ARRAY_SIZE(pixel_states_texture); ++i)
        texture_mask |= 1u << pixel_states_texture[i];
    for (i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i) states->textureState[i] = texture_mask;
    for (i = 0; i < ARRAY_SIZE(pixel_states_sampler); ++i)
        sampler_mask |= 1u << pixel_states_sampler[i];
    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i) states->samplerState[i] = sampler_mask;
    states->pixelShaderConstantsB = 0xffff;
    states->pixelShaderConstantsI = 0xffff;

    memset(states->ps_consts_f, 0xffu, sizeof(states->ps_consts_f));
}

static void stateblock_savedstates_set_vertex(struct wined3d_saved_states *states, const DWORD num_constants)
{
    DWORD texture_mask = 0;
    WORD sampler_mask = 0;
    unsigned int i;

    states->vertexDecl = 1;
    states->vertexShader = 1;
    states->alpha_to_coverage = 1;
    states->lights = 1;

    for (i = 0; i < ARRAY_SIZE(vertex_states_render); ++i)
    {
        DWORD rs = vertex_states_render[i];
        states->renderState[rs >> 5] |= 1u << (rs & 0x1f);
    }

    for (i = 0; i < ARRAY_SIZE(vertex_states_texture); ++i)
        texture_mask |= 1u << vertex_states_texture[i];
    for (i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i) states->textureState[i] = texture_mask;
    for (i = 0; i < ARRAY_SIZE(vertex_states_sampler); ++i)
        sampler_mask |= 1u << vertex_states_sampler[i];
    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i) states->samplerState[i] = sampler_mask;
    states->vertexShaderConstantsB = 0xffff;
    states->vertexShaderConstantsI = 0xffff;

    memset(states->vs_consts_f, 0xffu, sizeof(states->vs_consts_f));
}

void CDECL wined3d_stateblock_init_contained_states(struct wined3d_stateblock *stateblock)
{
    unsigned int i, j;

    for (i = 0; i <= WINEHIGHEST_RENDER_STATE >> 5; ++i)
    {
        DWORD map = stateblock->changed.renderState[i];
        for (j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            stateblock->contained_render_states[stateblock->num_contained_render_states] = (i << 5) | j;
            ++stateblock->num_contained_render_states;
        }
    }

    for (i = 0; i <= WINED3D_HIGHEST_TRANSFORM_STATE >> 5; ++i)
    {
        DWORD map = stateblock->changed.transform[i];
        for (j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            stateblock->contained_transform_states[stateblock->num_contained_transform_states] = (i << 5) | j;
            ++stateblock->num_contained_transform_states;
        }
    }

    for (i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
    {
        DWORD map = stateblock->changed.textureState[i];

        for(j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            stateblock->contained_tss_states[stateblock->num_contained_tss_states].stage = i;
            stateblock->contained_tss_states[stateblock->num_contained_tss_states].state = j;
            ++stateblock->num_contained_tss_states;
        }
    }

    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i)
    {
        DWORD map = stateblock->changed.samplerState[i];

        for (j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            stateblock->contained_sampler_states[stateblock->num_contained_sampler_states].stage = i;
            stateblock->contained_sampler_states[stateblock->num_contained_sampler_states].state = j;
            ++stateblock->num_contained_sampler_states;
        }
    }
}

static void stateblock_init_lights(struct wined3d_stateblock *stateblock, const struct rb_tree *src_tree)
{
    struct rb_tree *dst_tree = &stateblock->stateblock_state.light_state->lights_tree;
    struct wined3d_light_info *src_light;

    RB_FOR_EACH_ENTRY(src_light, src_tree, struct wined3d_light_info, entry)
    {
        struct wined3d_light_info *dst_light = heap_alloc(sizeof(*dst_light));

        *dst_light = *src_light;
        rb_put(dst_tree, (void *)(ULONG_PTR)dst_light->OriginalIndex, &dst_light->entry);
        dst_light->changed = true;
        list_add_tail(&stateblock->changed.changed_lights, &dst_light->changed_entry);
    }
}

ULONG CDECL wined3d_stateblock_incref(struct wined3d_stateblock *stateblock)
{
    unsigned int refcount = InterlockedIncrement(&stateblock->ref);

    TRACE("%p increasing refcount to %u.\n", stateblock, refcount);

    return refcount;
}

void state_unbind_resources(struct wined3d_state *state)
{
    struct wined3d_unordered_access_view *uav;
    struct wined3d_shader_resource_view *srv;
    struct wined3d_vertex_declaration *decl;
    struct wined3d_blend_state *blend_state;
    struct wined3d_rendertarget_view *rtv;
    struct wined3d_sampler *sampler;
    struct wined3d_buffer *buffer;
    struct wined3d_shader *shader;
    unsigned int i, j;

    if ((decl = state->vertex_declaration))
    {
        state->vertex_declaration = NULL;
        wined3d_vertex_declaration_decref(decl);
    }

    for (i = 0; i < WINED3D_MAX_STREAM_OUTPUT_BUFFERS; ++i)
    {
        if ((buffer = state->stream_output[i].buffer))
        {
            state->stream_output[i].buffer = NULL;
            wined3d_buffer_decref(buffer);
        }
    }

    for (i = 0; i < WINED3D_MAX_STREAMS; ++i)
    {
        if ((buffer = state->streams[i].buffer))
        {
            state->streams[i].buffer = NULL;
            wined3d_buffer_decref(buffer);
        }
    }

    if ((buffer = state->index_buffer))
    {
        state->index_buffer = NULL;
        wined3d_buffer_decref(buffer);
    }

    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        if ((shader = state->shader[i]))
        {
            state->shader[i] = NULL;
            wined3d_shader_decref(shader);
        }

        for (j = 0; j < MAX_CONSTANT_BUFFERS; ++j)
        {
            if ((buffer = state->cb[i][j].buffer))
            {
                state->cb[i][j].buffer = NULL;
                wined3d_buffer_decref(buffer);
            }
        }

        for (j = 0; j < MAX_SAMPLER_OBJECTS; ++j)
        {
            if ((sampler = state->sampler[i][j]))
            {
                state->sampler[i][j] = NULL;
                wined3d_sampler_decref(sampler);
            }
        }

        for (j = 0; j < MAX_SHADER_RESOURCE_VIEWS; ++j)
        {
            if ((srv = state->shader_resource_view[i][j]))
            {
                state->shader_resource_view[i][j] = NULL;
                wined3d_srv_bind_count_dec(srv);
                wined3d_shader_resource_view_decref(srv);
            }
        }
    }

    for (i = 0; i < WINED3D_PIPELINE_COUNT; ++i)
    {
        for (j = 0; j < MAX_UNORDERED_ACCESS_VIEWS; ++j)
        {
            if ((uav = state->unordered_access_view[i][j]))
            {
                state->unordered_access_view[i][j] = NULL;
                wined3d_unordered_access_view_decref(uav);
            }
        }
    }

    if ((blend_state = state->blend_state))
    {
        state->blend_state = NULL;
        wined3d_blend_state_decref(blend_state);
    }

    for (i = 0; i < ARRAY_SIZE(state->fb.render_targets); ++i)
    {
        if ((rtv = state->fb.render_targets[i]))
        {
            state->fb.render_targets[i] = NULL;
            wined3d_rendertarget_view_decref(rtv);
        }
    }

    if ((rtv = state->fb.depth_stencil))
    {
        state->fb.depth_stencil = NULL;
        wined3d_rendertarget_view_decref(rtv);
    }
}

static void wined3d_stateblock_state_cleanup(struct wined3d_stateblock_state *state)
{
    struct wined3d_light_info *light, *cursor;
    struct wined3d_vertex_declaration *decl;
    struct wined3d_texture *texture;
    struct wined3d_buffer *buffer;
    struct wined3d_shader *shader;
    unsigned int i;

    if ((decl = state->vertex_declaration))
    {
        state->vertex_declaration = NULL;
        wined3d_vertex_declaration_decref(decl);
    }

    for (i = 0; i < WINED3D_MAX_STREAMS; ++i)
    {
        if ((buffer = state->streams[i].buffer))
        {
            state->streams[i].buffer = NULL;
            wined3d_buffer_decref(buffer);
        }
    }

    if ((buffer = state->index_buffer))
    {
        state->index_buffer = NULL;
        wined3d_buffer_decref(buffer);
    }

    if ((shader = state->vs))
    {
        state->vs = NULL;
        wined3d_shader_decref(shader);
    }

    if ((shader = state->ps))
    {
        state->ps = NULL;
        wined3d_shader_decref(shader);
    }

    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i)
    {
        if ((texture = state->textures[i]))
        {
            state->textures[i] = NULL;
            wined3d_texture_decref(texture);
        }
    }

    RB_FOR_EACH_ENTRY_DESTRUCTOR(light, cursor, &state->light_state->lights_tree, struct wined3d_light_info, entry)
    {
        if (light->changed)
            list_remove(&light->changed_entry);
        rb_remove(&state->light_state->lights_tree, &light->entry);
        heap_free(light);
    }
}

void state_cleanup(struct wined3d_state *state)
{
    struct wined3d_light_info *light, *cursor;
    unsigned int i;

    if (!(state->flags & WINED3D_STATE_NO_REF))
        state_unbind_resources(state);

    for (i = 0; i < WINED3D_MAX_ACTIVE_LIGHTS; ++i)
    {
        state->light_state.lights[i] = NULL;
    }

    RB_FOR_EACH_ENTRY_DESTRUCTOR(light, cursor, &state->light_state.lights_tree, struct wined3d_light_info, entry)
    {
        if (light->changed)
            list_remove(&light->changed_entry);
        rb_remove(&state->light_state.lights_tree, &light->entry);
        heap_free(light);
    }
}

ULONG CDECL wined3d_stateblock_decref(struct wined3d_stateblock *stateblock)
{
    unsigned int refcount = InterlockedDecrement(&stateblock->ref);

    TRACE("%p decreasing refcount to %u\n", stateblock, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        wined3d_stateblock_state_cleanup(&stateblock->stateblock_state);
        heap_free(stateblock);
        wined3d_mutex_unlock();
    }

    return refcount;
}

struct wined3d_light_info *wined3d_light_state_get_light(const struct wined3d_light_state *state, unsigned int idx)
{
    struct rb_entry *entry;

    if (!(entry = rb_get(&state->lights_tree, (void *)(ULONG_PTR)idx)))
        return NULL;

    return RB_ENTRY_VALUE(entry, struct wined3d_light_info, entry);
}

static void set_light_changed(struct wined3d_stateblock *stateblock, struct wined3d_light_info *light_info)
{
    if (!light_info->changed)
    {
        list_add_tail(&stateblock->changed.changed_lights, &light_info->changed_entry);
        light_info->changed = true;
    }
    stateblock->changed.lights = 1;
}

HRESULT wined3d_light_state_set_light(struct wined3d_light_state *state, DWORD light_idx,
        const struct wined3d_light *params, struct wined3d_light_info **light_info)
{
    struct wined3d_light_info *object;

    if (!(object = wined3d_light_state_get_light(state, light_idx)))
    {
        TRACE("Adding new light.\n");
        if (!(object = heap_alloc_zero(sizeof(*object))))
        {
            ERR("Failed to allocate light info.\n");
            return E_OUTOFMEMORY;
        }

        object->glIndex = -1;
        object->OriginalIndex = light_idx;
        rb_put(&state->lights_tree, (void *)(ULONG_PTR)light_idx, &object->entry);
    }

    object->OriginalParms = *params;

    *light_info = object;
    return WINED3D_OK;
}

bool wined3d_light_state_enable_light(struct wined3d_light_state *state, const struct wined3d_d3d_info *d3d_info,
        struct wined3d_light_info *light_info, BOOL enable)
{
    unsigned int light_count, i;

    if (!(light_info->enabled = enable))
    {
        if (light_info->glIndex == -1)
        {
            TRACE("Light already disabled, nothing to do.\n");
            return false;
        }

        state->lights[light_info->glIndex] = NULL;
        light_info->glIndex = -1;
        return true;
    }

    if (light_info->glIndex != -1)
    {
        TRACE("Light already enabled, nothing to do.\n");
        return false;
    }

    /* Find a free light. */
    light_count = d3d_info->limits.active_light_count;
    for (i = 0; i < light_count; ++i)
    {
        if (state->lights[i])
            continue;

        state->lights[i] = light_info;
        light_info->glIndex = i;
        return true;
    }

    /* Our tests show that Windows returns D3D_OK in this situation, even with
     * D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE devices.
     * This is consistent among ddraw, d3d8 and d3d9. GetLightEnable returns
     * TRUE * as well for those lights.
     *
     * TODO: Test how this affects rendering. */
    WARN("Too many concurrently active lights.\n");
    return false;
}

static void wined3d_state_record_lights(struct wined3d_light_state *dst_state,
        const struct wined3d_light_state *src_state)
{
    const struct wined3d_light_info *src;
    struct wined3d_light_info *dst;

    /* Lights... For a recorded state block, we just had a chain of actions
     * to perform, so we need to walk that chain and update any actions which
     * differ. */
    RB_FOR_EACH_ENTRY(dst, &dst_state->lights_tree, struct wined3d_light_info, entry)
    {
        if ((src = wined3d_light_state_get_light(src_state, dst->OriginalIndex)))
        {
            dst->OriginalParms = src->OriginalParms;

            if (src->glIndex == -1 && dst->glIndex != -1)
            {
                /* Light disabled. */
                dst_state->lights[dst->glIndex] = NULL;
            }
            else if (src->glIndex != -1 && dst->glIndex == -1)
            {
                /* Light enabled. */
                dst_state->lights[src->glIndex] = dst;
            }
            dst->glIndex = src->glIndex;
        }
        else
        {
            /* This can happen if the light was originally created as a
             * default light for SetLightEnable() while recording. */
            WARN("Light %u in dst_state %p does not exist in src_state %p.\n",
                    dst->OriginalIndex, dst_state, src_state);

            dst->OriginalParms = WINED3D_default_light;
            if (dst->glIndex != -1)
            {
                dst_state->lights[dst->glIndex] = NULL;
                dst->glIndex = -1;
            }
        }
    }
}

void CDECL wined3d_stateblock_capture(struct wined3d_stateblock *stateblock,
        const struct wined3d_stateblock *device_state)
{
    const struct wined3d_stateblock_state *state = &device_state->stateblock_state;
    struct wined3d_range range;
    unsigned int i, start;
    uint32_t map;

    TRACE("stateblock %p, device_state %p.\n", stateblock, device_state);

    if (stateblock->changed.vertexShader && stateblock->stateblock_state.vs != state->vs)
    {
        TRACE("Updating vertex shader from %p to %p.\n", stateblock->stateblock_state.vs, state->vs);

        if (state->vs)
            wined3d_shader_incref(state->vs);
        if (stateblock->stateblock_state.vs)
            wined3d_shader_decref(stateblock->stateblock_state.vs);
        stateblock->stateblock_state.vs = state->vs;
    }

    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(stateblock->changed.vs_consts_f, WINED3D_MAX_VS_CONSTS_F, start, &range))
            break;

        memcpy(&stateblock->stateblock_state.vs_consts_f[range.offset], &state->vs_consts_f[range.offset],
                sizeof(*state->vs_consts_f) * range.size);
    }
    map = stateblock->changed.vertexShaderConstantsI;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_I, start, &range))
            break;

        memcpy(&stateblock->stateblock_state.vs_consts_i[range.offset], &state->vs_consts_i[range.offset],
                sizeof(*state->vs_consts_i) * range.size);
    }
    map = stateblock->changed.vertexShaderConstantsB;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_B, start, &range))
            break;

        memcpy(&stateblock->stateblock_state.vs_consts_b[range.offset], &state->vs_consts_b[range.offset],
                sizeof(*state->vs_consts_b) * range.size);
    }

    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(stateblock->changed.ps_consts_f, WINED3D_MAX_PS_CONSTS_F, start, &range))
            break;

        memcpy(&stateblock->stateblock_state.ps_consts_f[range.offset], &state->ps_consts_f[range.offset],
                sizeof(*state->ps_consts_f) * range.size);
    }
    map = stateblock->changed.pixelShaderConstantsI;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_I, start, &range))
            break;

        memcpy(&stateblock->stateblock_state.ps_consts_i[range.offset], &state->ps_consts_i[range.offset],
                sizeof(*state->ps_consts_i) * range.size);
    }
    map = stateblock->changed.pixelShaderConstantsB;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_B, start, &range))
            break;

        memcpy(&stateblock->stateblock_state.ps_consts_b[range.offset], &state->ps_consts_b[range.offset],
                sizeof(*state->ps_consts_b) * range.size);
    }

    if (stateblock->changed.transforms)
    {
        for (i = 0; i < stateblock->num_contained_transform_states; ++i)
        {
            enum wined3d_transform_state transform = stateblock->contained_transform_states[i];

            TRACE("Updating transform %#x.\n", transform);

            stateblock->stateblock_state.transforms[transform] = state->transforms[transform];
        }
    }

    if (stateblock->changed.indices
            && ((stateblock->stateblock_state.index_buffer != state->index_buffer)
                || (stateblock->stateblock_state.base_vertex_index != state->base_vertex_index)
                || (stateblock->stateblock_state.index_format != state->index_format)))
    {
        TRACE("Updating index buffer to %p, base vertex index to %d.\n",
                state->index_buffer, state->base_vertex_index);

        if (state->index_buffer)
            wined3d_buffer_incref(state->index_buffer);
        if (stateblock->stateblock_state.index_buffer)
            wined3d_buffer_decref(stateblock->stateblock_state.index_buffer);
        stateblock->stateblock_state.index_buffer = state->index_buffer;
        stateblock->stateblock_state.base_vertex_index = state->base_vertex_index;
        stateblock->stateblock_state.index_format = state->index_format;
    }

    if (stateblock->changed.vertexDecl && stateblock->stateblock_state.vertex_declaration != state->vertex_declaration)
    {
        TRACE("Updating vertex declaration from %p to %p.\n",
                stateblock->stateblock_state.vertex_declaration, state->vertex_declaration);

        if (state->vertex_declaration)
                wined3d_vertex_declaration_incref(state->vertex_declaration);
        if (stateblock->stateblock_state.vertex_declaration)
                wined3d_vertex_declaration_decref(stateblock->stateblock_state.vertex_declaration);
        stateblock->stateblock_state.vertex_declaration = state->vertex_declaration;
    }

    if (stateblock->changed.material
            && memcmp(&state->material, &stateblock->stateblock_state.material,
            sizeof(stateblock->stateblock_state.material)))
    {
        TRACE("Updating material.\n");

        stateblock->stateblock_state.material = state->material;
    }

    if (stateblock->changed.viewport
            && memcmp(&state->viewport, &stateblock->stateblock_state.viewport, sizeof(state->viewport)))
    {
        TRACE("Updating viewport.\n");

        stateblock->stateblock_state.viewport = state->viewport;
    }

    if (stateblock->changed.scissorRect
            && memcmp(&state->scissor_rect, &stateblock->stateblock_state.scissor_rect, sizeof(state->scissor_rect)))
    {
        TRACE("Updating scissor rect.\n");

        stateblock->stateblock_state.scissor_rect = state->scissor_rect;
    }

    map = stateblock->changed.streamSource;
    while (map)
    {
        i = wined3d_bit_scan(&map);

        if (stateblock->stateblock_state.streams[i].stride != state->streams[i].stride
                || stateblock->stateblock_state.streams[i].offset != state->streams[i].offset
                || stateblock->stateblock_state.streams[i].buffer != state->streams[i].buffer)
        {
            TRACE("stateblock %p, stream source %u, buffer %p, stride %u, offset %u.\n",
                    stateblock, i, state->streams[i].buffer, state->streams[i].stride,
                    state->streams[i].offset);

            stateblock->stateblock_state.streams[i].stride = state->streams[i].stride;
            if (stateblock->changed.store_stream_offset)
                stateblock->stateblock_state.streams[i].offset = state->streams[i].offset;

            if (state->streams[i].buffer)
                    wined3d_buffer_incref(state->streams[i].buffer);
            if (stateblock->stateblock_state.streams[i].buffer)
                    wined3d_buffer_decref(stateblock->stateblock_state.streams[i].buffer);
            stateblock->stateblock_state.streams[i].buffer = state->streams[i].buffer;
        }
    }

    map = stateblock->changed.streamFreq;
    while (map)
    {
        i = wined3d_bit_scan(&map);

        if (stateblock->stateblock_state.streams[i].frequency != state->streams[i].frequency
                || stateblock->stateblock_state.streams[i].flags != state->streams[i].flags)
        {
            TRACE("Updating stream frequency %u to %u flags to %#x.\n",
                    i, state->streams[i].frequency, state->streams[i].flags);

            stateblock->stateblock_state.streams[i].frequency = state->streams[i].frequency;
            stateblock->stateblock_state.streams[i].flags = state->streams[i].flags;
        }
    }

    map = stateblock->changed.clipplane;
    while (map)
    {
        i = wined3d_bit_scan(&map);

        if (memcmp(&stateblock->stateblock_state.clip_planes[i], &state->clip_planes[i], sizeof(state->clip_planes[i])))
        {
            TRACE("Updating clipplane %u.\n", i);
            stateblock->stateblock_state.clip_planes[i] = state->clip_planes[i];
        }
    }

    /* Render */
    for (i = 0; i < stateblock->num_contained_render_states; ++i)
    {
        enum wined3d_render_state rs = stateblock->contained_render_states[i];

        TRACE("Updating render state %#x to %u.\n", rs, state->rs[rs]);

        stateblock->stateblock_state.rs[rs] = state->rs[rs];
    }

    /* Texture states */
    for (i = 0; i < stateblock->num_contained_tss_states; ++i)
    {
        unsigned int stage = stateblock->contained_tss_states[i].stage;
        unsigned int texture_state = stateblock->contained_tss_states[i].state;

        TRACE("Updating texturestage state %u, %u to %#x (was %#x).\n", stage, texture_state,
                state->texture_states[stage][texture_state],
                stateblock->stateblock_state.texture_states[stage][texture_state]);

        stateblock->stateblock_state.texture_states[stage][texture_state] = state->texture_states[stage][texture_state];
    }

    /* Samplers */
    map = stateblock->changed.textures;
    while (map)
    {
        i = wined3d_bit_scan(&map);

        TRACE("Updating texture %u to %p (was %p).\n",
                i, state->textures[i], stateblock->stateblock_state.textures[i]);

        if (state->textures[i])
            wined3d_texture_incref(state->textures[i]);
        if (stateblock->stateblock_state.textures[i])
            wined3d_texture_decref(stateblock->stateblock_state.textures[i]);
        stateblock->stateblock_state.textures[i] = state->textures[i];
    }

    for (i = 0; i < stateblock->num_contained_sampler_states; ++i)
    {
        unsigned int stage = stateblock->contained_sampler_states[i].stage;
        unsigned int sampler_state = stateblock->contained_sampler_states[i].state;

        TRACE("Updating sampler state %u, %u to %#x (was %#x).\n", stage, sampler_state,
                state->sampler_states[stage][sampler_state],
                stateblock->stateblock_state.sampler_states[stage][sampler_state]);

        stateblock->stateblock_state.sampler_states[stage][sampler_state] = state->sampler_states[stage][sampler_state];
    }

    if (stateblock->changed.pixelShader && stateblock->stateblock_state.ps != state->ps)
    {
        if (state->ps)
            wined3d_shader_incref(state->ps);
        if (stateblock->stateblock_state.ps)
            wined3d_shader_decref(stateblock->stateblock_state.ps);
        stateblock->stateblock_state.ps = state->ps;
    }

    if (stateblock->changed.lights)
        wined3d_state_record_lights(stateblock->stateblock_state.light_state, state->light_state);

    if (stateblock->changed.alpha_to_coverage)
        stateblock->stateblock_state.alpha_to_coverage = state->alpha_to_coverage;

    TRACE("Capture done.\n");
}

void CDECL wined3d_stateblock_apply(const struct wined3d_stateblock *stateblock,
        struct wined3d_stateblock *device_state)
{
    const struct wined3d_stateblock_state *state = &stateblock->stateblock_state;
    struct wined3d_range range;
    unsigned int i, start;
    uint32_t map;

    TRACE("stateblock %p, device_state %p.\n", stateblock, device_state);

    if (stateblock->changed.vertexShader)
        wined3d_stateblock_set_vertex_shader(device_state, state->vs);
    if (stateblock->changed.pixelShader)
        wined3d_stateblock_set_pixel_shader(device_state, state->ps);

    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(stateblock->changed.vs_consts_f, WINED3D_MAX_VS_CONSTS_F, start, &range))
            break;
        wined3d_stateblock_set_vs_consts_f(device_state, range.offset, range.size, &state->vs_consts_f[range.offset]);
    }
    map = stateblock->changed.vertexShaderConstantsI;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_I, start, &range))
            break;
        wined3d_stateblock_set_vs_consts_i(device_state, range.offset, range.size, &state->vs_consts_i[range.offset]);
    }
    map = stateblock->changed.vertexShaderConstantsB;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_B, start, &range))
            break;
        wined3d_stateblock_set_vs_consts_b(device_state, range.offset, range.size, &state->vs_consts_b[range.offset]);
    }

    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(stateblock->changed.ps_consts_f, WINED3D_MAX_PS_CONSTS_F, start, &range))
            break;
        wined3d_stateblock_set_ps_consts_f(device_state, range.offset, range.size, &state->ps_consts_f[range.offset]);
    }
    map = stateblock->changed.pixelShaderConstantsI;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_I, start, &range))
            break;
        wined3d_stateblock_set_ps_consts_i(device_state, range.offset, range.size, &state->ps_consts_i[range.offset]);
    }
    map = stateblock->changed.pixelShaderConstantsB;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_B, start, &range))
            break;
        wined3d_stateblock_set_ps_consts_b(device_state, range.offset, range.size, &state->ps_consts_b[range.offset]);
    }

    if (stateblock->changed.transforms)
    {
        for (i = 0; i < stateblock->num_contained_transform_states; ++i)
        {
            enum wined3d_transform_state transform = stateblock->contained_transform_states[i];

            wined3d_stateblock_set_transform(device_state, transform, &state->transforms[transform]);
        }
    }

    if (stateblock->changed.lights)
    {
        const struct wined3d_light_info *light;

        LIST_FOR_EACH_ENTRY(light, &stateblock->changed.changed_lights, struct wined3d_light_info, changed_entry)
        {
            wined3d_stateblock_set_light(device_state, light->OriginalIndex, &light->OriginalParms);
            wined3d_stateblock_set_light_enable(device_state, light->OriginalIndex, light->glIndex != -1);
        }
    }

    if (stateblock->changed.alpha_to_coverage)
    {
        device_state->stateblock_state.alpha_to_coverage = state->alpha_to_coverage;
        device_state->changed.alpha_to_coverage = 1;
    }

    /* Render states. */
    for (i = 0; i < stateblock->num_contained_render_states; ++i)
    {
        enum wined3d_render_state rs = stateblock->contained_render_states[i];

        wined3d_stateblock_set_render_state(device_state, rs, state->rs[rs]);
    }

    /* Texture states. */
    for (i = 0; i < stateblock->num_contained_tss_states; ++i)
    {
        DWORD stage = stateblock->contained_tss_states[i].stage;
        DWORD texture_state = stateblock->contained_tss_states[i].state;

        wined3d_stateblock_set_texture_stage_state(device_state, stage, texture_state,
                state->texture_states[stage][texture_state]);
    }

    /* Sampler states. */
    for (i = 0; i < stateblock->num_contained_sampler_states; ++i)
    {
        DWORD stage = stateblock->contained_sampler_states[i].stage;
        DWORD sampler_state = stateblock->contained_sampler_states[i].state;

        wined3d_stateblock_set_sampler_state(device_state, stage, sampler_state,
                state->sampler_states[stage][sampler_state]);
    }

    if (stateblock->changed.indices)
    {
        wined3d_stateblock_set_index_buffer(device_state, state->index_buffer, state->index_format);
        wined3d_stateblock_set_base_vertex_index(device_state, state->base_vertex_index);
    }

    if (stateblock->changed.vertexDecl && state->vertex_declaration)
        wined3d_stateblock_set_vertex_declaration(device_state, state->vertex_declaration);

    if (stateblock->changed.material)
        wined3d_stateblock_set_material(device_state, &state->material);

    if (stateblock->changed.viewport)
        wined3d_stateblock_set_viewport(device_state, &state->viewport);

    if (stateblock->changed.scissorRect)
        wined3d_stateblock_set_scissor_rect(device_state, &state->scissor_rect);

    map = stateblock->changed.streamSource;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        wined3d_stateblock_set_stream_source(device_state, i, state->streams[i].buffer,
                state->streams[i].offset, state->streams[i].stride);
    }

    map = stateblock->changed.streamFreq;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        wined3d_stateblock_set_stream_source_freq(device_state, i,
                state->streams[i].frequency | state->streams[i].flags);
    }

    map = stateblock->changed.textures;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        wined3d_stateblock_set_texture(device_state, i, state->textures[i]);
    }

    map = stateblock->changed.clipplane;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        wined3d_stateblock_set_clip_plane(device_state, i, &state->clip_planes[i]);
    }

    TRACE("Applied stateblock %p.\n", stateblock);
}

void CDECL wined3d_stateblock_set_vertex_shader(struct wined3d_stateblock *stateblock, struct wined3d_shader *shader)
{
    TRACE("stateblock %p, shader %p.\n", stateblock, shader);

    if (shader)
        wined3d_shader_incref(shader);
    if (stateblock->stateblock_state.vs)
        wined3d_shader_decref(stateblock->stateblock_state.vs);
    stateblock->stateblock_state.vs = shader;
    stateblock->changed.vertexShader = TRUE;
}

static void wined3d_bitmap_set_bits(uint32_t *bitmap, unsigned int start, unsigned int count)
{
    const unsigned int word_bit_count = sizeof(*bitmap) * CHAR_BIT;
    const unsigned int shift = start % word_bit_count;
    uint32_t mask, last_mask;
    unsigned int mask_size;

    bitmap += start / word_bit_count;
    mask = ~0u << shift;
    mask_size = word_bit_count - shift;
    last_mask = (1u << (start + count) % word_bit_count) - 1;
    if (mask_size <= count)
    {
        *bitmap |= mask;
        ++bitmap;
        count -= mask_size;
        mask = ~0u;
    }
    if (count >= word_bit_count)
    {
        memset(bitmap, 0xffu, count / word_bit_count * sizeof(*bitmap));
        bitmap += count / word_bit_count;
        count = count % word_bit_count;
    }
    if (count)
        *bitmap |= mask & last_mask;
}

HRESULT CDECL wined3d_stateblock_set_vs_consts_f(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants)
{
    const struct wined3d_d3d_info *d3d_info = &stateblock->device->adapter->d3d_info;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    if (!constants || !wined3d_bound_range(start_idx, count, d3d_info->limits.vs_uniform_count))
        return WINED3DERR_INVALIDCALL;

    memcpy(&stateblock->stateblock_state.vs_consts_f[start_idx], constants, count * sizeof(*constants));
    wined3d_bitmap_set_bits(stateblock->changed.vs_consts_f, start_idx, count);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_set_vs_consts_i(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants)
{
    unsigned int i;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_I)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_I - start_idx)
        count = WINED3D_MAX_CONSTS_I - start_idx;

    memcpy(&stateblock->stateblock_state.vs_consts_i[start_idx], constants, count * sizeof(*constants));
    for (i = start_idx; i < count + start_idx; ++i)
        stateblock->changed.vertexShaderConstantsI |= (1u << i);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_set_vs_consts_b(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const BOOL *constants)
{
    unsigned int i;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_B)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_B - start_idx)
        count = WINED3D_MAX_CONSTS_B - start_idx;

    memcpy(&stateblock->stateblock_state.vs_consts_b[start_idx], constants, count * sizeof(*constants));
    for (i = start_idx; i < count + start_idx; ++i)
        stateblock->changed.vertexShaderConstantsB |= (1u << i);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_get_vs_consts_f(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, struct wined3d_vec4 *constants)
{
    const struct wined3d_d3d_info *d3d_info = &stateblock->device->adapter->d3d_info;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n", stateblock, start_idx, count, constants);

    if (!constants || !wined3d_bound_range(start_idx, count, d3d_info->limits.vs_uniform_count))
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &stateblock->stateblock_state.vs_consts_f[start_idx], count * sizeof(*constants));
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_get_vs_consts_i(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, struct wined3d_ivec4 *constants)
{
    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n", stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_I)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_I - start_idx)
        count = WINED3D_MAX_CONSTS_I - start_idx;

    memcpy(constants, &stateblock->stateblock_state.vs_consts_i[start_idx], count * sizeof(*constants));
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_get_vs_consts_b(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, BOOL *constants)
{
    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n", stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_B)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_B - start_idx)
        count = WINED3D_MAX_CONSTS_B - start_idx;

    memcpy(constants, &stateblock->stateblock_state.vs_consts_b[start_idx], count * sizeof(*constants));
    return WINED3D_OK;
}

void CDECL wined3d_stateblock_set_pixel_shader(struct wined3d_stateblock *stateblock, struct wined3d_shader *shader)
{
    TRACE("stateblock %p, shader %p.\n", stateblock, shader);

    if (shader)
        wined3d_shader_incref(shader);
    if (stateblock->stateblock_state.ps)
        wined3d_shader_decref(stateblock->stateblock_state.ps);
    stateblock->stateblock_state.ps = shader;
    stateblock->changed.pixelShader = TRUE;
}

HRESULT CDECL wined3d_stateblock_set_ps_consts_f(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants)
{
    const struct wined3d_d3d_info *d3d_info = &stateblock->device->adapter->d3d_info;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    if (!constants || !wined3d_bound_range(start_idx, count, d3d_info->limits.ps_uniform_count))
        return WINED3DERR_INVALIDCALL;

    memcpy(&stateblock->stateblock_state.ps_consts_f[start_idx], constants, count * sizeof(*constants));
    wined3d_bitmap_set_bits(stateblock->changed.ps_consts_f, start_idx, count);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_set_ps_consts_i(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants)
{
    unsigned int i;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_I)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_I - start_idx)
        count = WINED3D_MAX_CONSTS_I - start_idx;

    memcpy(&stateblock->stateblock_state.ps_consts_i[start_idx], constants, count * sizeof(*constants));
    for (i = start_idx; i < count + start_idx; ++i)
        stateblock->changed.pixelShaderConstantsI |= (1u << i);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_set_ps_consts_b(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const BOOL *constants)
{
    unsigned int i;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_B)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_B - start_idx)
        count = WINED3D_MAX_CONSTS_B - start_idx;

    memcpy(&stateblock->stateblock_state.ps_consts_b[start_idx], constants, count * sizeof(*constants));
    for (i = start_idx; i < count + start_idx; ++i)
        stateblock->changed.pixelShaderConstantsB |= (1u << i);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_get_ps_consts_f(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, struct wined3d_vec4 *constants)
{
    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n", stateblock, start_idx, count, constants);

    if (!constants || !wined3d_bound_range(start_idx, count, WINED3D_MAX_PS_CONSTS_F))
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &stateblock->stateblock_state.ps_consts_f[start_idx], count * sizeof(*constants));
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_get_ps_consts_i(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, struct wined3d_ivec4 *constants)
{
    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n", stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_I)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_I - start_idx)
        count = WINED3D_MAX_CONSTS_I - start_idx;

    memcpy(constants, &stateblock->stateblock_state.ps_consts_i[start_idx], count * sizeof(*constants));
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_get_ps_consts_b(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, BOOL *constants)
{
    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n", stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_B)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_B - start_idx)
        count = WINED3D_MAX_CONSTS_B - start_idx;

    memcpy(constants, &stateblock->stateblock_state.ps_consts_b[start_idx], count * sizeof(*constants));
    return WINED3D_OK;
}

void CDECL wined3d_stateblock_set_vertex_declaration(struct wined3d_stateblock *stateblock,
        struct wined3d_vertex_declaration *declaration)
{
    TRACE("stateblock %p, declaration %p.\n", stateblock, declaration);

    if (declaration)
        wined3d_vertex_declaration_incref(declaration);
    if (stateblock->stateblock_state.vertex_declaration)
        wined3d_vertex_declaration_decref(stateblock->stateblock_state.vertex_declaration);
    stateblock->stateblock_state.vertex_declaration = declaration;
    stateblock->changed.vertexDecl = TRUE;
}

void CDECL wined3d_stateblock_set_render_state(struct wined3d_stateblock *stateblock,
        enum wined3d_render_state state, unsigned int value)
{
    TRACE("stateblock %p, state %s (%#x), value %#x.\n", stateblock, debug_d3drenderstate(state), state, value);

    if (state > WINEHIGHEST_RENDER_STATE)
    {
        WARN("Unhandled render state %#x.\n", state);
        return;
    }

    stateblock->stateblock_state.rs[state] = value;
    stateblock->changed.renderState[state >> 5] |= 1u << (state & 0x1f);

    if (state == WINED3D_RS_POINTSIZE
            && (value == WINED3D_ALPHA_TO_COVERAGE_ENABLE || value == WINED3D_ALPHA_TO_COVERAGE_DISABLE))
    {
        stateblock->changed.alpha_to_coverage = 1;
        stateblock->stateblock_state.alpha_to_coverage = (value == WINED3D_ALPHA_TO_COVERAGE_ENABLE);
    }
}

void CDECL wined3d_stateblock_set_sampler_state(struct wined3d_stateblock *stateblock,
        UINT sampler_idx, enum wined3d_sampler_state state, unsigned int value)
{
    TRACE("stateblock %p, sampler_idx %u, state %s, value %#x.\n",
            stateblock, sampler_idx, debug_d3dsamplerstate(state), value);

    if (sampler_idx >= ARRAY_SIZE(stateblock->stateblock_state.sampler_states))
    {
        WARN("Invalid sampler %u.\n", sampler_idx);
        return;
    }

    stateblock->stateblock_state.sampler_states[sampler_idx][state] = value;
    stateblock->changed.samplerState[sampler_idx] |= 1u << state;
}

void CDECL wined3d_stateblock_set_texture_stage_state(struct wined3d_stateblock *stateblock,
        UINT stage, enum wined3d_texture_stage_state state, unsigned int value)
{
    TRACE("stateblock %p, stage %u, state %s, value %#x.\n",
            stateblock, stage, debug_d3dtexturestate(state), value);

    if (state > WINED3D_HIGHEST_TEXTURE_STATE)
    {
        WARN("Invalid state %#x passed.\n", state);
        return;
    }

    if (stage >= WINED3D_MAX_FFP_TEXTURES)
    {
        WARN("Attempting to set stage %u which is higher than the max stage %u, ignoring.\n",
                stage, WINED3D_MAX_FFP_TEXTURES - 1);
        return;
    }

    stateblock->stateblock_state.texture_states[stage][state] = value;
    stateblock->changed.textureState[stage] |= 1u << state;
}

void CDECL wined3d_stateblock_set_texture(struct wined3d_stateblock *stateblock,
        UINT stage, struct wined3d_texture *texture)
{
    TRACE("stateblock %p, stage %u, texture %p.\n", stateblock, stage, texture);

    if (stage >= ARRAY_SIZE(stateblock->stateblock_state.textures))
    {
        WARN("Ignoring invalid stage %u.\n", stage);
        return;
    }

    if (texture)
        wined3d_texture_incref(texture);
    if (stateblock->stateblock_state.textures[stage])
        wined3d_texture_decref(stateblock->stateblock_state.textures[stage]);
    stateblock->stateblock_state.textures[stage] = texture;
    stateblock->changed.textures |= 1u << stage;
}

void CDECL wined3d_stateblock_set_transform(struct wined3d_stateblock *stateblock,
        enum wined3d_transform_state d3dts, const struct wined3d_matrix *matrix)
{
    TRACE("stateblock %p, state %s, matrix %p.\n", stateblock, debug_d3dtstype(d3dts), matrix);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_11, matrix->_12, matrix->_13, matrix->_14);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_21, matrix->_22, matrix->_23, matrix->_24);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_31, matrix->_32, matrix->_33, matrix->_34);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_41, matrix->_42, matrix->_43, matrix->_44);

    stateblock->stateblock_state.transforms[d3dts] = *matrix;
    stateblock->changed.transform[d3dts >> 5] |= 1u << (d3dts & 0x1f);
    stateblock->changed.transforms = 1;
}

void CDECL wined3d_stateblock_multiply_transform(struct wined3d_stateblock *stateblock,
        enum wined3d_transform_state d3dts, const struct wined3d_matrix *matrix)
{
    struct wined3d_matrix *mat = &stateblock->stateblock_state.transforms[d3dts];

    TRACE("stateblock %p, state %s, matrix %p.\n", stateblock, debug_d3dtstype(d3dts), matrix);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_11, matrix->_12, matrix->_13, matrix->_14);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_21, matrix->_22, matrix->_23, matrix->_24);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_31, matrix->_32, matrix->_33, matrix->_34);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_41, matrix->_42, matrix->_43, matrix->_44);

    multiply_matrix(mat, mat, matrix);
    stateblock->changed.transform[d3dts >> 5] |= 1u << (d3dts & 0x1f);
    stateblock->changed.transforms = 1;
}

HRESULT CDECL wined3d_stateblock_set_clip_plane(struct wined3d_stateblock *stateblock,
        UINT plane_idx, const struct wined3d_vec4 *plane)
{
    TRACE("stateblock %p, plane_idx %u, plane %p.\n", stateblock, plane_idx, plane);

    if (plane_idx >= stateblock->device->adapter->d3d_info.limits.max_clip_distances)
    {
        TRACE("Application has requested clipplane this device doesn't support.\n");
        return WINED3DERR_INVALIDCALL;
    }

    stateblock->stateblock_state.clip_planes[plane_idx] = *plane;
    stateblock->changed.clipplane |= 1u << plane_idx;
    return S_OK;
}

void CDECL wined3d_stateblock_set_material(struct wined3d_stateblock *stateblock,
        const struct wined3d_material *material)
{
    TRACE("stateblock %p, material %p.\n", stateblock, material);

    stateblock->stateblock_state.material = *material;
    stateblock->changed.material = TRUE;
}

void CDECL wined3d_stateblock_set_viewport(struct wined3d_stateblock *stateblock,
        const struct wined3d_viewport *viewport)
{
    TRACE("stateblock %p, viewport %p.\n", stateblock, viewport);

    stateblock->stateblock_state.viewport = *viewport;
    stateblock->changed.viewport = TRUE;
}

void CDECL wined3d_stateblock_set_scissor_rect(struct wined3d_stateblock *stateblock, const RECT *rect)
{
    TRACE("stateblock %p, rect %s.\n", stateblock, wine_dbgstr_rect(rect));

    stateblock->stateblock_state.scissor_rect = *rect;
    stateblock->changed.scissorRect = TRUE;
}

void CDECL wined3d_stateblock_set_index_buffer(struct wined3d_stateblock *stateblock,
        struct wined3d_buffer *buffer, enum wined3d_format_id format_id)
{
    TRACE("stateblock %p, buffer %p, format %s.\n", stateblock, buffer, debug_d3dformat(format_id));

    if (buffer)
        wined3d_buffer_incref(buffer);
    if (stateblock->stateblock_state.index_buffer)
        wined3d_buffer_decref(stateblock->stateblock_state.index_buffer);
    stateblock->stateblock_state.index_buffer = buffer;
    stateblock->stateblock_state.index_format = format_id;
    stateblock->changed.indices = TRUE;
}

void CDECL wined3d_stateblock_set_base_vertex_index(struct wined3d_stateblock *stateblock, INT base_index)
{
    TRACE("stateblock %p, base_index %d.\n", stateblock, base_index);

    stateblock->stateblock_state.base_vertex_index = base_index;
}

HRESULT CDECL wined3d_stateblock_set_stream_source(struct wined3d_stateblock *stateblock,
        UINT stream_idx, struct wined3d_buffer *buffer, UINT offset, UINT stride)
{
    struct wined3d_stream_state *stream;

    TRACE("stateblock %p, stream_idx %u, buffer %p, stride %u.\n",
            stateblock, stream_idx, buffer, stride);

    if (stream_idx >= WINED3D_MAX_STREAMS)
    {
        WARN("Stream index %u out of range.\n", stream_idx);
        return WINED3DERR_INVALIDCALL;
    }

    stream = &stateblock->stateblock_state.streams[stream_idx];

    if (buffer)
        wined3d_buffer_incref(buffer);
    if (stream->buffer)
        wined3d_buffer_decref(stream->buffer);
    stream->buffer = buffer;
    stream->stride = stride;
    stream->offset = offset;
    stateblock->changed.streamSource |= 1u << stream_idx;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_set_stream_source_freq(struct wined3d_stateblock *stateblock,
        UINT stream_idx, UINT divider)
{
    struct wined3d_stream_state *stream;

    TRACE("stateblock %p, stream_idx %u, divider %#x.\n", stateblock, stream_idx, divider);

    if ((divider & WINED3DSTREAMSOURCE_INSTANCEDATA) && (divider & WINED3DSTREAMSOURCE_INDEXEDDATA))
    {
        WARN("INSTANCEDATA and INDEXEDDATA were set, returning D3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }
    if ((divider & WINED3DSTREAMSOURCE_INSTANCEDATA) && !stream_idx)
    {
        WARN("INSTANCEDATA used on stream 0, returning D3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }
    if (!divider)
    {
        WARN("Divider is 0, returning D3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    stream = &stateblock->stateblock_state.streams[stream_idx];
    stream->flags = divider & (WINED3DSTREAMSOURCE_INSTANCEDATA | WINED3DSTREAMSOURCE_INDEXEDDATA);
    stream->frequency = divider & 0x7fffff;
    stateblock->changed.streamFreq |= 1u << stream_idx;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_set_light(struct wined3d_stateblock *stateblock,
        UINT light_idx, const struct wined3d_light *light)
{
    struct wined3d_light_info *object = NULL;
    HRESULT hr;

    TRACE("stateblock %p, light_idx %u, light %p.\n", stateblock, light_idx, light);

    /* Check the parameter range. Need for speed most wanted sets junk lights
     * which confuse the GL driver. */
    if (!light)
        return WINED3DERR_INVALIDCALL;

    switch (light->type)
    {
        case WINED3D_LIGHT_POINT:
        case WINED3D_LIGHT_SPOT:
        case WINED3D_LIGHT_GLSPOT:
            /* Incorrect attenuation values can cause the gl driver to crash.
             * Happens with Need for speed most wanted. */
            if (light->attenuation0 < 0.0f || light->attenuation1 < 0.0f || light->attenuation2 < 0.0f)
            {
                WARN("Attenuation is negative, returning WINED3DERR_INVALIDCALL.\n");
                return WINED3DERR_INVALIDCALL;
            }
            break;

        case WINED3D_LIGHT_DIRECTIONAL:
        case WINED3D_LIGHT_PARALLELPOINT:
            /* Ignores attenuation */
            break;

        default:
            WARN("Light type out of range, returning WINED3DERR_INVALIDCALL.\n");
            return WINED3DERR_INVALIDCALL;
    }

    if (SUCCEEDED(hr = wined3d_light_state_set_light(stateblock->stateblock_state.light_state, light_idx, light, &object)))
        set_light_changed(stateblock, object);
    return hr;
}

HRESULT CDECL wined3d_stateblock_set_light_enable(struct wined3d_stateblock *stateblock, UINT light_idx, BOOL enable)
{
    struct wined3d_light_state *light_state = stateblock->stateblock_state.light_state;
    struct wined3d_light_info *light_info;
    HRESULT hr;

    TRACE("stateblock %p, light_idx %u, enable %#x.\n", stateblock, light_idx, enable);

    if (!(light_info = wined3d_light_state_get_light(light_state, light_idx)))
    {
        if (FAILED(hr = wined3d_light_state_set_light(light_state, light_idx, &WINED3D_default_light, &light_info)))
            return hr;
        set_light_changed(stateblock, light_info);
    }

    if (wined3d_light_state_enable_light(light_state, &stateblock->device->adapter->d3d_info, light_info, enable))
        set_light_changed(stateblock, light_info);

    return S_OK;
}

const struct wined3d_stateblock_state * CDECL wined3d_stateblock_get_state(const struct wined3d_stateblock *stateblock)
{
    return &stateblock->stateblock_state;
}

HRESULT CDECL wined3d_stateblock_get_light(const struct wined3d_stateblock *stateblock,
        UINT light_idx, struct wined3d_light *light, BOOL *enabled)
{
    struct wined3d_light_info *light_info;

    if (!(light_info = wined3d_light_state_get_light(&stateblock->light_state, light_idx)))
    {
        TRACE("Light %u is not defined.\n", light_idx);
        return WINED3DERR_INVALIDCALL;
    }
    *light = light_info->OriginalParms;
    *enabled = light_info->enabled ? 128 : 0;
    return WINED3D_OK;
}

static void init_default_render_states(unsigned int rs[WINEHIGHEST_RENDER_STATE + 1], const struct wined3d_d3d_info *d3d_info)
{
    union
    {
        struct wined3d_line_pattern lp;
        DWORD d;
    } lp;
    union
    {
        float f;
        DWORD d;
    } tmpfloat;

    rs[WINED3D_RS_ZENABLE] = WINED3D_ZB_TRUE;
    rs[WINED3D_RS_FILLMODE] = WINED3D_FILL_SOLID;
    rs[WINED3D_RS_SHADEMODE] = WINED3D_SHADE_GOURAUD;
    lp.lp.repeat_factor = 0;
    lp.lp.line_pattern = 0;
    rs[WINED3D_RS_LINEPATTERN] = lp.d;
    rs[WINED3D_RS_ZWRITEENABLE] = TRUE;
    rs[WINED3D_RS_ALPHATESTENABLE] = FALSE;
    rs[WINED3D_RS_LASTPIXEL] = TRUE;
    rs[WINED3D_RS_SRCBLEND] = WINED3D_BLEND_ONE;
    rs[WINED3D_RS_DESTBLEND] = WINED3D_BLEND_ZERO;
    rs[WINED3D_RS_CULLMODE] = WINED3D_CULL_BACK;
    rs[WINED3D_RS_ZFUNC] = WINED3D_CMP_LESSEQUAL;
    rs[WINED3D_RS_ALPHAFUNC] = WINED3D_CMP_ALWAYS;
    rs[WINED3D_RS_ALPHAREF] = 0;
    rs[WINED3D_RS_DITHERENABLE] = FALSE;
    rs[WINED3D_RS_ALPHABLENDENABLE] = FALSE;
    rs[WINED3D_RS_FOGENABLE] = FALSE;
    rs[WINED3D_RS_SPECULARENABLE] = FALSE;
    rs[WINED3D_RS_ZVISIBLE] = 0;
    rs[WINED3D_RS_FOGCOLOR] = 0;
    rs[WINED3D_RS_FOGTABLEMODE] = WINED3D_FOG_NONE;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_FOGSTART] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_FOGEND] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_FOGDENSITY] = tmpfloat.d;
    rs[WINED3D_RS_RANGEFOGENABLE] = FALSE;
    rs[WINED3D_RS_STENCILENABLE] = FALSE;
    rs[WINED3D_RS_STENCILFAIL] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_STENCILZFAIL] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_STENCILPASS] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_STENCILREF] = 0;
    rs[WINED3D_RS_STENCILMASK] = 0xffffffff;
    rs[WINED3D_RS_STENCILFUNC] = WINED3D_CMP_ALWAYS;
    rs[WINED3D_RS_STENCILWRITEMASK] = 0xffffffff;
    rs[WINED3D_RS_TEXTUREFACTOR] = 0xffffffff;
    rs[WINED3D_RS_WRAP0] = 0;
    rs[WINED3D_RS_WRAP1] = 0;
    rs[WINED3D_RS_WRAP2] = 0;
    rs[WINED3D_RS_WRAP3] = 0;
    rs[WINED3D_RS_WRAP4] = 0;
    rs[WINED3D_RS_WRAP5] = 0;
    rs[WINED3D_RS_WRAP6] = 0;
    rs[WINED3D_RS_WRAP7] = 0;
    rs[WINED3D_RS_CLIPPING] = TRUE;
    rs[WINED3D_RS_LIGHTING] = TRUE;
    rs[WINED3D_RS_AMBIENT] = 0;
    rs[WINED3D_RS_FOGVERTEXMODE] = WINED3D_FOG_NONE;
    rs[WINED3D_RS_COLORVERTEX] = TRUE;
    rs[WINED3D_RS_LOCALVIEWER] = TRUE;
    rs[WINED3D_RS_NORMALIZENORMALS] = FALSE;
    rs[WINED3D_RS_DIFFUSEMATERIALSOURCE] = WINED3D_MCS_COLOR1;
    rs[WINED3D_RS_SPECULARMATERIALSOURCE] = WINED3D_MCS_COLOR2;
    rs[WINED3D_RS_AMBIENTMATERIALSOURCE] = WINED3D_MCS_MATERIAL;
    rs[WINED3D_RS_EMISSIVEMATERIALSOURCE] = WINED3D_MCS_MATERIAL;
    rs[WINED3D_RS_VERTEXBLEND] = WINED3D_VBF_DISABLE;
    rs[WINED3D_RS_CLIPPLANEENABLE] = 0;
    rs[WINED3D_RS_SOFTWAREVERTEXPROCESSING] = FALSE;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_POINTSIZE] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_POINTSIZE_MIN] = tmpfloat.d;
    rs[WINED3D_RS_POINTSPRITEENABLE] = FALSE;
    rs[WINED3D_RS_POINTSCALEENABLE] = FALSE;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_POINTSCALE_A] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_POINTSCALE_B] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_POINTSCALE_C] = tmpfloat.d;
    rs[WINED3D_RS_MULTISAMPLEANTIALIAS] = TRUE;
    rs[WINED3D_RS_MULTISAMPLEMASK] = 0xffffffff;
    rs[WINED3D_RS_PATCHEDGESTYLE] = WINED3D_PATCH_EDGE_DISCRETE;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_PATCHSEGMENTS] = tmpfloat.d;
    rs[WINED3D_RS_DEBUGMONITORTOKEN] = 0xbaadcafe;
    tmpfloat.f = d3d_info->limits.pointsize_max;
    rs[WINED3D_RS_POINTSIZE_MAX] = tmpfloat.d;
    rs[WINED3D_RS_INDEXEDVERTEXBLENDENABLE] = FALSE;
    rs[WINED3D_RS_COLORWRITEENABLE] = 0x0000000f;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_TWEENFACTOR] = tmpfloat.d;
    rs[WINED3D_RS_BLENDOP] = WINED3D_BLEND_OP_ADD;
    rs[WINED3D_RS_POSITIONDEGREE] = WINED3D_DEGREE_CUBIC;
    rs[WINED3D_RS_NORMALDEGREE] = WINED3D_DEGREE_LINEAR;
    /* states new in d3d9 */
    rs[WINED3D_RS_SCISSORTESTENABLE] = FALSE;
    rs[WINED3D_RS_SLOPESCALEDEPTHBIAS] = 0;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_MINTESSELLATIONLEVEL] = tmpfloat.d;
    rs[WINED3D_RS_MAXTESSELLATIONLEVEL] = tmpfloat.d;
    rs[WINED3D_RS_ANTIALIASEDLINEENABLE] = FALSE;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_ADAPTIVETESS_X] = tmpfloat.d;
    rs[WINED3D_RS_ADAPTIVETESS_Y] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_ADAPTIVETESS_Z] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_ADAPTIVETESS_W] = tmpfloat.d;
    rs[WINED3D_RS_ENABLEADAPTIVETESSELLATION] = FALSE;
    rs[WINED3D_RS_TWOSIDEDSTENCILMODE] = FALSE;
    rs[WINED3D_RS_BACK_STENCILFAIL] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_BACK_STENCILZFAIL] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_BACK_STENCILPASS] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_BACK_STENCILFUNC] = WINED3D_CMP_ALWAYS;
    rs[WINED3D_RS_COLORWRITEENABLE1] = 0x0000000f;
    rs[WINED3D_RS_COLORWRITEENABLE2] = 0x0000000f;
    rs[WINED3D_RS_COLORWRITEENABLE3] = 0x0000000f;
    rs[WINED3D_RS_BLENDFACTOR] = 0xffffffff;
    rs[WINED3D_RS_SRGBWRITEENABLE] = 0;
    rs[WINED3D_RS_DEPTHBIAS] = 0;
    rs[WINED3D_RS_WRAP8] = 0;
    rs[WINED3D_RS_WRAP9] = 0;
    rs[WINED3D_RS_WRAP10] = 0;
    rs[WINED3D_RS_WRAP11] = 0;
    rs[WINED3D_RS_WRAP12] = 0;
    rs[WINED3D_RS_WRAP13] = 0;
    rs[WINED3D_RS_WRAP14] = 0;
    rs[WINED3D_RS_WRAP15] = 0;
    rs[WINED3D_RS_SEPARATEALPHABLENDENABLE] = FALSE;
    rs[WINED3D_RS_SRCBLENDALPHA] = WINED3D_BLEND_ONE;
    rs[WINED3D_RS_DESTBLENDALPHA] = WINED3D_BLEND_ZERO;
    rs[WINED3D_RS_BLENDOPALPHA] = WINED3D_BLEND_OP_ADD;
}

static void init_default_texture_state(unsigned int i, uint32_t stage[WINED3D_HIGHEST_TEXTURE_STATE + 1])
{
    stage[WINED3D_TSS_COLOR_OP] = i ? WINED3D_TOP_DISABLE : WINED3D_TOP_MODULATE;
    stage[WINED3D_TSS_COLOR_ARG1] = WINED3DTA_TEXTURE;
    stage[WINED3D_TSS_COLOR_ARG2] = WINED3DTA_CURRENT;
    stage[WINED3D_TSS_ALPHA_OP] = i ? WINED3D_TOP_DISABLE : WINED3D_TOP_SELECT_ARG1;
    stage[WINED3D_TSS_ALPHA_ARG1] = WINED3DTA_TEXTURE;
    stage[WINED3D_TSS_ALPHA_ARG2] = WINED3DTA_CURRENT;
    stage[WINED3D_TSS_BUMPENV_MAT00] = 0;
    stage[WINED3D_TSS_BUMPENV_MAT01] = 0;
    stage[WINED3D_TSS_BUMPENV_MAT10] = 0;
    stage[WINED3D_TSS_BUMPENV_MAT11] = 0;
    stage[WINED3D_TSS_TEXCOORD_INDEX] = i;
    stage[WINED3D_TSS_BUMPENV_LSCALE] = 0;
    stage[WINED3D_TSS_BUMPENV_LOFFSET] = 0;
    stage[WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS] = WINED3D_TTFF_DISABLE;
    stage[WINED3D_TSS_COLOR_ARG0] = WINED3DTA_CURRENT;
    stage[WINED3D_TSS_ALPHA_ARG0] = WINED3DTA_CURRENT;
    stage[WINED3D_TSS_RESULT_ARG] = WINED3DTA_CURRENT;
}

static void init_default_sampler_states(uint32_t states[WINED3D_MAX_COMBINED_SAMPLERS][WINED3D_HIGHEST_SAMPLER_STATE + 1])
{
    unsigned int i;

    for (i = 0 ; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i)
    {
        TRACE("Setting up default samplers states for sampler %u.\n", i);
        states[i][WINED3D_SAMP_ADDRESS_U] = WINED3D_TADDRESS_WRAP;
        states[i][WINED3D_SAMP_ADDRESS_V] = WINED3D_TADDRESS_WRAP;
        states[i][WINED3D_SAMP_ADDRESS_W] = WINED3D_TADDRESS_WRAP;
        states[i][WINED3D_SAMP_BORDER_COLOR] = 0;
        states[i][WINED3D_SAMP_MAG_FILTER] = WINED3D_TEXF_POINT;
        states[i][WINED3D_SAMP_MIN_FILTER] = WINED3D_TEXF_POINT;
        states[i][WINED3D_SAMP_MIP_FILTER] = WINED3D_TEXF_NONE;
        states[i][WINED3D_SAMP_MIPMAP_LOD_BIAS] = 0;
        states[i][WINED3D_SAMP_MAX_MIP_LEVEL] = 0;
        states[i][WINED3D_SAMP_MAX_ANISOTROPY] = 1;
        states[i][WINED3D_SAMP_SRGB_TEXTURE] = 0;
        /* TODO: Indicates which element of a multielement texture to use. */
        states[i][WINED3D_SAMP_ELEMENT_INDEX] = 0;
        /* TODO: Vertex offset in the presampled displacement map. */
        states[i][WINED3D_SAMP_DMAP_OFFSET] = 0;
    }
}

static void state_init_default(struct wined3d_state *state, const struct wined3d_d3d_info *d3d_info)
{
    struct wined3d_matrix identity;
    unsigned int i, j;

    TRACE("state %p, d3d_info %p.\n", state, d3d_info);

    get_identity_matrix(&identity);
    state->primitive_type = WINED3D_PT_UNDEFINED;
    state->patch_vertex_count = 0;

    /* Set some of the defaults for lights, transforms etc */
    state->transforms[WINED3D_TS_PROJECTION] = identity;
    state->transforms[WINED3D_TS_VIEW] = identity;
    for (i = 0; i < 256; ++i)
    {
        state->transforms[WINED3D_TS_WORLD_MATRIX(i)] = identity;
    }

    init_default_render_states(state->render_states, d3d_info);

    /* Texture Stage States - Put directly into state block, we will call function below */
    for (i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
    {
        TRACE("Setting up default texture states for texture Stage %u.\n", i);
        state->transforms[WINED3D_TS_TEXTURE0 + i] = identity;
        init_default_texture_state(i, state->texture_states[i]);
    }

    state->blend_factor.r = 1.0f;
    state->blend_factor.g = 1.0f;
    state->blend_factor.b = 1.0f;
    state->blend_factor.a = 1.0f;

    state->sample_mask = 0xffffffff;

    for (i = 0; i < WINED3D_MAX_STREAMS; ++i)
        state->streams[i].frequency = 1;

    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        for (j = 0; j < MAX_CONSTANT_BUFFERS; ++j)
            state->cb[i][j].size = WINED3D_MAX_CONSTANT_BUFFER_SIZE * 16;
    }
}

static int lights_compare(const void *key, const struct rb_entry *entry)
{
    const struct wined3d_light_info *light = RB_ENTRY_VALUE(entry, struct wined3d_light_info, entry);
    unsigned int original_index = (ULONG_PTR)key;

    return wined3d_uint32_compare(light->OriginalIndex, original_index);
}

void state_init(struct wined3d_state *state, const struct wined3d_d3d_info *d3d_info,
        uint32_t flags, enum wined3d_feature_level feature_level)
{
    state->feature_level = feature_level;
    state->flags = flags;

    rb_init(&state->light_state.lights_tree, lights_compare);

    if (flags & WINED3D_STATE_INIT_DEFAULT)
        state_init_default(state, d3d_info);
}

static bool wined3d_select_feature_level(const struct wined3d_adapter *adapter,
        const enum wined3d_feature_level *levels, unsigned int level_count,
        enum wined3d_feature_level *selected_level)
{
    const struct wined3d_d3d_info *d3d_info = &adapter->d3d_info;
    unsigned int i;

    for (i = 0; i < level_count; ++i)
    {
        if (levels[i] && d3d_info->feature_level >= levels[i])
        {
            *selected_level = levels[i];
            return true;
        }
    }

    FIXME_(winediag)("None of the requested D3D feature levels is supported on this GPU "
            "with the current shader backend.\n");
    return false;
}

HRESULT CDECL wined3d_state_create(struct wined3d_device *device,
        const enum wined3d_feature_level *levels, unsigned int level_count, struct wined3d_state **state)
{
    enum wined3d_feature_level feature_level;
    struct wined3d_state *object;

    TRACE("device %p, levels %p, level_count %u, state %p.\n", device, levels, level_count, state);

    if (!wined3d_select_feature_level(device->adapter, levels, level_count, &feature_level))
        return E_FAIL;

    TRACE("Selected feature level %s.\n", wined3d_debug_feature_level(feature_level));

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;
    state_init(object, &device->adapter->d3d_info, WINED3D_STATE_INIT_DEFAULT, feature_level);

    *state = object;
    return S_OK;
}

enum wined3d_feature_level CDECL wined3d_state_get_feature_level(const struct wined3d_state *state)
{
    TRACE("state %p.\n", state);

    return state->feature_level;
}

void CDECL wined3d_state_destroy(struct wined3d_state *state)
{
    TRACE("state %p.\n", state);

    state_cleanup(state);
    heap_free(state);
}

static void stateblock_state_init_default(struct wined3d_stateblock_state *state,
        const struct wined3d_d3d_info *d3d_info)
{
    struct wined3d_matrix identity;
    unsigned int i;

    get_identity_matrix(&identity);

    state->transforms[WINED3D_TS_PROJECTION] = identity;
    state->transforms[WINED3D_TS_VIEW] = identity;
    for (i = 0; i < 256; ++i)
    {
        state->transforms[WINED3D_TS_WORLD_MATRIX(i)] = identity;
    }

    init_default_render_states(state->rs, d3d_info);

    for (i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
    {
        state->transforms[WINED3D_TS_TEXTURE0 + i] = identity;
        init_default_texture_state(i, state->texture_states[i]);
    }

    init_default_sampler_states(state->sampler_states);

    for (i = 0; i < WINED3D_MAX_STREAMS; ++i)
        state->streams[i].frequency = 1;
}

static void wined3d_stateblock_state_init(struct wined3d_stateblock_state *state,
        const struct wined3d_device *device, uint32_t flags)
{
    rb_init(&state->light_state->lights_tree, lights_compare);

    if (flags & WINED3D_STATE_INIT_DEFAULT)
        stateblock_state_init_default(state, &device->adapter->d3d_info);

}

static HRESULT stateblock_init(struct wined3d_stateblock *stateblock, const struct wined3d_stateblock *device_state,
        struct wined3d_device *device, enum wined3d_stateblock_type type)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;

    stateblock->ref = 1;
    stateblock->device = device;
    stateblock->stateblock_state.light_state = &stateblock->light_state;
    wined3d_stateblock_state_init(&stateblock->stateblock_state, device,
            type == WINED3D_SBT_PRIMARY ? WINED3D_STATE_INIT_DEFAULT : 0);

    stateblock->changed.store_stream_offset = 1;
    list_init(&stateblock->changed.changed_lights);

    if (type == WINED3D_SBT_RECORDED || type == WINED3D_SBT_PRIMARY)
        return WINED3D_OK;

    TRACE("Updating changed flags appropriate for type %#x.\n", type);

    switch (type)
    {
        case WINED3D_SBT_ALL:
            stateblock_init_lights(stateblock, &device_state->stateblock_state.light_state->lights_tree);
            stateblock_savedstates_set_all(&stateblock->changed,
                    d3d_info->limits.vs_uniform_count, d3d_info->limits.ps_uniform_count);
            break;

        case WINED3D_SBT_PIXEL_STATE:
            stateblock_savedstates_set_pixel(&stateblock->changed,
                    d3d_info->limits.ps_uniform_count);
            break;

        case WINED3D_SBT_VERTEX_STATE:
            stateblock_init_lights(stateblock, &device_state->stateblock_state.light_state->lights_tree);
            stateblock_savedstates_set_vertex(&stateblock->changed,
                    d3d_info->limits.vs_uniform_count);
            break;

        default:
            FIXME("Unrecognized state block type %#x.\n", type);
            break;
    }

    wined3d_stateblock_init_contained_states(stateblock);
    wined3d_stateblock_capture(stateblock, device_state);

    /* According to the tests, stream offset is not updated in the captured state if
     * the state was captured on state block creation. This is not the case for
     * state blocks initialized with BeginStateBlock / EndStateBlock, multiple
     * captures get stream offsets updated. */
    stateblock->changed.store_stream_offset = 0;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_create(struct wined3d_device *device, const struct wined3d_stateblock *device_state,
        enum wined3d_stateblock_type type, struct wined3d_stateblock **stateblock)
{
    struct wined3d_stateblock *object;
    HRESULT hr;

    TRACE("device %p, device_state %p, type %#x, stateblock %p.\n",
            device, device_state, type, stateblock);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = stateblock_init(object, device_state, device, type);
    if (FAILED(hr))
    {
        WARN("Failed to initialize stateblock, hr %#lx.\n", hr);
        heap_free(object);
        return hr;
    }

    TRACE("Created stateblock %p.\n", object);
    *stateblock = object;

    return WINED3D_OK;
}

void CDECL wined3d_stateblock_reset(struct wined3d_stateblock *stateblock)
{
    TRACE("stateblock %p.\n", stateblock);

    wined3d_stateblock_state_cleanup(&stateblock->stateblock_state);
    memset(&stateblock->stateblock_state, 0, sizeof(stateblock->stateblock_state));
    stateblock->stateblock_state.light_state = &stateblock->light_state;
    wined3d_stateblock_state_init(&stateblock->stateblock_state, stateblock->device, WINED3D_STATE_INIT_DEFAULT);
}

static void wined3d_device_set_base_vertex_index(struct wined3d_device *device, int base_index)
{
    TRACE("device %p, base_index %d.\n", device, base_index);

    device->cs->c.state->base_vertex_index = base_index;
}

static void wined3d_device_set_vs_consts_b(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const BOOL *constants)
{
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n", device, start_idx, count, constants);

    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set BOOL constant %u to %#x.\n", start_idx + i, constants[i]);
    }

    wined3d_device_context_push_constants(&device->cs->c, WINED3D_PUSH_CONSTANTS_VS_B, start_idx, count, constants);
}

static void wined3d_device_set_vs_consts_i(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants)
{
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n", device, start_idx, count, constants);

    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set ivec4 constant %u to %s.\n", start_idx + i, debug_ivec4(&constants[i]));
    }

    wined3d_device_context_push_constants(&device->cs->c, WINED3D_PUSH_CONSTANTS_VS_I, start_idx, count, constants);
}

static void wined3d_device_set_vs_consts_f(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants)
{
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n", device, start_idx, count, constants);

    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set vec4 constant %u to %s.\n", start_idx + i, debug_vec4(&constants[i]));
    }

    wined3d_device_context_push_constants(&device->cs->c, WINED3D_PUSH_CONSTANTS_VS_F, start_idx, count, constants);
}

static void wined3d_device_set_ps_consts_b(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const BOOL *constants)
{
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n", device, start_idx, count, constants);

    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set BOOL constant %u to %#x.\n", start_idx + i, constants[i]);
    }

    wined3d_device_context_push_constants(&device->cs->c, WINED3D_PUSH_CONSTANTS_PS_B, start_idx, count, constants);
}

static void wined3d_device_set_ps_consts_i(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants)
{
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n", device, start_idx, count, constants);

    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set ivec4 constant %u to %s.\n", start_idx + i, debug_ivec4(&constants[i]));
    }

    wined3d_device_context_push_constants(&device->cs->c, WINED3D_PUSH_CONSTANTS_PS_I, start_idx, count, constants);
}

static void wined3d_device_set_ps_consts_f(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants)
{
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n", device, start_idx, count, constants);

    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set vec4 constant %u to %s.\n", start_idx + i, debug_vec4(&constants[i]));
    }

    wined3d_device_context_push_constants(&device->cs->c, WINED3D_PUSH_CONSTANTS_PS_F, start_idx, count, constants);
}

/* Note lights are real special cases. Although the device caps state only
 * e.g. 8 are supported, you can reference any indexes you want as long as
 * that number max are enabled at any one point in time. Therefore since the
 * indices can be anything, we need a hashmap of them. However, this causes
 * stateblock problems. When capturing the state block, I duplicate the
 * hashmap, but when recording, just build a chain pretty much of commands to
 * be replayed. */
static void wined3d_device_context_set_light(struct wined3d_device_context *context,
        unsigned int light_idx, const struct wined3d_light *light)
{
    struct wined3d_light_info *object = NULL;
    float rho;

    if (FAILED(wined3d_light_state_set_light(&context->state->light_state, light_idx, light, &object)))
        return;

    /* Initialize the object. */
    TRACE("Light %u setting to type %#x, diffuse %s, specular %s, ambient %s, "
            "position {%.8e, %.8e, %.8e}, direction {%.8e, %.8e, %.8e}, "
            "range %.8e, falloff %.8e, theta %.8e, phi %.8e.\n",
            light_idx, light->type, debug_color(&light->diffuse),
            debug_color(&light->specular), debug_color(&light->ambient),
            light->position.x, light->position.y, light->position.z,
            light->direction.x, light->direction.y, light->direction.z,
            light->range, light->falloff, light->theta, light->phi);

    switch (light->type)
    {
        case WINED3D_LIGHT_POINT:
            /* Position */
            object->position.x = light->position.x;
            object->position.y = light->position.y;
            object->position.z = light->position.z;
            object->position.w = 1.0f;
            object->cutoff = 180.0f;
            /* FIXME: Range */
            break;

        case WINED3D_LIGHT_DIRECTIONAL:
            /* Direction */
            object->direction.x = -light->direction.x;
            object->direction.y = -light->direction.y;
            object->direction.z = -light->direction.z;
            object->direction.w = 0.0f;
            object->exponent = 0.0f;
            object->cutoff = 180.0f;
            break;

        case WINED3D_LIGHT_SPOT:
            /* Position */
            object->position.x = light->position.x;
            object->position.y = light->position.y;
            object->position.z = light->position.z;
            object->position.w = 1.0f;

            /* Direction */
            object->direction.x = light->direction.x;
            object->direction.y = light->direction.y;
            object->direction.z = light->direction.z;
            object->direction.w = 0.0f;

            /* opengl-ish and d3d-ish spot lights use too different models
             * for the light "intensity" as a function of the angle towards
             * the main light direction, so we only can approximate very
             * roughly. However, spot lights are rather rarely used in games
             * (if ever used at all). Furthermore if still used, probably
             * nobody pays attention to such details. */
            if (!light->falloff)
            {
                /* Falloff = 0 is easy, because d3d's and opengl's spot light
                 * equations have the falloff resp. exponent parameter as an
                 * exponent, so the spot light lighting will always be 1.0 for
                 * both of them, and we don't have to care for the rest of the
                 * rather complex calculation. */
                object->exponent = 0.0f;
            }
            else
            {
                rho = light->theta + (light->phi - light->theta) / (2 * light->falloff);
                if (rho < 0.0001f)
                    rho = 0.0001f;
                object->exponent = -0.3f / logf(cosf(rho / 2));
            }

            if (object->exponent > 128.0f)
                object->exponent = 128.0f;

            object->cutoff = (float)(light->phi * 90 / M_PI);
            /* FIXME: Range */
            break;

        case WINED3D_LIGHT_PARALLELPOINT:
            object->position.x = light->position.x;
            object->position.y = light->position.y;
            object->position.z = light->position.z;
            object->position.w = 1.0f;
            break;

        default:
            FIXME("Unrecognized light type %#x.\n", light->type);
    }

    wined3d_device_context_emit_set_light(context, object);
}

static void wined3d_device_set_light_enable(struct wined3d_device *device, unsigned int light_idx, bool enable)
{
    struct wined3d_light_state *light_state = &device->cs->c.state->light_state;
    struct wined3d_light_info *light_info;

    TRACE("device %p, light_idx %u, enable %#x.\n", device, light_idx, enable);

    /* Special case - enabling an undefined light creates one with a strict set of parameters. */
    if (!(light_info = wined3d_light_state_get_light(light_state, light_idx)))
    {
        TRACE("Light enabled requested but light not defined, so defining one!\n");
        wined3d_device_context_set_light(&device->cs->c, light_idx, &WINED3D_default_light);

        if (!(light_info = wined3d_light_state_get_light(light_state, light_idx)))
        {
            ERR("Adding default lights has failed dismally.\n");
            return;
        }
    }

    if (wined3d_light_state_enable_light(light_state, &device->adapter->d3d_info, light_info, enable))
        wined3d_device_context_emit_set_light_enable(&device->cs->c, light_idx, enable);
}

static void wined3d_device_set_clip_plane(struct wined3d_device *device,
        unsigned int plane_idx, const struct wined3d_vec4 *plane)
{
    struct wined3d_vec4 *clip_planes = device->cs->c.state->clip_planes;

    TRACE("device %p, plane_idx %u, plane %p.\n", device, plane_idx, plane);

    if (!memcmp(&clip_planes[plane_idx], plane, sizeof(*plane)))
    {
        TRACE("Application is setting old values over, nothing to do.\n");
        return;
    }

    clip_planes[plane_idx] = *plane;

    wined3d_device_context_emit_set_clip_plane(&device->cs->c, plane_idx, plane);
}

static void resolve_depth_buffer(struct wined3d_device *device)
{
    const struct wined3d_state *state = device->cs->c.state;
    struct wined3d_rendertarget_view *src_view;
    struct wined3d_resource *dst_resource;
    struct wined3d_texture *dst_texture;

    if (!(dst_texture = wined3d_state_get_ffp_texture(state, 0)))
        return;
    dst_resource = &dst_texture->resource;
    if (!dst_resource->format->depth_size)
        return;
    if (!(src_view = state->fb.depth_stencil))
        return;

    wined3d_device_context_resolve_sub_resource(&device->cs->c, dst_resource, 0,
            src_view->resource, src_view->sub_resource_idx, dst_resource->format->id);
}

static void wined3d_device_set_render_state(struct wined3d_device *device,
        enum wined3d_render_state state, unsigned int value)
{
    if (value == device->cs->c.state->render_states[state])
    {
        TRACE("Application is setting the old value over, nothing to do.\n");
    }
    else
    {
        device->cs->c.state->render_states[state] = value;
        wined3d_device_context_emit_set_render_state(&device->cs->c, state, value);
    }

    if (state == WINED3D_RS_POINTSIZE && value == WINED3D_RESZ_CODE)
    {
        TRACE("RESZ multisampled depth buffer resolve triggered.\n");
        resolve_depth_buffer(device);
    }
}

static void wined3d_device_set_texture_stage_state(struct wined3d_device *device,
        unsigned int stage, enum wined3d_texture_stage_state state, uint32_t value)
{
    TRACE("device %p, stage %u, state %s, value %#x.\n",
            device, stage, debug_d3dtexturestate(state), value);

    if (value == device->cs->c.state->texture_states[stage][state])
    {
        TRACE("Application is setting the old value over, nothing to do.\n");
        return;
    }

    device->cs->c.state->texture_states[stage][state] = value;

    wined3d_device_context_emit_set_texture_state(&device->cs->c, stage, state, value);
}

static void wined3d_device_set_texture(struct wined3d_device *device,
        unsigned int stage, struct wined3d_texture *texture)
{
    enum wined3d_shader_type shader_type = WINED3D_SHADER_TYPE_PIXEL;
    struct wined3d_shader_resource_view *srv = NULL, *prev;
    struct wined3d_state *state = device->cs->c.state;

    TRACE("device %p, stage %u, texture %p.\n", device, stage, texture);

    if (stage >= WINED3D_VERTEX_SAMPLER_OFFSET)
    {
        shader_type = WINED3D_SHADER_TYPE_VERTEX;
        stage -= WINED3D_VERTEX_SAMPLER_OFFSET;
    }

    if (texture && !(srv = wined3d_texture_acquire_identity_srv(texture)))
        return;

    prev = state->shader_resource_view[shader_type][stage];
    TRACE("Previous texture %p.\n", prev);

    if (srv == prev)
    {
        TRACE("App is setting the same texture again, nothing to do.\n");
        return;
    }

    state->shader_resource_view[shader_type][stage] = srv;

    if (srv)
        wined3d_shader_resource_view_incref(srv);
    wined3d_device_context_emit_set_texture(&device->cs->c, shader_type, stage, srv);
    if (prev)
        wined3d_shader_resource_view_decref(prev);

    return;
}

static void wined3d_device_set_material(struct wined3d_device *device, const struct wined3d_material *material)
{
    TRACE("device %p, material %p.\n", device, material);

    device->cs->c.state->material = *material;
    wined3d_device_context_emit_set_material(&device->cs->c, material);
}

static void wined3d_device_set_transform(struct wined3d_device *device,
        enum wined3d_transform_state state, const struct wined3d_matrix *matrix)
{
    TRACE("device %p, state %s, matrix %p.\n", device, debug_d3dtstype(state), matrix);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_11, matrix->_12, matrix->_13, matrix->_14);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_21, matrix->_22, matrix->_23, matrix->_24);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_31, matrix->_32, matrix->_33, matrix->_34);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_41, matrix->_42, matrix->_43, matrix->_44);

    /* If the new matrix is the same as the current one,
     * we cut off any further processing. this seems to be a reasonable
     * optimization because as was noticed, some apps (warcraft3 for example)
     * tend towards setting the same matrix repeatedly for some reason.
     *
     * From here on we assume that the new matrix is different, wherever it matters. */
    if (!memcmp(&device->cs->c.state->transforms[state], matrix, sizeof(*matrix)))
    {
        TRACE("The application is setting the same matrix over again.\n");
        return;
    }

    device->cs->c.state->transforms[state] = *matrix;
    wined3d_device_context_emit_set_transform(&device->cs->c, state, matrix);
}

static enum wined3d_texture_address get_texture_address_mode(const struct wined3d_texture *texture,
        enum wined3d_texture_address t)
{
    if (t < WINED3D_TADDRESS_WRAP || t > WINED3D_TADDRESS_MIRROR_ONCE)
    {
        FIXME("Unrecognized or unsupported texture address mode %#x.\n", t);
        return WINED3D_TADDRESS_WRAP;
    }

    /* Cubemaps are always set to clamp, regardless of the sampler state. */
    if ((texture->resource.usage & WINED3DUSAGE_LEGACY_CUBEMAP)
            || ((texture->flags & WINED3D_TEXTURE_COND_NP2) && t == WINED3D_TADDRESS_WRAP))
        return WINED3D_TADDRESS_CLAMP;

    return t;
}

static void sampler_desc_from_sampler_states(struct wined3d_sampler_desc *desc,
        const uint32_t *sampler_states, const struct wined3d_texture *texture)
{
    const struct wined3d_d3d_info *d3d_info = &texture->resource.device->adapter->d3d_info;

    desc->address_u = get_texture_address_mode(texture, sampler_states[WINED3D_SAMP_ADDRESS_U]);
    desc->address_v = get_texture_address_mode(texture, sampler_states[WINED3D_SAMP_ADDRESS_V]);
    desc->address_w = get_texture_address_mode(texture, sampler_states[WINED3D_SAMP_ADDRESS_W]);
    wined3d_color_from_d3dcolor((struct wined3d_color *)desc->border_color,
            sampler_states[WINED3D_SAMP_BORDER_COLOR]);
    if (sampler_states[WINED3D_SAMP_MAG_FILTER] > WINED3D_TEXF_ANISOTROPIC)
        FIXME("Unrecognized or unsupported WINED3D_SAMP_MAG_FILTER %#x.\n",
                sampler_states[WINED3D_SAMP_MAG_FILTER]);
    desc->mag_filter = min(max(sampler_states[WINED3D_SAMP_MAG_FILTER], WINED3D_TEXF_POINT), WINED3D_TEXF_LINEAR);
    if (sampler_states[WINED3D_SAMP_MIN_FILTER] > WINED3D_TEXF_ANISOTROPIC)
        FIXME("Unrecognized or unsupported WINED3D_SAMP_MIN_FILTER %#x.\n",
                sampler_states[WINED3D_SAMP_MIN_FILTER]);
    desc->min_filter = min(max(sampler_states[WINED3D_SAMP_MIN_FILTER], WINED3D_TEXF_POINT), WINED3D_TEXF_LINEAR);
    if (sampler_states[WINED3D_SAMP_MIP_FILTER] > WINED3D_TEXF_ANISOTROPIC)
        FIXME("Unrecognized or unsupported WINED3D_SAMP_MIP_FILTER %#x.\n",
                sampler_states[WINED3D_SAMP_MIP_FILTER]);
    desc->mip_filter = min(max(sampler_states[WINED3D_SAMP_MIP_FILTER], WINED3D_TEXF_NONE), WINED3D_TEXF_LINEAR);
    desc->lod_bias = int_to_float(sampler_states[WINED3D_SAMP_MIPMAP_LOD_BIAS]);
    desc->min_lod = -1000.0f;
    desc->max_lod = 1000.0f;

    /* The LOD is already clamped to texture->level_count in wined3d_stateblock_set_texture_lod(). */
    if (texture->flags & WINED3D_TEXTURE_COND_NP2)
        desc->mip_base_level = 0;
    else if (desc->mip_filter == WINED3D_TEXF_NONE)
        desc->mip_base_level = texture->lod;
    else
        desc->mip_base_level = min(max(sampler_states[WINED3D_SAMP_MAX_MIP_LEVEL], texture->lod), texture->level_count - 1);

    desc->max_anisotropy = sampler_states[WINED3D_SAMP_MAX_ANISOTROPY];
    if ((sampler_states[WINED3D_SAMP_MAG_FILTER] != WINED3D_TEXF_ANISOTROPIC
                && sampler_states[WINED3D_SAMP_MIN_FILTER] != WINED3D_TEXF_ANISOTROPIC
                && sampler_states[WINED3D_SAMP_MIP_FILTER] != WINED3D_TEXF_ANISOTROPIC)
            || (texture->flags & WINED3D_TEXTURE_COND_NP2))
        desc->max_anisotropy = 1;
    desc->compare = texture->resource.format_caps & WINED3D_FORMAT_CAP_SHADOW;
    desc->comparison_func = WINED3D_CMP_LESSEQUAL;

    /* Only use the LSB of the WINED3D_SAMP_SRGB_TEXTURE value. This matches
     * the behaviour of the AMD Windows driver.
     *
     * Might & Magic: Heroes VI - Shades of Darkness sets
     * WINED3D_SAMP_SRGB_TEXTURE to a large value that looks like a
     * pointer—presumably by accident—and expects sRGB decoding to be
     * disabled. */
    desc->srgb_decode = sampler_states[WINED3D_SAMP_SRGB_TEXTURE] & 0x1;

    if (!(texture->resource.format_caps & WINED3D_FORMAT_CAP_FILTERING))
    {
        desc->mag_filter = WINED3D_TEXF_POINT;
        desc->min_filter = WINED3D_TEXF_POINT;
        desc->mip_filter = WINED3D_TEXF_NONE;
    }

    if (texture->flags & WINED3D_TEXTURE_COND_NP2)
    {
        desc->mip_filter = WINED3D_TEXF_NONE;
        if (d3d_info->normalized_texrect)
            desc->min_filter = WINED3D_TEXF_POINT;
    }
}

void CDECL wined3d_device_apply_stateblock(struct wined3d_device *device,
        struct wined3d_stateblock *stateblock)
{
    bool set_blend_state = false, set_depth_stencil_state = false, set_rasterizer_state = false;
    const struct wined3d_stateblock_state *state = &stateblock->stateblock_state;
    const struct wined3d_saved_states *changed = &stateblock->changed;
    const unsigned int word_bit_count = sizeof(DWORD) * CHAR_BIT;
    struct wined3d_device_context *context = &device->cs->c;
    unsigned int i, j, start, idx;
    bool set_depth_bounds = false;
    struct wined3d_range range;
    uint32_t map;

    TRACE("device %p, stateblock %p.\n", device, stateblock);

    if (changed->vertexShader)
        wined3d_device_context_set_shader(context, WINED3D_SHADER_TYPE_VERTEX, state->vs);
    if (changed->pixelShader)
        wined3d_device_context_set_shader(context, WINED3D_SHADER_TYPE_PIXEL, state->ps);

    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(changed->vs_consts_f, WINED3D_MAX_VS_CONSTS_F, start, &range))
            break;

        wined3d_device_set_vs_consts_f(device, range.offset, range.size, &state->vs_consts_f[range.offset]);
    }

    map = changed->vertexShaderConstantsI;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_I, start, &range))
            break;

        wined3d_device_set_vs_consts_i(device, range.offset, range.size, &state->vs_consts_i[range.offset]);
    }

    map = changed->vertexShaderConstantsB;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_B, start, &range))
            break;

        wined3d_device_set_vs_consts_b(device, range.offset, range.size, &state->vs_consts_b[range.offset]);
    }

    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(changed->ps_consts_f, WINED3D_MAX_PS_CONSTS_F, start, &range))
            break;

        wined3d_device_set_ps_consts_f(device, range.offset, range.size, &state->ps_consts_f[range.offset]);
    }

    map = changed->pixelShaderConstantsI;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_I, start, &range))
            break;

        wined3d_device_set_ps_consts_i(device, range.offset, range.size, &state->ps_consts_i[range.offset]);
    }

    map = changed->pixelShaderConstantsB;
    for (start = 0; ; start = range.offset + range.size)
    {
        if (!wined3d_bitmap_get_range(&map, WINED3D_MAX_CONSTS_B, start, &range))
            break;

        wined3d_device_set_ps_consts_b(device, range.offset, range.size, &state->ps_consts_b[range.offset]);
    }

    if (changed->lights)
    {
        struct wined3d_light_info *light, *cursor;

        LIST_FOR_EACH_ENTRY_SAFE(light, cursor, &changed->changed_lights, struct wined3d_light_info, changed_entry)
        {
            wined3d_device_context_set_light(context, light->OriginalIndex, &light->OriginalParms);
            wined3d_device_set_light_enable(device, light->OriginalIndex, light->glIndex != -1);
            list_remove(&light->changed_entry);
            light->changed = false;
        }
    }

    for (i = 0; i < ARRAY_SIZE(changed->renderState); ++i)
    {
        map = changed->renderState[i];
        while (map)
        {
            j = wined3d_bit_scan(&map);
            idx = i * word_bit_count + j;

            switch (idx)
            {
                case WINED3D_RS_BLENDFACTOR:
                case WINED3D_RS_MULTISAMPLEMASK:
                case WINED3D_RS_ALPHABLENDENABLE:
                case WINED3D_RS_SRCBLEND:
                case WINED3D_RS_DESTBLEND:
                case WINED3D_RS_BLENDOP:
                case WINED3D_RS_SEPARATEALPHABLENDENABLE:
                case WINED3D_RS_SRCBLENDALPHA:
                case WINED3D_RS_DESTBLENDALPHA:
                case WINED3D_RS_BLENDOPALPHA:
                case WINED3D_RS_COLORWRITEENABLE:
                case WINED3D_RS_COLORWRITEENABLE1:
                case WINED3D_RS_COLORWRITEENABLE2:
                case WINED3D_RS_COLORWRITEENABLE3:
                    set_blend_state = true;
                    break;

                case WINED3D_RS_BACK_STENCILFAIL:
                case WINED3D_RS_BACK_STENCILFUNC:
                case WINED3D_RS_BACK_STENCILPASS:
                case WINED3D_RS_BACK_STENCILZFAIL:
                case WINED3D_RS_STENCILENABLE:
                case WINED3D_RS_STENCILFAIL:
                case WINED3D_RS_STENCILFUNC:
                case WINED3D_RS_STENCILREF:
                case WINED3D_RS_STENCILMASK:
                case WINED3D_RS_STENCILPASS:
                case WINED3D_RS_STENCILWRITEMASK:
                case WINED3D_RS_STENCILZFAIL:
                case WINED3D_RS_TWOSIDEDSTENCILMODE:
                case WINED3D_RS_ZENABLE:
                case WINED3D_RS_ZFUNC:
                case WINED3D_RS_ZWRITEENABLE:
                    set_depth_stencil_state = true;
                    break;

                case WINED3D_RS_FILLMODE:
                case WINED3D_RS_CULLMODE:
                case WINED3D_RS_SLOPESCALEDEPTHBIAS:
                case WINED3D_RS_DEPTHBIAS:
                case WINED3D_RS_SCISSORTESTENABLE:
                case WINED3D_RS_ANTIALIASEDLINEENABLE:
                    set_rasterizer_state = true;
                    break;

                case WINED3D_RS_ADAPTIVETESS_X:
                case WINED3D_RS_ADAPTIVETESS_Z:
                case WINED3D_RS_ADAPTIVETESS_W:
                    set_depth_bounds = true;
                    break;

                case WINED3D_RS_ADAPTIVETESS_Y:
                    break;

                case WINED3D_RS_ANTIALIAS:
                    if (state->rs[WINED3D_RS_ANTIALIAS])
                        FIXME("Antialias not supported yet.\n");
                    break;

                case WINED3D_RS_TEXTUREPERSPECTIVE:
                    break;

                case WINED3D_RS_WRAPU:
                    if (state->rs[WINED3D_RS_WRAPU])
                        FIXME("Render state WINED3D_RS_WRAPU not implemented yet.\n");
                    break;

                case WINED3D_RS_WRAPV:
                    if (state->rs[WINED3D_RS_WRAPV])
                        FIXME("Render state WINED3D_RS_WRAPV not implemented yet.\n");
                    break;

                case WINED3D_RS_MONOENABLE:
                    if (state->rs[WINED3D_RS_MONOENABLE])
                        FIXME("Render state WINED3D_RS_MONOENABLE not implemented yet.\n");
                    break;

                case WINED3D_RS_ROP2:
                    if (state->rs[WINED3D_RS_ROP2])
                        FIXME("Render state WINED3D_RS_ROP2 not implemented yet.\n");
                    break;

                case WINED3D_RS_PLANEMASK:
                    if (state->rs[WINED3D_RS_PLANEMASK])
                        FIXME("Render state WINED3D_RS_PLANEMASK not implemented yet.\n");
                    break;

                case WINED3D_RS_LASTPIXEL:
                    if (!state->rs[WINED3D_RS_LASTPIXEL])
                    {
                        static bool warned;
                        if (!warned)
                        {
                            FIXME("Last Pixel Drawing Disabled, not handled yet.\n");
                            warned = true;
                        }
                    }
                    break;

                case WINED3D_RS_ZVISIBLE:
                    if (state->rs[WINED3D_RS_ZVISIBLE])
                        FIXME("WINED3D_RS_ZVISIBLE not implemented.\n");
                    break;

                case WINED3D_RS_SUBPIXEL:
                    if (state->rs[WINED3D_RS_SUBPIXEL])
                        FIXME("Render state WINED3D_RS_SUBPIXEL not implemented yet.\n");
                    break;

                case WINED3D_RS_SUBPIXELX:
                    if (state->rs[WINED3D_RS_SUBPIXELX])
                        FIXME("Render state WINED3D_RS_SUBPIXELX not implemented yet.\n");
                    break;

                case WINED3D_RS_STIPPLEDALPHA:
                    if (state->rs[WINED3D_RS_STIPPLEDALPHA])
                        FIXME("Stippled Alpha not supported yet.\n");
                    break;

                case WINED3D_RS_STIPPLEENABLE:
                    if (state->rs[WINED3D_RS_STIPPLEENABLE])
                        FIXME("Render state WINED3D_RS_STIPPLEENABLE not implemented yet.\n");
                    break;

                case WINED3D_RS_MIPMAPLODBIAS:
                    if (state->rs[WINED3D_RS_MIPMAPLODBIAS])
                        FIXME("Render state WINED3D_RS_MIPMAPLODBIAS not implemented yet.\n");
                    break;

                case WINED3D_RS_ANISOTROPY:
                    if (state->rs[WINED3D_RS_ANISOTROPY])
                        FIXME("Render state WINED3D_RS_ANISOTROPY not implemented yet.\n");
                    break;

                case WINED3D_RS_FLUSHBATCH:
                    if (state->rs[WINED3D_RS_FLUSHBATCH])
                        FIXME("Render state WINED3D_RS_FLUSHBATCH not implemented yet.\n");
                    break;

                case WINED3D_RS_TRANSLUCENTSORTINDEPENDENT:
                    if (state->rs[WINED3D_RS_TRANSLUCENTSORTINDEPENDENT])
                        FIXME("Render state WINED3D_RS_TRANSLUCENTSORTINDEPENDENT not implemented yet.\n");
                    break;

                case WINED3D_RS_WRAP0:
                case WINED3D_RS_WRAP1:
                case WINED3D_RS_WRAP2:
                case WINED3D_RS_WRAP3:
                case WINED3D_RS_WRAP4:
                case WINED3D_RS_WRAP5:
                case WINED3D_RS_WRAP6:
                case WINED3D_RS_WRAP7:
                case WINED3D_RS_WRAP8:
                case WINED3D_RS_WRAP9:
                case WINED3D_RS_WRAP10:
                case WINED3D_RS_WRAP11:
                case WINED3D_RS_WRAP12:
                case WINED3D_RS_WRAP13:
                case WINED3D_RS_WRAP14:
                case WINED3D_RS_WRAP15:
                {
                    static unsigned int once;

                    if ((state->rs[idx]) && !once++)
                        FIXME("(WINED3D_RS_WRAP0) Texture wrapping not yet supported.\n");
                    break;
                }

                case WINED3D_RS_EXTENTS:
                    if (state->rs[WINED3D_RS_EXTENTS])
                        FIXME("Render state WINED3D_RS_EXTENTS not implemented yet.\n");
                    break;

                case WINED3D_RS_COLORKEYBLENDENABLE:
                    if (state->rs[WINED3D_RS_COLORKEYBLENDENABLE])
                        FIXME("Render state WINED3D_RS_COLORKEYBLENDENABLE not implemented yet.\n");
                    break;

                case WINED3D_RS_SOFTWAREVERTEXPROCESSING:
                {
                    static unsigned int once;

                    if ((state->rs[WINED3D_RS_SOFTWAREVERTEXPROCESSING]) && !once++)
                        FIXME("Software vertex processing not implemented.\n");
                    break;
                }

                case WINED3D_RS_PATCHEDGESTYLE:
                    if (state->rs[WINED3D_RS_PATCHEDGESTYLE] != WINED3D_PATCH_EDGE_DISCRETE)
                        FIXME("WINED3D_RS_PATCHEDGESTYLE %#x not yet implemented.\n",
                                state->rs[WINED3D_RS_PATCHEDGESTYLE]);
                    break;

                case WINED3D_RS_PATCHSEGMENTS:
                {
                    union
                    {
                        uint32_t d;
                        float f;
                    } tmpvalue;
                    tmpvalue.f = 1.0f;

                    if (state->rs[WINED3D_RS_PATCHSEGMENTS] != tmpvalue.d)
                    {
                        static bool displayed = false;

                        tmpvalue.d = state->rs[WINED3D_RS_PATCHSEGMENTS];
                        if(!displayed)
                            FIXME("(WINED3D_RS_PATCHSEGMENTS,%f) not yet implemented.\n", tmpvalue.f);

                        displayed = true;
                    }
                    break;
                }

                case WINED3D_RS_DEBUGMONITORTOKEN:
                    WARN("token: %#x.\n", state->rs[WINED3D_RS_DEBUGMONITORTOKEN]);
                    break;

                case WINED3D_RS_INDEXEDVERTEXBLENDENABLE:
                    break;

                case WINED3D_RS_TWEENFACTOR:
                    break;

                case WINED3D_RS_POSITIONDEGREE:
                    if (state->rs[WINED3D_RS_POSITIONDEGREE] != WINED3D_DEGREE_CUBIC)
                        FIXME("WINED3D_RS_POSITIONDEGREE %#x not yet implemented.\n",
                                state->rs[WINED3D_RS_POSITIONDEGREE]);
                    break;

                case WINED3D_RS_NORMALDEGREE:
                    if (state->rs[WINED3D_RS_NORMALDEGREE] != WINED3D_DEGREE_LINEAR)
                        FIXME("WINED3D_RS_NORMALDEGREE %#x not yet implemented.\n",
                                state->rs[WINED3D_RS_NORMALDEGREE]);
                    break;

                case WINED3D_RS_MINTESSELLATIONLEVEL:
                    break;

                case WINED3D_RS_MAXTESSELLATIONLEVEL:
                    break;

                case WINED3D_RS_ENABLEADAPTIVETESSELLATION:
                    if (state->rs[WINED3D_RS_ENABLEADAPTIVETESSELLATION])
                        FIXME("WINED3D_RS_ENABLEADAPTIVETESSELLATION %#x not yet implemented.\n",
                                state->rs[WINED3D_RS_ENABLEADAPTIVETESSELLATION]);
                    break;

                default:
                    wined3d_device_set_render_state(device, idx, state->rs[idx]);
                    break;
            }
        }
    }

    if (set_rasterizer_state)
    {
        struct wined3d_rasterizer_state *rasterizer_state;
        struct wined3d_rasterizer_state_desc desc;
        struct wine_rb_entry *entry;
        union
        {
            DWORD d;
            float f;
        } bias;

        memset(&desc, 0, sizeof(desc));
        desc.fill_mode = state->rs[WINED3D_RS_FILLMODE];
        desc.cull_mode = state->rs[WINED3D_RS_CULLMODE];
        bias.d = state->rs[WINED3D_RS_DEPTHBIAS];
        desc.depth_bias = bias.f;
        bias.d = state->rs[WINED3D_RS_SLOPESCALEDEPTHBIAS];
        desc.scale_bias = bias.f;
        desc.depth_clip = TRUE;
        desc.scissor = state->rs[WINED3D_RS_SCISSORTESTENABLE];
        desc.line_antialias = state->rs[WINED3D_RS_ANTIALIASEDLINEENABLE];

        if ((entry = wine_rb_get(&device->rasterizer_states, &desc)))
        {
            rasterizer_state = WINE_RB_ENTRY_VALUE(entry, struct wined3d_rasterizer_state, entry);
            wined3d_device_context_set_rasterizer_state(context, rasterizer_state);
        }
        else if (SUCCEEDED(wined3d_rasterizer_state_create(device, &desc, NULL,
                &wined3d_null_parent_ops, &rasterizer_state)))
        {
            wined3d_device_context_set_rasterizer_state(context, rasterizer_state);
            if (wine_rb_put(&device->rasterizer_states, &desc, &rasterizer_state->entry) == -1)
            {
                ERR("Failed to insert rasterizer state.\n");
                wined3d_rasterizer_state_decref(rasterizer_state);
            }
        }
    }

    if (set_blend_state || changed->alpha_to_coverage
            || wined3d_bitmap_is_set(changed->renderState, WINED3D_RS_ADAPTIVETESS_Y))
    {
        struct wined3d_blend_state *blend_state;
        struct wined3d_blend_state_desc desc;
        struct wine_rb_entry *entry;
        struct wined3d_color colour;
        unsigned int sample_mask;

        memset(&desc, 0, sizeof(desc));
        desc.alpha_to_coverage = state->alpha_to_coverage;
        desc.independent = FALSE;
        if (state->rs[WINED3D_RS_ADAPTIVETESS_Y] == WINED3DFMT_ATOC)
            desc.alpha_to_coverage = TRUE;
        desc.rt[0].enable = state->rs[WINED3D_RS_ALPHABLENDENABLE];
        desc.rt[0].src = state->rs[WINED3D_RS_SRCBLEND];
        desc.rt[0].dst = state->rs[WINED3D_RS_DESTBLEND];
        desc.rt[0].op = state->rs[WINED3D_RS_BLENDOP];
        if (state->rs[WINED3D_RS_SEPARATEALPHABLENDENABLE])
        {
            desc.rt[0].src_alpha = state->rs[WINED3D_RS_SRCBLENDALPHA];
            desc.rt[0].dst_alpha = state->rs[WINED3D_RS_DESTBLENDALPHA];
            desc.rt[0].op_alpha = state->rs[WINED3D_RS_BLENDOPALPHA];
        }
        else
        {
            desc.rt[0].src_alpha = state->rs[WINED3D_RS_SRCBLEND];
            desc.rt[0].dst_alpha = state->rs[WINED3D_RS_DESTBLEND];
            desc.rt[0].op_alpha = state->rs[WINED3D_RS_BLENDOP];
        }
        desc.rt[0].writemask = state->rs[WINED3D_RS_COLORWRITEENABLE];
        desc.rt[1].writemask = state->rs[WINED3D_RS_COLORWRITEENABLE1];
        desc.rt[2].writemask = state->rs[WINED3D_RS_COLORWRITEENABLE2];
        desc.rt[3].writemask = state->rs[WINED3D_RS_COLORWRITEENABLE3];
        if (desc.rt[1].writemask != desc.rt[0].writemask
                || desc.rt[2].writemask != desc.rt[0].writemask
                || desc.rt[3].writemask != desc.rt[0].writemask)
        {
            desc.independent = TRUE;
            for (i = 1; i < 4; ++i)
            {
                desc.rt[i].enable = desc.rt[0].enable;
                desc.rt[i].src = desc.rt[0].src;
                desc.rt[i].dst = desc.rt[0].dst;
                desc.rt[i].op = desc.rt[0].op;
                desc.rt[i].src_alpha = desc.rt[0].src_alpha;
                desc.rt[i].dst_alpha = desc.rt[0].dst_alpha;
                desc.rt[i].op_alpha = desc.rt[0].op_alpha;
            }
        }

        if (wined3d_bitmap_is_set(changed->renderState, WINED3D_RS_BLENDFACTOR))
            wined3d_color_from_d3dcolor(&colour, state->rs[WINED3D_RS_BLENDFACTOR]);
        else
            wined3d_device_context_get_blend_state(context, &colour, &sample_mask);

        if ((entry = wine_rb_get(&device->blend_states, &desc)))
        {
            blend_state = WINE_RB_ENTRY_VALUE(entry, struct wined3d_blend_state, entry);
            wined3d_device_context_set_blend_state(context, blend_state, &colour,
                    state->rs[WINED3D_RS_MULTISAMPLEMASK]);
        }
        else if (SUCCEEDED(wined3d_blend_state_create(device, &desc, NULL,
                &wined3d_null_parent_ops, &blend_state)))
        {
            wined3d_device_context_set_blend_state(context, blend_state, &colour,
                    state->rs[WINED3D_RS_MULTISAMPLEMASK]);
            if (wine_rb_put(&device->blend_states, &desc, &blend_state->entry) == -1)
            {
                ERR("Failed to insert blend state.\n");
                wined3d_blend_state_decref(blend_state);
            }
        }
    }

    if (set_depth_stencil_state)
    {
        struct wined3d_depth_stencil_state *depth_stencil_state;
        struct wined3d_depth_stencil_state_desc desc;
        struct wine_rb_entry *entry;
        unsigned int stencil_ref;

        memset(&desc, 0, sizeof(desc));
        switch (state->rs[WINED3D_RS_ZENABLE])
        {
            case WINED3D_ZB_FALSE:
                desc.depth = FALSE;
                break;

            case WINED3D_ZB_USEW:
                FIXME("W buffer is not well handled.\n");
            case WINED3D_ZB_TRUE:
                desc.depth = TRUE;
                break;

            default:
                FIXME("Unrecognized depth buffer type %#x.\n", state->rs[WINED3D_RS_ZENABLE]);
        }
        desc.depth_write = state->rs[WINED3D_RS_ZWRITEENABLE];
        desc.depth_func = state->rs[WINED3D_RS_ZFUNC];
        desc.stencil = state->rs[WINED3D_RS_STENCILENABLE];
        desc.stencil_read_mask = state->rs[WINED3D_RS_STENCILMASK];
        desc.stencil_write_mask = state->rs[WINED3D_RS_STENCILWRITEMASK];
        desc.front.fail_op = state->rs[WINED3D_RS_STENCILFAIL];
        desc.front.depth_fail_op = state->rs[WINED3D_RS_STENCILZFAIL];
        desc.front.pass_op = state->rs[WINED3D_RS_STENCILPASS];
        desc.front.func = state->rs[WINED3D_RS_STENCILFUNC];

        if (state->rs[WINED3D_RS_TWOSIDEDSTENCILMODE])
        {
            desc.back.fail_op = state->rs[WINED3D_RS_BACK_STENCILFAIL];
            desc.back.depth_fail_op = state->rs[WINED3D_RS_BACK_STENCILZFAIL];
            desc.back.pass_op = state->rs[WINED3D_RS_BACK_STENCILPASS];
            desc.back.func = state->rs[WINED3D_RS_BACK_STENCILFUNC];
        }
        else
        {
            desc.back = desc.front;
        }

        if (wined3d_bitmap_is_set(changed->renderState, WINED3D_RS_STENCILREF))
            stencil_ref = state->rs[WINED3D_RS_STENCILREF];
        else
            wined3d_device_context_get_depth_stencil_state(context, &stencil_ref);

        if ((entry = wine_rb_get(&device->depth_stencil_states, &desc)))
        {
            depth_stencil_state = WINE_RB_ENTRY_VALUE(entry, struct wined3d_depth_stencil_state, entry);
            wined3d_device_context_set_depth_stencil_state(context, depth_stencil_state, stencil_ref);
        }
        else if (SUCCEEDED(wined3d_depth_stencil_state_create(device, &desc, NULL,
                &wined3d_null_parent_ops, &depth_stencil_state)))
        {
            wined3d_device_context_set_depth_stencil_state(context, depth_stencil_state, stencil_ref);
            if (wine_rb_put(&device->depth_stencil_states, &desc, &depth_stencil_state->entry) == -1)
            {
                ERR("Failed to insert depth/stencil state.\n");
                wined3d_depth_stencil_state_decref(depth_stencil_state);
            }
        }
    }

    if (set_depth_bounds)
    {
        wined3d_device_context_set_depth_bounds(context,
                state->rs[WINED3D_RS_ADAPTIVETESS_X] == WINED3DFMT_NVDB,
                int_to_float(state->rs[WINED3D_RS_ADAPTIVETESS_Z]),
                int_to_float(state->rs[WINED3D_RS_ADAPTIVETESS_W]));
    }

    for (i = 0; i < ARRAY_SIZE(changed->textureState); ++i)
    {
        map = changed->textureState[i];
        while (map)
        {
            j = wined3d_bit_scan(&map);
            wined3d_device_set_texture_stage_state(device, i, j, state->texture_states[i][j]);
        }
    }

    for (i = 0; i < ARRAY_SIZE(changed->samplerState); ++i)
    {
        enum wined3d_shader_type shader_type = WINED3D_SHADER_TYPE_PIXEL;
        struct wined3d_sampler_desc desc;
        struct wined3d_texture *texture;
        struct wined3d_sampler *sampler;
        unsigned int bind_index = i;
        struct wine_rb_entry *entry;

        if (!changed->samplerState[i] && !(changed->textures & (1u << i)))
            continue;

        if (!(texture = state->textures[i]))
            continue;

        memset(&desc, 0, sizeof(desc));
        sampler_desc_from_sampler_states(&desc, state->sampler_states[i], texture);

        if (i >= WINED3D_VERTEX_SAMPLER_OFFSET)
        {
            shader_type = WINED3D_SHADER_TYPE_VERTEX;
            bind_index -= WINED3D_VERTEX_SAMPLER_OFFSET;
        }

        if ((entry = wine_rb_get(&device->samplers, &desc)))
        {
            sampler = WINE_RB_ENTRY_VALUE(entry, struct wined3d_sampler, entry);

            wined3d_device_context_set_samplers(context, shader_type, bind_index, 1, &sampler);
        }
        else if (SUCCEEDED(wined3d_sampler_create(device, &desc, NULL, &wined3d_null_parent_ops, &sampler)))
        {
            wined3d_device_context_set_samplers(context, shader_type, bind_index, 1, &sampler);

            if (wine_rb_put(&device->samplers, &desc, &sampler->entry) == -1)
            {
                ERR("Failed to insert sampler.\n");
                wined3d_sampler_decref(sampler);
            }
        }
    }

    if (changed->transforms)
    {
        for (i = 0; i < ARRAY_SIZE(changed->transform); ++i)
        {
            map = changed->transform[i];
            while (map)
            {
                j = wined3d_bit_scan(&map);
                idx = i * word_bit_count + j;
                wined3d_device_set_transform(device, idx, &state->transforms[idx]);
            }
        }
    }

    if (changed->indices)
        wined3d_device_context_set_index_buffer(context, state->index_buffer, state->index_format, 0);
    wined3d_device_set_base_vertex_index(device, state->base_vertex_index);
    if (changed->vertexDecl)
        wined3d_device_context_set_vertex_declaration(context, state->vertex_declaration);
    if (changed->material)
        wined3d_device_set_material(device, &state->material);
    if (changed->viewport)
        wined3d_device_context_set_viewports(context, 1, &state->viewport);
    if (changed->scissorRect)
        wined3d_device_context_set_scissor_rects(context, 1, &state->scissor_rect);

    map = changed->streamSource | changed->streamFreq;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        wined3d_device_context_set_stream_sources(context, i, 1, &state->streams[i]);
    }

    map = changed->textures;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        wined3d_device_set_texture(device, i, state->textures[i]);
    }

    map = changed->clipplane;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        wined3d_device_set_clip_plane(device, i, &state->clip_planes[i]);
    }

    assert(list_empty(&stateblock->changed.changed_lights));
    memset(&stateblock->changed, 0, sizeof(stateblock->changed));
    list_init(&stateblock->changed.changed_lights);

    TRACE("Applied stateblock %p.\n", stateblock);
}

unsigned int CDECL wined3d_stateblock_set_texture_lod(struct wined3d_stateblock *stateblock,
        struct wined3d_texture *texture, unsigned int lod)
{
    struct wined3d_resource *resource;
    unsigned int old = texture->lod;

    TRACE("texture %p, lod %u.\n", texture, lod);

    /* The d3d9:texture test shows that SetLOD is ignored on non-managed
     * textures. The call always returns 0, and GetLOD always returns 0. */
    resource = &texture->resource;
    if (!(resource->usage & WINED3DUSAGE_MANAGED))
    {
        TRACE("Ignoring LOD on texture with resource access %s.\n",
                wined3d_debug_resource_access(resource->access));
        return 0;
    }

    if (lod >= texture->level_count)
        lod = texture->level_count - 1;

    if (texture->lod != lod)
    {
        texture->lod = lod;

        for (unsigned int i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i)
        {
            /* Mark the texture as changed. The next time the appplication
             * draws from this texture, wined3d_device_apply_stateblock() will
             * recompute the texture LOD.
             *
             * We only need to do this for the primary stateblock.
             * If a recording stateblock uses a texture whose LOD is changed,
             * that texture will be invalidated on the primary stateblock
             * anyway when the recording stateblock is applied. */
            if (stateblock->stateblock_state.textures[i] == texture)
                stateblock->changed.textures |= (1u << i);
        }
    }

    return old;
}
