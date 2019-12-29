/*
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2011, 2013-2014 Stefan Dösinger for CodeWeavers
 * Copyright 2007-2008 Henri Verbeet
 * Copyright 2006-2008 Roderick Colenbrander
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
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

#include "config.h"
#include "wine/port.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);

static const DWORD surface_simple_locations = WINED3D_LOCATION_SYSMEM
        | WINED3D_LOCATION_USER_MEMORY | WINED3D_LOCATION_BUFFER;

/* Works correctly only for <= 4 bpp formats. */
static void get_color_masks(const struct wined3d_format *format, DWORD *masks)
{
    masks[0] = ((1u << format->red_size) - 1) << format->red_offset;
    masks[1] = ((1u << format->green_size) - 1) << format->green_offset;
    masks[2] = ((1u << format->blue_size) - 1) << format->blue_offset;
}

static BOOL texture2d_is_full_rect(const struct wined3d_texture *texture, unsigned int level, const RECT *r)
{
    unsigned int t;

    t = wined3d_texture_get_level_width(texture, level);
    if ((r->left && r->right) || abs(r->right - r->left) != t)
        return FALSE;
    t = wined3d_texture_get_level_height(texture, level);
    if ((r->top && r->bottom) || abs(r->bottom - r->top) != t)
        return FALSE;
    return TRUE;
}

static void texture2d_depth_blt_fbo(const struct wined3d_device *device, struct wined3d_context *context,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx, DWORD src_location,
        const RECT *src_rect, struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        DWORD dst_location, const RECT *dst_rect)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context->gl_info;
    DWORD src_mask, dst_mask;
    GLbitfield gl_mask;

    TRACE("device %p, src_texture %p, src_sub_resource_idx %u, src_location %s, src_rect %s, "
            "dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_rect %s.\n", device,
            src_texture, src_sub_resource_idx, wined3d_debug_location(src_location), wine_dbgstr_rect(src_rect),
            dst_texture, dst_sub_resource_idx, wined3d_debug_location(dst_location), wine_dbgstr_rect(dst_rect));

    src_mask = src_texture->resource.format_flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);
    dst_mask = dst_texture->resource.format_flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);

    if (src_mask != dst_mask)
    {
        ERR("Incompatible formats %s and %s.\n",
                debug_d3dformat(src_texture->resource.format->id),
                debug_d3dformat(dst_texture->resource.format->id));
        return;
    }

    if (!src_mask)
    {
        ERR("Not a depth / stencil format: %s.\n",
                debug_d3dformat(src_texture->resource.format->id));
        return;
    }

    gl_mask = 0;
    if (src_mask & WINED3DFMT_FLAG_DEPTH)
        gl_mask |= GL_DEPTH_BUFFER_BIT;
    if (src_mask & WINED3DFMT_FLAG_STENCIL)
        gl_mask |= GL_STENCIL_BUFFER_BIT;

    /* Make sure the locations are up-to-date. Loading the destination
     * surface isn't required if the entire surface is overwritten. */
    wined3d_texture_load_location(src_texture, src_sub_resource_idx, context, src_location);
    if (!texture2d_is_full_rect(dst_texture, dst_sub_resource_idx % dst_texture->level_count, dst_rect))
        wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, dst_location);
    else
        wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, context, dst_location);

    wined3d_context_gl_apply_fbo_state_blit(context_gl, GL_READ_FRAMEBUFFER, NULL, 0,
            &src_texture->resource, src_sub_resource_idx, src_location);
    wined3d_context_gl_check_fbo_status(context_gl, GL_READ_FRAMEBUFFER);

    wined3d_context_gl_apply_fbo_state_blit(context_gl, GL_DRAW_FRAMEBUFFER, NULL, 0,
            &dst_texture->resource, dst_sub_resource_idx, dst_location);
    wined3d_context_gl_set_draw_buffer(context_gl, GL_NONE);
    wined3d_context_gl_check_fbo_status(context_gl, GL_DRAW_FRAMEBUFFER);
    context_invalidate_state(context, STATE_FRAMEBUFFER);

    if (gl_mask & GL_DEPTH_BUFFER_BIT)
    {
        gl_info->gl_ops.gl.p_glDepthMask(GL_TRUE);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ZWRITEENABLE));
    }
    if (gl_mask & GL_STENCIL_BUFFER_BIT)
    {
        if (context->gl_info->supported[EXT_STENCIL_TWO_SIDE])
        {
            gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            context_invalidate_state(context, STATE_RENDER(WINED3D_RS_TWOSIDEDSTENCILMODE));
        }
        gl_info->gl_ops.gl.p_glStencilMask(~0U);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_STENCILWRITEMASK));
    }

    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SCISSORTESTENABLE));

    gl_info->fbo_ops.glBlitFramebuffer(src_rect->left, src_rect->top, src_rect->right, src_rect->bottom,
            dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, gl_mask, GL_NEAREST);
    checkGLcall("glBlitFramebuffer()");
}

static void prepare_blit_quirks(const struct wined3d_device *device, const struct wined3d_gl_info *gl_info)
{
    /* ATI Macs blend blits even though GL_EXT_framebuffer_blit says that this
     * operation does not blend. This sometimes even happens if GL_BLEND is
     * disabled, although in my glut test case blending has to be enabled for
     * the bug to occur. While we're at it, disable blending.
     *
     * Tracked by crossover hacks bug 5391. */
    gl_info->gl_ops.gl.p_glDisable(GL_BLEND);
    gl_info->gl_ops.gl.p_glBlendFunc(GL_ONE, GL_ZERO);
    device_invalidate_state(device, STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE));

    gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
    gl_info->gl_ops.gl.p_glAlphaFunc(GL_ALWAYS, 0.0);
    device_invalidate_state(device, STATE_RENDER(WINED3D_RS_ALPHATESTENABLE));
    gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST);
    gl_info->gl_ops.gl.p_glStencilFunc(GL_ALWAYS, 0, 0);
    device_invalidate_state(device, STATE_RENDER(WINED3D_RS_STENCILENABLE));
}

/* Blit between surface locations. Onscreen on different swapchains is not supported.
 * Depth / stencil is not supported. Context activation is done by the caller. */
void texture2d_blt_fbo(struct wined3d_device *device, struct wined3d_context *context,
        enum wined3d_texture_filter_type filter, struct wined3d_texture *src_texture,
        unsigned int src_sub_resource_idx, DWORD src_location, const RECT *src_rect,
        struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx, DWORD dst_location,
        const RECT *dst_rect)
{
    struct wined3d_texture *required_texture, *restore_texture;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    unsigned int restore_idx;
    GLenum gl_filter;
    GLenum buffer;
    RECT s, d;

    TRACE("device %p, context %p, filter %s, src_texture %p, src_sub_resource_idx %u, src_location %s, "
            "src_rect %s, dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_rect %s.\n",
            device, context, debug_d3dtexturefiltertype(filter), src_texture, src_sub_resource_idx,
            wined3d_debug_location(src_location), wine_dbgstr_rect(src_rect), dst_texture,
            dst_sub_resource_idx, wined3d_debug_location(dst_location), wine_dbgstr_rect(dst_rect));

    switch (filter)
    {
        case WINED3D_TEXF_LINEAR:
            gl_filter = GL_LINEAR;
            break;

        default:
            FIXME("Unsupported filter mode %s (%#x).\n", debug_d3dtexturefiltertype(filter), filter);
            /* fall through */
        case WINED3D_TEXF_NONE:
        case WINED3D_TEXF_POINT:
            gl_filter = GL_NEAREST;
            break;
    }

    /* Resolve the source surface first if needed. */
    if (wined3d_texture_gl_is_multisample_location(wined3d_texture_gl(src_texture), src_location)
            && (src_texture->resource.format->id != dst_texture->resource.format->id
                || abs(src_rect->bottom - src_rect->top) != abs(dst_rect->bottom - dst_rect->top)
                || abs(src_rect->right - src_rect->left) != abs(dst_rect->right - dst_rect->left)))
        src_location = WINED3D_LOCATION_RB_RESOLVED;

    /* Make sure the locations are up-to-date. Loading the destination
     * surface isn't required if the entire surface is overwritten. (And is
     * in fact harmful if we're being called by surface_load_location() with
     * the purpose of loading the destination surface.) */
    wined3d_texture_load_location(src_texture, src_sub_resource_idx, context, src_location);
    if (!texture2d_is_full_rect(dst_texture, dst_sub_resource_idx % dst_texture->level_count, dst_rect))
        wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, dst_location);
    else
        wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, context, dst_location);

    /* Acquire a context for the front-buffer, even though we may be blitting
     * to/from a back-buffer. Since context_acquire() doesn't take the
     * resource location into account, it may consider the back-buffer to be
     * offscreen. */
    if (src_location == WINED3D_LOCATION_DRAWABLE)
        required_texture = src_texture->swapchain->front_buffer;
    else if (dst_location == WINED3D_LOCATION_DRAWABLE)
        required_texture = dst_texture->swapchain->front_buffer;
    else
        required_texture = NULL;

    restore_texture = context->current_rt.texture;
    restore_idx = context->current_rt.sub_resource_idx;
    if (restore_texture != required_texture)
        context = context_acquire(device, required_texture, 0);
    else
        restore_texture = NULL;

    context_gl = wined3d_context_gl(context);
    if (!context_gl->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping blit.\n");
        return;
    }

    gl_info = context->gl_info;

    if (src_location == WINED3D_LOCATION_DRAWABLE)
    {
        TRACE("Source texture %p is onscreen.\n", src_texture);
        buffer = wined3d_texture_get_gl_buffer(src_texture);
        s = *src_rect;
        wined3d_texture_translate_drawable_coords(src_texture, context_gl->window, &s);
        src_rect = &s;
    }
    else
    {
        TRACE("Source texture %p is offscreen.\n", src_texture);
        buffer = GL_COLOR_ATTACHMENT0;
    }

    wined3d_context_gl_apply_fbo_state_blit(context_gl, GL_READ_FRAMEBUFFER,
            &src_texture->resource, src_sub_resource_idx, NULL, 0, src_location);
    gl_info->gl_ops.gl.p_glReadBuffer(buffer);
    checkGLcall("glReadBuffer()");
    wined3d_context_gl_check_fbo_status(context_gl, GL_READ_FRAMEBUFFER);

    if (dst_location == WINED3D_LOCATION_DRAWABLE)
    {
        TRACE("Destination texture %p is onscreen.\n", dst_texture);
        buffer = wined3d_texture_get_gl_buffer(dst_texture);
        d = *dst_rect;
        wined3d_texture_translate_drawable_coords(dst_texture, context_gl->window, &d);
        dst_rect = &d;
    }
    else
    {
        TRACE("Destination texture %p is offscreen.\n", dst_texture);
        buffer = GL_COLOR_ATTACHMENT0;
    }

    wined3d_context_gl_apply_fbo_state_blit(context_gl, GL_DRAW_FRAMEBUFFER,
            &dst_texture->resource, dst_sub_resource_idx, NULL, 0, dst_location);
    wined3d_context_gl_set_draw_buffer(context_gl, buffer);
    wined3d_context_gl_check_fbo_status(context_gl, GL_DRAW_FRAMEBUFFER);
    context_invalidate_state(context, STATE_FRAMEBUFFER);

    gl_info->gl_ops.gl.p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE1));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE2));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE3));

    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SCISSORTESTENABLE));

    if (gl_info->quirks & WINED3D_CX_QUIRK_BLIT)
        prepare_blit_quirks(device, gl_info);

    gl_info->fbo_ops.glBlitFramebuffer(src_rect->left, src_rect->top, src_rect->right, src_rect->bottom,
            dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, GL_COLOR_BUFFER_BIT, gl_filter);
    checkGLcall("glBlitFramebuffer()");

    if (dst_location == WINED3D_LOCATION_DRAWABLE && dst_texture->swapchain->front_buffer == dst_texture)
        gl_info->gl_ops.gl.p_glFlush();

    if (restore_texture)
        context_restore(context, restore_texture, restore_idx);
}

