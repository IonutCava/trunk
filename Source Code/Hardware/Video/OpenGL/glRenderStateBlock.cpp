#include "Headers/glRenderStateBlock.h"
#include "Headers/glResources.h"
#include "Headers/glEnumTable.h"
#include "Core/Headers/Console.h"

#define SHOULD_TOGGLE(state) (!oldState || oldState->_descriptor.state != _descriptor.state)

#define TOGGLE_NO_CHECK(state, enumValue) if(_descriptor.state) GLCheck(glEnable(enumValue));  \
									      else GLCheck(glDisable(enumValue));

#define TOGGLE_WITH_CHECK(state, enumValue) if(!oldState || oldState->_descriptor.state != _descriptor.state) { \
											    if(_descriptor.state) GLCheck(glEnable(enumValue));  \
											    else GLCheck(glDisable(enumValue)); \
											}
glRenderStateBlock::glRenderStateBlock(const RenderStateBlockDescriptor& descriptor) : RenderStateBlock(),
   _descriptor(descriptor),
   _hashValue(descriptor.getGUID())
{
}

glRenderStateBlock::~glRenderStateBlock(){
}

void glRenderStateBlock::activate(glRenderStateBlock* oldState){
   TOGGLE_WITH_CHECK(_blendEnable, GL_BLEND);
   TOGGLE_WITH_CHECK(_zEnable, GL_DEPTH_TEST);
   TOGGLE_WITH_CHECK(_stencilEnable, GL_STENCIL_TEST);
   TOGGLE_WITH_CHECK(_vertexColorEnable, GL_COLOR_MATERIAL);
   TOGGLE_WITH_CHECK(_cullMode, GL_CULL_FACE);

   if(SHOULD_TOGGLE(_blendSrc) || SHOULD_TOGGLE(_blendDest)){
      GLCheck(glBlendFunc(glBlendTable[_descriptor._blendSrc], glBlendTable[_descriptor._blendDest]));
   }

   if(SHOULD_TOGGLE(_blendOp)){
      GLCheck(glBlendEquation(glBlendOpTable[_descriptor._blendOp]));
   }

   if(SHOULD_TOGGLE(_writeRedChannel) ||
	  SHOULD_TOGGLE(_writeBlueChannel) ||
	  SHOULD_TOGGLE(_writeGreenChannel) ||
	  SHOULD_TOGGLE(_writeAlphaChannel)) {
		GLCheck(glColorMask(_descriptor._writeRedChannel,
							_descriptor._writeBlueChannel,
							_descriptor._writeGreenChannel,
							_descriptor._writeAlphaChannel));
   }

   if(SHOULD_TOGGLE(_cullMode)) {
      GLCheck(glCullFace(glCullModeTable[_descriptor._cullMode]));
   }

   if(SHOULD_TOGGLE(_zFunc)){
      GLCheck(glDepthFunc(glCompareFuncTable[_descriptor._zFunc]));
   }

   if(SHOULD_TOGGLE(_zBias)){
      if (_descriptor._zBias == 0){
         GLCheck(glDisable(GL_POLYGON_OFFSET_FILL));
      } else {
         GLfloat units  = _descriptor._zUnits;
		 GLfloat factor = _descriptor._zBias;
         GLCheck(glEnable(GL_POLYGON_OFFSET_FILL));
         GLCheck(glPolygonOffset( factor, units));
      }
   }

   if(SHOULD_TOGGLE(_zWriteEnable)){
      GLCheck(glDepthMask(_descriptor._zWriteEnable));
   }

   if(SHOULD_TOGGLE(_stencilFunc) ||
	  SHOULD_TOGGLE(_stencilRef)  ||
	  SHOULD_TOGGLE(_stencilMask)) {
      GLCheck(glStencilFunc(glCompareFuncTable[_descriptor._stencilFunc],
		            _descriptor._stencilRef,
					_descriptor._stencilMask));
   }

   if(SHOULD_TOGGLE(_stencilFailOp)  ||
	  SHOULD_TOGGLE(_stencilZFailOp) ||
	  SHOULD_TOGGLE(_stencilPassOp)) {
      GLCheck(glStencilOp(glStencilOpTable[_descriptor._stencilFailOp],
		                  glStencilOpTable[_descriptor._stencilZFailOp],
						  glStencilOpTable[_descriptor._stencilPassOp]));
   }

   if(SHOULD_TOGGLE(_stencilWriteMask)){
      GLCheck(glStencilMask(_descriptor._stencilWriteMask));
   }

   if(SHOULD_TOGGLE(_fillMode)){
      GLCheck(glPolygonMode(GL_FRONT_AND_BACK, glFillModeTable[_descriptor._fillMode]));
   }
}