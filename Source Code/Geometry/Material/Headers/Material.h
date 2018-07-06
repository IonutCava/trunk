/*
   Copyright (c) 2014 DIVIDE-Studio
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

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "core.h"
#include "Utility/Headers/XMLParser.h"
#include "Core/Resources/Headers/Resource.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Core/Resources/Headers/ResourceDescriptor.h"

class Texture;
class ShaderProgram;
class RenderStateBlock;
class ResourceDescriptor;
class RenderStateBlockDescriptor;
typedef Texture Texture2D;
enum RenderStage;
enum BlendProperty;
class Material : public Resource {
public:
    /// ShaderData stores information needed by the shader code to properly shade objects
    struct ShaderData{
        vec4<F32> _diffuse;  /* diffuse component */
        vec4<F32> _ambient;  /* ambient component */
        vec4<F32> _specular; /* specular component*/
        vec4<F32> _emissive; /* emissive component*/
        F32 _shininess;      /* specular exponent */
        F32 _opacity;        /* material opacity value*/
        I32 _textureCount;

        ShaderData() : _textureCount(0),
            _ambient(vec4<F32>(vec3<F32>(1.0f) / 5.0f, 1)),
            _diffuse(vec4<F32>(vec3<F32>(1.0f) / 1.5f, 1)),
            _specular(0.8f, 0.8f, 0.8f, 1.0f),
            _emissive(0.6f, 0.6f, 0.6f, 1.0f),
            _shininess(5),
            _opacity(1.0f) {}
    };

    /// ShaderInfo stores information about the shader programs used by this material
    struct ShaderInfo {
        ShaderProgram* _shaderRef;
        std::string _shader;
        bool _computedShader;
        vectorImpl<std::string> _shaderDefines;
        P32 _matId;

        ShaderInfo()
        {
            _shaderRef = nullptr;
            _shader = "";
            _computedShader = false;
            _matId.i = 0;
        }
    };

    enum BumpMethod {
        BUMP_NONE = 0,   //<Use phong
        BUMP_NORMAL = 1, //<Normal mapping
        BUMP_PARALLAX = 2,
        BUMP_RELIEF = 3,
        BUMP_PLACEHOLDER = 4
    };

    enum TextureUsage {
        TEXTURE_NORMALMAP = 0,
        TEXTURE_OPACITY = 1,
        TEXTURE_SPECULAR = 2,
        TEXTURE_UNIT0 = 3
    };

    /// How should each texture be added
    enum TextureOperation {
        TextureOperation_Multiply = 0x0,
        TextureOperation_Add = 0x1,
        TextureOperation_Subtract = 0x2,
        TextureOperation_Divide = 0x3,
        TextureOperation_SmoothAdd = 0x4,
        TextureOperation_SignedAdd = 0x5,
        TextureOperation_Combine = 0x6,
        TextureOperation_Decal = 0x7,
        TextureOperation_Blend = 0x8,
        TextureOperation_Replace = 0x9,
        TextureOperation_PLACEHOLDER = 0x10
    };

    enum GeometryShaderType {
        GS_TRIANGLES = 0,
        GS_LINES,
        GS_POINTS
    };

    enum TranslucencySource {
        TRANSLUCENT_DIFFUSE = 0,
        TRANSLUCENT_OPACITY,
        TRANSLUCENT_OPACITY_MAP,
        TRANSLUCENT_DIFFUSE_MAP,
        TRANSLUCENT_NONE
    };
    /// Not used yet but implemented for shading model selection in shaders
    /// This enum matches the ASSIMP one on a 1-to-1 basis
    enum ShadingMode {
        SHADING_FLAT = 0x1,
        SHADING_GOURAUD = 0x2,
        SHADING_PHONG = 0x3,
        SHADING_BLINN = 0x4,
        SHADING_TOON = 0x5,
        SHADING_OREN_NAYAR = 0x6,
        SHADING_MINNAERT = 0x7,
        SHADING_COOK_TORRANCE = 0x8,
        SHADING_NONE = 0x9,
        SHADING_FRESNEL = 0xa,
        ShadingMode_PLACEHOLDER = 0xb
    };