BOOL fbo_blitter_supported(enum wined3d_blit_op blit_op, const struct wined3d_gl_info *gl_info,
        const struct wined3d_resource *src_resource, DWORD src_location,
        const struct wined3d_resource *dst_resource, DWORD dst_location)
{
    const struct wined3d_format *src_format = src_resource->format;
    const struct wined3d_format *dst_format = dst_resource->format;

    if ((wined3d_settings.offscreen_rendering_mode != ORM_FBO) || !gl_info->fbo_ops.glBlitFramebuffer)
        return FALSE;

    /* Source and/or destination need to be on the GL side */
    if (!(src_resource->access & dst_resource->access & WINED3D_RESOURCE_ACCESS_GPU))
        return FALSE;

    if (src_resource->type != WINED3D_RTYPE_TEXTURE_2D)
        return FALSE;

    switch (blit_op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT:
            if (!((src_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FBO_ATTACHABLE)
                    || (src_resource->bind_flags & WINED3D_BIND_RENDER_TARGET)))
                return FALSE;
            if (!((dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FBO_ATTACHABLE)
                    || (dst_resource->bind_flags & WINED3D_BIND_RENDER_TARGET)))
                return FALSE;
            if ((src_format->id != dst_format->id || dst_location == WINED3D_LOCATION_DRAWABLE)
                    && (!is_identity_fixup(src_format->color_fixup) || !is_identity_fixup(dst_format->color_fixup)))
                return FALSE;
            break;

        case WINED3D_BLIT_OP_DEPTH_BLIT:
            if (!(src_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
                return FALSE;
            if (!(dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
                return FALSE;
            /* Accept pure swizzle fixups for depth formats. In general we
             * ignore the stencil component (if present) at the moment and the
             * swizzle is not relevant with just the depth component. */
            if (is_complex_fixup(src_format->color_fixup) || is_complex_fixup(dst_format->color_fixup)
                    || is_scaling_fixup(src_format->color_fixup) || is_scaling_fixup(dst_format->color_fixup))
                return FALSE;
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

/* See also float_16_to_32() in wined3d_private.h */
static inline unsigned short float_32_to_16(const float *in)
{
    int exp = 0;
    float tmp = fabsf(*in);
    unsigned int mantissa;
    unsigned short ret;

    /* Deal with special numbers */
    if (*in == 0.0f)
        return 0x0000;
    if (isnan(*in))
        return 0x7c01;
    if (isinf(*in))
        return (*in < 0.0f ? 0xfc00 : 0x7c00);

    if (tmp < (float)(1u << 10))
    {
        do
        {
            tmp = tmp * 2.0f;
            exp--;
        } while (tmp < (float)(1u << 10));
    }
    else if (tmp >= (float)(1u << 11))
    {
        do
        {
            tmp /= 2.0f;
            exp++;
        } while (tmp >= (float)(1u << 11));
    }

    mantissa = (unsigned int)tmp;
    if (tmp - mantissa >= 0.5f)
        ++mantissa; /* Round to nearest, away from zero. */

    exp += 10;  /* Normalize the mantissa. */
    exp += 15;  /* Exponent is encoded with excess 15. */

    if (exp > 30) /* too big */
    {
        ret = 0x7c00; /* INF */
    }
    else if (exp <= 0)
    {
        /* exp == 0: Non-normalized mantissa. Returns 0x0000 (=0.0) for too small numbers. */
        while (exp <= 0)
        {
            mantissa = mantissa >> 1;
            ++exp;
        }
        ret = mantissa & 0x3ff;
    }
    else
    {
        ret = (exp << 10) | (mantissa & 0x3ff);
    }

    ret |= ((*in < 0.0f ? 1 : 0) << 15); /* Add the sign */
    return ret;
}

static void convert_r32_float_r16_float(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    unsigned short *dst_s;
    const float *src_f;
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        src_f = (const float *)(src + y * pitch_in);
        dst_s = (unsigned short *) (dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            dst_s[x] = float_32_to_16(src_f + x);
        }
    }
}

static void convert_r5g6b5_x8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    static const unsigned char convert_5to8[] =
    {
        0x00, 0x08, 0x10, 0x19, 0x21, 0x29, 0x31, 0x3a,
        0x42, 0x4a, 0x52, 0x5a, 0x63, 0x6b, 0x73, 0x7b,
        0x84, 0x8c, 0x94, 0x9c, 0xa5, 0xad, 0xb5, 0xbd,
        0xc5, 0xce, 0xd6, 0xde, 0xe6, 0xef, 0xf7, 0xff,
    };
    static const unsigned char convert_6to8[] =
    {
        0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c,
        0x20, 0x24, 0x28, 0x2d, 0x31, 0x35, 0x39, 0x3d,
        0x41, 0x45, 0x49, 0x4d, 0x51, 0x55, 0x59, 0x5d,
        0x61, 0x65, 0x69, 0x6d, 0x71, 0x75, 0x79, 0x7d,
        0x82, 0x86, 0x8a, 0x8e, 0x92, 0x96, 0x9a, 0x9e,
        0xa2, 0xa6, 0xaa, 0xae, 0xb2, 0xb6, 0xba, 0xbe,
        0xc2, 0xc6, 0xca, 0xce, 0xd2, 0xd7, 0xdb, 0xdf,
        0xe3, 0xe7, 0xeb, 0xef, 0xf3, 0xf7, 0xfb, 0xff,
    };
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const WORD *src_line = (const WORD *)(src + y * pitch_in);
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            WORD pixel = src_line[x];
            dst_line[x] = 0xff000000u
                    | convert_5to8[(pixel & 0xf800u) >> 11] << 16
                    | convert_6to8[(pixel & 0x07e0u) >> 5] << 8
                    | convert_5to8[(pixel & 0x001fu)];
        }
    }
}

/* We use this for both B8G8R8A8 -> B8G8R8X8 and B8G8R8X8 -> B8G8R8A8, since
 * in both cases we're just setting the X / Alpha channel to 0xff. */
static void convert_a8r8g8b8_x8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const DWORD *src_line = (const DWORD *)(src + y * pitch_in);
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);

        for (x = 0; x < w; ++x)
        {
            dst_line[x] = 0xff000000 | (src_line[x] & 0xffffff);
        }
    }
}

static inline BYTE cliptobyte(int x)
{
    return (BYTE)((x < 0) ? 0 : ((x > 255) ? 255 : x));
}

static void convert_yuy2_x8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    int c2, d, e, r2 = 0, g2 = 0, b2 = 0;
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const BYTE *src_line = src + y * pitch_in;
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* YUV to RGB conversion formulas from http://en.wikipedia.org/wiki/YUV:
             *     C = Y - 16; D = U - 128; E = V - 128;
             *     R = cliptobyte((298 * C + 409 * E + 128) >> 8);
             *     G = cliptobyte((298 * C - 100 * D - 208 * E + 128) >> 8);
             *     B = cliptobyte((298 * C + 516 * D + 128) >> 8);
             * Two adjacent YUY2 pixels are stored as four bytes: Y0 U Y1 V .
             * U and V are shared between the pixels. */
            if (!(x & 1)) /* For every even pixel, read new U and V. */
            {
                d = (int) src_line[1] - 128;
                e = (int) src_line[3] - 128;
                r2 = 409 * e + 128;
                g2 = - 100 * d - 208 * e + 128;
                b2 = 516 * d + 128;
            }
            c2 = 298 * ((int) src_line[0] - 16);
            dst_line[x] = 0xff000000
                | cliptobyte((c2 + r2) >> 8) << 16    /* red   */
                | cliptobyte((c2 + g2) >> 8) << 8     /* green */
                | cliptobyte((c2 + b2) >> 8);         /* blue  */
                /* Scale RGB values to 0..255 range,
                 * then clip them if still not in range (may be negative),
                 * then shift them within DWORD if necessary. */
            src_line += 2;
        }
    }
}

static void convert_yuy2_r5g6b5(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    unsigned int x, y;
    int c2, d, e, r2 = 0, g2 = 0, b2 = 0;

    TRACE("Converting %ux%u pixels, pitches %u %u\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const BYTE *src_line = src + y * pitch_in;
        WORD *dst_line = (WORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* YUV to RGB conversion formulas from http://en.wikipedia.org/wiki/YUV:
             *     C = Y - 16; D = U - 128; E = V - 128;
             *     R = cliptobyte((298 * C + 409 * E + 128) >> 8);
             *     G = cliptobyte((298 * C - 100 * D - 208 * E + 128) >> 8);
             *     B = cliptobyte((298 * C + 516 * D + 128) >> 8);
             * Two adjacent YUY2 pixels are stored as four bytes: Y0 U Y1 V .
             * U and V are shared between the pixels. */
            if (!(x & 1)) /* For every even pixel, read new U and V. */
            {
                d = (int) src_line[1] - 128;
                e = (int) src_line[3] - 128;
                r2 = 409 * e + 128;
                g2 = - 100 * d - 208 * e + 128;
                b2 = 516 * d + 128;
            }
            c2 = 298 * ((int) src_line[0] - 16);
            dst_line[x] = (cliptobyte((c2 + r2) >> 8) >> 3) << 11   /* red   */
                | (cliptobyte((c2 + g2) >> 8) >> 2) << 5            /* green */
                | (cliptobyte((c2 + b2) >> 8) >> 3);                /* blue  */
                /* Scale RGB values to 0..255 range,
                 * then clip them if still not in range (may be negative),
                 * then shift them within DWORD if necessary. */
            src_line += 2;
        }
    }
}

struct d3dfmt_converter_desc
{
    enum wined3d_format_id from, to;
    void (*convert)(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h);
};

static const struct d3dfmt_converter_desc converters[] =
{
    {WINED3DFMT_R32_FLOAT,      WINED3DFMT_R16_FLOAT,       convert_r32_float_r16_float},
    {WINED3DFMT_B5G6R5_UNORM,   WINED3DFMT_B8G8R8X8_UNORM,  convert_r5g6b5_x8r8g8b8},
    {WINED3DFMT_B8G8R8A8_UNORM, WINED3DFMT_B8G8R8X8_UNORM,  convert_a8r8g8b8_x8r8g8b8},
    {WINED3DFMT_B8G8R8X8_UNORM, WINED3DFMT_B8G8R8A8_UNORM,  convert_a8r8g8b8_x8r8g8b8},
    {WINED3DFMT_YUY2,           WINED3DFMT_B8G8R8X8_UNORM,  convert_yuy2_x8r8g8b8},
    {WINED3DFMT_YUY2,           WINED3DFMT_B5G6R5_UNORM,    convert_yuy2_r5g6b5},
};

static inline const struct d3dfmt_converter_desc *find_converter(enum wined3d_format_id from,
        enum wined3d_format_id to)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(converters); ++i)
    {
        if (converters[i].from == from && converters[i].to == to)
            return &converters[i];
    }

    return NULL;
}

static struct wined3d_texture *surface_convert_format(struct wined3d_texture *src_texture,
        unsigned int sub_resource_idx, const struct wined3d_format *dst_format)
{
    unsigned int texture_level = sub_resource_idx % src_texture->level_count;
    const struct wined3d_format *src_format = src_texture->resource.format;
    struct wined3d_device *device = src_texture->resource.device;
    const struct d3dfmt_converter_desc *conv = NULL;
    struct wined3d_context_gl *context_gl = NULL;
    unsigned int src_row_pitch, src_slice_pitch;
    struct wined3d_context *context = NULL;
    struct wined3d_texture *dst_texture;
    struct wined3d_bo_address src_data;
    struct wined3d_resource_desc desc;
    DWORD map_binding;

    if (!(conv = find_converter(src_format->id, dst_format->id)) && (!device->d3d_initialized
            || !is_identity_fixup(src_format->color_fixup) || src_format->conv_byte_count
            || !is_identity_fixup(dst_format->color_fixup) || dst_format->conv_byte_count
            || (src_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_COMPRESSED)))
    {
        FIXME("Cannot find a conversion function from format %s to %s.\n",
                debug_d3dformat(src_format->id), debug_d3dformat(dst_format->id));
        return NULL;
    }

    /* FIXME: Multisampled conversion? */
    desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    desc.format = dst_format->id;
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = WINED3DUSAGE_SCRATCH | WINED3DUSAGE_PRIVATE;
    desc.bind_flags = 0;
    desc.access = WINED3D_RESOURCE_ACCESS_CPU | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
    desc.width = wined3d_texture_get_level_width(src_texture, texture_level);
    desc.height = wined3d_texture_get_level_height(src_texture, texture_level);
    desc.depth = 1;
    desc.size = 0;
    if (FAILED(wined3d_texture_create(device, &desc, 1, 1, WINED3D_TEXTURE_CREATE_DISCARD,
            NULL, NULL, &wined3d_null_parent_ops, &dst_texture)))
    {
        ERR("Failed to create a destination texture for conversion.\n");
        return NULL;
    }

    if (device->d3d_initialized)
    {
        context = context_acquire(device, NULL, 0);
        context_gl = wined3d_context_gl(context);
    }

    map_binding = src_texture->resource.map_binding;
    if (!wined3d_texture_load_location(src_texture, sub_resource_idx, context, map_binding))
        ERR("Failed to load the source sub-resource into %s.\n", wined3d_debug_location(map_binding));
    wined3d_texture_get_pitch(src_texture, texture_level, &src_row_pitch, &src_slice_pitch);
    wined3d_texture_get_memory(src_texture, sub_resource_idx, &src_data, map_binding);

    if (conv)
    {
        unsigned int dst_row_pitch, dst_slice_pitch;
        struct wined3d_bo_address dst_data;
        const BYTE *src;
        BYTE *dst;

        map_binding = dst_texture->resource.map_binding;
        if (!wined3d_texture_load_location(dst_texture, 0, context, map_binding))
            ERR("Failed to load the destination sub-resource into %s.\n", wined3d_debug_location(map_binding));
        wined3d_texture_get_pitch(dst_texture, 0, &dst_row_pitch, &dst_slice_pitch);
        wined3d_texture_get_memory(dst_texture, 0, &dst_data, map_binding);

        src = wined3d_context_gl_map_bo_address(context_gl, &src_data,
                src_texture->sub_resources[sub_resource_idx].size, GL_PIXEL_UNPACK_BUFFER, WINED3D_MAP_READ);
        dst = wined3d_context_gl_map_bo_address(context_gl, &dst_data,
                dst_texture->sub_resources[0].size, GL_PIXEL_UNPACK_BUFFER, WINED3D_MAP_WRITE);

        conv->convert(src, dst, src_row_pitch, dst_row_pitch, desc.width, desc.height);

        wined3d_texture_invalidate_location(dst_texture, 0, ~map_binding);
        wined3d_context_gl_unmap_bo_address(context_gl, &dst_data, GL_PIXEL_UNPACK_BUFFER);
        wined3d_context_gl_unmap_bo_address(context_gl, &src_data, GL_PIXEL_UNPACK_BUFFER);
    }
    else
    {
        struct wined3d_box src_box = {0, 0, desc.width, desc.height, 0, 1};

        TRACE("Using upload conversion.\n");

        wined3d_texture_prepare_location(dst_texture, 0, context, WINED3D_LOCATION_TEXTURE_RGB);
        dst_texture->texture_ops->texture_upload_data(context, wined3d_const_bo_address(&src_data),
                src_format, &src_box, src_row_pitch, src_slice_pitch,
                dst_texture, 0, WINED3D_LOCATION_TEXTURE_RGB, 0, 0, 0);

        wined3d_texture_validate_location(dst_texture, 0, WINED3D_LOCATION_TEXTURE_RGB);
        wined3d_texture_invalidate_location(dst_texture, 0, ~WINED3D_LOCATION_TEXTURE_RGB);
    }

    if (context)
        context_release(context);

    return dst_texture;
}

void texture2d_read_from_framebuffer(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, DWORD src_location, DWORD dst_location)
{
    struct wined3d_resource *resource = &texture->resource;
    struct wined3d_device *device = resource->device;
    const struct wined3d_format_gl *format_gl;
    struct wined3d_texture *restore_texture;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    unsigned int row_pitch, slice_pitch;
    unsigned int width, height, level;
    struct wined3d_bo_address data;
    unsigned int restore_idx;
    BYTE *row, *top, *bottom;
    BOOL src_is_upside_down;
    unsigned int i;
    BYTE *mem;

    wined3d_texture_get_memory(texture, sub_resource_idx, &data, dst_location);

    restore_texture = context->current_rt.texture;
    restore_idx = context->current_rt.sub_resource_idx;
    if (restore_texture != texture || restore_idx != sub_resource_idx)
        context = context_acquire(device, texture, sub_resource_idx);
    else
        restore_texture = NULL;
    context_gl = wined3d_context_gl(context);
    gl_info = context->gl_info;

    if (src_location != resource->draw_binding)
    {
        wined3d_context_gl_apply_fbo_state_blit(context_gl, GL_READ_FRAMEBUFFER,
                resource, sub_resource_idx, NULL, 0, src_location);
        wined3d_context_gl_check_fbo_status(context_gl, GL_READ_FRAMEBUFFER);
        context_invalidate_state(context, STATE_FRAMEBUFFER);
    }
    else
    {
        wined3d_context_gl_apply_blit_state(context_gl, device);
    }

    /* Select the correct read buffer, and give some debug output.
     * There is no need to keep track of the current read buffer or reset it,
     * every part of the code that reads sets the read buffer as desired.
     */
    if (src_location != WINED3D_LOCATION_DRAWABLE || wined3d_resource_is_offscreen(resource))
    {
        /* Mapping the primary render target which is not on a swapchain.
         * Read from the back buffer. */
        TRACE("Mapping offscreen render target.\n");
        gl_info->gl_ops.gl.p_glReadBuffer(wined3d_context_gl_get_offscreen_gl_buffer(context_gl));
        src_is_upside_down = TRUE;
    }
    else
    {
        /* Onscreen surfaces are always part of a swapchain */
        GLenum buffer = wined3d_texture_get_gl_buffer(texture);
        TRACE("Mapping %#x buffer.\n", buffer);
        gl_info->gl_ops.gl.p_glReadBuffer(buffer);
        src_is_upside_down = FALSE;
    }
    checkGLcall("glReadBuffer");

    if (data.buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data.buffer_object));
        checkGLcall("glBindBuffer");
    }

    level = sub_resource_idx % texture->level_count;
    wined3d_texture_get_pitch(texture, level, &row_pitch, &slice_pitch);
    format_gl = wined3d_format_gl(resource->format);

    /* Setup pixel store pack state -- to glReadPixels into the correct place */
    gl_info->gl_ops.gl.p_glPixelStorei(GL_PACK_ROW_LENGTH, row_pitch / format_gl->f.byte_count);
    checkGLcall("glPixelStorei");

    width = wined3d_texture_get_level_width(texture, level);
    height = wined3d_texture_get_level_height(texture, level);
    gl_info->gl_ops.gl.p_glReadPixels(0, 0, width, height,
            format_gl->format, format_gl->type, data.addr);
    checkGLcall("glReadPixels");

    /* Reset previous pixel store pack state */
    gl_info->gl_ops.gl.p_glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    checkGLcall("glPixelStorei");

    if (!src_is_upside_down)
    {
        /* glReadPixels returns the image upside down, and there is no way to
         * prevent this. Flip the lines in software. */

        if (!(row = heap_alloc(row_pitch)))
            goto error;

        if (data.buffer_object)
        {
            mem = ADDRSPACECAST(void*, GL_EXTCALL(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE)));
            checkGLcall("glMapBuffer");
        }
        else
            mem = data.addr;

        top = mem;
        bottom = mem + row_pitch * (height - 1);
        for (i = 0; i < height / 2; i++)
        {
            memcpy(row, top, row_pitch);
            memcpy(top, bottom, row_pitch);
            memcpy(bottom, row, row_pitch);
            top += row_pitch;
            bottom -= row_pitch;
        }
        heap_free(row);

        if (data.buffer_object)
            GL_EXTCALL(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
    }

