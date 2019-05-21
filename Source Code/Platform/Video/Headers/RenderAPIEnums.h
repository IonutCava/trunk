/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#pragma once
#ifndef _GFX_ENUMS_H
#define _GFX_ENUMS_H

namespace Divide {

enum class RenderAPI : U8 {
    None,      ///< not supported yet
    OpenGL,    ///< 4.x+
    OpenGLES,  ///< 3.x+
    Vulkan,    ///< not supported yet
    COUNT
};

enum class RenderTargetUsage : U8 {
    SCREEN = 0,
    HI_Z = 1,
    HI_Z_REFLECT = 2,
    OIT = 3,
    REFLECTION_PLANAR = 4,
    REFLECTION_PLANAR_BLUR = 5,
    REFRACTION_PLANAR = 6,
    REFLECTION_CUBE = 7,
    ENVIRONMENT = 8,
    SHADOW = 9,
    EDITOR = 10,
    OTHER = 11,
    COUNT
};

/// State the various attribute locations to use in shaders with VAO/VB's
enum class AttribLocation : U8 {
    POSITION = 0,
    TEXCOORD = 1,
    NORMAL = 2,
    TANGENT = 3,
    COLOR = 4,
    BONE_WEIGHT = 5,
    BONE_INDICE = 6,
    WIDTH = 7,
    GENERIC = 8,
    COUNT
};

enum class ShaderBufferLocation : U8 {
    GPU_BLOCK = 0,
    ATOMIC_COUNTER = 0,
    GPU_COMMANDS,
    LIGHT_NORMAL,
    LIGHT_SHADOW,
    LIGHT_INDICES,
    NODE_INFO,
    BONE_TRANSFORMS,
    SCENE_DATA,
    TERRAIN_DATA,
    GRASS_DATA,
    TREE_DATA,
    CMD_BUFFER,
    COUNT
};

enum class RenderStage : U8 {
    SHADOW = 0,
    REFLECTION = 1,
    REFRACTION = 2,
    DISPLAY = 3,
    COUNT
};

enum class RenderPassType : U8 {
    PRE_PASS = 0,
    MAIN_PASS = 1,
    OIT_PASS = 2,
    COUNT
};

enum class PBType : U8 { 
    PB_TEXTURE_1D, 
    PB_TEXTURE_2D, 
    PB_TEXTURE_3D,
    COUNT
};

enum class PrimitiveType : U8 {
    API_POINTS = 0x0000,
    LINES = 0x0001,
    LINE_LOOP = 0x0002,
    LINE_STRIP = 0x0003,
    TRIANGLES = 0x0004,
    TRIANGLE_STRIP = 0x0005,
    TRIANGLE_FAN = 0x0006,
    QUAD_STRIP = 0x0007,
    POLYGON = 0x0008,
    PATCH = 0x0009,
    COUNT
};

/// Specifies how the red, green, blue, and alpha source blending factors are computed.
enum class BlendProperty : U8 {
    ZERO = 0,
    ONE,
    SRC_COLOR,
    INV_SRC_COLOR,
    /// Transparency is best implemented using blend function (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)
    /// with primitives sorted from farthest to nearest.
    SRC_ALPHA,
    INV_SRC_ALPHA,
    DEST_ALPHA,
    INV_DEST_ALPHA,
    DEST_COLOR,
    INV_DEST_COLOR,
    /// Polygon antialiasing is optimized using blend function
    /// (SRC_ALPHA_SATURATE, GL_ONE)
    /// with polygons sorted from nearest to farthest.
    SRC_ALPHA_SAT,
    /// Place all properties above this.
    COUNT
};

/// Specifies how source and destination colours are combined.
enum class BlendOperation : U8 {
    /// The ADD equation is useful for antialiasing and transparency, among
    /// other things.
    ADD = 0,
    SUBTRACT,
    REV_SUBTRACT,
    /// The MIN and MAX equations are useful for applications that analyze image
    /// data
    /// (image thresholding against a constant colour, for example).
    MIN,
    /// The MIN and MAX equations are useful for applications that analyze image
    /// data
    /// (image thresholding against a constant colour, for example).
    MAX,
    /// Place all properties above this.
    COUNT
};

/// Valid comparison functions for most states
/// YYY = test value using this function
enum class ComparisonFunction : U8 {
    /// Never passes.
    NEVER = 0,
    /// Passes if the incoming YYY value is less than the stored YYY value.
    LESS,
    /// Passes if the incoming YYY value is equal to the stored YYY value.
    EQUAL,
    /// Passes if the incoming YYY value is less than or equal to the stored YYY
    /// value.
    LEQUAL,
    /// Passes if the incoming YYY value is greater than the stored YYY value.
    GREATER,
    /// Passes if the incoming YYY value is not equal to the stored YYY value.
    NEQUAL,
    /// Passes if the incoming YYY value is greater than or equal to the stored
    /// YYY value.
    GEQUAL,
    /// Always passes.
    ALWAYS,
    /// Place all properties above this.
    COUNT
};

/// Specifies whether front- or back-facing facets are candidates for culling.
enum class CullMode : U8 {
    NONE = 0,
    /// Cull Back facing polygons
    CW,
    /// Cull Front facing polygons
    CCW,
    /// Cull All polygons
    ALL,
    /// Place all properties above this.
    COUNT
};

/// Available shader stages
enum class ShaderType : U8 {
    FRAGMENT = 0,
    VERTEX = 1,
    GEOMETRY = 2,
    TESSELATION_CTRL = 3,
    TESSELATION_EVAL = 4,
    COMPUTE = 5,
    COUNT
};

/// Valid front and back stencil test actions
enum class StencilOperation : U8 {
    /// Keeps the current value.
    KEEP = 0,
    /// Sets the stencil buffer value to 0.
    ZERO,
    /// Sets the stencil buffer value to ref, as specified by StencilFunc.
    REPLACE,
    /// Increments the current stencil buffer value. Clamps to the maximum
    /// representable unsigned value.
    INCR,
    ///  Decrements the current stencil buffer value. Clamps to 0.
    DECR,
    /// Bitwise inverts the current stencil buffer value.
    INV,
    /// Increments the current stencil buffer value.
    /// Wraps stencil buffer value to zero when incrementing the maximum
    /// representable unsigned value.
    INCR_WRAP,
    /// Decrements the current stencil buffer value.
    /// Wraps stencil buffer value to the maximum representable unsigned value
    /// when decrementing a stencil buffer value of zero.
    DECR_WRAP,
    /// Place all properties above this.
    COUNT
};

/// Defines all available fill modes for primitives
enum class FillMode : U8 {
    /// Polygon vertices that are marked as the start of a boundary edge are
    /// drawn as points.
    POINT = 1,
    /// Boundary edges of the polygon are drawn as line segments.
    WIREFRAME,
    /// The interior of the polygon is filled.
    SOLID,
    /// Place all properties above this.
    COUNT
};

enum class TextureType : U8 {
    TEXTURE_1D = 0,
    TEXTURE_2D,
    TEXTURE_3D,
    TEXTURE_CUBE_MAP,
    TEXTURE_2D_ARRAY,
    TEXTURE_CUBE_ARRAY,
    TEXTURE_2D_MS,
    TEXTURE_2D_ARRAY_MS,
    COUNT
};

enum class TextureFilter : U8 {
    LINEAR = 0x0000,
    NEAREST = 0x0001,
    NEAREST_MIPMAP_NEAREST = 0x0002,
    LINEAR_MIPMAP_NEAREST = 0x0003,
    NEAREST_MIPMAP_LINEAR = 0x0004,
    LINEAR_MIPMAP_LINEAR = 0x0005,
    COUNT
};

enum class TextureWrap : U8 {
    /** Texture coordinates outside [0...1] are clamped to the nearest valid
       value.     */
    CLAMP = 0x0,
    CLAMP_TO_EDGE = 0x1,
    CLAMP_TO_BORDER = 0x2,
    /** If the texture coordinates for a pixel are outside [0...1] the texture
       is not applied to that pixel */
    DECAL = 0x3,
    REPEAT = 0x4,
    MIRROR_REPEAT = 0x5,
    COUNT
};

enum class GFXImageFormat : U8 {
    RED = 0,
    RG,
    BGR,
    RGB,
    BGRA,
    RGBA,
    DEPTH_COMPONENT,
    COMPRESSED_RGB_DXT1,
    COMPRESSED_RGBA_DXT1,
    COMPRESSED_RGBA_DXT3,
    COMPRESSED_RGBA_DXT5,
    COUNT
};

enum class GFXDataFormat : U8 {
    UNSIGNED_BYTE = 0x0000,
    UNSIGNED_SHORT = 0x0001,
    UNSIGNED_INT = 0x0002,
    SIGNED_BYTE = 0x0003,
    SIGNED_SHORT = 0x0004,
    SIGNED_INT = 0x0005,
    FLOAT_16 = 0x0006,
    FLOAT_32 = 0x0008,
    COUNT
};

enum class GPUVendor : U8 {
    NVIDIA = 0,
    AMD,
    INTEL,
    MICROSOFT,
    IMAGINATION_TECH,
    ARM,
    QUALCOMM,
    VIVANTE,
    ALPHAMOSAIC,
    WEBGL, //Khronos
    OTHER,
    COUNT
};

enum class GPURenderer : U8 {
    UNKNOWN = 0,
    ADRENO,
    GEFORCE,
    INTEL,
    MALI,
    POWERVR,
    RADEON,
    VIDEOCORE,
    VIVANTE,
    WEBGL,
    GDI, //Driver not working properly?
    COUNT
};

enum class BufferStorageType : U8 {
    AUTO = 0,
    IMMUTABLE,
    NORMAL
};

enum class BufferUpdateFrequency : U8 {
    ONCE = 0,       //STATIC
    OCASSIONAL = 1, //DYNAMIC
    OFTEN = 2,      //STREAM
    COUNT
};

enum class QueryType : U8 {
    TIME = 0,
    PRIMITIVES_GENERATED,
    GPU_TIME,
    COUNT
};

};  // namespace Divide

#endif
