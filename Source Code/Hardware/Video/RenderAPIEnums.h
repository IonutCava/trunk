/*“Copyright 2009-2012 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GFX_ENUMS_H
#define _GFX_ENUMS_H

/// no need to include the entire resource header for one define
#ifndef toBit
#define toBit(X) (1 << (X))
#endif 

enum RENDER_STAGE {
    DEFERRED_STAGE			   = toBit(1),
    SHADOW_STAGE			   = toBit(2),
    REFLECTION_STAGE		   = toBit(3),
    SSAO_STAGE				   = toBit(4),
    BLOOM_STAGE                = toBit(5),
    FINAL_STAGE				   = toBit(6),
    DEPTH_STAGE				   = toBit(7),
	ENVIRONMENT_MAPPING_STAGE  = toBit(8),
	//Place all stages above this
	INVALID_STAGE		       = toBit(10)
};

enum FBO_TYPE {
	FBO_PLACEHOLDER, 
	FBO_2D_COLOR,
	FBO_CUBE_COLOR,
	FBO_2D_DEPTH, 
	FBO_2D_DEFERRED
};

enum TEXTURE_FORMAT {
	RGB, 
	RGBA, 
	BGRA, 
	LUMINANCE, 
	LUMINANCE_ALPHA
};

enum TEXTURE_FORMAT_INTERNAL {
	RGBA32F,
	RGBA16F,
	RGBA8I,
	RGBA8, 
	RGB16F, 
	RGB8I,
	RGB16,
	RGB8
};

enum RENDER_API {
	OpenGL,
	OpenGLES,
	Direct3D,///< not supported yet
	Software,///< not supported yet
	None,    ///< not supported yet
	GFX_RENDER_API_PLACEHOLDER
};

enum RENDER_API_VERSION{
	OpenGL1x,  ///< not supported yet
	OpenGL2x,
	OpenGL3x,
	OpenGL4x,  ///< not supported yet
	Direct3D8, ///< not supported yet
	Direct3D9, ///< not supported yet
	Direct3D10,///< not supported yet
	Direct3D11,///< not supported yet
	GFX_RENDER_API_VER_PLACEHOLDER
};

enum PRIMITIVE_TYPE {

	API_POINTS      = 0x0000,
	LINES           = 0x0001,
	LINE_LOOP       = 0x0002,
	LINE_STRIP      = 0x0003,
	TRIANGLES       = 0x0004,
	TRIANGLE_STRIP  = 0x0005,
	TRIANGLE_FAN    = 0x0006,
	QUADS           = 0x0007,
	QUAD_STRIP      = 0x0008,
	POLYGON         = 0x0009
	
	
};

enum VERTEX_DATA_FORMAT{

	_U8              = 0x0000,
	_U16             = 0x0001,
	_U32             = 0x0002,
	_I8              = 0x0003,
	_I16             = 0x0004,
	_I32             = 0x0005
};

enum RENDER_DETAIL_LEVEL{

	HIGH = 2,
	MEDIUM = 1,
	LOW = 0
};
      
/// Specifies how the red, green, blue, and alpha source blending factors are computed.
enum BLEND_PROPERTY{
   BLEND_PROPERTY_Zero = 0,
   BLEND_PROPERTY_One,
   BLEND_PROPERTY_SrcColor,
   BLEND_PROPERTY_InvSrcColor,
   /// Transparency is best implemented using blend function (SRC_ALPHA, ONE_MINUS_SRC_ALPHA) with primitives sorted from farthest to nearest.
   BLEND_PROPERTY_SrcAlpha,
   BLEND_PROPERTY_InvSrcAlpha,
   BLEND_PROPERTY_DestAlpha,
   BLEND_PROPERTY_InvDestAlpha,
   BLEND_PROPERTY_DestColor,
   BLEND_PROPERTY_InvDestColor,
   /// Polygon antialiasing is optimized using blend function (SRC_ALPHA_SATURATE, GL_ONE) with polygons sorted from nearest to farthest.
   BLEND_PROPERTY_SrcAlphaSat,
   ///Place all properties above this.
   BLEND_PROPERTY_PLACEHOLDER
};

/// Specifies how source and destination colors are combined.
enum BLEND_OPERATION {
   /// The ADD equation is useful for antialiasing and transparency, among other things.
   BLEND_OPERATION_Add = 0,
   BLEND_OPERATION_Subtract,
   BLEND_OPERATION_RevSubtract,
   /// The MIN and MAX equations are useful for applications that analyze image data
   /// (image thresholding against a constant color, for example).
   BLEND_OPERATION_Min,
   /// The MIN and MAX equations are useful for applications that analyze image data
   /// (image thresholding against a constant color, for example).
   BLEND_OPERATION_Max,
   /// Place all properties above this.
   BLEND_OPERATION_PLACEHOLDER
};

/// Valid comparison functions for most states
/// YYY = test value using this function
enum COMPARE_FUNC {
   /// Never passes.
   COMPARE_FUNC_Never = 0,
   /// Passes if the incoming YYY value is less than the stored YYY value.
   COMPARE_FUNC_Less,
   /// Passes if the incoming YYY value is equal to the stored YYY value.
   COMPARE_FUNC_Equal,
   /// Passes if the incoming YYY value is less than or equal to the stored YYY value.
   COMPARE_FUNC_LessEqual,
   /// Passes if the incoming YYY value is greater than the stored YYY value.
   COMPARE_FUNC_Greater,
   /// Passes if the incoming YYY value is not equal to the stored YYY value.
   COMPARE_FUNC_NotEqual,
   /// Passes if the incoming YYY value is greater than or equal to the stored YYY value.
   COMPARE_FUNC_GreaterEqual,
   /// Always passes.
   COMPARE_FUNC_Always,
   /// Place all properties above this.
   COMPARE_FUNC_PLACEHOLDER
};


/// Specifies whether front- or back-facing facets are candidates for culling.
enum CULL_MODE {
   CULL_MODE_None = 0,
   /// Cull Back facing polygons
   CULL_MODE_CW,
   /// Cull Front facing polygons
   CULL_MODE_CCW,
   /// Cull All polygons
   CULL_MODE_ALL,
    ///Place all properties above this.
   CULL_MODE_PLACEHOLDER
};
                        
/// Valid front and back stencil test actions                   
enum STENCIL_OPERATION {
   /// Keeps the current value.
   STENCIL_OPERATION_Keep = 0,
   /// Sets the stencil buffer value to 0.
   STENCIL_OPERATION_Zero,
   /// Sets the stencil buffer value to ref, as specified by StencilFunc.
   STENCIL_OPERATION_Replace,
   /// Increments the current stencil buffer value. Clamps to the maximum representable unsigned value.
   STENCIL_OPERATION_Incr,
   ///  Decrements the current stencil buffer value. Clamps to 0.
   STENCIL_OPERATION_Decr,
   /// Bitwise inverts the current stencil buffer value.
   STENCIL_OPERATION_Invert,
   /// Increments the current stencil buffer value.
   /// Wraps stencil buffer value to zero when incrementing the maximum representable unsigned value.
   STENCIL_OPERATION_IncrWrap,
   /// Decrements the current stencil buffer value.
   /// Wraps stencil buffer value to the maximum representable unsigned value when decrementing a stencil buffer value of zero.
   STENCIL_OPERATION_DecrWrap,
   /// Place all properties above this.
   STENCIL_OPERATION_PLACEHOLDER
};

///Defines all available fill modes for primitives
enum FILL_MODE {
   ///Polygon vertices that are marked as the start of a boundary edge are drawn as points. 
   FILL_MODE_Point = 1,
   ///Boundary edges of the polygon are drawn as line segments.
   FILL_MODE_Wireframe,
   ///The interior of the polygon is filled.
   FILL_MODE_Solid,
    ///Place all properties above this.
   FILL_MODE_PLACEHOLDER
};

enum TEXTURE_TYPE {
	TEXTURE_1D = 0,
	TEXTURE_2D,
    TEXTURE_3D,
    TEXTURE_CUBE_MAP,
	TEXTURE_TYPE_PLACEHOLDER
};

#endif