error:
    if (data.buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    if (restore_texture)
        context_restore(context, restore_texture, restore_idx);
}

/* Read the framebuffer contents into a texture. Note that this function
 * doesn't do any kind of flipping. Using this on an onscreen surface will
 * result in a flipped D3D texture.
 *
 * Context activation is done by the caller. This function may temporarily
 * switch to a different context and restore the original one before return. */
void texture2d_load_fb_texture(struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, BOOL srgb, struct wined3d_context *context)
{
    struct wined3d_texture *restore_texture;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    struct wined3d_resource *resource;
    unsigned int restore_idx, level;
    struct wined3d_device *device;
    GLenum target;

    resource = &texture_gl->t.resource;
    device = resource->device;
    restore_texture = context->current_rt.texture;
    restore_idx = context->current_rt.sub_resource_idx;
    if (restore_texture != &texture_gl->t || restore_idx != sub_resource_idx)
        context = context_acquire(device, &texture_gl->t, sub_resource_idx);
    else
        restore_texture = NULL;
    context_gl = wined3d_context_gl(context);

    gl_info = context->gl_info;
    device_invalidate_state(device, STATE_FRAMEBUFFER);

    wined3d_texture_gl_prepare_texture(texture_gl, context_gl, srgb);
    wined3d_texture_gl_bind_and_dirtify(texture_gl, context_gl, srgb);

    TRACE("Reading back offscreen render target %p, %u.\n", texture_gl, sub_resource_idx);

    if (wined3d_resource_is_offscreen(resource))
        gl_info->gl_ops.gl.p_glReadBuffer(wined3d_context_gl_get_offscreen_gl_buffer(context_gl));
    else
        gl_info->gl_ops.gl.p_glReadBuffer(wined3d_texture_get_gl_buffer(&texture_gl->t));
    checkGLcall("glReadBuffer");

    level = sub_resource_idx % texture_gl->t.level_count;
    target = wined3d_texture_gl_get_sub_resource_target(texture_gl, sub_resource_idx);
    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(target, level, 0, 0, 0, 0,
            wined3d_texture_get_level_width(&texture_gl->t, level),
            wined3d_texture_get_level_height(&texture_gl->t, level));
    checkGLcall("glCopyTexSubImage2D");

    if (restore_texture)
        context_restore(context, restore_texture, restore_idx);
}

/* Does a direct frame buffer -> texture copy. Stretching is done with single
 * pixel copy calls. */
static void fb_copy_to_texture_direct(struct wined3d_texture_gl *dst_texture, unsigned int dst_sub_resource_idx,
        const RECT *dst_rect_in, struct wined3d_texture_gl *src_texture, unsigned int src_sub_resource_idx,
        const RECT *src_rect, enum wined3d_texture_filter_type filter)
{
    struct wined3d_device *device = dst_texture->t.resource.device;
    unsigned int src_height, src_level, dst_level;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    float xrel, yrel;
    struct wined3d_context *context;
    BOOL upsidedown = FALSE;
    RECT dst_rect = *dst_rect_in;
    GLenum dst_target;

    /* Make sure that the top pixel is always above the bottom pixel, and keep a separate upside down flag
     * glCopyTexSubImage is a bit picky about the parameters we pass to it
     */
    if(dst_rect.top > dst_rect.bottom) {
        UINT tmp = dst_rect.bottom;
        dst_rect.bottom = dst_rect.top;
        dst_rect.top = tmp;
        upsidedown = TRUE;
    }

    context = context_acquire(device, &src_texture->t, src_sub_resource_idx);
    context_gl = wined3d_context_gl(context);
    gl_info = context->gl_info;
    wined3d_context_gl_apply_blit_state(context_gl, device);
    wined3d_texture_load(&dst_texture->t, context, FALSE);

    /* Bind the target texture */
    wined3d_context_gl_bind_texture(context_gl, dst_texture->target, dst_texture->texture_rgb.name);
    if (wined3d_resource_is_offscreen(&src_texture->t.resource))
    {
        TRACE("Reading from an offscreen target\n");
        upsidedown = !upsidedown;
        gl_info->gl_ops.gl.p_glReadBuffer(wined3d_context_gl_get_offscreen_gl_buffer(context_gl));
    }
    else
    {
        gl_info->gl_ops.gl.p_glReadBuffer(wined3d_texture_get_gl_buffer(&src_texture->t));
    }
    checkGLcall("glReadBuffer");

    xrel = (float) (src_rect->right - src_rect->left) / (float) (dst_rect.right - dst_rect.left);
    yrel = (float) (src_rect->bottom - src_rect->top) / (float) (dst_rect.bottom - dst_rect.top);

    if ((xrel - 1.0f < -eps) || (xrel - 1.0f > eps))
    {
        FIXME_(d3d_perf)("Doing a pixel by pixel copy from the framebuffer to a texture.\n");

        if (filter != WINED3D_TEXF_NONE && filter != WINED3D_TEXF_POINT)
            ERR("Texture filtering not supported in direct blit.\n");
    }
    else if ((filter != WINED3D_TEXF_NONE && filter != WINED3D_TEXF_POINT)
            && ((yrel - 1.0f < -eps) || (yrel - 1.0f > eps)))
    {
        ERR("Texture filtering not supported in direct blit\n");
    }

    src_level = src_sub_resource_idx % src_texture->t.level_count;
    dst_level = dst_sub_resource_idx % dst_texture->t.level_count;

    src_height = wined3d_texture_get_level_height(&src_texture->t, src_level);
    dst_target = wined3d_texture_gl_get_sub_resource_target(dst_texture, dst_sub_resource_idx);
    if (upsidedown
            && !((xrel - 1.0f < -eps) || (xrel - 1.0f > eps))
            && !((yrel - 1.0f < -eps) || (yrel - 1.0f > eps)))
    {
        /* Upside down copy without stretching is nice, one glCopyTexSubImage call will do. */
        gl_info->gl_ops.gl.p_glCopyTexSubImage2D(dst_target, dst_level,
                dst_rect.left /*xoffset */, dst_rect.top /* y offset */,
                src_rect->left, src_height - src_rect->bottom,
                dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);
    }
    else
    {
        LONG row;
        UINT yoffset = src_height - src_rect->top + dst_rect.top - 1;
        /* I have to process this row by row to swap the image,
         * otherwise it would be upside down, so stretching in y direction
         * doesn't cost extra time
         *
         * However, stretching in x direction can be avoided if not necessary
         */
        for(row = dst_rect.top; row < dst_rect.bottom; row++) {
            if ((xrel - 1.0f < -eps) || (xrel - 1.0f > eps))
            {
                /* Well, that stuff works, but it's very slow.
                 * find a better way instead
                 */
                LONG col;

                for (col = dst_rect.left; col < dst_rect.right; ++col)
                {
                    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(dst_target, dst_level,
                            dst_rect.left + col /* x offset */, row /* y offset */,
                            src_rect->left + col * xrel, yoffset - (int) (row * yrel), 1, 1);
                }
            }
            else
            {
                gl_info->gl_ops.gl.p_glCopyTexSubImage2D(dst_target, dst_level,
                        dst_rect.left /* x offset */, row /* y offset */,
                        src_rect->left, yoffset - (int) (row * yrel), dst_rect.right - dst_rect.left, 1);
            }
        }
    }
    checkGLcall("glCopyTexSubImage2D");

    context_release(context);

    /* The texture is now most up to date - If the surface is a render target
     * and has a drawable, this path is never entered. */
    wined3d_texture_validate_location(&dst_texture->t, dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_invalidate_location(&dst_texture->t, dst_sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);
}

/* Uses the hardware to stretch and flip the image */
static void fb_copy_to_texture_hwstretch(struct wined3d_texture_gl *dst_texture, unsigned int dst_sub_resource_idx,
        const RECT *dst_rect_in, struct wined3d_texture_gl *src_texture, unsigned int src_sub_resource_idx,
        const RECT *src_rect, enum wined3d_texture_filter_type filter)
{
    unsigned int src_width, src_height, src_pow2_width, src_pow2_height, src_level;
    struct wined3d_device *device = dst_texture->t.resource.device;
    GLenum src_target, dst_target, texture_target;
    GLuint src, backup = 0;
    float left, right, top, bottom; /* Texture coordinates */
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    struct wined3d_context *context;
    GLenum drawBuffer = GL_BACK;
    GLenum offscreen_buffer;
    BOOL noBackBufferBackup;
    BOOL src_offscreen;
    BOOL upsidedown = FALSE;
    RECT dst_rect = *dst_rect_in;

    TRACE("Using hwstretch blit\n");

    src_target = wined3d_texture_gl_get_sub_resource_target(src_texture, src_sub_resource_idx);
    dst_target = wined3d_texture_gl_get_sub_resource_target(dst_texture, dst_sub_resource_idx);

    /* Activate the Proper context for reading from the source surface, set it up for blitting */
    context = context_acquire(device, &src_texture->t, src_sub_resource_idx);
    context_gl = wined3d_context_gl(context);
    gl_info = context->gl_info;
    wined3d_context_gl_apply_ffp_blit_state(context_gl, device);
    wined3d_texture_load(&dst_texture->t, context, FALSE);

    offscreen_buffer = wined3d_context_gl_get_offscreen_gl_buffer(context_gl);
    src_level = src_sub_resource_idx % src_texture->t.level_count;
    src_width = wined3d_texture_get_level_width(&src_texture->t, src_level);
    src_height = wined3d_texture_get_level_height(&src_texture->t, src_level);
    src_pow2_width = wined3d_texture_get_level_pow2_width(&src_texture->t, src_level);
    src_pow2_height = wined3d_texture_get_level_pow2_height(&src_texture->t, src_level);

    src_offscreen = wined3d_resource_is_offscreen(&src_texture->t.resource);
    noBackBufferBackup = src_offscreen && wined3d_settings.offscreen_rendering_mode == ORM_FBO;
    if (!noBackBufferBackup && !src_texture->texture_rgb.name)
    {
        /* Get it a description */
        wined3d_texture_load(&src_texture->t, context, FALSE);
    }

    /* Try to use an aux buffer for drawing the rectangle. This way it doesn't need restoring.
     * This way we don't have to wait for the 2nd readback to finish to leave this function.
     */
    if (context_gl->aux_buffers >= 2)
    {
        /* Got more than one aux buffer? Use the 2nd aux buffer */
        drawBuffer = GL_AUX1;
    }
    else if ((!src_offscreen || offscreen_buffer == GL_BACK) && context_gl->aux_buffers >= 1)
    {
        /* Only one aux buffer, but it isn't used (Onscreen rendering, or non-aux orm)? Use it! */
        drawBuffer = GL_AUX0;
    }

    if (noBackBufferBackup)
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &backup);
        checkGLcall("glGenTextures");
        wined3d_context_gl_bind_texture(context_gl, GL_TEXTURE_2D, backup);
        texture_target = GL_TEXTURE_2D;
    }
    else
    {
        /* Backup the back buffer and copy the source buffer into a texture to draw an upside down stretched quad. If
         * we are reading from the back buffer, the backup can be used as source texture
         */
        texture_target = src_target;
        wined3d_context_gl_bind_texture(context_gl, texture_target, src_texture->texture_rgb.name);
        gl_info->gl_ops.gl.p_glEnable(texture_target);
        checkGLcall("glEnable(texture_target)");

        /* For now invalidate the texture copy of the back buffer. Drawable and sysmem copy are untouched */
        src_texture->t.sub_resources[src_sub_resource_idx].locations &= ~WINED3D_LOCATION_TEXTURE_RGB;
    }

    /* Make sure that the top pixel is always above the bottom pixel, and keep a separate upside down flag
     * glCopyTexSubImage is a bit picky about the parameters we pass to it
     */
    if(dst_rect.top > dst_rect.bottom) {
        UINT tmp = dst_rect.bottom;
        dst_rect.bottom = dst_rect.top;
        dst_rect.top = tmp;
        upsidedown = TRUE;
    }

    if (src_offscreen)
    {
        TRACE("Reading from an offscreen target\n");
        upsidedown = !upsidedown;
        gl_info->gl_ops.gl.p_glReadBuffer(offscreen_buffer);
    }
    else
    {
        gl_info->gl_ops.gl.p_glReadBuffer(wined3d_texture_get_gl_buffer(&src_texture->t));
    }

    /* TODO: Only back up the part that will be overwritten */
    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(texture_target, 0, 0, 0, 0, 0, src_width, src_height);

    checkGLcall("glCopyTexSubImage2D");

    /* No issue with overriding these - the sampler is dirty due to blit usage */
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, wined3d_gl_mag_filter(filter));
    checkGLcall("glTexParameteri");
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER,
            wined3d_gl_min_mip_filter(filter, WINED3D_TEXF_NONE));
    checkGLcall("glTexParameteri");

    if (!src_texture->t.swapchain || &src_texture->t == src_texture->t.swapchain->back_buffers[0])
    {
        src = backup ? backup : src_texture->texture_rgb.name;
    }
    else
    {
        gl_info->gl_ops.gl.p_glReadBuffer(GL_FRONT);
        checkGLcall("glReadBuffer(GL_FRONT)");

        gl_info->gl_ops.gl.p_glGenTextures(1, &src);
        checkGLcall("glGenTextures(1, &src)");
        wined3d_context_gl_bind_texture(context_gl, GL_TEXTURE_2D, src);

        /* TODO: Only copy the part that will be read. Use src_rect->left,
         * src_rect->bottom as origin, but with the width watch out for power
         * of 2 sizes. */
        gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, src_pow2_width,
                src_pow2_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        checkGLcall("glTexImage2D");
        gl_info->gl_ops.gl.p_glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, src_width, src_height);

        gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");
        gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");

        gl_info->gl_ops.gl.p_glReadBuffer(GL_BACK);
        checkGLcall("glReadBuffer(GL_BACK)");

        if (texture_target != GL_TEXTURE_2D)
        {
            gl_info->gl_ops.gl.p_glDisable(texture_target);
            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_2D);
            texture_target = GL_TEXTURE_2D;
        }
    }
    checkGLcall("glEnd and previous");

    left = src_rect->left;
    right = src_rect->right;

    if (!upsidedown)
    {
        top = src_height - src_rect->top;
        bottom = src_height - src_rect->bottom;
    }
    else
    {
        top = src_height - src_rect->bottom;
        bottom = src_height - src_rect->top;
    }

    if (src_texture->t.flags & WINED3D_TEXTURE_NORMALIZED_COORDS)
    {
        left /= src_pow2_width;
        right /= src_pow2_width;
        top /= src_pow2_height;
        bottom /= src_pow2_height;
    }

    /* draw the source texture stretched and upside down. The correct surface is bound already */
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    wined3d_context_gl_set_draw_buffer(context_gl, drawBuffer);
    gl_info->gl_ops.gl.p_glReadBuffer(drawBuffer);

    gl_info->gl_ops.gl.p_glBegin(GL_QUADS);
        /* bottom left */
        gl_info->gl_ops.gl.p_glTexCoord2f(left, bottom);
        gl_info->gl_ops.gl.p_glVertex2i(0, 0);

        /* top left */
        gl_info->gl_ops.gl.p_glTexCoord2f(left, top);
        gl_info->gl_ops.gl.p_glVertex2i(0, dst_rect.bottom - dst_rect.top);

        /* top right */
        gl_info->gl_ops.gl.p_glTexCoord2f(right, top);
        gl_info->gl_ops.gl.p_glVertex2i(dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);

        /* bottom right */
        gl_info->gl_ops.gl.p_glTexCoord2f(right, bottom);
        gl_info->gl_ops.gl.p_glVertex2i(dst_rect.right - dst_rect.left, 0);
    gl_info->gl_ops.gl.p_glEnd();
    checkGLcall("glEnd and previous");

    if (texture_target != dst_target)
    {
        gl_info->gl_ops.gl.p_glDisable(texture_target);
        gl_info->gl_ops.gl.p_glEnable(dst_target);
        texture_target = dst_target;
    }

    /* Now read the stretched and upside down image into the destination texture */
    wined3d_context_gl_bind_texture(context_gl, texture_target, dst_texture->texture_rgb.name);
    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(texture_target,
                        0,
                        dst_rect.left, dst_rect.top, /* xoffset, yoffset */
                        0, 0, /* We blitted the image to the origin */
                        dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);
    checkGLcall("glCopyTexSubImage2D");

    if (drawBuffer == GL_BACK)
    {
        /* Write the back buffer backup back. */
        if (backup)
        {
            if (texture_target != GL_TEXTURE_2D)
            {
                gl_info->gl_ops.gl.p_glDisable(texture_target);
                gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_2D);
                texture_target = GL_TEXTURE_2D;
            }
            wined3d_context_gl_bind_texture(context_gl, GL_TEXTURE_2D, backup);
        }
        else
        {
            if (texture_target != src_target)
            {
                gl_info->gl_ops.gl.p_glDisable(texture_target);
                gl_info->gl_ops.gl.p_glEnable(src_target);
                texture_target = src_target;
            }
            wined3d_context_gl_bind_texture(context_gl, src_target, src_texture->texture_rgb.name);
        }

        gl_info->gl_ops.gl.p_glBegin(GL_QUADS);
            /* top left */
            gl_info->gl_ops.gl.p_glTexCoord2f(0.0f, 0.0f);
            gl_info->gl_ops.gl.p_glVertex2i(0, src_height);

            /* bottom left */
            gl_info->gl_ops.gl.p_glTexCoord2f(0.0f, (float)src_height / (float)src_pow2_height);
            gl_info->gl_ops.gl.p_glVertex2i(0, 0);

            /* bottom right */
            gl_info->gl_ops.gl.p_glTexCoord2f((float)src_width / (float)src_pow2_width,
                    (float)src_height / (float)src_pow2_height);
            gl_info->gl_ops.gl.p_glVertex2i(src_width, 0);

            /* top right */
            gl_info->gl_ops.gl.p_glTexCoord2f((float)src_width / (float)src_pow2_width, 0.0f);
            gl_info->gl_ops.gl.p_glVertex2i(src_width, src_height);
        gl_info->gl_ops.gl.p_glEnd();
    }
    gl_info->gl_ops.gl.p_glDisable(texture_target);
    checkGLcall("glDisable(texture_target)");

    /* Cleanup */
    if (src != src_texture->texture_rgb.name && src != backup)
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &src);
        checkGLcall("glDeleteTextures(1, &src)");
    }
    if (backup)
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &backup);
        checkGLcall("glDeleteTextures(1, &backup)");
    }

    context_release(context);

    /* The texture is now most up to date - If the surface is a render target
     * and has a drawable, this path is never entered. */
    wined3d_texture_validate_location(&dst_texture->t, dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_invalidate_location(&dst_texture->t, dst_sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);
}

