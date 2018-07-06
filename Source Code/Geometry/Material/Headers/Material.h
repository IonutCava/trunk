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

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "core.h"
#include "Core/Resources/Headers/Resource.h"


class Texture;
class ShaderProgram;
class RenderStateBlock;
typedef Texture Texture2D;
struct RenderStateBlockDescriptor;
enum RENDER_STAGE;

class Material : public Resource{

public:
  enum TextureUsage{
	  TEXTURE_BASE = 0,
	  TEXTURE_BUMP = 1,
	  TEXTURE_NORMALMAP = 2,
	  TEXTURE_SECOND = 3,
	  TEXTURE_OPACITY = 4,
	  TEXTURE_SPECULAR = 5,
	  TEXTURE_DISPLACEMENT = 6,
	  TEXTURE_EMISSIVE = 7,
	  TEXTURE_AMBIENT = 8,
	  TEXTURE_SHININESS = 9,
	  TEXTURE_MIRROR = 10,
	  TEXTURE_LIGHTMAP = 11
  };
  /// How should each texture be added
  enum TextureOperation {
	TextureOperation_Multiply    = 0x0,
	TextureOperation_Add         = 0x1,
	TextureOperation_Subtract    = 0x2,
	TextureOperation_Divide      = 0x3,
	TextureOperation_SmoothAdd   = 0x4,
	TextureOperation_SignedAdd   = 0x5,
	TextureOperation_Combine     = 0x6,
	TextureOperation_Decal       = 0x7,
	TextureOperation_Blend       = 0x8,
	TextureOperation_Replace     = 0x9,
	TextureOperation_PLACEHOLDER = 0x10
  };

  /// Not used yet but implemented for shading model selection in shaders
  /// This enum matches the ASSIMP one on a 1-to-1 basis
  enum ShadingMode {

	SHADING_FLAT          = 0x1,
    SHADING_GOURAUD       = 0x2,
	SHADING_PHONG         = 0x3,
	SHADING_BLINN         = 0x4,
	SHADING_TOON          = 0x5,
    SHADING_OREN_NAYAR    = 0x6,
	SHADING_MINNAERT      = 0x7,
    SHADING_COOK_TORRANCE = 0x8,
	SHADING_NONE          = 0x9,
	SHADING_FRESNEL       = 0xa
};

typedef unordered_map<TextureUsage, Texture2D*> textureMap;
typedef unordered_map<TextureUsage, TextureOperation> textureOperation;

public:
  Material();
  ~Material();

  bool load(const std::string& name) {_name = name; return true;}
  bool unload();

  inline void setHardwareSkinning(bool state)    {_hardwareSkinning = state;}
  inline void setAmbient(const vec4<F32>& value) {_ambient = value; _materialMatrix.setCol(0,value);}
  inline void setDiffuse(const vec4<F32>& value) {_diffuse = value; _materialMatrix.setCol(1,value);}
  inline void setSpecular(const vec4<F32>& value) {_specular = value; _materialMatrix.setCol(2,value);}
  inline void setEmissive(const vec3<F32>& value) {_emissive = value; _materialMatrix.setCol(3,vec4<F32>(_shininess,value.x,value.y,value.z));}
  inline void setShininess(F32 value) {_shininess = value; _materialMatrix.setCol(3,vec4<F32>(value,_emissive.x,_emissive.y,_emissive.z));}
  inline void setOpacityValue(F32 value) {_opacity = value;}
  inline void setCastsShadows(bool state) {_castsShadows = state;}
  inline void setReceivesShadows(bool state) {_receiveShadows = state;}
  inline void setShadingMode(ShadingMode mode) {_shadingMode = mode;}
  		 void setDoubleSided(bool state);
		 void setTexture(TextureUsage textureUsage, Texture2D* const texture, TextureOperation op = TextureOperation_Replace);

  ShaderProgram*    setShaderProgram(const std::string& shader);
  RenderStateBlock* setRenderStateBlock(const RenderStateBlockDescriptor& descriptor, RENDER_STAGE renderStage);

  inline mat4<F32>& getMaterialMatrix()  {return _materialMatrix;}

         P32   getMaterialId();
  inline bool  getCastsShadows()    {return _castsShadows;}
  inline bool  getReceivesShadows() {return _receiveShadows;}
  inline F32   getOpacityValue()    {return _opacity;}
  inline U8    getTextureCount()    {return _textures.size();}

  inline ShadingMode             getShadingMode()                          {return _shadingMode;}
  inline RenderStateBlock*       getRenderState(RENDER_STAGE currentStage) {return _defaultRenderStates[currentStage];}
  inline U32               const getTextureOperation(TextureUsage textureUsage) {return _textureOperationTable[_operations[textureUsage]];}
         Texture2D*	       const getTexture(TextureUsage textureUsage);
         ShaderProgram*    const getShaderProgram();

  inline bool isDirty()       {return _dirty;}
  inline bool isDoubleSided() {return _doubleSided;}
         bool isTranslucent();

  inline bool shaderProgramChanged() {
	  if(_shaderProgramChanged) {
		  _shaderProgramChanged = false;
		  return true;
	  }else{
		  return false;
	  }
  }


  TextureOperation getTextureOperation(U32 op);

  void computeLightShaders(); //Set shaders;
  void createCopy();
  void removeCopy();
  void dumpToXML();

private:
  vec4<F32> _diffuse;           /* diffuse component */
  vec4<F32> _ambient;           /* ambient component */
  vec4<F32> _specular;          /* specular component */
  vec3<F32> _emissive;          /* emissive component */
  F32 _shininess;          /* specular exponent */
  F32 _opacity;			   /* material opacity value*/
  mat4<F32> _materialMatrix; /* all properties bundled togheter */
  ShadingMode _shadingMode;
  std::string _shader; /*shader name*/
  bool _computedLightShaders;
  P32  _matId;
  bool _dirty;
  bool _doubleSided;
  bool _castsShadows;
  bool _receiveShadows;
  bool _shaderProgramChanged; ///< Used for tracking VBO binding with shader
  bool _hardwareSkinning;     ///< Use shaders that have bone transforms implemented
  ShaderProgram* _shaderRef;

  /// use this map to add more render states mapped to a specific state
  /// 3 render state's: Normal, reflection and shadow
  unordered_map<RENDER_STAGE, RenderStateBlock* > _defaultRenderStates;
  /// use this map to add textures to the material
  textureMap _textures;
  /// use the bellow map to define texture operation
  textureOperation _operations;
  /// Map texture operation to values the shader understands
  U32 _textureOperationTable[TextureOperation_PLACEHOLDER];

  boost::mutex _materialMutex;
};

#endif