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

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "Utility/Headers/XMLParser.h"
#include "Utility/Headers/StateTracker.h"
#include "Core/Resources/Headers/Resource.h"
#include "Core/Resources/Headers/ResourceDescriptor.h"

#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

class RenderStateBlock;
class ResourceDescriptor;
class RenderStateBlockDescriptor;
enum class RenderStage : U32;
enum class BlendProperty : U32;

class Material : public Resource {
   public:
    enum class BumpMethod : U32 {
        BUMP_NONE = 0,    //<Use phong
        BUMP_NORMAL = 1,  //<Normal mapping
        BUMP_PARALLAX = 2,
        BUMP_RELIEF = 3,
        COUNT
    };

    /// How should each texture be added
    enum class TextureOperation : U32 {
        TextureOperation_Multiply = 0,
        TextureOperation_Add = 1,
        TextureOperation_Subtract = 2,
        TextureOperation_Divide = 3,
        TextureOperation_SmoothAdd = 4,
        TextureOperation_SignedAdd = 5,
        TextureOperation_Decal = 6,
        TextureOperation_Replace = 7,
        COUNT
    };

    enum class TranslucencySource : U32 {
        TRANSLUCENT_DIFFUSE = 0,
        TRANSLUCENT_OPACITY_MAP,
        TRANSLUCENT_DIFFUSE_MAP,
        TRANSLUCENT_NONE,
        COUNT
    };
    /// Not used yet but implemented for shading model selection in shaders
    /// This enum matches the ASSIMP one on a 1-to-1 basis
    enum class ShadingMode : U32 {
        SHADING_FLAT = 0x1,
        SHADING_PHONG = 0x2,
        SHADING_BLINN_PHONG = 0x3,
        SHADING_TOON = 0x4,
        SHADING_OREN_NAYAR = 0x5,
        SHADING_COOK_TORRANCE = 0x6,
        COUNT
    };

    /// ShaderData stores information needed by the shader code to properly
    /// shade objects
    struct ShaderData {
        vec4<F32> _diffuse;  /* diffuse component */
        vec4<F32> _ambient;  /* ambient component */
        vec4<F32> _specular; /* specular component*/
        vec4<F32> _emissive; /* emissive component*/
        F32 _shininess;      /* specular exponent */
        I32 _textureCount;

        ShaderData()
            : _textureCount(0),
              _ambient(vec4<F32>(vec3<F32>(1.0f) / 5.0f, 1)),
              _diffuse(vec4<F32>(vec3<F32>(1.0f) / 1.5f, 1)),
              _specular(0.8f, 0.8f, 0.8f, 1.0f),
              _emissive(0.6f, 0.6f, 0.6f, 1.0f),
              _shininess(5) {}

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

    /// ShaderInfo stores information about the shader programs used by this
    /// material
    struct ShaderInfo {
        enum class ShaderCompilationStage : U32 {
            SHADER_STAGE_REQUESTED = 0,
            SHADER_STAGE_QUEUED = 1,
            SHADER_STAGE_COMPUTED = 2,
            SHADER_STAGE_ERROR = 3,
            COUNT
        };

        ShaderProgram* _shaderRef;
        stringImpl _shader;
        ShaderCompilationStage _shaderCompStage;
        bool _isCustomShader;
        vectorImpl<stringImpl> _shaderDefines;

        ShaderProgram* const getProgram() const;

        inline StateTracker<bool>& getTrackedBools() { return _trackedBools; }

        ShaderInfo() {
            _shaderRef = nullptr;
            _shader = "";
            _shaderCompStage = ShaderCompilationStage::COUNT;
            _isCustomShader = false;
            for (U32 i = 0; i < to_uint(ShaderType::COUNT); ++i) {
                memset(_shadingFunction[i], 0,
                       to_uint(BumpMethod::COUNT) * sizeof(U32));
            }
        }

        ShaderInfo& operator=(const ShaderInfo& other) {
            _shaderRef = other.getProgram();
            _shaderRef->AddRef();
            _shader = other._shader;
            _shaderCompStage = other._shaderCompStage;
            _isCustomShader = other._isCustomShader;
            for (U32 i = 0; i < to_uint(ShaderType::COUNT); ++i) {
                for (U32 j = 0; j < to_uint(BumpMethod::COUNT); ++j) {
                    _shadingFunction[i][j] = other._shadingFunction[i][j];
                }
            }
            _trackedBools = other._trackedBools;
            return *this;
        }

        U32 _shadingFunction[to_const_uint(ShaderType::COUNT)][to_const_uint(
            BumpMethod::COUNT)];

       protected:
        StateTracker<bool> _trackedBools;
    };

