#include "Headers/glResources.h"
#include "Headers/glEnumTable.h"
 
GLenum glBlendTable[BLEND_PROPERTY_PLACEHOLDER];
GLenum glBlendOpTable[BLEND_OPERATION_PLACEHOLDER];
GLenum glCompareFuncTable[COMPARE_FUNC_PLACEHOLDER];
GLenum glStencilOpTable[STENCIL_OPERATION_PLACEHOLDER];
GLenum glCullModeTable[CULL_MODE_PLACEHOLDER];
GLenum glFillModeTable[FILL_MODE_PLACEHOLDER];
GLenum glTextureTypeTable[TEXTURE_TYPE_PLACEHOLDER];

namespace GL_ENUM_TABLE {
	void fill(){

	   glBlendTable[BLEND_PROPERTY_Zero] = GL_ZERO;
	   glBlendTable[BLEND_PROPERTY_One] = GL_ONE;
	   glBlendTable[BLEND_PROPERTY_SrcColor] = GL_SRC_COLOR;
	   glBlendTable[BLEND_PROPERTY_InvSrcColor] = GL_ONE_MINUS_SRC_COLOR;
	   glBlendTable[BLEND_PROPERTY_SrcAlpha] = GL_SRC_ALPHA;
	   glBlendTable[BLEND_PROPERTY_InvSrcAlpha] = GL_ONE_MINUS_SRC_ALPHA;
	   glBlendTable[BLEND_PROPERTY_DestAlpha] = GL_DST_ALPHA;
	   glBlendTable[BLEND_PROPERTY_InvDestAlpha] = GL_ONE_MINUS_DST_ALPHA;
	   glBlendTable[BLEND_PROPERTY_DestColor] = GL_DST_COLOR;
	   glBlendTable[BLEND_PROPERTY_InvDestColor] = GL_ONE_MINUS_DST_COLOR;
	   glBlendTable[BLEND_PROPERTY_SrcAlphaSat] = GL_SRC_ALPHA_SATURATE;

	   glBlendOpTable[BLEND_OPERATION_Add] = GL_FUNC_ADD;
	   glBlendOpTable[BLEND_OPERATION_Subtract] = GL_FUNC_SUBTRACT;
	   glBlendOpTable[BLEND_OPERATION_RevSubtract] = GL_FUNC_REVERSE_SUBTRACT;
	   glBlendOpTable[BLEND_OPERATION_Min] = GL_MIN;
	   glBlendOpTable[BLEND_OPERATION_Max] = GL_MAX;

	   glCompareFuncTable[COMPARE_FUNC_Never] = GL_NEVER;
	   glCompareFuncTable[COMPARE_FUNC_Less] = GL_LESS;
	   glCompareFuncTable[COMPARE_FUNC_Equal] = GL_EQUAL;
	   glCompareFuncTable[COMPARE_FUNC_LessEqual] = GL_LEQUAL;
	   glCompareFuncTable[COMPARE_FUNC_Greater] = GL_GREATER;
	   glCompareFuncTable[COMPARE_FUNC_NotEqual] = GL_NOTEQUAL;
	   glCompareFuncTable[COMPARE_FUNC_GreaterEqual] = GL_GEQUAL;
	   glCompareFuncTable[COMPARE_FUNC_Always] = GL_ALWAYS;
	   
	   glStencilOpTable[STENCIL_OPERATION_Keep] = GL_KEEP;
	   glStencilOpTable[STENCIL_OPERATION_Zero] = GL_ZERO;
	   glStencilOpTable[STENCIL_OPERATION_Replace] = GL_REPLACE;
	   glStencilOpTable[STENCIL_OPERATION_Incr] = GL_INCR;
	   glStencilOpTable[STENCIL_OPERATION_Decr] = GL_DECR;
	   glStencilOpTable[STENCIL_OPERATION_Invert] = GL_INVERT;
	   glStencilOpTable[STENCIL_OPERATION_IncrWrap] = GL_INCR_WRAP;
	   glStencilOpTable[STENCIL_OPERATION_DecrWrap] = GL_DECR_WRAP;

	   glCullModeTable[CULL_MODE_None] = GL_BACK;
	   glCullModeTable[CULL_MODE_CW] = GL_BACK;
	   glCullModeTable[CULL_MODE_CCW] = GL_FRONT;
	   glCullModeTable[CULL_MODE_ALL] = GL_FRONT_AND_BACK;

	   glFillModeTable[FILL_MODE_Point] = GL_POINT;
	   glFillModeTable[FILL_MODE_Wireframe] = GL_LINE;
	   glFillModeTable[FILL_MODE_Solid] = GL_FILL;

	   glTextureTypeTable[TEXTURE_1D] = GL_TEXTURE_1D;
	   glTextureTypeTable[TEXTURE_2D] = GL_TEXTURE_2D;
	   glTextureTypeTable[TEXTURE_3D] = GL_TEXTURE_3D;
	   glTextureTypeTable[TEXTURE_CUBE_MAP] = GL_TEXTURE_CUBE_MAP;
	}
}
