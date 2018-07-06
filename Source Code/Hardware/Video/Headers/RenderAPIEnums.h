/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GFX_ENUMS_H
#define _GFX_ENUMS_H

/// no need to include the entire resource header for one define
#ifndef toBit
#define toBit(X) (1 << (X))
#endif

///Fixed pipeline functionality should be avoided. Both D3D and OGL should have these matrices
enum MATRIX_MODE{
    VIEW_MATRIX = 0,
    PROJECTION_MATRIX = 1,
	VIEW_PROJECTION_MATRIX = 2,
    VIEW_PROJECTION_INV_MATRIX  = 3, //<ViewProjection matrix's invers: inverse (VIEW_PROJECTION_MATRIX)
    TEXTURE_MATRIX = 4
};

///Using multiple threads for streaming and issuing API specific construction commands to the rendering API will
///cause problems with libraries such as ASSIMP or with the scenegraph. Having 2 rendering contexts with a single
///display list, one for rendering and one for loading seems the best approach (for now)
enum CurrentContext {
    GFX_RENDERING_CONTEXT = 0,
    GFX_LOADING_CONTEXT
};

enum FogMode {
    FOG_NONE = 0,
    FOG_LINEAR,
    FOG_EXP,
    FOG_EXP2
};

enum RenderStage {
    DEFERRED_STAGE			   = toBit(1),
    SHADOW_STAGE			   = toBit(2),
    REFLECTION_STAGE		   = toBit(3),
    SSAO_STAGE				   = toBit(4),
    BLOOM_STAGE                = toBit(5),
    DOF_STAGE                  = toBit(6),
    LIGHT_SHAFT_STAGE          = toBit(7),
    FINAL_STAGE				   = toBit(8),
    ENVIRONMENT_MAPPING_STAGE  = toBit(9),
    FXAA_STAGE                 = toBit(10),
    DISPLAY_STAGE              = DEFERRED_STAGE | FINAL_STAGE,
    POSTFX_STAGE               = SSAO_STAGE | BLOOM_STAGE | DOF_STAGE | LIGHT_SHAFT_STAGE,
    DEPTH_STAGE                = SSAO_STAGE | DOF_STAGE | SHADOW_STAGE,
    //Place all stages above this
    INVALID_STAGE		       = toBit(11)
};

enum PBOType {
    PBO_TEXTURE_1D,
    PBO_TEXTURE_2D,
    PBO_TEXTURE_3D
};

enum FBOType {
    FBO_PLACEHOLDER,
    FBO_2D_COLOR,
    FBO_2D_ARRAY_COLOR, ///<FBO that uses texture arrays
    FBO_2D_COLOR_MS,///<Multisampled FBO with fallback to FBO_2D_COLOR
    FBO_CUBE_COLOR,
    FBO_CUBE_COLOR_ARRAY,
    FBO_CUBE_DEPTH_ARRAY,
    FBO_2D_DEPTH,  ///< This is the same as 2D_COLOR with color writes disabled.
    FBO_2D_ARRAY_DEPTH, ///< This is the same as 2D_DEPTH but uses array textures
    FBO_CUBE_DEPTH,
    FBO_2D_DEFERRED
};

enum RenderAPI {
    OpenGL,
    OpenGLES,
    Direct3D,///< not supported yet
    Software,///< not supported yet
    None,    ///< not supported yet
    GFX_RENDER_API_PLACEHOLDER
};

enum RenderAPIVersion{
    OpenGL1x,  ///< support dropped
    OpenGL2x,  ///< support dropped
    OpenGL3x,
    OpenGL4x,  ///< not supported yet
    Direct3D8, ///< support dropped
    Direct3D9, ///< support dropped
    Direct3D10,///< not supported yet
    Direct3D11,///< not supported yet
    GFX_RENDER_API_VER_PLACEHOLDER
};

enum PrimitiveType {
    API_POINTS      = 0x0000,
    LINES           = 0x0001,
    LINE_LOOP       = 0x0002,
    LINE_STRIP      = 0x0003,
    TRIANGLES       = 0x0004,
    TRIANGLE_STRIP  = 0x0005,
    TRIANGLE_FAN    = 0x0006,
    QUADS           = 0x0007,
    QUAD_STRIP      = 0x0008,
    POLYGON         = 0x0009,
    PrimitiveType_PLACEHOLDER = 0x0010
};

