/*
   Copyright (c) 2015 DIVIDE-Studio
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

#include "Utility/Headers/XMLParser.h"
#include "Utility/Headers/StateTracker.h"
#include "Core/Resources/Headers/Resource.h"
#include "Core/Resources/Headers/ResourceDescriptor.h"

#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

class Texture;
class RenderStateBlock;
class ResourceDescriptor;
class RenderStateBlockDescriptor;
enum RenderStage : I32;
enum BlendProperty : I32;

class Material : public Resource {
public:
  enum BumpMethod {
        BUMP_NONE = 0,   //<Use phong
        BUMP_NORMAL = 1, //<Normal mapping
        BUMP_PARALLAX = 2,
        BUMP_RELIEF = 3,
        BumpMethod_PLACEHOLDER = 4
    };

    /// How should each texture be added
    enum TextureOperation {
        TextureOperation_Multiply = 0x0,
        TextureOperation_Add = 0x1,
        TextureOperation_Subtract = 0x2,
        TextureOperation_Divide = 0x3,
        TextureOperation_SmoothAdd = 0x4,
        TextureOperation_SignedAdd = 0x5,
        TextureOperation_Decal = 0x6,
        TextureOperation_Replace = 0x7,
        TextureOperation_PLACEHOLDER = 0x8
    };

    enum TranslucencySource {
        TRANSLUCENT_DIFFUSE = 0,
        TRANSLUCENT_OPACITY_MAP,
        TRANSLUCENT_DIFFUSE_MAP,
        TRANSLUCENT_NONE
    };
    /// Not used yet but implemented for shading model selection in shaders
    /// This enum matches the ASSIMP one on a 1-to-1 basis
    enum ShadingMode {
        SHADING_FLAT = 0x1,
        SHADING_PHONG = 0x2,
        SHADING_BLINN_PHONG = 0x3,
        SHADING_TOON = 0x4,
        SHADING_OREN_NAYAR = 0x5,
        SHADING_COOK_TORRANCE = 0x6,
        ShadingMode_PLACEHOLDER = 0xb
    };

    /// ShaderData stores information needed by the shader code to properly shade objects
    struct ShaderData{
        vec4<F32> _diffuse;  /* diffuse component */
        vec4<F32> _ambient;  /* ambient component */
        vec4<F32> _specular; /* specular component*/
        vec4<F32> _emissive; /* emissive component*/
        F32 _shininess;      /* specular exponent */
        I32 _textureCount;

        ShaderData() : _textureCount(0),
            _ambient(vec4<F32>(vec3<F32>(1.0f) / 5.0f, 1)),
            _diffuse(vec4<F32>(vec3<F32>(1.0f) / 1.5f, 1)),
            _specular(0.8f, 0.8f, 0.8f, 1.0f),
            _emissive(0.6f, 0.6f, 0.6f, 1.0f),
            _shininess(5)
        {
        }

        ShaderData& operator=(const ShaderData& other) {
            _diffuse.set(other._diffuse);
            _ambient.set(other._ambient); 
            _specular.set(other._specular);
            _emissive.set(other._emissive);
            _shininess = other._shininess;     
            _textureCount = other._textureCount;
            return *this;
        }
    };

    /// ShaderInfo stores information about the shader programs used by this material
    struct ShaderInfo {

        enum ShaderCompilationStage {
            SHADER_STAGE_REQUESTED = 0,
            SHADER_STAGE_QUEUED = 1,
            SHADER_STAGE_COMPUTED = 2,
            SHADER_STAGE_ERROR = 3,
            ShaderCompilationStage_PLACEHOLDER = 4
        };

        ShaderProgram* _shaderRef;
        stringImpl _shader;
        ShaderCompilationStage _shaderCompStage;
        bool _isCustomShader;
        vectorImpl<stringImpl> _shaderDefines;

        ShaderProgram* const getProgram() const;

        inline StateTracker<bool>& getTrackedBools() { return _trackedBools; }

        ShaderInfo()
        {
            _shaderRef = nullptr;
            _shader = "";
            _shaderCompStage = ShaderCompilationStage_PLACEHOLDER;
            _isCustomShader = false;
            for (U8 i = 0; i < ShaderType_PLACEHOLDER; ++i){
                memset(_shadingFunction[i], 0, BumpMethod_PLACEHOLDER * sizeof(U32));
            }
        }

        ShaderInfo& operator=(const ShaderInfo& other){
            _shaderRef = other.getProgram();
            _shaderRef->AddRef();
            _shader = other._shader;
            _shaderCompStage = other._shaderCompStage;
            _isCustomShader = other._isCustomShader;
            for (U8 i = 0; i < ShaderType_PLACEHOLDER; ++i){
                for (U8 j = 0; j < BumpMethod_PLACEHOLDER; ++j){
                    _shadingFunction[i][j] = other._shadingFunction[i][j];
                }
            }
            _trackedBools = other._trackedBools;
            return *this;
        }

        U32 _shadingFunction[ShaderType_PLACEHOLDER][BumpMethod_PLACEHOLDER];

    protected:
        StateTracker<bool> _trackedBools;
    };
  