static HRESULT wined3d_texture_blt_special(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const RECT *dst_rect, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        const RECT *src_rect, DWORD flags, const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter)
{
    struct wined3d_swapchain *src_swapchain, *dst_swapchain;
    const struct wined3d_rendertarget_view *rtv;

    TRACE("dst_texture %p, dst_sub_resource_idx %u, dst_rect %s, src_texture %p, "
            "src_sub_resource_idx %u, src_rect %s, flags %#x, fx %p, filter %s.\n",
            dst_texture, dst_sub_resource_idx, wine_dbgstr_rect(dst_rect), src_texture, src_sub_resource_idx,
            wine_dbgstr_rect(src_rect), flags, fx, debug_d3dtexturefiltertype(filter));

    if (dst_texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(dst_texture->resource.type));
        return WINED3DERR_INVALIDCALL;
    }

    /* Get the swapchain. One of the surfaces has to be a primary surface. */
    if (!(dst_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        WARN("Destination resource is not GPU accessible, rejecting GL blit.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!(src_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        WARN("Source resource is not GPU accessible, rejecting GL blit.\n");
        return WINED3DERR_INVALIDCALL;
    }

    src_swapchain = src_texture->swapchain;
    dst_swapchain = dst_texture->swapchain;

    /* Early sort out of cases where no render target is used */
    if (!(rtv = dst_texture->resource.device->fb.render_targets[0]) || (!src_swapchain && !dst_swapchain
            && (&src_texture->resource != rtv->resource || src_sub_resource_idx != rtv->sub_resource_idx)
            && (&dst_texture->resource != rtv->resource || dst_sub_resource_idx != rtv->sub_resource_idx)))
    {
        TRACE("No surface is render target, not using hardware blit.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* No destination color keying supported */
    if (flags & (WINED3D_BLT_DST_CKEY | WINED3D_BLT_DST_CKEY_OVERRIDE))
    {
        /* Can we support that with glBlendFunc if blitting to the frame buffer? */
        TRACE("Destination color key not supported in accelerated Blit, falling back to software\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_swapchain && dst_swapchain == src_swapchain)
    {
        FIXME("Implement hardware blit between two surfaces on the same swapchain\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_swapchain && src_swapchain)
    {
        FIXME("Implement hardware blit between two different swapchains\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_swapchain)
    {
        /* Handled with regular texture -> swapchain blit */
        if (&src_texture->resource == rtv->resource && src_sub_resource_idx == rtv->sub_resource_idx)
            TRACE("Blit from active render target to a swapchain\n");
    }
    else if (src_swapchain && &dst_texture->resource == rtv->resource
            && dst_sub_resource_idx == rtv->sub_resource_idx)
    {
        FIXME("Implement blit from a swapchain to the active render target\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!dst_swapchain && (src_swapchain || (&src_texture->resource == rtv->resource
            && src_sub_resource_idx == rtv->sub_resource_idx)))
    {
        unsigned int src_level, src_width, src_height;
        /* Blit from render target to texture */
        BOOL stretchx;

        /* P8 read back is not implemented */
        if (src_texture->resource.format->id == WINED3DFMT_P8_UINT
                || dst_texture->resource.format->id == WINED3DFMT_P8_UINT)
        {
            TRACE("P8 read back not supported by frame buffer to texture blit\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (flags & (WINED3D_BLT_SRC_CKEY | WINED3D_BLT_SRC_CKEY_OVERRIDE))
        {
            TRACE("Color keying not supported by frame buffer to texture blit\n");
            return WINED3DERR_INVALIDCALL;
            /* Destination color key is checked above */
        }

        if (dst_rect->right - dst_rect->left != src_rect->right - src_rect->left)
            stretchx = TRUE;
        else
            stretchx = FALSE;

        /* Blt is a pretty powerful call, while glCopyTexSubImage2D is not. glCopyTexSubImage cannot
         * flip the image nor scale it.
         *
         * -> If the app asks for an unscaled, upside down copy, just perform one glCopyTexSubImage2D call
         * -> If the app wants an image width an unscaled width, copy it line per line
         * -> If the app wants an image that is scaled on the x axis, and the destination rectangle is smaller
         *    than the frame buffer, draw an upside down scaled image onto the fb, read it back and restore the
         *    back buffer. This is slower than reading line per line, thus not used for flipping
         * -> If the app wants a scaled image with a dest rect that is bigger than the fb, it has to be copied
         *    pixel by pixel. */
        src_level = src_sub_resource_idx % src_texture->level_count;
        src_width = wined3d_texture_get_level_width(src_texture, src_level);
        src_height = wined3d_texture_get_level_height(src_texture, src_level);
        if (!stretchx || dst_rect->right - dst_rect->left > src_width
                || dst_rect->bottom - dst_rect->top > src_height)
        {
            TRACE("No stretching in x direction, using direct framebuffer -> texture copy.\n");
            fb_copy_to_texture_direct(wined3d_texture_gl(dst_texture), dst_sub_resource_idx, dst_rect,
                    wined3d_texture_gl(src_texture), src_sub_resource_idx, src_rect, filter);
        }
        else
        {
            TRACE("Using hardware stretching to flip / stretch the texture.\n");
            fb_copy_to_texture_hwstretch(wined3d_texture_gl(dst_texture), dst_sub_resource_idx, dst_rect,
                    wined3d_texture_gl(src_texture), src_sub_resource_idx, src_rect, filter);
        }

        return WINED3D_OK;
    }

    /* Default: Fall back to the generic blt. Not an error, a TRACE is enough */
    TRACE("Didn't find any usable render target setup for hw blit, falling back to software\n");
    return WINED3DERR_INVALIDCALL;
}

/* Context activation is done by the caller. */
static void fbo_blitter_destroy(struct wined3d_blitter *blitter, struct wined3d_context *context)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_destroy(next, context);

    heap_free(blitter);
}

static void fbo_blitter_clear(struct wined3d_blitter *blitter, struct wined3d_device *device,
        unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
        const RECT *draw_rect, DWORD flags, const struct wined3d_color *colour, float depth, DWORD stencil)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_clear(next, device, rt_count, fb, rect_count,
                clear_rects, draw_rect, flags, colour, depth, stencil);
}

static DWORD fbo_blitter_blit(struct wined3d_blitter *blitter, enum wined3d_blit_op op,
        struct wined3d_context *context, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        DWORD src_location, const RECT *src_rect, struct wined3d_texture *dst_texture,
        unsigned int dst_sub_resource_idx, DWORD dst_location, const RECT *dst_rect,
        const struct wined3d_color_key *colour_key, enum wined3d_texture_filter_type filter)
{
    struct wined3d_resource *src_resource, *dst_resource;
    enum wined3d_blit_op blit_op = op;
    struct wined3d_device *device;
    struct wined3d_blitter *next;

    TRACE("blitter %p, op %#x, context %p, src_texture %p, src_sub_resource_idx %u, src_location %s, src_rect %s, "
            "dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_rect %s, colour_key %p, filter %s.\n",
            blitter, op, context, src_texture, src_sub_resource_idx, wined3d_debug_location(src_location),
            wine_dbgstr_rect(src_rect), dst_texture, dst_sub_resource_idx, wined3d_debug_location(dst_location),
            wine_dbgstr_rect(dst_rect), colour_key, debug_d3dtexturefiltertype(filter));

    src_resource = &src_texture->resource;
    dst_resource = &dst_texture->resource;

    device = dst_resource->device;

    if (blit_op == WINED3D_BLIT_OP_RAW_BLIT && dst_resource->format->id == src_resource->format->id)
    {
        if (dst_resource->format_flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
            blit_op = WINED3D_BLIT_OP_DEPTH_BLIT;
        else
            blit_op = WINED3D_BLIT_OP_COLOR_BLIT;
    }

    if (!fbo_blitter_supported(blit_op, context->gl_info,
            src_resource, src_location, dst_resource, dst_location))
    {
        if (!(next = blitter->next))
        {
            ERR("No blitter to handle blit op %#x.\n", op);
            return dst_location;
        }

        TRACE("Forwarding to blitter %p.\n", next);
        return next->ops->blitter_blit(next, op, context, src_texture, src_sub_resource_idx, src_location,
                src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect, colour_key, filter);
    }

    if (blit_op == WINED3D_BLIT_OP_COLOR_BLIT)
    {
        TRACE("Colour blit.\n");
        texture2d_blt_fbo(device, context, filter, src_texture, src_sub_resource_idx, src_location,
                src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect);
        return dst_location;
    }

    if (blit_op == WINED3D_BLIT_OP_DEPTH_BLIT)
    {
        TRACE("Depth/stencil blit.\n");
        texture2d_depth_blt_fbo(device, context, src_texture, src_sub_resource_idx, src_location,
                src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect);
        return dst_location;
    }

    ERR("This blitter does not implement blit op %#x.\n", blit_op);
    return dst_location;
}

static const struct wined3d_blitter_ops fbo_blitter_ops =
{
    fbo_blitter_destroy,
    fbo_blitter_clear,
    fbo_blitter_blit,
};

void wined3d_fbo_blitter_create(struct wined3d_blitter **next, const struct wined3d_gl_info *gl_info)
{
    struct wined3d_blitter *blitter;

    if ((wined3d_settings.offscreen_rendering_mode != ORM_FBO) || !gl_info->fbo_ops.glBlitFramebuffer)
        return;

    if (!(blitter = heap_alloc(sizeof(*blitter))))
        return;

    TRACE("Created blitter %p.\n", blitter);

    blitter->ops = &fbo_blitter_ops;
    blitter->next = *next;
    *next = blitter;
}

/* Context activation is done by the caller. */
static void raw_blitter_destroy(struct wined3d_blitter *blitter, struct wined3d_context *context)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_destroy(next, context);

    heap_free(blitter);
}

/* Context activation is done by the caller. */
static void raw_blitter_clear(struct wined3d_blitter *blitter, struct wined3d_device *device,
        unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
        const RECT *draw_rect, DWORD flags, const struct wined3d_color *colour, float depth, DWORD stencil)
{
    struct wined3d_blitter *next;

    if (!(next = blitter->next))
    {
        ERR("No blitter to handle clear.\n");
        return;
    }

    TRACE("Forwarding to blitter %p.\n", next);
    next->ops->blitter_clear(next, device, rt_count, fb, rect_count,
            clear_rects, draw_rect, flags, colour, depth, stencil);
}

/* Context activation is done by the caller. */
static DWORD raw_blitter_blit(struct wined3d_blitter *blitter, enum wined3d_blit_op op,
        struct wined3d_context *context, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        DWORD src_location, const RECT *src_rect, struct wined3d_texture *dst_texture,
        unsigned int dst_sub_resource_idx, DWORD dst_location, const RECT *dst_rect,
        const struct wined3d_color_key *colour_key, enum wined3d_texture_filter_type filter)
{
    struct wined3d_texture_gl *src_texture_gl = wined3d_texture_gl(src_texture);
    struct wined3d_texture_gl *dst_texture_gl = wined3d_texture_gl(dst_texture);
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int src_level, src_layer, dst_level, dst_layer;
    struct wined3d_blitter *next;
    GLuint src_name, dst_name;
    DWORD location;

    /* If we would need to copy from a renderbuffer or drawable, we'd probably
     * be better of using the FBO blitter directly, since we'd need to use it
     * to copy the resource contents to the texture anyway. */
    if (op != WINED3D_BLIT_OP_RAW_BLIT
            || (src_texture->resource.format->id == dst_texture->resource.format->id
            && (!(src_location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
            || !(dst_location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB)))))
    {
        if (!(next = blitter->next))
        {
            ERR("No blitter to handle blit op %#x.\n", op);
            return dst_location;
        }

        TRACE("Forwarding to blitter %p.\n", next);
        return next->ops->blitter_blit(next, op, context, src_texture, src_sub_resource_idx, src_location,
                src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect, colour_key, filter);
    }

    TRACE("Blit using ARB_copy_image.\n");

    src_level = src_sub_resource_idx % src_texture->level_count;
    src_layer = src_sub_resource_idx / src_texture->level_count;

    dst_level = dst_sub_resource_idx % dst_texture->level_count;
    dst_layer = dst_sub_resource_idx / dst_texture->level_count;

    location = src_location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB);
    if (!location)
        location = src_texture->flags & WINED3D_TEXTURE_IS_SRGB
                ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB;
    if (!wined3d_texture_load_location(src_texture, src_sub_resource_idx, context, location))
        ERR("Failed to load the source sub-resource into %s.\n", wined3d_debug_location(location));
    src_name = wined3d_texture_gl_get_texture_name(src_texture_gl,
            context, location == WINED3D_LOCATION_TEXTURE_SRGB);

    location = dst_location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB);
    if (!location)
        location = dst_texture->flags & WINED3D_TEXTURE_IS_SRGB
                ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB;
    if (texture2d_is_full_rect(dst_texture, dst_level, dst_rect))
    {
        if (!wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, context, location))
            ERR("Failed to prepare the destination sub-resource into %s.\n", wined3d_debug_location(location));
    }
    else
    {
        if (!wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, location))
            ERR("Failed to load the destination sub-resource into %s.\n", wined3d_debug_location(location));
    }
    dst_name = wined3d_texture_gl_get_texture_name(dst_texture_gl,
            context, location == WINED3D_LOCATION_TEXTURE_SRGB);

    GL_EXTCALL(glCopyImageSubData(src_name, src_texture_gl->target, src_level,
            src_rect->left, src_rect->top, src_layer, dst_name, dst_texture_gl->target, dst_level,
            dst_rect->left, dst_rect->top, dst_layer, src_rect->right - src_rect->left,
            src_rect->bottom - src_rect->top, 1));
    checkGLcall("copy image data");

    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, location);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~location);
    if (!wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, dst_location))
        ERR("Failed to load the destination sub-resource into %s.\n", wined3d_debug_location(dst_location));

    return dst_location | location;
}

static const struct wined3d_blitter_ops raw_blitter_ops =
{
    raw_blitter_destroy,
    raw_blitter_clear,
    raw_blitter_blit,
};

void wined3d_raw_blitter_create(struct wined3d_blitter **next, const struct wined3d_gl_info *gl_info)
{
    struct wined3d_blitter *blitter;

    if (!gl_info->supported[ARB_COPY_IMAGE])
        return;

    if (!(blitter = heap_alloc(sizeof(*blitter))))
        return;

    TRACE("Created blitter %p.\n", blitter);

    blitter->ops = &raw_blitter_ops;
    blitter->next = *next;
    *next = blitter;
}

/* Context activation is done by the caller. */
static void ffp_blitter_destroy(struct wined3d_blitter *blitter, struct wined3d_context *context)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_destroy(next, context);

    heap_free(blitter);
}