enum RenderDetailLevel{
    DETAIL_LOW = 0,
    DETAIL_MEDIUM = 1,
    DETAIL_HIGH = 2
};

enum FullScreenAntiAliasingMethod{
    FS_MSAA = 1,
    FS_FXAA = 2,
    FS_SMAA = 3,
    FS_MSwFXAA = 4 ///<MSAA rendering + FXAA post processing if possible
};

/// Specifies how the red, green, blue, and alpha source blending factors are computed.
enum BlendProperty{
   BLEND_PROPERTY_ZERO = 0,
   BLEND_PROPERTY_ONE,
   BLEND_PROPERTY_SRC_COLOR,
   BLEND_PROPERTY_INV_SRC_COLOR,
   /// Transparency is best implemented using blend function (SRC_ALPHA, ONE_MINUS_SRC_ALPHA) with primitives sorted from farthest to nearest.
   BLEND_PROPERTY_SRC_ALPHA,
   BLEND_PROPERTY_INV_SRC_ALPHA,
   BLEND_PROPERTY_DEST_ALPHA,
   BLEND_PROPERTY_INV_DEST_ALPHA,
   BLEND_PROPERTY_DEST_COLOR,
   BLEND_PROPERTY_INV_DEST_COLOR,
   /// Polygon antialiasing is optimized using blend function (SRC_ALPHA_SATURATE, GL_ONE) with polygons sorted from nearest to farthest.
   BLEND_PROPERTY_SRC_ALPHA_SAT,
   ///Place all properties above this.
   BlendProperty_PLACEHOLDER
};

/// Specifies how source and destination colors are combined.
enum BlendOperation {
   /// The ADD equation is useful for antialiasing and transparency, among other things.
   BLEND_OPERATION_ADD = 0,
   BLEND_OPERATION_SUBTRACT,
   BLEND_OPERATION_REV_SUBTRACT,
   /// The MIN and MAX equations are useful for applications that analyze image data
   /// (image thresholding against a constant color, for example).
   BLEND_OPERATION_MIN,
   /// The MIN and MAX equations are useful for applications that analyze image data
   /// (image thresholding against a constant color, for example).
   BLEND_OPERATION_MAX,
   /// Place all properties above this.
   BlendOperation_PLACEHOLDER
};

/// Valid comparison functions for most states
/// YYY = test value using this function
enum ComparisonFunction {
   /// Never passes.
   CMP_FUNC_NEVER = 0,
   /// Passes if the incoming YYY value is less than the stored YYY value.
   CMP_FUNC_LESS,
   /// Passes if the incoming YYY value is equal to the stored YYY value.
   CMP_FUNC_EQUAL,
   /// Passes if the incoming YYY value is less than or equal to the stored YYY value.
   CMP_FUNC_LEQUAL,
   /// Passes if the incoming YYY value is greater than the stored YYY value.
   CMP_FUNC_GREATER,
   /// Passes if the incoming YYY value is not equal to the stored YYY value.
   CMP_FUNC_NEQUAL,
   /// Passes if the incoming YYY value is greater than or equal to the stored YYY value.
   CMP_FUNC_GEQUAL,
   /// Always passes.
   CMP_FUNC_ALWAYS,
   /// Place all properties above this.
   ComparisonFunction_PLACEHOLDER
};

/// Specifies whether front- or back-facing facets are candidates for culling.
enum CullMode {
   CULL_MODE_NONE = 0,
   /// Cull Back facing polygons
   CULL_MODE_CW,
   /// Cull Front facing polygons
   CULL_MODE_CCW,
   /// Cull All polygons
   CULL_MODE_ALL,
    ///Place all properties above this.
   CullMode_PLACEHOLDER
};