public:
    Material();
    ~Material();

    /// Return a new instance of this material with the name composed of the base material's name and the give name suffix.
    /// Call RemoveResource on the returned pointer to free memory. (clone calls CreateResource internally!)
    Material* clone(const stringImpl& nameSuffix);
    bool unload();
    void update(const U64 deltaTime);

    inline void setDiffuse(const vec4<F32>& value)  { 
        _dirty = true; 
        _shaderData._diffuse = value;  
        if (value.a < 0.95f) {
            _translucencyCheck  = false;
        }
    }

    inline void setAmbient(const vec4<F32>& value)  { _dirty = true; _shaderData._ambient = value;  }
    inline void setSpecular(const vec4<F32>& value) { _dirty = true; _shaderData._specular = value; }
    inline void setEmissive(const vec3<F32>& value) { _dirty = true; _shaderData._emissive = value; }

    inline void setHardwareSkinning(const bool state) { _dirty = true; _hardwareSkinning = state; }
    inline void setOpacity(F32 value)   { _dirty = true; _shaderData._diffuse.a = value; _translucencyCheck = false; }
    inline void setShininess(F32 value) { _dirty = true; _shaderData._shininess = value; }

    inline void useAlphaTest(const bool state)          { _useAlphaTest = state; }
    inline void setShadingMode(const ShadingMode& mode) { _shadingMode = mode; }

    void setDoubleSided(const bool state, const bool useAlphaTest = true);
    void setTexture(ShaderProgram::TextureUsage textureUsageSlot, 
                    Texture* const texture, 
                    const TextureOperation& op = TextureOperation_Replace);
    /// Add a texture <-> bind slot pair to be bound with the default textures on each "bindTexture" call
    inline void addCustomTexture(Texture* texture, U32 offset) {
        // custom textures are not material dependencies!
        _customTextures.push_back(std::make_pair(texture, offset));
    }

    /// Remove the custom texture assigned to the specified offset
    inline bool removeCustomTexture(U32 index) {
        vectorImpl<std::pair<Texture*, U32 > >::iterator it = std::find_if(_customTextures.begin(), _customTextures.end(), 
                                                                            [&index]( const std::pair<Texture*, U32>& tex )->bool {
                                                                                return tex.second == index;
                                                                            });
        if(it == _customTextures.end())
            return false;

        _customTextures.erase(it);

        return true;
    }
        
    ///Set the desired bump mapping method.
    void setBumpMethod(const BumpMethod& newBumpMethod);
    ///Shader modifiers add tokens to the end of the shader name.
    ///Add as many tokens as needed but separate them with a ".". i.e: "Tree.NoWind.Glow"
    inline void addShaderModifier(const stringImpl& shaderModifier) { _shaderModifier = shaderModifier; }
    ///Shader defines, separated by commas, are added to the generated shader
    ///The shader generator appends "#define " to the start of each define
    ///For example, to define max light count and max shadow casters add this string:
    ///"MAX_LIGHT_COUNT 4, MAX_SHADOW_CASTERS 2"
    ///The above strings becomes, in the shader:
    ///#define MAX_LIGHT_COUNT 4
    ///#define MAX_SHADOW_CASTERS 2
    inline void addShaderDefines(RenderStage renderStage, const stringImpl& shaderDefines) {
        _shaderInfo[renderStage]._shaderDefines.push_back(shaderDefines);
    }

    inline void addShaderDefines(const stringImpl& shaderDefines)    {
        addShaderDefines(FINAL_STAGE, shaderDefines);
        addShaderDefines(Z_PRE_PASS_STAGE, shaderDefines);
        addShaderDefines(SHADOW_STAGE, shaderDefines);
        addShaderDefines(REFLECTION_STAGE, shaderDefines);
    }

    ///toggle multi-threaded shader loading on or off for this material
    inline void setShaderLoadThreaded(const bool state) {_shaderThreadedLoad = state;}
    void setShaderProgram(const stringImpl& shader, 
                          const RenderStage& renderStage, 
                          const bool computeOnAdd, 
                          const DELEGATE_CBK<>& shaderCompileCallback = DELEGATE_CBK<>());
    inline void setShaderProgram(const stringImpl& shader, 
                                 const bool computeOnAdd, 
                                 const DELEGATE_CBK<>& shaderCompileCallback = DELEGATE_CBK<>()){
        setShaderProgram(shader, FINAL_STAGE, computeOnAdd, shaderCompileCallback);
        setShaderProgram(shader, Z_PRE_PASS_STAGE, computeOnAdd, shaderCompileCallback);
        setShaderProgram(shader, SHADOW_STAGE, computeOnAdd, shaderCompileCallback);
        setShaderProgram(shader, REFLECTION_STAGE, computeOnAdd, shaderCompileCallback);
    }
    size_t setRenderStateBlock(const RenderStateBlockDescriptor& descriptor, const RenderStage& renderStage);
    
    inline void setParallaxFactor(F32 factor) { _parallaxFactor = std::min(0.01f, factor); }

    void getSortKeys(I32& shaderKey, I32& textureKey) const;

    inline void getMaterialMatrix(mat4<F32>& retMatrix) const {
        retMatrix.setCol(0,_shaderData._ambient);
        retMatrix.setCol(1,_shaderData._diffuse);
        retMatrix.setCol(2,_shaderData._specular);
        retMatrix.setCol(3,vec4<F32>(_shaderData._emissive, _shaderData._shininess));
    }

    inline F32 getParallaxFactor() const { return _parallaxFactor;}
    inline U8  getTextureCount()   const { return _shaderData._textureCount;}

                    size_t            getRenderStateBlock(RenderStage currentStage);
    inline       Texture*     const getTexture(ShaderProgram::TextureUsage textureUsage) {return _textures[textureUsage];}
                ShaderInfo&           getShaderInfo(RenderStage renderStage = FINAL_STAGE);
    
    inline const TextureOperation& getTextureOperation() const { return _operation; }
    inline const ShaderData&       getShaderData()       const { return _shaderData; }
    inline const ShadingMode&      getShadingMode()      const { return _shadingMode; }
    inline const BumpMethod&       getBumpMethod()       const { return _bumpMethod; }

    void bindTextures();

    void clean();
    bool isTranslucent();

    inline void dumpToFile(bool state) { _dumpToFile  = state;}
    inline bool isDirty()       const {return _dirty;}
    inline bool isDoubleSided() const {return _doubleSided;}
    inline bool useAlphaTest()  const {return _useAlphaTest;}

    // Checks if the shader needed for the current stage is already constructed. Returns false if the shader was already ready.
    bool computeShader(const RenderStage& renderStage, const bool computeOnAdd, const DELEGATE_CBK<>& shaderCompileCallback); 
    
    static void unlockShaderQueue()   {_shaderQueueLocked = false; }
    static void serializeShaderLoad(const bool state) { _serializeShaderLoad = state; }

