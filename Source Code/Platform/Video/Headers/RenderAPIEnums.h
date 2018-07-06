/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _GFX_ENUMS_H
#define _GFX_ENUMS_H

#include "Platform/DataTypes/Headers/PlatformDefines.h"

namespace Divide {

/// State the various attribute locations to use in shaders with VAO/VB's
enum class AttribLocation : U32 {
    VERTEX_POSITION = 0,
    VERTEX_COLOR = 1,
    VERTEX_NORMAL = 2,
    VERTEX_TEXCOORD = 3,
    VERTEX_TANGENT = 4,
    VERTEX_BONE_WEIGHT = 5,
    VERTEX_BONE_INDICE = 6
};

enum class ShaderBufferLocation : U32 {
    GPU_BLOCK = 0,
    LIGHT_NORMAL = 1,
    LIGHT_SHADOW = 2,
    NODE_INFO = 3,
    BONE_TRANSFORMS = 4,
    UNIFORMS = 5
};

/// Fixed pipeline functionality should be avoided. Both D3D and OGL should have
/// these matrices
enum class MATRIX_MODE : U32 {
    VIEW = 0,
    VIEW_INV = 1,
    PROJECTION = 2,
    PROJECTION_INV = 3,
    VIEW_PROJECTION = 4,
    // ViewProjection matrix's invers: inverse (VIEW_PROJECTION)
    VIEW_PROJECTION_INV = 5,  
    TEXTURE = 6
};

/// Using multiple threads for streaming and issuing API specific construction
/// commands to the rendering API will
/// cause problems with libraries such as ASSIMP or with the scenegraph. Having
/// 2 rendering contexts with a single
/// display list, one for rendering and one for loading seems the best approach
/// (for now)
enum class CurrentContext : U32 { 
    GFX_RENDERING_CTX = 0, 
    GFX_LOADING_CTX = 1
};

enum class RenderStage : U32 {
    SHADOW = 0,
    REFLECTION = 1,
    DISPLAY = 2,
    Z_PRE_PASS = 3,
    COUNT
};

enum class ClipPlaneIndex : U32 {
    CLIP_PLANE_0 = 0,
    CLIP_PLANE_1 = 1,
    CLIP_PLANE_2 = 2,
    CLIP_PLANE_3 = 3,
    CLIP_PLANE_4 = 4,
    CLIP_PLANE_5 = 5,
    COUNT
};

enum class PBType : U32 { 
    PB_TEXTURE_1D, 
    PB_TEXTURE_2D, 
    PB_TEXTURE_3D,
    COUNT
};

enum class PrimitiveType : U32 {
    API_POINTS = 0x0000,
    LINES = 0x0001,
    LINE_LOOP = 0x0002,
    LINE_STRIP = 0x0003,
    TRIANGLES = 0x0004,
    TRIANGLE_STRIP = 0x0005,
    TRIANGLE_FAN = 0x0006,
    QUAD_STRIP = 0x0007,
    POLYGON = 0x0008,
    COUNT
};

enum class RenderDetailLevel : U32 {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
    ULTRA = 3,
    COUNT
};

/// Specifies how the red, green, blue, and alpha source blending factors are
/// computed.
enum class BlendProperty : U32 {
    ZERO = 0,
    ONE,
    SRC_COLOR,
    INV_SRC_COLOR,
    /// Transparency is best implemented using blend function (SRC_ALPHA,
    /// ONE_MINUS_SRC_ALPHA)
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

/// Specifies how source and destination colors are combined.
enum class BlendOperation : U32 {
    /// The ADD equation is useful for antialiasing and transparency, among
    /// other things.
    ADD = 0,
    SUBTRACT,
    REV_SUBTRACT,
    /// The MIN and MAX equations are useful for applications that analyze image
    /// data
    /// (image thresholding against a constant color, for example).
    MIN,
    /// The MIN and MAX equations are useful for applications that analyze image
    /// data
    /// (image thresholding against a constant color, for example).
    MAX,
    /// Place all properties above this.
    COUNT
};

/// Valid comparison functions for most states
/// YYY = test value using this function
enum class ComparisonFunction : U32 {
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
enum class CullMode : U32 {
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
enum class ShaderType : U32 {
    FRAGMENT = 0,
    VERTEX = 1,
    GEOMETRY = 2,
    TESSELATION_CTRL = 3,
    TESSELATION_EVAL = 4,
    COMPUTE = 5,
    COUNT
};

/// Valid front and back stencil test actions
enum class StencilOperation : U32 {
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
enum class FillMode : U32 {
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

enum class TextureType : U32 {
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

enum class TextureFilter : U32 {
    LINEAR = 0x0000,
    NEAREST = 0x0001,
    NEAREST_MIPMAP_NEAREST = 0x0002,
    LINEAR_MIPMAP_NEAREST = 0x0003,
    NEAREST_MIPMAP_LINEAR = 0x0004,
    LINEAR_MIPMAP_LINEAR = 0x0005,
    COUNT
};

enum class TextureWrap : U32 {
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

enum class GFXImageFormat : U32 {
    LUMINANCE = 0,
    LUMINANCE_ALPHA,
    LUMINANCE_ALPHA16F,
    LUMINANCE_ALPHA32F,
    INTENSITY,
    ALPHA,
    RED,
    RED8,
    RED16,
    RED16F,
    RED32,
    RED32F,
    BLUE,
    GREEN,
    RG,
    RG8,
    RG16,
    RG16F,
    RG32,
    RG32F,
    BGR,
    RGB,
    RGB8,
    SRGB8,
    RGB8I,
    RGB16,
    RGB16F,
    BGRA,
    RGBA,
    RGBA4,
    RGBA8,
    SRGBA8,
    RGBA8I,
    RGBA16F,
    RGBA32F,
    DEPTH_COMPONENT,
    DEPTH_COMPONENT16,
    DEPTH_COMPONENT24,
    DEPTH_COMPONENT32,
    DEPTH_COMPONENT32F,
    COUNT
};

enum class GFXDataFormat : U32 {
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

enum class GPUVendor : U32 {
    NVIDIA = 0,
    AMD,
    INTEL,
    OTHER,
    COUNT
};

};  // namespace Divide

#endif