   public:
    Material();
    ~Material();

    /// Return a new instance of this material with the name composed of the
    /// base material's name and the give name suffix.
    /// Call RemoveResource on the returned pointer to free memory. (clone calls
    /// CreateResource internally!)
    Material* clone(const stringImpl& nameSuffix);
    bool unload();
    void update(const U64 deltaTime);

    inline void setDiffuse(const vec4<F32>& value) {
        _dirty = true;
        _shaderData._diffuse = value;
        if (value.a < 0.95f) {
            _translucencyCheck = false;
        }
    }

    inline void setAmbient(const vec4<F32>& value) {
        _dirty = true;
        _shaderData._ambient = value;
    }
    inline void setSpecular(const vec4<F32>& value) {
        _dirty = true;
        _shaderData._specular = value;
    }
    inline void setEmissive(const vec3<F32>& value) {
        _dirty = true;
        _shaderData._emissive = value;
    }

    inline void setHardwareSkinning(const bool state) {
        _dirty = true;
        _hardwareSkinning = state;
    }
    inline void setOpacity(F32 value) {
        _dirty = true;
        _shaderData._diffuse.a = value;
        _translucencyCheck = false;
    }
    inline void setShininess(F32 value) {
        _dirty = true;
        _shaderData._shininess = value;
    }

    inline void useAlphaTest(const bool state) { _useAlphaTest = state; }
    inline void setShadingMode(const ShadingMode& mode) { _shadingMode = mode; }

    void setDoubleSided(const bool state, const bool useAlphaTest = true);
    void setTexture(ShaderProgram::TextureUsage textureUsageSlot,
                    Texture* const texture,
                    const TextureOperation& op =
                        TextureOperation::TextureOperation_Replace);
    /// Add a texture <-> bind slot pair to be bound with the default textures
    /// on each "bindTexture" call
    inline void addCustomTexture(Texture* texture, U8 offset) {
        // custom textures are not material dependencies!
        _customTextures.push_back(std::make_pair(texture, offset));
    }

    /// Remove the custom texture assigned to the specified offset
    inline bool removeCustomTexture(U8 index) {
        vectorImpl<std::pair<Texture*, U8>>::iterator it =
            std::find_if(std::begin(_customTextures), std::end(_customTextures),
                         [&index](const std::pair<Texture*, U8>& tex)
                             -> bool { return tex.second == index; });
        if (it == std::end(_customTextures)) {
            return false;
        }

        _customTextures.erase(it);

        return true;
    }

    /// Set the desired bump mapping method.
    void setBumpMethod(const BumpMethod& newBumpMethod);
    /// Shader modifiers add tokens to the end of the shader name.
    /// Add as many tokens as needed but separate them with a ".". i.e:
    /// "Tree.NoWind.Glow"
    inline void addShaderModifier(const stringImpl& shaderModifier) {
        _shaderModifier = shaderModifier;
    }
    /// Shader defines, separated by commas, are added to the generated shader
    /// The shader generator appends "#define " to the start of each define
    /// For example, to define max light count and max shadow casters add this
    /// string:
    ///"MAX_LIGHT_COUNT 4, MAX_SHADOW_CASTERS 2"
    /// The above strings becomes, in the shader:
    ///#define MAX_LIGHT_COUNT 4
    ///#define MAX_SHADOW_CASTERS 2
    inline void setShaderDefines(RenderStage renderStage,
                                 const stringImpl& shaderDefines) {
        _shaderInfo[to_uint(renderStage)]._shaderDefines.push_back(
            shaderDefines);
    }

    inline void setShaderDefines(const stringImpl& shaderDefines) {
        setShaderDefines(RenderStage::DISPLAY_STAGE, shaderDefines);
        setShaderDefines(RenderStage::Z_PRE_PASS_STAGE, shaderDefines);
        setShaderDefines(RenderStage::SHADOW_STAGE, shaderDefines);
        setShaderDefines(RenderStage::REFLECTION_STAGE, shaderDefines);
    }

    /// toggle multi-threaded shader loading on or off for this material
    inline void setShaderLoadThreaded(const bool state) {
        _shaderThreadedLoad = state;
    }
    void setShaderProgram(
        const stringImpl& shader, const RenderStage& renderStage,
        const bool computeOnAdd,
        const DELEGATE_CBK<>& shaderCompileCallback = DELEGATE_CBK<>());
    inline void setShaderProgram(
        const stringImpl& shader, const bool computeOnAdd,
        const DELEGATE_CBK<>& shaderCompileCallback = DELEGATE_CBK<>()) {
        setShaderProgram(shader, RenderStage::DISPLAY_STAGE, computeOnAdd,
                         shaderCompileCallback);
        setShaderProgram(shader, RenderStage::Z_PRE_PASS_STAGE, computeOnAdd,
                         shaderCompileCallback);
        setShaderProgram(shader, RenderStage::SHADOW_STAGE, computeOnAdd,
                         shaderCompileCallback);
        setShaderProgram(shader, RenderStage::REFLECTION_STAGE, computeOnAdd,
                         shaderCompileCallback);
    }
    size_t setRenderStateBlock(const RenderStateBlockDescriptor& descriptor,
                               const RenderStage& renderStage);