static BOOL ffp_blit_supported(enum wined3d_blit_op blit_op, const struct wined3d_context *context,
        const struct wined3d_resource *src_resource, DWORD src_location,
        const struct wined3d_resource *dst_resource, DWORD dst_location)
{
    const struct wined3d_format *src_format = src_resource->format;
    const struct wined3d_format *dst_format = dst_resource->format;
    BOOL decompress;

    if (src_resource->type != WINED3D_RTYPE_TEXTURE_2D)
        return FALSE;

    decompress = (src_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_COMPRESSED)
            && !(dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_COMPRESSED);
    if (!decompress && !(src_resource->access & dst_resource->access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        TRACE("Source or destination resource is not GPU accessible.\n");
        return FALSE;
    }

    if (blit_op == WINED3D_BLIT_OP_RAW_BLIT && dst_format->id == src_format->id)
    {
        if (dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
            blit_op = WINED3D_BLIT_OP_DEPTH_BLIT;
        else
            blit_op = WINED3D_BLIT_OP_COLOR_BLIT;
    }

    switch (blit_op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT_CKEY:
            if (context->d3d_info->shader_color_key)
            {
                TRACE("Color keying requires converted textures.\n");
                return FALSE;
            }
        case WINED3D_BLIT_OP_COLOR_BLIT:
        case WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST:
            if (!context->gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
                return FALSE;

            if (TRACE_ON(d3d))
            {
                TRACE("Checking support for fixup:\n");
                dump_color_fixup_desc(src_format->color_fixup);
            }

            /* We only support identity conversions. */
            if (!is_identity_fixup(src_format->color_fixup)
                    || !is_identity_fixup(dst_format->color_fixup))
            {
                if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER
                        && dst_format->id == src_format->id && dst_location == WINED3D_LOCATION_DRAWABLE)
                {
                    WARN("Claiming fixup support because of ORM_BACKBUFFER.\n");
                }
                else
                {
                    TRACE("Fixups are not supported.\n");
                    return FALSE;
                }
            }

            if (!(dst_resource->bind_flags & WINED3D_BIND_RENDER_TARGET))
            {
                TRACE("Can only blit to render targets.\n");
                return FALSE;
            }
            return TRUE;

        default:
            TRACE("Unsupported blit operation %#x.\n", blit_op);
            return FALSE;
    }
}

static BOOL ffp_blitter_use_cpu_clear(struct wined3d_rendertarget_view *view)
{
    struct wined3d_resource *resource;
    struct wined3d_texture *texture;
    DWORD locations;

    resource = view->resource;
    if (resource->type == WINED3D_RTYPE_BUFFER)
        return !(resource->access & WINED3D_RESOURCE_ACCESS_GPU);

    texture = texture_from_resource(resource);
    locations = texture->sub_resources[view->sub_resource_idx].locations;
    if (locations & (resource->map_binding | WINED3D_LOCATION_DISCARDED))
        return !(resource->access & WINED3D_RESOURCE_ACCESS_GPU)
                || (texture->flags & WINED3D_TEXTURE_PIN_SYSMEM);

    return !(resource->access & WINED3D_RESOURCE_ACCESS_GPU)
            && !(texture->flags & WINED3D_TEXTURE_CONVERTED);
}

static void ffp_blitter_clear(struct wined3d_blitter *blitter, struct wined3d_device *device,
        unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
        const RECT *draw_rect, DWORD flags, const struct wined3d_color *colour, float depth, DWORD stencil)
{
    struct wined3d_rendertarget_view *view, *previous = NULL;
    BOOL have_identical_size = TRUE;
    struct wined3d_fb_state tmp_fb;
    unsigned int next_rt_count = 0;
    struct wined3d_blitter *next;
    DWORD next_flags = 0;
    unsigned int i;

    if (flags & WINED3DCLEAR_TARGET)
    {
        for (i = 0; i < rt_count; ++i)
        {
            if (!(view = fb->render_targets[i]))
                continue;

            if (ffp_blitter_use_cpu_clear(view)
                    || (!(view->resource->bind_flags & WINED3D_BIND_RENDER_TARGET)
                    && (wined3d_settings.offscreen_rendering_mode != ORM_FBO
                    || !(view->format_flags & WINED3DFMT_FLAG_FBO_ATTACHABLE))))
            {
                next_flags |= WINED3DCLEAR_TARGET;
                flags &= ~WINED3DCLEAR_TARGET;
                next_rt_count = rt_count;
                rt_count = 0;
                break;
            }

            /* FIXME: We should reject colour fills on formats with fixups,
             * but this would break P8 colour fills for example. */
        }
    }

    if ((flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL)) && (view = fb->depth_stencil)
            && (!view->format->depth_size || (flags & WINED3DCLEAR_ZBUFFER))
            && (!view->format->stencil_size || (flags & WINED3DCLEAR_STENCIL))
            && ffp_blitter_use_cpu_clear(view))
    {
        next_flags |= flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL);
        flags &= ~(WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL);
    }

    if (flags)
    {
        for (i = 0; i < rt_count; ++i)
        {
            if (!(view = fb->render_targets[i]))
                continue;

            if (previous && (previous->width != view->width || previous->height != view->height))
                have_identical_size = FALSE;
            previous = view;
        }
        if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
        {
            view = fb->depth_stencil;

            if (previous && (previous->width != view->width || previous->height != view->height))
                have_identical_size = FALSE;
            previous = view;
        }

        if (have_identical_size)
        {
            device_clear_render_targets(device, rt_count, fb, rect_count,
                    clear_rects, draw_rect, flags, colour, depth, stencil);
        }
        else
        {
            for (i = 0; i < rt_count; ++i)
            {
                if (!(view = fb->render_targets[i]))
                    continue;

                tmp_fb.render_targets[0] = view;
                tmp_fb.depth_stencil = NULL;
                device_clear_render_targets(device, 1, &tmp_fb, rect_count,
                        clear_rects, draw_rect, WINED3DCLEAR_TARGET, colour, depth, stencil);
            }
            if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
            {
                tmp_fb.render_targets[0] = NULL;
                tmp_fb.depth_stencil = fb->depth_stencil;
                device_clear_render_targets(device, 0, &tmp_fb, rect_count,
                        clear_rects, draw_rect, flags & ~WINED3DCLEAR_TARGET, colour, depth, stencil);
            }
        }
    }

    if (next_flags && (next = blitter->next))
        next->ops->blitter_clear(next, device, next_rt_count, fb, rect_count,
                clear_rects, draw_rect, next_flags, colour, depth, stencil);
}

static DWORD ffp_blitter_blit(struct wined3d_blitter *blitter, enum wined3d_blit_op op,
        struct wined3d_context *context, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        DWORD src_location, const RECT *src_rect, struct wined3d_texture *dst_texture,
        unsigned int dst_sub_resource_idx, DWORD dst_location, const RECT *dst_rect,
        const struct wined3d_color_key *color_key, enum wined3d_texture_filter_type filter)
{
    struct wined3d_texture_gl *src_texture_gl = wined3d_texture_gl(src_texture);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_resource *src_resource, *dst_resource;
    struct wined3d_texture *staging_texture = NULL;
    struct wined3d_color_key old_blt_key;
    struct wined3d_device *device;
    struct wined3d_blitter *next;
    DWORD old_color_key_flags;
    RECT r;

    src_resource = &src_texture->resource;
    dst_resource = &dst_texture->resource;
    device = dst_resource->device;

    if (!ffp_blit_supported(op, context, src_resource, src_location, dst_resource, dst_location))
    {
        if ((next = blitter->next))
            return next->ops->blitter_blit(next, op, context, src_texture, src_sub_resource_idx, src_location,
                    src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect, color_key, filter);
    }

    TRACE("Blt from texture %p, %u to rendertarget %p, %u.\n",
            src_texture, src_sub_resource_idx, dst_texture, dst_sub_resource_idx);

    old_blt_key = src_texture->async.src_blt_color_key;
    old_color_key_flags = src_texture->async.color_key_flags;
    wined3d_texture_set_color_key(src_texture, WINED3D_CKEY_SRC_BLT, color_key);

    if (!(src_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        struct wined3d_resource_desc desc;
        struct wined3d_box upload_box;
        unsigned int src_level;
        HRESULT hr;

        TRACE("Source texture is not GPU accessible, creating a staging texture.\n");

        src_level = src_sub_resource_idx % src_texture->level_count;
        desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
        desc.format = src_texture->resource.format->id;
        desc.multisample_type = src_texture->resource.multisample_type;
        desc.multisample_quality = src_texture->resource.multisample_quality;
        desc.usage = WINED3DUSAGE_PRIVATE;
        desc.bind_flags = 0;
        desc.access = WINED3D_RESOURCE_ACCESS_GPU;
        desc.width = wined3d_texture_get_level_width(src_texture, src_level);
        desc.height = wined3d_texture_get_level_height(src_texture, src_level);
        desc.depth = 1;
        desc.size = 0;

        if (FAILED(hr = wined3d_texture_create(device, &desc, 1, 1, 0,
                NULL, NULL, &wined3d_null_parent_ops, &staging_texture)))
        {
            ERR("Failed to create staging texture, hr %#x.\n", hr);
            return dst_location;
        }

        wined3d_box_set(&upload_box, 0, 0, desc.width, desc.height, 0, desc.depth);
        wined3d_texture_upload_from_texture(staging_texture, 0, 0, 0, 0,
                src_texture, src_sub_resource_idx, &upload_box);

        src_texture = staging_texture;
        src_texture_gl = wined3d_texture_gl(src_texture);
        src_sub_resource_idx = 0;
    }
    else
    {
        /* Make sure the surface is up-to-date. This should probably use
         * surface_load_location() and worry about the destination surface
         * too, unless we're overwriting it completely. */
        wined3d_texture_load(src_texture, context, FALSE);
    }

    wined3d_context_gl_apply_ffp_blit_state(context_gl, device);

    if (dst_location == WINED3D_LOCATION_DRAWABLE)
    {
        r = *dst_rect;
        wined3d_texture_translate_drawable_coords(dst_texture, context_gl->window, &r);
        dst_rect = &r;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        GLenum buffer;

        if (dst_location == WINED3D_LOCATION_DRAWABLE)
        {
            TRACE("Destination texture %p is onscreen.\n", dst_texture);
            buffer = wined3d_texture_get_gl_buffer(dst_texture);
        }
        else
        {
            TRACE("Destination texture %p is offscreen.\n", dst_texture);
            buffer = GL_COLOR_ATTACHMENT0;
        }
        wined3d_context_gl_apply_fbo_state_blit(context_gl, GL_DRAW_FRAMEBUFFER,
                dst_resource, dst_sub_resource_idx, NULL, 0, dst_location);
        wined3d_context_gl_set_draw_buffer(context_gl, buffer);
        wined3d_context_gl_check_fbo_status(context_gl, GL_DRAW_FRAMEBUFFER);
        context_invalidate_state(context, STATE_FRAMEBUFFER);
    }

    gl_info->gl_ops.gl.p_glEnable(src_texture_gl->target);
    checkGLcall("glEnable(target)");

    if (op == WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST || color_key)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_ALPHA_TEST);
        checkGLcall("glEnable(GL_ALPHA_TEST)");
    }

    if (color_key)
    {
        /* For P8 surfaces, the alpha component contains the palette index.
         * Which means that the colorkey is one of the palette entries. In
         * other cases pixels that should be masked away have alpha set to 0. */
        if (src_texture->resource.format->id == WINED3DFMT_P8_UINT)
            gl_info->gl_ops.gl.p_glAlphaFunc(GL_NOTEQUAL,
                    (float)src_texture->async.src_blt_color_key.color_space_low_value / 255.0f);
        else
            gl_info->gl_ops.gl.p_glAlphaFunc(GL_NOTEQUAL, 0.0f);
        checkGLcall("glAlphaFunc");
    }

    wined3d_context_gl_draw_textured_quad(context_gl, src_texture_gl,
            src_sub_resource_idx, src_rect, dst_rect, filter);

    if (op == WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST || color_key)
    {
        gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable(GL_ALPHA_TEST)");
    }

    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_2D);
    checkGLcall("glDisable(GL_TEXTURE_2D)");
    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
    }
    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_RECTANGLE_ARB);
        checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
    }

    if (dst_texture->swapchain && dst_texture->swapchain->front_buffer == dst_texture)
        gl_info->gl_ops.gl.p_glFlush();

    /* Restore the color key parameters */
    wined3d_texture_set_color_key(src_texture, WINED3D_CKEY_SRC_BLT,
            (old_color_key_flags & WINED3D_CKEY_SRC_BLT) ? &old_blt_key : NULL);

    if (staging_texture)
        wined3d_texture_decref(staging_texture);

    return dst_location;
}

