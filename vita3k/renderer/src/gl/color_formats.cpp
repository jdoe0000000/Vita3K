// Vita3K emulator project
// Copyright (C) 2022 Vita3K team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <renderer/functions.h>
#include <renderer/gl/functions.h>

#include <gxm/functions.h>
#include <map>
#include <util/log.h>

namespace renderer::color {
size_t bits_per_pixel(SceGxmColorBaseFormat base_format);
}

namespace renderer::gl {
namespace color {

static const GLint swizzle_abgr[4] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
static const GLint swizzle_argb[4] = { GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA };
static const GLint swizzle_rgba[4] = { GL_ALPHA, GL_BLUE, GL_GREEN, GL_RED };
static const GLint swizzle_bgra[4] = { GL_GREEN, GL_BLUE, GL_ALPHA, GL_RED };

static const GLint swizzle_rgb[4] = { GL_BLUE, GL_GREEN, GL_RED, GL_ONE };
static const GLint swizzle_bgr[4] = { GL_RED, GL_GREEN, GL_BLUE, GL_ONE };

static const GLint swizzle_gr[4] = { GL_RED, GL_GREEN, GL_ZERO, GL_ONE };
static const GLint swizzle_rg[4] = { GL_GREEN, GL_RED, GL_ZERO, GL_ONE };
static const GLint swizzle_ar[4] = { GL_RED, GL_ZERO, GL_ZERO, GL_GREEN };
static const GLint swizzle_ra[4] = { GL_RED, GL_ZERO, GL_ZERO, GL_GREEN };

static const GLint *translate_swizzle(SceGxmColorSwizzle4Mode mode) {
    switch (mode) {
    case SCE_GXM_COLOR_SWIZZLE4_ABGR:
        return swizzle_abgr;
    case SCE_GXM_COLOR_SWIZZLE4_ARGB:
        return swizzle_argb;
    case SCE_GXM_COLOR_SWIZZLE4_RGBA:
        return swizzle_rgba;
    case SCE_GXM_COLOR_SWIZZLE4_BGRA:
        return swizzle_bgra;
    default:
        break;
    }

    return swizzle_abgr;
}

static const GLint *translate_swizzle(SceGxmColorSwizzle3Mode mode) {
    switch (mode) {
    case SCE_GXM_COLOR_SWIZZLE3_BGR:
        return swizzle_bgr;
    case SCE_GXM_COLOR_SWIZZLE3_RGB:
        return swizzle_rgb;
    }

    return swizzle_bgr;
}

static const GLint *translate_swizzle(SceGxmColorSwizzle2Mode mode) {
    switch (mode) {
    case SCE_GXM_COLOR_SWIZZLE2_GR:
        return swizzle_gr;
    case SCE_GXM_COLOR_SWIZZLE2_RG:
        return swizzle_rg;
    case SCE_GXM_COLOR_SWIZZLE2_RA:
        return swizzle_ra;
    case SCE_GXM_COLOR_SWIZZLE2_AR:
        return swizzle_ar;
    }

    return swizzle_gr;
}

// Translate popular color base format that can be bit-casted for purposes
GLenum translate_internal_format(SceGxmColorBaseFormat base_format) {
    switch (base_format) {
    case SCE_GXM_COLOR_BASE_FORMAT_U8U8U8U8:
        return GL_RGBA8;

    case SCE_GXM_COLOR_BASE_FORMAT_S8S8S8S8:
        return GL_RGBA8_SNORM;

    case SCE_GXM_COLOR_BASE_FORMAT_F16F16F16F16:
        return GL_RGBA16F;

    case SCE_GXM_COLOR_BASE_FORMAT_U2U10U10U10:
        return GL_RGBA;

    case SCE_GXM_COLOR_BASE_FORMAT_F32F32:
        return GL_RG32F;

    default:
        return GL_RGBA;
    }
}

GLenum translate_format(SceGxmColorBaseFormat base_format) {
    switch (base_format) {
    case SCE_GXM_COLOR_BASE_FORMAT_U8U8U8U8:
        return GL_RGBA;

    case SCE_GXM_COLOR_BASE_FORMAT_S8S8S8S8:
        return GL_RGBA;

    case SCE_GXM_COLOR_BASE_FORMAT_U2U10U10U10:
        return GL_RGBA;

    case SCE_GXM_COLOR_BASE_FORMAT_F32F32:
        return GL_RG;

    default:
        return GL_RGBA;
    }
}

GLenum translate_type(SceGxmColorBaseFormat base_format) {
    switch (base_format) {
    case SCE_GXM_COLOR_BASE_FORMAT_U8U8U8U8:
        return GL_UNSIGNED_BYTE;

    case SCE_GXM_COLOR_BASE_FORMAT_S8S8S8S8:
        return GL_BYTE;

    case SCE_GXM_COLOR_BASE_FORMAT_F16F16F16F16:
        return GL_HALF_FLOAT;

    case SCE_GXM_COLOR_BASE_FORMAT_U2U10U10U10:
        return GL_UNSIGNED_INT_2_10_10_10_REV;

    case SCE_GXM_COLOR_BASE_FORMAT_F32F32:
        return GL_FLOAT;

    default:
        return GL_UNSIGNED_BYTE;
    }
}

const GLint *translate_swizzle(SceGxmColorFormat fmt) {
    const SceGxmColorBaseFormat base_format = gxm::get_base_format(fmt);
    const uint32_t swizzle = fmt & SCE_GXM_COLOR_SWIZZLE_MASK;
    switch (base_format) {
    // 1 Component.
    case SCE_GXM_COLOR_BASE_FORMAT_U8U8U8U8:
    case SCE_GXM_COLOR_BASE_FORMAT_S8S8S8S8:
    case SCE_GXM_COLOR_BASE_FORMAT_F16F16F16F16:
    case SCE_GXM_COLOR_BASE_FORMAT_U2U10U10U10:
    case SCE_GXM_COLOR_BASE_FORMAT_U2F10F10F10:
        return translate_swizzle(static_cast<SceGxmColorSwizzle4Mode>(swizzle));

    case SCE_GXM_COLOR_BASE_FORMAT_SE5M9M9M9:
    case SCE_GXM_COLOR_BASE_FORMAT_U5U6U5:
        return translate_swizzle(static_cast<SceGxmColorSwizzle3Mode>(swizzle));

    case SCE_GXM_COLOR_BASE_FORMAT_F32F32:
        return translate_swizzle(static_cast<SceGxmColorSwizzle2Mode>(swizzle));

    default:
        break;
    }

    return swizzle_abgr;
}

size_t bytes_per_pixel(SceGxmColorBaseFormat base_format) {
    return gxm::bits_per_pixel(base_format) >> 3;
}

size_t bytes_per_pixel_in_gl_storage(SceGxmColorBaseFormat base_format) {
    switch (base_format) {
    case SCE_GXM_COLOR_BASE_FORMAT_U8U8U8U8:
    case SCE_GXM_COLOR_BASE_FORMAT_S8S8S8S8:
    case SCE_GXM_COLOR_BASE_FORMAT_U2U10U10U10:
        return 4;
    case SCE_GXM_COLOR_BASE_FORMAT_F16F16F16F16:
    case SCE_GXM_COLOR_BASE_FORMAT_F32F32:
        return 8;
    default:
        break;
    }

    return 4;
}

bool is_write_surface_stored_rawly(SceGxmColorBaseFormat base_format) {
    return (base_format == SCE_GXM_COLOR_BASE_FORMAT_F16F16F16F16);
}

bool is_write_surface_non_linearity_filtering(SceGxmColorBaseFormat base_format) {
    return ((base_format == SCE_GXM_COLOR_BASE_FORMAT_F32) || (base_format == SCE_GXM_COLOR_BASE_FORMAT_F32F32));
}

GLenum get_raw_store_internal_type(SceGxmColorBaseFormat base_format) {
    switch (base_format) {
    case SCE_GXM_COLOR_BASE_FORMAT_F16F16F16F16:
        return GL_RGBA16UI;

    default:
        break;
    }

    return GL_RGBA16UI;
}

GLenum get_raw_store_upload_format_type(SceGxmColorBaseFormat base_format) {
    switch (base_format) {
    case SCE_GXM_COLOR_BASE_FORMAT_F16F16F16F16:
        return GL_RGBA_INTEGER;

    default:
        break;
    }

    return GL_RGBA_INTEGER;
}

GLenum get_raw_store_upload_data_type(SceGxmColorBaseFormat base_format) {
    switch (base_format) {
    case SCE_GXM_COLOR_BASE_FORMAT_F16F16F16F16:
        return GL_UNSIGNED_SHORT;

    default:
        break;
    }

    return GL_RGBA_INTEGER;
}

bool convert_base_texture_format_to_base_color_format(SceGxmTextureBaseFormat format, SceGxmColorBaseFormat &color_format) {
    static const std::map<std::uint32_t, std::uint32_t> TEXTURE_TO_COLOR_FORMAT_MAPPING = {
        { SCE_GXM_TEXTURE_BASE_FORMAT_U8U8U8U8, SCE_GXM_COLOR_BASE_FORMAT_U8U8U8U8 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U8U8U8, SCE_GXM_COLOR_BASE_FORMAT_U8U8U8 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U5U6U5, SCE_GXM_COLOR_BASE_FORMAT_U5U6U5 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U1U5U5U5, SCE_GXM_COLOR_BASE_FORMAT_U1U5U5U5 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U4U4U4U4, SCE_GXM_COLOR_BASE_FORMAT_U4U4U4U4 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U8U3U3U2, SCE_GXM_COLOR_BASE_FORMAT_U8U3U3U2 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_F16, SCE_GXM_COLOR_BASE_FORMAT_F16 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_F16F16, SCE_GXM_COLOR_BASE_FORMAT_F16F16 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_F32, SCE_GXM_COLOR_BASE_FORMAT_F32 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_S16, SCE_GXM_COLOR_BASE_FORMAT_S16 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_S16S16, SCE_GXM_COLOR_BASE_FORMAT_S16S16 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U16, SCE_GXM_COLOR_BASE_FORMAT_U16 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U16U16, SCE_GXM_COLOR_BASE_FORMAT_U16U16 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U2U10U10U10, SCE_GXM_COLOR_BASE_FORMAT_U2U10U10U10 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U8, SCE_GXM_COLOR_BASE_FORMAT_U8 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_S8, SCE_GXM_COLOR_BASE_FORMAT_S8 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_S5S5U6, SCE_GXM_COLOR_BASE_FORMAT_S5S5U6 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U8U8, SCE_GXM_COLOR_BASE_FORMAT_U8U8 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_S8S8, SCE_GXM_COLOR_BASE_FORMAT_S8S8 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_S8S8S8S8, SCE_GXM_COLOR_BASE_FORMAT_S8S8S8S8 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_F16F16F16F16, SCE_GXM_COLOR_BASE_FORMAT_F16F16F16F16 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_F32F32, SCE_GXM_COLOR_BASE_FORMAT_F32F32 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_F11F11F10, SCE_GXM_COLOR_BASE_FORMAT_F11F11F10 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_SE5M9M9M9, SCE_GXM_COLOR_BASE_FORMAT_SE5M9M9M9 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_SE5M9M9M9, SCE_GXM_COLOR_BASE_FORMAT_SE5M9M9M9 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U2F10F10F10, SCE_GXM_COLOR_BASE_FORMAT_U2F10F10F10 },
        { SCE_GXM_TEXTURE_BASE_FORMAT_U32U32, SCE_GXM_COLOR_BASE_FORMAT_F32F32 }
    };

    auto ite = TEXTURE_TO_COLOR_FORMAT_MAPPING.find(format);
    if (ite == TEXTURE_TO_COLOR_FORMAT_MAPPING.end())
        return false;

    color_format = static_cast<SceGxmColorBaseFormat>(ite->second);
    return true;
}
} // namespace color
} // namespace renderer::gl