    inline void setParallaxFactor(F32 factor) {
        _parallaxFactor = std::min(0.01f, factor);
    }

    void getSortKeys(I32& shaderKey, I32& textureKey) const;

    inline void getMaterialMatrix(mat4<F32>& retMatrix) const {
        retMatrix.setCol(0, _shaderData._ambient);
        retMatrix.setCol(1, _shaderData._diffuse);
        retMatrix.setCol(2, _shaderData._specular);
        retMatrix.setCol(
            3, vec4<F32>(_shaderData._emissive, _shaderData._shininess));
    }

    inline F32 getParallaxFactor() const { return _parallaxFactor; }
    inline U8 getTextureCount() const { return _shaderData._textureCount; }

    size_t getRenderStateBlock(RenderStage currentStage);
    inline Texture* const getTexture(ShaderProgram::TextureUsage textureUsage) {
        return _textures[to_uint(textureUsage)];
    }
    ShaderInfo& getShaderInfo(
        RenderStage renderStage = RenderStage::DISPLAY_STAGE);

    inline const TextureOperation& getTextureOperation() const {
        return _operation;
    }
    inline const ShaderData& getShaderData() const { return _shaderData; }
    inline const ShadingMode& getShadingMode() const { return _shadingMode; }
    inline const BumpMethod& getBumpMethod() const { return _bumpMethod; }

    void getTextureData(TextureDataContainer& textureData);

    void clean();
    bool isTranslucent();

    inline void dumpToFile(bool state) { _dumpToFile = state; }
    inline bool isDirty() const { return _dirty; }
    inline bool isDoubleSided() const { return _doubleSided; }
    inline bool useAlphaTest() const { return _useAlphaTest; }

    // Checks if the shader needed for the current stage is already constructed.
    // Returns false if the shader was already ready.
    bool computeShader(const RenderStage& renderStage, const bool computeOnAdd,
                       const DELEGATE_CBK<>& shaderCompileCallback);

    static void unlockShaderQueue() { _shaderQueueLocked = false; }
    static void serializeShaderLoad(const bool state) {
        _serializeShaderLoad = state;
    }

   private:
    void getTextureData(ShaderProgram::TextureUsage slot,
                        TextureDataContainer& container);

    void recomputeShaders();
    void computeShaderInternal();
    void setShaderProgramInternal(const stringImpl& shader,
                                  const RenderStage& renderStage,
                                  const bool computeOnAdd,
                                  const DELEGATE_CBK<>& shaderCompileCallback);

    static bool isShaderQueueLocked() { return _shaderQueueLocked; }

    static void lockShaderQueue() {
        if (_serializeShaderLoad) {
            _shaderQueueLocked = true;
        }
    }

   private:
    static bool _shaderQueueLocked;
    static bool _serializeShaderLoad;

    std::queue<std::tuple<U32, ResourceDescriptor, DELEGATE_CBK<>>>
        _shaderComputeQueue;
    ShadingMode _shadingMode;
    /// use for special shader tokens, such as "Tree"
    stringImpl _shaderModifier;
    vectorImpl<TranslucencySource> _translucencySource;
    /// parallax/relief factor (higher value > more pronounced effect)
    F32 _parallaxFactor;
    bool _dirty;
    bool _dumpToFile;
    bool _translucencyCheck;
    /// use discard if true / blend if otherwise
    bool _useAlphaTest;
    bool _doubleSided;
    /// Use shaders that have bone transforms implemented
    bool _hardwareSkinning;
    ShaderInfo _shaderInfo[to_const_uint(RenderStage::COUNT)];
    size_t _defaultRenderStates[to_const_uint(RenderStage::COUNT)];

    bool _shaderThreadedLoad;
    /// if we should recompute only fragment
    bool _computedShaderTextures;

    /// use this map to add textures to the material
    vectorImpl<Texture*> _textures;
    vectorImpl<std::pair<Texture*, U8>> _customTextures;

    /// use the below map to define texture operation
    TextureOperation _operation;

    BumpMethod _bumpMethod;

    ShaderData _shaderData;
};

};  // namespace Divide

#endif