private:
    void recomputeShaders();
    void computeShaderInternal();
    void setShaderProgramInternal(const stringImpl& shader, 
                                  const RenderStage& renderStage, 
                                  const bool computeOnAdd, 
                                  const DELEGATE_CBK<>& shaderCompileCallback);

    static bool isShaderQueueLocked() {
        return _shaderQueueLocked; 
    }

    static void lockShaderQueue() {
        if(_serializeShaderLoad) {
        _shaderQueueLocked = true; 
        }
    }
    
private:
    static bool _shaderQueueLocked;
    static bool _serializeShaderLoad;

    std::queue<std::tuple<RenderStage, ResourceDescriptor, DELEGATE_CBK<> >> _shaderComputeQueue;
    ShadingMode _shadingMode;
    stringImpl _shaderModifier; //<use for special shader tokens, such as "Tree"
    vectorImpl<TranslucencySource > _translucencySource;
    F32 _parallaxFactor; //< parallax/relief factor (higher value > more pronounced effect)
    bool _dirty;
    bool _dumpToFile;
    bool _translucencyCheck;
    bool _useAlphaTest; //< use discard if true / blend if otherwise
    bool _doubleSided;
    bool _hardwareSkinning;     ///< Use shaders that have bone transforms implemented
    typedef hashMapImpl<RenderStage, ShaderInfo, hashAlg::hash<I32>> shaderInfoMap;
    shaderInfoMap _shaderInfo;

    bool        _shaderThreadedLoad;
    bool        _computedShaderTextures;//<if we should recompute only fragment shader on texture change
    /// use this map to add more render states mapped to a specific state
    /// 3 render state's: Normal, reflection and shadow
    typedef hashMapImpl<RenderStage, size_t /*renderStateBlockHash*/, hashAlg::hash<I32>> renderStateBlockMap;
    renderStateBlockMap _defaultRenderStates;

    /// use this map to add textures to the material
    vectorImpl<Texture* > _textures;
    vectorImpl<std::pair<Texture*, U32> > _customTextures;

    /// use the below map to define texture operation
    TextureOperation _operation;

    BumpMethod _bumpMethod;

    ShaderData _shaderData;
};

}; //namespace Divide

#endif