static const struct wined3d_blitter_ops ffp_blitter_ops =
{
    ffp_blitter_destroy,
    ffp_blitter_clear,
    ffp_blitter_blit,
};

void wined3d_ffp_blitter_create(struct wined3d_blitter **next, const struct wined3d_gl_info *gl_info)
{
    struct wined3d_blitter *blitter;

    if (!(blitter = heap_alloc(sizeof(*blitter))))
        return;

    TRACE("Created blitter %p.\n", blitter);

    blitter->ops = &ffp_blitter_ops;
    blitter->next = *next;
    *next = blitter;
}

/* Context activation is done by the caller. */
static void cpu_blitter_destroy(struct wined3d_blitter *blitter, struct wined3d_context *context)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_destroy(next, context);

    heap_free(blitter);
}

static HRESULT surface_cpu_blt_compressed(const BYTE *src_data, BYTE *dst_data,
        UINT src_pitch, UINT dst_pitch, UINT update_w, UINT update_h,
        const struct wined3d_format *format, DWORD flags, const struct wined3d_blt_fx *fx)
{
    UINT row_block_count;
    const BYTE *src_row;
    BYTE *dst_row;
    UINT x, y;

    src_row = src_data;
    dst_row = dst_data;

    row_block_count = (update_w + format->block_width - 1) / format->block_width;

    if (!flags)
    {
        for (y = 0; y < update_h; y += format->block_height)
        {
            memcpy(dst_row, src_row, row_block_count * format->block_byte_count);
            src_row += src_pitch;
            dst_row += dst_pitch;
        }

        return WINED3D_OK;
    }

    if (flags == WINED3D_BLT_FX && fx->fx == WINEDDBLTFX_MIRRORUPDOWN)
    {
        src_row += (((update_h / format->block_height) - 1) * src_pitch);

        switch (format->id)
        {
            case WINED3DFMT_DXT1:
                for (y = 0; y < update_h; y += format->block_height)
                {
                    struct block
                    {
                        WORD color[2];
                        BYTE control_row[4];
                    };

                    const struct block *s = (const struct block *)src_row;
                    struct block *d = (struct block *)dst_row;

                    for (x = 0; x < row_block_count; ++x)
                    {
                        d[x].color[0] = s[x].color[0];
                        d[x].color[1] = s[x].color[1];
                        d[x].control_row[0] = s[x].control_row[3];
                        d[x].control_row[1] = s[x].control_row[2];
                        d[x].control_row[2] = s[x].control_row[1];
                        d[x].control_row[3] = s[x].control_row[0];
                    }
                    src_row -= src_pitch;
                    dst_row += dst_pitch;
                }
                return WINED3D_OK;

            case WINED3DFMT_DXT2:
            case WINED3DFMT_DXT3:
                for (y = 0; y < update_h; y += format->block_height)
                {
                    struct block
                    {
                        WORD alpha_row[4];
                        WORD color[2];
                        BYTE control_row[4];
                    };

                    const struct block *s = (const struct block *)src_row;
                    struct block *d = (struct block *)dst_row;

                    for (x = 0; x < row_block_count; ++x)
                    {
                        d[x].alpha_row[0] = s[x].alpha_row[3];
                        d[x].alpha_row[1] = s[x].alpha_row[2];
                        d[x].alpha_row[2] = s[x].alpha_row[1];
                        d[x].alpha_row[3] = s[x].alpha_row[0];
                        d[x].color[0] = s[x].color[0];
                        d[x].color[1] = s[x].color[1];
                        d[x].control_row[0] = s[x].control_row[3];
                        d[x].control_row[1] = s[x].control_row[2];
                        d[x].control_row[2] = s[x].control_row[1];
                        d[x].control_row[3] = s[x].control_row[0];
                    }
                    src_row -= src_pitch;
                    dst_row += dst_pitch;
                }
                return WINED3D_OK;

            default:
                FIXME("Compressed flip not implemented for format %s.\n",
                        debug_d3dformat(format->id));
                return E_NOTIMPL;
        }
    }

    FIXME("Unsupported blit on compressed surface (format %s, flags %#x, DDFX %#x).\n",
            debug_d3dformat(format->id), flags, flags & WINED3D_BLT_FX ? fx->fx : 0);

    return E_NOTIMPL;
}

static HRESULT surface_cpu_blt(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const struct wined3d_box *dst_box, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        const struct wined3d_box *src_box, DWORD flags, const struct wined3d_blt_fx *fx,
        enum wined3d_texture_filter_type filter)
{
    unsigned int bpp, src_height, src_width, dst_height, dst_width, row_byte_count;
    struct wined3d_device *device = dst_texture->resource.device;
    const struct wined3d_format *src_format, *dst_format;
    struct wined3d_texture *converted_texture = NULL;
    struct wined3d_bo_address src_data, dst_data;
    struct wined3d_context_gl *context_gl = NULL;
    unsigned int src_fmt_flags, dst_fmt_flags;
    struct wined3d_map_desc dst_map, src_map;
    struct wined3d_context *context = NULL;
    unsigned int x, sx, xinc, y, sy, yinc;
    unsigned int texture_level;
    HRESULT hr = WINED3D_OK;
    BOOL same_sub_resource;
    DWORD map_binding;
    const BYTE *sbase;
    const BYTE *sbuf;
    BYTE *dbuf;

    TRACE("dst_texture %p, dst_sub_resource_idx %u, dst_box %s, src_texture %p, "
            "src_sub_resource_idx %u, src_box %s, flags %#x, fx %p, filter %s.\n",
            dst_texture, dst_sub_resource_idx, debug_box(dst_box), src_texture,
            src_sub_resource_idx, debug_box(src_box), flags, fx, debug_d3dtexturefiltertype(filter));

    if (device->d3d_initialized)
    {
        context = context_acquire(device, NULL, 0);
        context_gl = wined3d_context_gl(context);
    }