/// Valid front and back stencil test actions
enum StencilOperation {
   /// Keeps the current value.
   STENCIL_OPERATION_KEEP = 0,
   /// Sets the stencil buffer value to 0.
   STENCIL_OPERATION_ZERO,
   /// Sets the stencil buffer value to ref, as specified by StencilFunc.
   STENCIL_OPERATION_REPLACE,
   /// Increments the current stencil buffer value. Clamps to the maximum representable unsigned value.
   STENCIL_OPERATION_INCR,
   ///  Decrements the current stencil buffer value. Clamps to 0.
   STENCIL_OPERATION_DECR,
   /// Bitwise inverts the current stencil buffer value.
   STENCIL_OPERATION_INV,
   /// Increments the current stencil buffer value.
   /// Wraps stencil buffer value to zero when incrementing the maximum representable unsigned value.
   STENCIL_OPERATION_INCR_WRAP,
   /// Decrements the current stencil buffer value.
   /// Wraps stencil buffer value to the maximum representable unsigned value when decrementing a stencil buffer value of zero.
   STENCIL_OPERATION_DECR_WRAP,
   /// Place all properties above this.
   StencilOperation_PLACEHOLDER
};

///Defines all available fill modes for primitives
enum FillMode {
   ///Polygon vertices that are marked as the start of a boundary edge are drawn as points.
   FILL_MODE_POINT = 1,
   ///Boundary edges of the polygon are drawn as line segments.
   FILL_MODE_WIREFRAME,
   ///The interior of the polygon is filled.
   FILL_MODE_SOLID,
    ///Place all properties above this.
   FillMode_PLACEHOLDER
};

enum TextureType {
    TEXTURE_1D = 0,
    TEXTURE_2D,
    TEXTURE_3D,
    TEXTURE_CUBE_MAP,
    TEXTURE_2D_ARRAY,
    TEXTURE_CUBE_ARRAY,
    TEXTURE_2D_MS,
    TEXTURE_2D_ARRAY_MS,
    TextureType_PLACEHOLDER
};

enum TextureFilter{
    TEXTURE_FILTER_LINEAR	= 0x0000,
    TEXTURE_FILTER_NEAREST   = 0x0001,
    TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST = 0x0002,
    TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST  = 0x0003,
    TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR  = 0x0004,
    TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR   = 0x0005,
    TextureFilter_PLACEHOLDER            = 0x0006
};

enum TextureWrap {
    /** Texture coordinates outside [0...1] are clamped to the nearest valid value.	 */
    TEXTURE_CLAMP = 0x0,
    TEXTURE_CLAMP_TO_EDGE = 0x1,
    TEXTURE_CLAMP_TO_BORDER = 0x2,
    /** If the texture coordinates for a pixel are outside [0...1] the texture is not applied to that pixel */
    TEXTURE_DECAL = 0x3,
    TEXTURE_REPEAT = 0x4,
    TextureWrap_PLACEHOLDER = 0x5
};

enum GFXImageFormat{
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
    RGB8I,
    RGB16,
    RGB16F,
    BGRA,
    RGBA,
    RGBA4,
    RGBA8,
    RGBA8I,
    RGBA16F,
    RGBA32F,
    DEPTH_COMPONENT,
    DEPTH_COMPONENT16,
    DEPTH_COMPONENT24,
    DEPTH_COMPONENT32,
    IMAGE_FORMAT_PLACEHOLDER
};

enum GFXDataFormat{
    UNSIGNED_BYTE    = 0x0000,
    UNSIGNED_SHORT   = 0x0001,
    UNSIGNED_INT     = 0x0002,
    SIGNED_BYTE      = 0x0003,
    SIGNED_SHORT     = 0x0004,
    SIGNED_INT       = 0x0005,
    FLOAT_16         = 0x0006,
    FLOAT_32         = 0x0008,
    GDF_PLACEHOLDER
};

enum GPUVendor {
    GPU_VENDOR_NVIDIA = 0,
    GPU_VENDOR_AMD,
    GPU_VENDOR_INTEL,
    GPU_VENDOR_OTHER,
    GPU_VENDOR_PLACEHOLDER
};

enum GPURenderer{
    GPU_RENDERER_PLACEHOLDER
};
#endif