public:
    Material();
    ~Material();

    bool unload();
    void update(const U64 deltaTime);

    inline void setAmbient(const vec4<F32>& value, U8 index = 0)  { _dirty = true; _shaderData[index]._ambient = value;  _materialMatrix[index].setCol(0, value); }
    inline void setDiffuse(const vec4<F32>& value, U8 index = 0)  { _dirty = true; _shaderData[index]._diffuse = value;  _materialMatrix[index].setCol(1, value); }
    inline void setSpecular(const vec4<F32>& value, U8 index = 0) { _dirty = true; _shaderData[index]._specular = value; _materialMatrix[index].setCol(2, value); }
    inline void setEmissive(const vec3<F32>& value, U8 index = 0) { _dirty = true; _shaderData[index]._emissive = value; _materialMatrix[index].setCol(3, vec4<F32>(_shaderData[index]._shininess, value.x, value.y, value.z)); }

    inline void setGeometryInputType(GeometryShaderType gsType)   { _dirty = true; _gsInputType = gsType; }
    inline void setHardwareSkinning(const bool state) { _dirty = true; _hardwareSkinning = state; }
    inline void setOpacity(F32 value, U8 index = 0)   { _dirty = true; _shaderData[index]._opacity = value; }
    inline void setShininess(F32 value, U8 index = 0) {
        _dirty = true;
        _shaderData[index]._shininess = value;
        _materialMatrix[index].setCol(3, vec4<F32>(value, _shaderData[index]._emissive.x,
            _shaderData[index]._emissive.y,
            _shaderData[index]._emissive.z));
    }

    inline void useAlphaTest(const bool state)          { _useAlphaTest = state; }
    inline void setShadingMode(const ShadingMode& mode) { _shadingMode = mode; }

    void setDoubleSided(const bool state);
    void setTexture(U32 textureUsageSlot, Texture2D* const texture, const TextureOperation& op = TextureOperation_Replace, U8 index = 0);
    ///Set the desired bump mapping method. If force == true, the shader is updated immediately
    void setBumpMethod(const BumpMethod& newBumpMethod, const bool force = false);
    void setBumpMethod(U32 newBumpMethod, const bool force = false);
    ///Shader modifiers add tokens to the end of the shader name.
    ///Add as many tokens as needed but separate them with a ".". i.e: "Tree.NoWind.Glow"
    inline void addShaderModifier(const std::string& shaderModifier) { _shaderModifier = shaderModifier; }
    ///Shader defines, separated by commas, are added to the generated shader
    ///The shader generator appends "#define " to the start of each define
    ///For example, to define max light count and max shadow casters add this string:
    ///"MAX_LIGHT_COUNT 4, MAX_SHADOW_CASTERS 2"
    ///The above strings becomes, in the shader:
    ///#define MAX_LIGHT_COUNT 4
    ///#define MAX_SHADOW_CASTERS 2
    inline void addShaderDefines(RenderStage renderStage, const std::string& shaderDefines) {
        _shaderInfo[renderStage]._shaderDefines.push_back(shaderDefines);
    }
    inline void addShaderDefines(const std::string& shaderDefines)	{
        addShaderDefines(FINAL_STAGE, shaderDefines);
        addShaderDefines(Z_PRE_PASS_STAGE, shaderDefines);
        addShaderDefines(SHADOW_STAGE, shaderDefines);
    }

    ///toggle multi-threaded shader loading on or off for this material
    inline void setShaderLoadThreaded(const bool state) {_shaderThreadedLoad = state;}
    void setShaderProgram(const std::string& shader, const RenderStage& renderStage = FINAL_STAGE);
    RenderStateBlock* setRenderStateBlock(const RenderStateBlockDescriptor& descriptor,const RenderStage& renderStage);

    inline P32 getMaterialId(const RenderStage& renderStage = FINAL_STAGE) { return _shaderInfo[renderStage]._matId; }

    inline const mat4<F32>& getMaterialMatrix(U8 index = 0)  const {return _materialMatrix[index];}
               
    inline F32   getOpacityValue(U8 index = 0)    const {return _shaderData[index]._opacity;}
    inline U8    getTextureCount(U8 index = 0)    const {return _shaderData[index]._textureCount;}

    inline RenderStateBlock*       getRenderState(RenderStage currentStage)       {return _defaultRenderStates[currentStage];}
    inline Texture2D*	     const getTexture(U32 textureUsage)                   {return _textures[textureUsage];}
           ShaderProgram*    const getShaderProgram(RenderStage renderStage = FINAL_STAGE);

    inline const TextureOperation& getTextureOperation(U32 textureUsage)   const {
        return _operations[textureUsage >= TEXTURE_UNIT0 ? textureUsage - TEXTURE_UNIT0 : 0];
    }

    inline const ShaderData&  getShaderData(U8 index = 0) const {return _shaderData[index];}
    inline const ShadingMode& getShadingMode()            const {return _shadingMode;}
    inline const BumpMethod&  getBumpMethod()             const {return _bumpMethod;}

    void clean();
    bool isTranslucent(U8 index = 0);

    inline bool isDirty()       const {return _dirty;}
    inline bool isDoubleSided() const {return _doubleSided;}
    inline bool useAlphaTest()  const {return _useAlphaTest;}
    inline TranslucencySource   getTranslucencySource() const {return _translucencySource;}

    // Checks if the shader needed for the current stage is already constructed. Returns false if the shader was already ready.
    bool computeShader(bool force = false,const RenderStage& renderStage = FINAL_STAGE); //Set shaders;

private:
    void computeShaderInternal();

private:
    std::queue<std::pair<RenderStage, ResourceDescriptor> > _shaderComputeQueue;
    vectorImpl<mat4<F32> > _materialMatrix; /* all properties bundled togheter */
    ShadingMode _shadingMode;
    std::string _shaderModifier; //<use for special shader tokens, such as "Tree"
    TranslucencySource _translucencySource;
    bool _dirty;
    bool _translucencyCheck;
    bool _useAlphaTest; //< use discard if true / blend if otherwise
    bool _doubleSided;
    bool _hardwareSkinning;     ///< Use shaders that have bone transforms implemented
    GeometryShaderType _gsInputType;        ///< Use triangles, lines or points as geometry shader input
    Unordered_map<RenderStage, ShaderInfo > _shaderInfo;

    bool        _shaderThreadedLoad;
    bool        _computedShaderTextures;//<if we should recompute only fragment shader on texture change
    /// use this map to add more render states mapped to a specific state
    /// 3 render state's: Normal, reflection and shadow
    Unordered_map<RenderStage, RenderStateBlock* > _defaultRenderStates;
    /// use this map to add textures to the material
    Texture2D* _textures[Config::MAX_TEXTURE_STORAGE];
    /// use the bellow map to define texture operation
    TextureOperation _operations[Config::MAX_TEXTURE_STORAGE - TEXTURE_UNIT0];
    BumpMethod _bumpMethod;

    vectorImpl<ShaderData> _shaderData;
};

#endif