    if (src_texture == dst_texture && src_sub_resource_idx == dst_sub_resource_idx)
    {
        same_sub_resource = TRUE;

        map_binding = dst_texture->resource.map_binding;
        texture_level = dst_sub_resource_idx % dst_texture->level_count;
        if (!wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, map_binding))
            ERR("Failed to load the destination sub-resource into %s.\n", wined3d_debug_location(map_binding));
        wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~map_binding);
        wined3d_texture_get_pitch(dst_texture, texture_level, &dst_map.row_pitch, &dst_map.slice_pitch);
        wined3d_texture_get_memory(dst_texture, dst_sub_resource_idx, &dst_data, map_binding);
        dst_map.data = wined3d_context_gl_map_bo_address(context_gl, &dst_data,
                dst_texture->sub_resources[dst_sub_resource_idx].size,
                GL_PIXEL_UNPACK_BUFFER, WINED3D_MAP_READ | WINED3D_MAP_WRITE);

        src_map = dst_map;
        src_format = dst_texture->resource.format;
        dst_format = src_format;
        dst_fmt_flags = dst_texture->resource.format_flags;
        src_fmt_flags = dst_fmt_flags;
    }
    else
    {
        same_sub_resource = FALSE;
        dst_format = dst_texture->resource.format;
        dst_fmt_flags = dst_texture->resource.format_flags;
        if (!(flags & WINED3D_BLT_RAW) && dst_texture->resource.format->id != src_texture->resource.format->id)
        {
            if (!(converted_texture = surface_convert_format(src_texture, src_sub_resource_idx, dst_format)))
            {
                FIXME("Cannot convert %s to %s.\n", debug_d3dformat(src_texture->resource.format->id),
                        debug_d3dformat(dst_texture->resource.format->id));
                if (context)
                    context_release(context);
                return WINED3DERR_NOTAVAILABLE;
            }
            src_texture = converted_texture;
            src_sub_resource_idx = 0;
        }
        src_format = src_texture->resource.format;
        src_fmt_flags = src_texture->resource.format_flags;

        map_binding = src_texture->resource.map_binding;
        texture_level = src_sub_resource_idx % src_texture->level_count;
        if (!wined3d_texture_load_location(src_texture, src_sub_resource_idx, context, map_binding))
            ERR("Failed to load the source sub-resource into %s.\n", wined3d_debug_location(map_binding));
        wined3d_texture_get_pitch(src_texture, texture_level, &src_map.row_pitch, &src_map.slice_pitch);
        wined3d_texture_get_memory(src_texture, src_sub_resource_idx, &src_data, map_binding);
        src_map.data = wined3d_context_gl_map_bo_address(context_gl, &src_data,
                src_texture->sub_resources[src_sub_resource_idx].size, GL_PIXEL_UNPACK_BUFFER, WINED3D_MAP_READ);

        map_binding = dst_texture->resource.map_binding;
        texture_level = dst_sub_resource_idx % dst_texture->level_count;
        if (!wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, map_binding))
            ERR("Failed to load the destination sub-resource into %s.\n", wined3d_debug_location(map_binding));
        wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~map_binding);
        wined3d_texture_get_pitch(dst_texture, texture_level, &dst_map.row_pitch, &dst_map.slice_pitch);
        wined3d_texture_get_memory(dst_texture, dst_sub_resource_idx, &dst_data, map_binding);
        dst_map.data = wined3d_context_gl_map_bo_address(context_gl, &dst_data,
                dst_texture->sub_resources[dst_sub_resource_idx].size, GL_PIXEL_UNPACK_BUFFER, WINED3D_MAP_WRITE);
    }
    flags &= ~WINED3D_BLT_RAW;

    bpp = dst_format->byte_count;
    src_height = src_box->bottom - src_box->top;
    src_width = src_box->right - src_box->left;
    dst_height = dst_box->bottom - dst_box->top;
    dst_width = dst_box->right - dst_box->left;
    row_byte_count = dst_width * bpp;

    sbase = (BYTE *)src_map.data
            + ((src_box->top / src_format->block_height) * src_map.row_pitch)
            + ((src_box->left / src_format->block_width) * src_format->block_byte_count);
    dbuf = (BYTE *)dst_map.data
            + ((dst_box->top / dst_format->block_height) * dst_map.row_pitch)
            + ((dst_box->left / dst_format->block_width) * dst_format->block_byte_count);

    if (src_fmt_flags & dst_fmt_flags & WINED3DFMT_FLAG_BLOCKS)
    {
        TRACE("%s -> %s copy.\n", debug_d3dformat(src_format->id), debug_d3dformat(dst_format->id));

        if (same_sub_resource)
        {
            FIXME("Only plain blits supported on compressed surfaces.\n");
            hr = E_NOTIMPL;
            goto release;
        }

        if (src_height != dst_height || src_width != dst_width)
        {
            WARN("Stretching not supported on compressed surfaces.\n");
            hr = WINED3DERR_INVALIDCALL;
            goto release;
        }

        hr = surface_cpu_blt_compressed(sbase, dbuf,
                src_map.row_pitch, dst_map.row_pitch, dst_width, dst_height,
                src_format, flags, fx);
        goto release;
    }

    if (filter != WINED3D_TEXF_NONE && filter != WINED3D_TEXF_POINT
            && (src_width != dst_width || src_height != dst_height))
    {
        /* Can happen when d3d9 apps do a StretchRect() call which isn't handled in GL. */
        FIXME("Filter %s not supported in software blit.\n", debug_d3dtexturefiltertype(filter));
    }

    xinc = (src_width << 16) / dst_width;
    yinc = (src_height << 16) / dst_height;

    if (!flags)
    {
        /* No effects, we can cheat here. */
        if (dst_width == src_width)
        {
            if (dst_height == src_height)
            {
                /* No stretching in either direction. This needs to be as fast
                 * as possible. */
                sbuf = sbase;

                /* Check for overlapping surfaces. */
                if (!same_sub_resource || dst_box->top < src_box->top
                        || dst_box->right <= src_box->left || src_box->right <= dst_box->left)
                {
                    /* No overlap, or dst above src, so copy from top downwards. */
                    for (y = 0; y < dst_height; ++y)
                    {
                        memcpy(dbuf, sbuf, row_byte_count);
                        sbuf += src_map.row_pitch;
                        dbuf += dst_map.row_pitch;
                    }
                }
                else if (dst_box->top > src_box->top)
                {
                    /* Copy from bottom upwards. */
                    sbuf += src_map.row_pitch * dst_height;
                    dbuf += dst_map.row_pitch * dst_height;
                    for (y = 0; y < dst_height; ++y)
                    {
                        sbuf -= src_map.row_pitch;
                        dbuf -= dst_map.row_pitch;
                        memcpy(dbuf, sbuf, row_byte_count);
                    }
                }
                else
                {
                    /* Src and dst overlapping on the same line, use memmove. */
                    for (y = 0; y < dst_height; ++y)
                    {
                        memmove(dbuf, sbuf, row_byte_count);
                        sbuf += src_map.row_pitch;
                        dbuf += dst_map.row_pitch;
                    }
                }
            }
            else
            {
                /* Stretching in y direction only. */
                for (y = sy = 0; y < dst_height; ++y, sy += yinc)
                {
                    sbuf = sbase + (sy >> 16) * src_map.row_pitch;
                    memcpy(dbuf, sbuf, row_byte_count);
                    dbuf += dst_map.row_pitch;
                }
            }
        }
        else
        {
            /* Stretching in X direction. */
            unsigned int last_sy = ~0u;
            for (y = sy = 0; y < dst_height; ++y, sy += yinc)
            {
                sbuf = sbase + (sy >> 16) * src_map.row_pitch;

                if ((sy >> 16) == (last_sy >> 16))
                {
                    /* This source row is the same as last source row -
                     * Copy the already stretched row. */
                    memcpy(dbuf, dbuf - dst_map.row_pitch, row_byte_count);
                }
                else
                {
#define STRETCH_ROW(type) \
do { \
    const type *s = (const type *)sbuf; \
    type *d = (type *)dbuf; \
    for (x = sx = 0; x < dst_width; ++x, sx += xinc) \
        d[x] = s[sx >> 16]; \
} while(0)

                    switch(bpp)
                    {
                        case 1:
                            STRETCH_ROW(BYTE);
                            break;
                        case 2:
                            STRETCH_ROW(WORD);
                            break;
                        case 4:
                            STRETCH_ROW(DWORD);
                            break;
                        case 3:
                        {
                            const BYTE *s;
                            BYTE *d = dbuf;
                            for (x = sx = 0; x < dst_width; x++, sx+= xinc)
                            {
                                DWORD pixel;

                                s = sbuf + 3 * (sx >> 16);
                                pixel = s[0] | (s[1] << 8) | (s[2] << 16);
                                d[0] = (pixel      ) & 0xff;
                                d[1] = (pixel >>  8) & 0xff;
                                d[2] = (pixel >> 16) & 0xff;
                                d += 3;
                            }
                            break;
                        }
                        default:
                            FIXME("Stretched blit not implemented for bpp %u.\n", bpp * 8);
                            hr = WINED3DERR_NOTAVAILABLE;
                            goto error;
                    }
#undef STRETCH_ROW
                }
                dbuf += dst_map.row_pitch;
                last_sy = sy;
            }
        }
    }
    else
    {
        LONG dstyinc = dst_map.row_pitch, dstxinc = bpp;
        DWORD keylow = 0xffffffff, keyhigh = 0, keymask = 0xffffffff;
        DWORD destkeylow = 0x0, destkeyhigh = 0xffffffff, destkeymask = 0xffffffff;
        if (flags & (WINED3D_BLT_SRC_CKEY | WINED3D_BLT_DST_CKEY
                | WINED3D_BLT_SRC_CKEY_OVERRIDE | WINED3D_BLT_DST_CKEY_OVERRIDE))
        {
            /* The color keying flags are checked for correctness in ddraw. */
            if (flags & WINED3D_BLT_SRC_CKEY)
            {
                keylow  = src_texture->async.src_blt_color_key.color_space_low_value;
                keyhigh = src_texture->async.src_blt_color_key.color_space_high_value;
            }
            else if (flags & WINED3D_BLT_SRC_CKEY_OVERRIDE)
            {
                keylow = fx->src_color_key.color_space_low_value;
                keyhigh = fx->src_color_key.color_space_high_value;
            }

            if (flags & WINED3D_BLT_DST_CKEY)
            {
                /* Destination color keys are taken from the source surface! */
                destkeylow = src_texture->async.dst_blt_color_key.color_space_low_value;
                destkeyhigh = src_texture->async.dst_blt_color_key.color_space_high_value;
            }
            else if (flags & WINED3D_BLT_DST_CKEY_OVERRIDE)
            {
                destkeylow = fx->dst_color_key.color_space_low_value;
                destkeyhigh = fx->dst_color_key.color_space_high_value;
            }

            if (bpp == 1)
            {
                keymask = 0xff;
            }
            else
            {
                DWORD masks[3];
                get_color_masks(src_format, masks);
                keymask = masks[0] | masks[1] | masks[2];
            }
            flags &= ~(WINED3D_BLT_SRC_CKEY | WINED3D_BLT_DST_CKEY
                    | WINED3D_BLT_SRC_CKEY_OVERRIDE | WINED3D_BLT_DST_CKEY_OVERRIDE);
        }

        if (flags & WINED3D_BLT_FX)
        {
            BYTE *dTopLeft, *dTopRight, *dBottomLeft, *dBottomRight, *tmp;
            LONG tmpxy;
            dTopLeft     = dbuf;
            dTopRight    = dbuf + ((dst_width - 1) * bpp);
            dBottomLeft  = dTopLeft + ((dst_height - 1) * dst_map.row_pitch);
            dBottomRight = dBottomLeft + ((dst_width - 1) * bpp);

            if (fx->fx & WINEDDBLTFX_ARITHSTRETCHY)
            {
                /* I don't think we need to do anything about this flag. */
                WARN("Nothing done for WINEDDBLTFX_ARITHSTRETCHY.\n");
            }
            if (fx->fx & WINEDDBLTFX_MIRRORLEFTRIGHT)
            {
                tmp          = dTopRight;
                dTopRight    = dTopLeft;
                dTopLeft     = tmp;
                tmp          = dBottomRight;
                dBottomRight = dBottomLeft;
                dBottomLeft  = tmp;
                dstxinc = dstxinc * -1;
            }
            if (fx->fx & WINEDDBLTFX_MIRRORUPDOWN)
            {
                tmp          = dTopLeft;
                dTopLeft     = dBottomLeft;
                dBottomLeft  = tmp;
                tmp          = dTopRight;
                dTopRight    = dBottomRight;
                dBottomRight = tmp;
                dstyinc = dstyinc * -1;
            }
            if (fx->fx & WINEDDBLTFX_NOTEARING)
            {
                /* I don't think we need to do anything about this flag. */
                WARN("Nothing done for WINEDDBLTFX_NOTEARING.\n");
            }
            if (fx->fx & WINEDDBLTFX_ROTATE180)
            {
                tmp          = dBottomRight;
                dBottomRight = dTopLeft;
                dTopLeft     = tmp;
                tmp          = dBottomLeft;
                dBottomLeft  = dTopRight;
                dTopRight    = tmp;
                dstxinc = dstxinc * -1;
                dstyinc = dstyinc * -1;
            }
            if (fx->fx & WINEDDBLTFX_ROTATE270)
            {
                tmp          = dTopLeft;
                dTopLeft     = dBottomLeft;
                dBottomLeft  = dBottomRight;
                dBottomRight = dTopRight;
                dTopRight    = tmp;
                tmpxy   = dstxinc;
                dstxinc = dstyinc;
                dstyinc = tmpxy;
                dstxinc = dstxinc * -1;
            }
            if (fx->fx & WINEDDBLTFX_ROTATE90)
            {
                tmp          = dTopLeft;
                dTopLeft     = dTopRight;
                dTopRight    = dBottomRight;
                dBottomRight = dBottomLeft;
                dBottomLeft  = tmp;
                tmpxy   = dstxinc;
                dstxinc = dstyinc;
                dstyinc = tmpxy;
                dstyinc = dstyinc * -1;
            }
            if (fx->fx & WINEDDBLTFX_ZBUFFERBASEDEST)
            {
                /* I don't think we need to do anything about this flag. */
                WARN("Nothing done for WINEDDBLTFX_ZBUFFERBASEDEST.\n");
            }
            dbuf = dTopLeft;
            flags &= ~(WINED3D_BLT_FX);
        }

#define COPY_COLORKEY_FX(type) \
do { \
    const type *s; \
    type *d = (type *)dbuf, *dx, tmp; \
    for (y = sy = 0; y < dst_height; ++y, sy += yinc) \
    { \
        s = (const type *)(sbase + (sy >> 16) * src_map.row_pitch); \
        dx = d; \
        for (x = sx = 0; x < dst_width; ++x, sx += xinc) \
        { \
            tmp = s[sx >> 16]; \
            if (((tmp & keymask) < keylow || (tmp & keymask) > keyhigh) \
                    && ((dx[0] & destkeymask) >= destkeylow && (dx[0] & destkeymask) <= destkeyhigh)) \
            { \
                dx[0] = tmp; \
            } \
            dx = (type *)(((BYTE *)dx) + dstxinc); \
        } \
        d = (type *)(((BYTE *)d) + dstyinc); \
    } \
} while(0)

        switch (bpp)
        {
            case 1:
                COPY_COLORKEY_FX(BYTE);
                break;
            case 2:
                COPY_COLORKEY_FX(WORD);
                break;
            case 4:
                COPY_COLORKEY_FX(DWORD);
                break;
            case 3:
            {
                const BYTE *s;
                BYTE *d = dbuf, *dx;
                for (y = sy = 0; y < dst_height; ++y, sy += yinc)
                {
                    sbuf = sbase + (sy >> 16) * src_map.row_pitch;
                    dx = d;
                    for (x = sx = 0; x < dst_width; ++x, sx+= xinc)
                    {
                        DWORD pixel, dpixel = 0;
                        s = sbuf + 3 * (sx>>16);
                        pixel = s[0] | (s[1] << 8) | (s[2] << 16);
                        dpixel = dx[0] | (dx[1] << 8 ) | (dx[2] << 16);
                        if (((pixel & keymask) < keylow || (pixel & keymask) > keyhigh)
                                && ((dpixel & keymask) >= destkeylow || (dpixel & keymask) <= keyhigh))
                        {
                            dx[0] = (pixel      ) & 0xff;
                            dx[1] = (pixel >>  8) & 0xff;
                            dx[2] = (pixel >> 16) & 0xff;
                        }
                        dx += dstxinc;
                    }
                    d += dstyinc;
                }
                break;
            }
            default:
                FIXME("%s color-keyed blit not implemented for bpp %u.\n",
                      (flags & WINED3D_BLT_SRC_CKEY) ? "Source" : "Destination", bpp * 8);
                hr = WINED3DERR_NOTAVAILABLE;
                goto error;
#undef COPY_COLORKEY_FX
        }
    }

error:
    if (flags)
        FIXME("    Unsupported flags %#x.\n", flags);

release:
    wined3d_context_gl_unmap_bo_address(context_gl, &dst_data, GL_PIXEL_UNPACK_BUFFER);
    if (!same_sub_resource)
        wined3d_context_gl_unmap_bo_address(context_gl, &src_data, GL_PIXEL_UNPACK_BUFFER);
    if (SUCCEEDED(hr) && dst_texture->swapchain && dst_texture->swapchain->front_buffer == dst_texture)
    {
        SetRect(&dst_texture->swapchain->front_buffer_update,
                dst_box->left, dst_box->top, dst_box->right, dst_box->bottom);
        dst_texture->swapchain->swapchain_ops->swapchain_frontbuffer_updated(dst_texture->swapchain);
    }
    if (converted_texture)
        wined3d_texture_decref(converted_texture);
    if (context)
        context_release(context);

    return hr;
}

static void surface_cpu_blt_colour_fill(struct wined3d_rendertarget_view *view,
        const struct wined3d_box *box, const struct wined3d_color *colour)
{
    struct wined3d_device *device = view->resource->device;
    struct wined3d_context_gl *context_gl = NULL;
    struct wined3d_context *context = NULL;
    struct wined3d_texture *texture;
    struct wined3d_bo_address data;
    unsigned int x, y, w, h, bpp;
    struct wined3d_map_desc map;
    DWORD map_binding;
    BYTE *row;
    DWORD c;

    TRACE("view %p, box %s, colour %s.\n", view, debug_box(box), debug_color(colour));

    if (view->format_flags & WINED3DFMT_FLAG_BLOCKS)
    {
        FIXME("Not implemented for format %s.\n", debug_d3dformat(view->format->id));
        return;
    }

    if (view->format->id != view->resource->format->id)
        FIXME("View format %s doesn't match resource format %s.\n",
                debug_d3dformat(view->format->id), debug_d3dformat(view->resource->format->id));

    if (view->resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Not implemented for buffers.\n");
        return;
    }

    if (device->d3d_initialized)
    {
        context = context_acquire(device, NULL, 0);
        context_gl = wined3d_context_gl(context);
    }

    c = wined3d_format_convert_from_float(view->format, colour);
    bpp = view->format->byte_count;
    w = min(box->right, view->width) - min(box->left, view->width);
    h = min(box->bottom, view->height) - min(box->top, view->height);

    texture = texture_from_resource(view->resource);
    map_binding = texture->resource.map_binding;
    if (!wined3d_texture_load_location(texture, view->sub_resource_idx, context, map_binding))
        ERR("Failed to load the sub-resource into %s.\n", wined3d_debug_location(map_binding));
    wined3d_texture_invalidate_location(texture, view->sub_resource_idx, ~map_binding);
    wined3d_texture_get_pitch(texture, view->sub_resource_idx % texture->level_count,
            &map.row_pitch, &map.slice_pitch);
    wined3d_texture_get_memory(texture, view->sub_resource_idx, &data, map_binding);
    map.data = wined3d_context_gl_map_bo_address(context_gl, &data,
            texture->sub_resources[view->sub_resource_idx].size, GL_PIXEL_UNPACK_BUFFER, WINED3D_MAP_WRITE);
    map.data = (BYTE *)map.data
            + (box->front * map.slice_pitch)
            + ((box->top / view->format->block_height) * map.row_pitch)
            + ((box->left / view->format->block_width) * view->format->block_byte_count);

    switch (bpp)
    {
        case 1:
            for (x = 0; x < w; ++x)
            {
                ((BYTE *)map.data)[x] = c;
            }
            break;

        case 2:
            for (x = 0; x < w; ++x)
            {
                ((WORD *)map.data)[x] = c;
            }
            break;

        case 3:
        {
            row = map.data;
            for (x = 0; x < w; ++x, row += 3)
            {
                row[0] = (c      ) & 0xff;
                row[1] = (c >>  8) & 0xff;
                row[2] = (c >> 16) & 0xff;
            }
            break;
        }
        case 4:
            for (x = 0; x < w; ++x)
            {
                ((DWORD *)map.data)[x] = c;
            }
            break;

        default:
            FIXME("Not implemented for bpp %u.\n", bpp);
            wined3d_resource_unmap(view->resource, view->sub_resource_idx);
            return;
    }

    row = map.data;
    for (y = 1; y < h; ++y)
    {
        row += map.row_pitch;
        memcpy(row, map.data, w * bpp);
    }

    wined3d_context_gl_unmap_bo_address(context_gl, &data, GL_PIXEL_UNPACK_BUFFER);
    if (context)
        context_release(context);
}

