/*“Copyright 2009-2013 DIVIDE-Studio”*/
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
#include "Utility/Headers/XMLParser.h"
#include "Core/Resources/Headers/Resource.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"

class Texture;
class ShaderProgram;
class RenderStateBlock;
typedef Texture Texture2D;
struct RenderStateBlockDescriptor;
enum RenderStage;
enum BlendProperty;
class Material : public Resource{

public:
    struct ShaderData{
        vec4<F32> _diffuse;  /* diffuse component */
        vec4<F32> _ambient;  /* ambient component */
        vec4<F32> _specular; /* specular component*/
        vec4<F32> _emissive; /* emissive component*/
        F32 _shininess;      /* specular exponent */
        F32 _opacity;        /* material opacity value*/    
        I32 _textureCount;

        ShaderData() : _textureCount(0),
                       _ambient(vec4<F32>(vec3<F32>(1.0f,1.0f,1.0f)/1.5f,1)),
                       _diffuse(vec4<F32>(vec3<F32>(1.0f,1.0f,1.0f)/1.5f,1)),
                       _specular(1.0f,1.0f,1.0f,1.0f),
                       _emissive(0.6f,0.6f,0.6f,1.0f),
                       _shininess(5),
                       _opacity(1.0f) {}
    };

  enum BumpMethod{
	  BUMP_NONE = 0,   //<Use phong
	  BUMP_NORMAL = 1, //<Normal mapping
	  BUMP_PARALLAX = 2,
	  BUMP_RELIEF = 3,
	  BUMP_PLACEHOLDER = 4
  };

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

typedef Unordered_map<TextureUsage, Texture2D*> textureMap;
typedef Unordered_map<TextureUsage, TextureOperation> textureOperation;

public:
  Material();
  ~Material();

  bool unload();

  inline void setHardwareSkinning(bool state)     {_hardwareSkinning = state;}
  inline void setAmbient(const vec4<F32>& value)  {_shaderData._ambient = value; _materialMatrix.setCol(0,value);}
  inline void setDiffuse(const vec4<F32>& value)  {_shaderData._diffuse = value; _materialMatrix.setCol(1,value);}
  inline void setSpecular(const vec4<F32>& value) {_shaderData._specular = value; _materialMatrix.setCol(2,value);}
  inline void setEmissive(const vec3<F32>& value) {_shaderData._emissive = value; _materialMatrix.setCol(3,vec4<F32>(_shaderData._shininess,value.x,value.y,value.z));}

  inline void setShininess(F32 value) {
      _shaderData._shininess = value;
      _materialMatrix.setCol(3,vec4<F32>(value,
                                         _shaderData._emissive.x,
                                         _shaderData._emissive.y,
                                         _shaderData._emissive.z));
  }

  inline void setOpacityValue(F32 value) {_shaderData._opacity = value;}
  void setCastsShadows(bool state);
  inline void setReceivesShadows(bool state) {_receiveShadows = state;}
  inline void setShadingMode(ShadingMode mode) {_shadingMode = mode;}
  		 void setDoubleSided(bool state);
		 void setTexture(TextureUsage textureUsage, Texture2D* const texture, TextureOperation op = TextureOperation_Replace);
		 ///Set the desired bump mapping method. If force == true, the shader is updated immediately
		 void setBumpMethod(BumpMethod newBumpMethod,bool force = false);
		 void setBumpMethod(U32 newBumpMethod,bool force = false);
		 ///Shader modifiers add tokens to the end of the shader name.
		 ///Add as many tokens as needed but separate them with a ".". i.e: "Tree.NoWind.Glow"
		 void addShaderModifier(const std::string& shaderModifier,bool force = false);
		 ///Shader defines, separated by commas, are added to the generated shader
		 ///The shader generator appends "#define " to the start of each define
		 ///For example, to define max light count and max shadow casters add this string:
		 ///"MAX_LIGHT_COUNT 4, MAX_SHADOW_CASTERS 2"
		 ///The above strings becomes, in the shader:
		 ///#define MAX_LIGHT COUNT 4
		 ///#define MAX_SHADOW_CASTERS 2
		 void addShaderDefines(const std::string& shaderDefines,bool force = false);
		 ShaderProgram*    setShaderProgram(const std::string& shader,RenderStage renderStage = FINAL_STAGE);
	     RenderStateBlock* setRenderStateBlock(const RenderStateBlockDescriptor& descriptor, RenderStage renderStage);

  inline mat4<F32>& getMaterialMatrix()  {return _materialMatrix;}

         P32   getMaterialId(RenderStage renderStage = FINAL_STAGE);
  inline bool  getCastsShadows()    {return _castsShadows;}
  inline bool  getReceivesShadows() {return _receiveShadows;}
  inline F32   getOpacityValue()    {return _shaderData._opacity;}
  inline U8    getTextureCount()    {return _shaderData._textureCount;}
  inline U32                     getBumpMethod()                                {return _bumpMethodTable[_bumpMethod];}
  inline ShadingMode             getShadingMode()                               {return _shadingMode;}
  inline RenderStateBlock*       getRenderState(RenderStage currentStage)       {return _defaultRenderStates[currentStage];}
  inline U32               const getTextureOperation(TextureUsage textureUsage) {return _textureOperationTable[_operations[textureUsage]];}
  inline Texture2D*	       const getTexture(TextureUsage textureUsage)          {return _textures[textureUsage];}

  inline ShaderProgram*    const getShaderProgram(RenderStage renderStage = FINAL_STAGE)      {return _shaderRef[renderStage];}
  inline const ShaderData&       getShaderData() {return _shaderData;}

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

  void computeShader(bool force = false, RenderStage renderStage = FINAL_STAGE); //Set shaders;
  inline void dumpToXML() {XML::dumpMaterial(this);}

private:
  
  mat4<F32> _materialMatrix; /* all properties bundled togheter */
  ShadingMode _shadingMode;
  std::string _shaderModifier; //<use for special shader tokens, such as "Tree"
  vectorImpl<std::string > _shaderDefines; //<Add shader preprocessor defines;
  bool _dirty;
  bool _doubleSided;
  bool _castsShadows;
  bool _receiveShadows;
  bool _shaderProgramChanged; ///< Used for tracking VBO binding with shader
  bool _hardwareSkinning;     ///< Use shaders that have bone transforms implemented
  Unordered_map<RenderStage, ShaderProgram* > _shaderRef;

  ///Unordered maps have a lot of overhead, so use _shader[0] for final_stage, _shader[1] for shadow_stage, etc
  std::string _shader[2]; /*shader name*/
  bool        _computedShader[2];
  P32         _matId[2];
  /// use this map to add more render states mapped to a specific state
  /// 3 render state's: Normal, reflection and shadow
  Unordered_map<RenderStage, RenderStateBlock* > _defaultRenderStates;
  /// use this map to add textures to the material
  textureMap _textures;
  /// use the bellow map to define texture operation
  textureOperation _operations;
  /// Map texture operation to values the shader understands
  U32 _textureOperationTable[TextureOperation_PLACEHOLDER];
  U32 _bumpMethodTable[BUMP_PLACEHOLDER];

  BumpMethod _bumpMethod;

  ShaderData _shaderData;
};

#endif