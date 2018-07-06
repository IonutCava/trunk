#include "glRenderStateBlock.h"
#include "glResources.h"
#include "glEnumTable.h"
#include "Core\Headers\Console.h"

void GLCheckError(const std::string& File, unsigned int Line, char* operation);

#define SHOULD_TOGGLE(state) (!oldState || oldState->_descriptor.state != _descriptor.state)

#define TOGGLE_NO_CHECK(state, enum) if(_descriptor.state) { \
										GLCheck(glEnable(enum));  \
									} else { \
										GLCheck(glDisable(enum)); \
									}

#define TOGGLE_WITH_CHECK(state, enum) if(!oldState || oldState->_descriptor.state != _descriptor.state) { \
											if(_descriptor.state) { \
												glEnable(enum);  \
											} else { \
												glDisable(enum); \
											} \
									   }

glRenderStateBlock::glRenderStateBlock(const RenderStateBlockDescriptor& descriptor) :
   _descriptor(descriptor),
   _hashValue(descriptor.getHash()){
}

glRenderStateBlock::~glRenderStateBlock(){
}

void glRenderStateBlock::activate(glRenderStateBlock* oldState){

   TOGGLE_WITH_CHECK(_blendEnable, GL_BLEND);
   TOGGLE_WITH_CHECK(_zEnable, GL_DEPTH_TEST);
   TOGGLE_WITH_CHECK(_fixedLighting, GL_LIGHTING);
   TOGGLE_WITH_CHECK(_stencilEnable, GL_STENCIL_TEST);
   TOGGLE_WITH_CHECK(_alphaTestEnable, GL_ALPHA_TEST);
   TOGGLE_WITH_CHECK(_vertexColorEnable, GL_COLOR_MATERIAL);

   if(SHOULD_TOGGLE(_blendSrc) || SHOULD_TOGGLE(_blendDest)){
      GLCheck(glBlendFunc(glBlendTable[_descriptor._blendSrc], glBlendTable[_descriptor._blendDest]));
   }

   if(SHOULD_TOGGLE(_blendOp)){
      GLCheck(glBlendEquation(glBlendOpTable[_descriptor._blendOp]));
   }

   if(SHOULD_TOGGLE(_alphaTestFunc) || SHOULD_TOGGLE(_alphaTestRef)){
      GLCheck(glAlphaFunc(glCompareFuncTable[_descriptor._alphaTestFunc], 
		                   (F32) _descriptor._alphaTestRef * 1.0f/255.0f));
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
      TOGGLE_NO_CHECK(_cullMode, GL_CULL_FACE);
      GLCheck(glCullFace(glCullModeTable[_descriptor._cullMode]));
   }

   if(SHOULD_TOGGLE(_zFunc)){
     GLCheck(glDepthFunc(glCompareFuncTable[_descriptor._zFunc]));
   }

   if(SHOULD_TOGGLE(_zBias)){
      if (_descriptor._zBias == 0){
         GLCheck(glDisable(GL_POLYGON_OFFSET_FILL));
      } else {
         F32 bias = _descriptor._zBias * 10000.0f;
         GLCheck(glEnable(GL_POLYGON_OFFSET_FILL));
         GLCheck(glPolygonOffset(bias, bias));
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

   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

   if(SHOULD_TOGGLE(_fillMode)){
      GLCheck(glPolygonMode(GL_FRONT_AND_BACK, glFillModeTable[_descriptor._fillMode]));
   }
}