static void cpu_blitter_clear(struct wined3d_blitter *blitter, struct wined3d_device *device,
        unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
        const RECT *draw_rect, DWORD flags, const struct wined3d_color *colour, float depth, DWORD stencil)
{
    struct wined3d_color c = {depth, 0.0f, 0.0f, 0.0f};
    struct wined3d_rendertarget_view *view;
    struct wined3d_box box;
    unsigned int i, j;

    if (!rect_count)
    {
        rect_count = 1;
        clear_rects = draw_rect;
    }

    for (i = 0; i < rect_count; ++i)
    {
        box.left = max(clear_rects[i].left, draw_rect->left);
        box.top = max(clear_rects[i].top, draw_rect->top);
        box.right = min(clear_rects[i].right, draw_rect->right);
        box.bottom = min(clear_rects[i].bottom, draw_rect->bottom);
        box.front = 0;
        box.back = 1;

        if (box.left >= box.right || box.top >= box.bottom)
            continue;

        if (flags & WINED3DCLEAR_TARGET)
        {
            for (j = 0; j < rt_count; ++j)
            {
                if ((view = fb->render_targets[j]))
                    surface_cpu_blt_colour_fill(view, &box, colour);
            }
        }

        if ((flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL)) && (view = fb->depth_stencil))
        {
            if ((view->format->depth_size && !(flags & WINED3DCLEAR_ZBUFFER))
                    || (view->format->stencil_size && !(flags & WINED3DCLEAR_STENCIL)))
                FIXME("Clearing %#x on %s.\n", flags, debug_d3dformat(view->format->id));

            surface_cpu_blt_colour_fill(view, &box, &c);
        }
    }
}

static DWORD cpu_blitter_blit(struct wined3d_blitter *blitter, enum wined3d_blit_op op,
        struct wined3d_context *context, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        DWORD src_location, const RECT *src_rect, struct wined3d_texture *dst_texture,
        unsigned int dst_sub_resource_idx, DWORD dst_location, const RECT *dst_rect,
        const struct wined3d_color_key *color_key, enum wined3d_texture_filter_type filter)
{
    struct wined3d_box dst_box = {dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, 0, 1};
    struct wined3d_box src_box = {src_rect->left, src_rect->top, src_rect->right, src_rect->bottom, 0, 1};
    struct wined3d_blt_fx fx;
    DWORD flags = 0;

    memset(&fx, 0, sizeof(fx));
    switch (op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT:
        case WINED3D_BLIT_OP_DEPTH_BLIT:
        case WINED3D_BLIT_OP_RAW_BLIT:
            break;
        case WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST:
            flags |= WINED3D_BLT_ALPHA_TEST;
            break;
        case WINED3D_BLIT_OP_COLOR_BLIT_CKEY:
            flags |= WINED3D_BLT_SRC_CKEY_OVERRIDE | WINED3D_BLT_FX;
            fx.src_color_key = *color_key;
            break;
        default:
            FIXME("Unhandled op %#x.\n", op);
            break;
    }

    if (FAILED(surface_cpu_blt(dst_texture, dst_sub_resource_idx, &dst_box,
            src_texture, src_sub_resource_idx, &src_box, flags, &fx, filter)))
        ERR("Failed to blit.\n");
    wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, dst_location);

    return dst_location | (dst_texture->sub_resources[dst_sub_resource_idx].locations
            & dst_texture->resource.map_binding);
}

static const struct wined3d_blitter_ops cpu_blitter_ops =
{
    cpu_blitter_destroy,
    cpu_blitter_clear,
    cpu_blitter_blit,
};

struct wined3d_blitter *wined3d_cpu_blitter_create(void)
{
    struct wined3d_blitter *blitter;

    if (!(blitter = heap_alloc(sizeof(*blitter))))
        return NULL;

    TRACE("Created blitter %p.\n", blitter);

    blitter->ops = &cpu_blitter_ops;
    blitter->next = NULL;

    return blitter;
}

HRESULT texture2d_blt(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const struct wined3d_box *dst_box, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        const struct wined3d_box *src_box, DWORD flags, const struct wined3d_blt_fx *fx,
        enum wined3d_texture_filter_type filter)
{
    struct wined3d_texture_sub_resource *src_sub_resource, *dst_sub_resource;
    struct wined3d_device *device = dst_texture->resource.device;
    struct wined3d_swapchain *src_swapchain, *dst_swapchain;
    const struct wined3d_color_key *colour_key = NULL;
    DWORD dst_location, valid_locations;
    DWORD src_ds_flags, dst_ds_flags;
    struct wined3d_context *context;
    enum wined3d_blit_op blit_op;
    BOOL scale, convert, resolve;
    RECT src_rect, dst_rect;

    static const DWORD simple_blit = WINED3D_BLT_SRC_CKEY
            | WINED3D_BLT_SRC_CKEY_OVERRIDE
            | WINED3D_BLT_ALPHA_TEST
            | WINED3D_BLT_RAW;

    TRACE("dst_texture %p, dst_sub_resource_idx %u, dst_box %s, src_texture %p, "
            "src_sub_resource_idx %u, src_box %s, flags %#x, fx %p, filter %s.\n",
            dst_texture, dst_sub_resource_idx, debug_box(dst_box), src_texture, src_sub_resource_idx,
            debug_box(src_box), flags, fx, debug_d3dtexturefiltertype(filter));
    TRACE("Usage is %s.\n", debug_d3dusage(dst_texture->resource.usage));

    if (fx)
    {
        TRACE("fx %#x.\n", fx->fx);
        TRACE("dst_color_key {0x%08x, 0x%08x}.\n",
                fx->dst_color_key.color_space_low_value,
                fx->dst_color_key.color_space_high_value);
        TRACE("src_color_key {0x%08x, 0x%08x}.\n",
                fx->src_color_key.color_space_low_value,
                fx->src_color_key.color_space_high_value);
    }

    dst_sub_resource = &dst_texture->sub_resources[dst_sub_resource_idx];
    src_sub_resource = &src_texture->sub_resources[src_sub_resource_idx];

    if (src_sub_resource->locations & WINED3D_LOCATION_DISCARDED)
    {
        WARN("Source sub-resource is discarded, nothing to do.\n");
        return WINED3D_OK;
    }

    SetRect(&src_rect, src_box->left, src_box->top, src_box->right, src_box->bottom);
    SetRect(&dst_rect, dst_box->left, dst_box->top, dst_box->right, dst_box->bottom);

    if (!fx || !(fx->fx))
        flags &= ~WINED3D_BLT_FX;

    /* WINED3D_BLT_DO_NOT_WAIT appeared in DX7. */
    if (flags & WINED3D_BLT_DO_NOT_WAIT)
    {
        static unsigned int once;

        if (!once++)
            FIXME("Can't handle WINED3D_BLT_DO_NOT_WAIT flag.\n");
    }

    flags &= ~(WINED3D_BLT_SYNCHRONOUS | WINED3D_BLT_DO_NOT_WAIT | WINED3D_BLT_WAIT);

    if (!device->d3d_initialized)
    {
        WARN("D3D not initialized, using fallback.\n");
        goto cpu;
    }

    if (flags & ~simple_blit)
    {
        WARN_(d3d_perf)("Using fallback for complex blit (%#x).\n", flags);
        goto fallback;
    }

    src_swapchain = src_texture->swapchain;
    dst_swapchain = dst_texture->swapchain;

    /* This isn't strictly needed. FBO blits for example could deal with
     * cross-swapchain blits by first downloading the source to a texture
     * before switching to the destination context. We just have this here to
     * not have to deal with the issue, since cross-swapchain blits should be
     * rare. */
    if (src_swapchain && dst_swapchain && src_swapchain != dst_swapchain)
    {
        FIXME("Using fallback for cross-swapchain blit.\n");
        goto fallback;
    }

    scale = src_box->right - src_box->left != dst_box->right - dst_box->left
            || src_box->bottom - src_box->top != dst_box->bottom - dst_box->top;
    convert = src_texture->resource.format->id != dst_texture->resource.format->id;
    resolve = src_texture->resource.multisample_type != dst_texture->resource.multisample_type;

    dst_ds_flags = dst_texture->resource.format_flags
            & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);
    src_ds_flags = src_texture->resource.format_flags
            & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);

    if (src_ds_flags || dst_ds_flags)
    {
        TRACE("Depth/stencil blit.\n");

        if (dst_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU)
            dst_location = dst_texture->resource.draw_binding;
        else
            dst_location = dst_texture->resource.map_binding;

        if ((flags & WINED3D_BLT_RAW) || (!scale && !convert && !resolve))
            blit_op = WINED3D_BLIT_OP_RAW_BLIT;
        else
            blit_op = WINED3D_BLIT_OP_DEPTH_BLIT;

        context = context_acquire(device, dst_texture, dst_sub_resource_idx);
        valid_locations = device->blitter->ops->blitter_blit(device->blitter, blit_op, context,
                src_texture, src_sub_resource_idx, src_texture->resource.draw_binding, &src_rect,
                dst_texture, dst_sub_resource_idx, dst_location, &dst_rect, NULL, filter);
        context_release(context);

        wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, valid_locations);
        wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~valid_locations);

        return WINED3D_OK;
    }

    TRACE("Colour blit.\n");

    /* In principle this would apply to depth blits as well, but we don't
     * implement those in the CPU blitter at the moment. */
    if ((dst_sub_resource->locations & dst_texture->resource.map_binding)
            && (src_sub_resource->locations & src_texture->resource.map_binding))
    {
        if (scale)
            TRACE("Not doing sysmem blit because of scaling.\n");
        else if (convert)
            TRACE("Not doing sysmem blit because of format conversion.\n");
        else
            goto cpu;
    }

    blit_op = WINED3D_BLIT_OP_COLOR_BLIT;
    if (flags & WINED3D_BLT_SRC_CKEY_OVERRIDE)
    {
        colour_key = &fx->src_color_key;
        blit_op = WINED3D_BLIT_OP_COLOR_BLIT_CKEY;
    }
    else if (flags & WINED3D_BLT_SRC_CKEY)
    {
        colour_key = &src_texture->async.src_blt_color_key;
        blit_op = WINED3D_BLIT_OP_COLOR_BLIT_CKEY;
    }
    else if (flags & WINED3D_BLT_ALPHA_TEST)
    {
        blit_op = WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST;
    }
    else if ((src_sub_resource->locations & surface_simple_locations)
            && !(dst_sub_resource->locations & surface_simple_locations))
    {
        /* Upload */
        if (scale)
            TRACE("Not doing upload because of scaling.\n");
        else if (convert)
            TRACE("Not doing upload because of format conversion.\n");
        else if (dst_texture->resource.format->conv_byte_count)
            TRACE("Not doing upload because the destination format needs conversion.\n");
        else
        {
            wined3d_texture_upload_from_texture(dst_texture, dst_sub_resource_idx, dst_box->left,
                    dst_box->top, dst_box->front, src_texture, src_sub_resource_idx, src_box);
            if (!wined3d_resource_is_offscreen(&dst_texture->resource))
            {
                context = context_acquire(device, dst_texture, dst_sub_resource_idx);
                wined3d_texture_load_location(dst_texture, dst_sub_resource_idx,
                        context, dst_texture->resource.draw_binding);
                context_release(context);
            }
            return WINED3D_OK;
        }
    }
    else if (!(src_sub_resource->locations & surface_simple_locations)
            && (dst_sub_resource->locations & dst_texture->resource.map_binding)
            && !(dst_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        /* Download */
        if (scale)
            TRACE("Not doing download because of scaling.\n");
        else if (convert)
            TRACE("Not doing download because of format conversion.\n");
        else if (src_texture->resource.format->conv_byte_count)
            TRACE("Not doing download because the source format needs conversion.\n");
        else if (wined3d_texture_gl_is_multisample_location(wined3d_texture_gl(src_texture),
                WINED3D_LOCATION_TEXTURE_RGB))
            TRACE("Not doing download because of multisample source.\n");
        else if (!texture2d_is_full_rect(src_texture, src_sub_resource_idx % src_texture->level_count, &src_rect))
            TRACE("Not doing download because of partial download (src).\n");
        else if (!texture2d_is_full_rect(dst_texture, dst_sub_resource_idx % dst_texture->level_count, &dst_rect))
            TRACE("Not doing download because of partial download (dst).\n");
        else
        {
            wined3d_texture_download_from_texture(dst_texture, dst_sub_resource_idx, src_texture,
                    src_sub_resource_idx);
            return WINED3D_OK;
        }
    }
    else if (dst_swapchain && dst_swapchain->back_buffers
            && dst_texture == dst_swapchain->front_buffer
            && src_texture == dst_swapchain->back_buffers[0])
    {
        /* Use present for back -> front blits. The idea behind this is that
         * present is potentially faster than a blit, in particular when FBO
         * blits aren't available. Some ddraw applications like Half-Life and
         * Prince of Persia 3D use Blt() from the backbuffer to the
         * frontbuffer instead of doing a Flip(). D3d8 and d3d9 applications
         * can't blit directly to the frontbuffer. */
        enum wined3d_swap_effect swap_effect = dst_swapchain->desc.swap_effect;

        TRACE("Using present for backbuffer -> frontbuffer blit.\n");

        /* Set the swap effect to COPY, we don't want the backbuffer to become
         * undefined. */
        dst_swapchain->desc.swap_effect = WINED3D_SWAP_EFFECT_COPY;
        wined3d_swapchain_present(dst_swapchain, NULL, NULL,
                dst_swapchain->win_handle, dst_swapchain->swap_interval, 0);
        dst_swapchain->desc.swap_effect = swap_effect;

        return WINED3D_OK;
    }

    if ((flags & WINED3D_BLT_RAW) || (blit_op == WINED3D_BLIT_OP_COLOR_BLIT && !scale && !convert && !resolve))
        blit_op = WINED3D_BLIT_OP_RAW_BLIT;

    if (dst_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU)
        dst_location = dst_texture->resource.draw_binding;
    else
        dst_location = dst_texture->resource.map_binding;

    context = context_acquire(device, dst_texture, dst_sub_resource_idx);
    valid_locations = device->blitter->ops->blitter_blit(device->blitter, blit_op, context,
            src_texture, src_sub_resource_idx, src_texture->resource.draw_binding, &src_rect,
            dst_texture, dst_sub_resource_idx, dst_location, &dst_rect, colour_key, filter);
    context_release(context);

    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, valid_locations);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~valid_locations);

    return WINED3D_OK;

fallback:
    /* Special cases for render targets. */
    if (SUCCEEDED(wined3d_texture_blt_special(dst_texture, dst_sub_resource_idx, &dst_rect,
            src_texture, src_sub_resource_idx, &src_rect, flags, fx, filter)))
        return WINED3D_OK;

cpu:
    return surface_cpu_blt(dst_texture, dst_sub_resource_idx, dst_box,
            src_texture, src_sub_resource_idx, src_box, flags, fx, filter);
}
