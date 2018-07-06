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
        NONE = 0,    //<Use phong
        NORMAL = 1,  //<Normal mapping
        PARALLAX = 2,
        RELIEF = 3,
        COUNT
    };

    /// How should each texture be added
    enum class TextureOperation : U32 {
        MULTIPLY = 0,
        ADD = 1,
        SUBTRACT = 2,
        DIVIDE = 3,
        SMOOTH_ADD = 4,
        SIGNED_ADD = 5,
        DECAL = 6,
        REPLACE = 7,
        COUNT
    };

    enum class TranslucencySource : U32 {
        DIFFUSE = 0,
        OPACITY_MAP,
        DIFFUSE_MAP,
        NONE,
        COUNT
    };
    /// Not used yet but implemented for shading model selection in shaders
    /// This enum matches the ASSIMP one on a 1-to-1 basis
    enum class ShadingMode : U32 {
        FLAT = 0x1,
        PHONG = 0x2,
        BLINN_PHONG = 0x3,
        TOON = 0x4,
        OREN_NAYAR = 0x5,
        COOK_TORRANCE = 0x6,
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
            REQUESTED = 0,
            QUEUED = 1,
            COMPUTED = 2,
            UNHANDLED = 3,
            PENDING = 4,
            CUSTOM = 5,
            COUNT
        };

        ShaderProgram* _shaderRef;
        stringImpl _shader;
        ShaderCompilationStage _shaderCompStage;
        vectorImpl<stringImpl> _shaderDefines;

        ShaderProgram* const getProgram() const;

        inline StateTracker<bool>& getTrackedBools() { return _trackedBools; }

        ShaderInfo() {
            _shaderRef = nullptr;
            _shader = "";
            _shaderCompStage = ShaderCompilationStage::UNHANDLED;
            for (U32 i = 0; i < to_uint(ShaderType::COUNT); ++i) {
                _shadingFunction[i].fill(0);
            }
        }

        ShaderInfo& operator=(const ShaderInfo& other) {
            _shaderRef = other._shaderRef;
            if (_shaderRef != nullptr) {
                _shaderRef->AddRef();
            }
            _shader = other._shader;
            _shaderCompStage = other._shaderCompStage;
            for (U32 i = 0; i < to_uint(ShaderType::COUNT); ++i) {
                for (U32 j = 0; j < to_uint(BumpMethod::COUNT); ++j) {
                    _shadingFunction[i][j] = other._shadingFunction[i][j];
                }
            }
            _trackedBools = other._trackedBools;
            return *this;
        }

        std::array<std::array<U32, to_const_uint(BumpMethod::COUNT)>,
                   to_const_uint(ShaderType::COUNT)> _shadingFunction;

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
                    const TextureOperation& op = TextureOperation::REPLACE);
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
        vectorImpl<stringImpl>& defines =
            _shaderInfo[to_uint(renderStage)]._shaderDefines;
        if (std::find(std::begin(defines), end(defines), shaderDefines) ==
            std::end(defines)) {
            defines.push_back(shaderDefines);
        }
    }

    inline void setShaderDefines(const stringImpl& shaderDefines) {
        setShaderDefines(RenderStage::DISPLAY, shaderDefines);
        setShaderDefines(RenderStage::Z_PRE_PASS, shaderDefines);
        setShaderDefines(RenderStage::SHADOW, shaderDefines);
        setShaderDefines(RenderStage::REFLECTION, shaderDefines);
    }

    /// toggle multi-threaded shader loading on or off for this material
    inline void setShaderLoadThreaded(const bool state) {
        _shaderThreadedLoad = state;
    }
    void setShaderProgram(
        const stringImpl& shader,
        RenderStage renderStage,
        const bool computeOnAdd,
        const DELEGATE_CBK<>& shaderCompileCallback = DELEGATE_CBK<>());
    inline void setShaderProgram(
        const stringImpl& shader,
        const bool computeOnAdd,
        const DELEGATE_CBK<>& shaderCompileCallback = DELEGATE_CBK<>()) {
        setShaderProgram(shader, RenderStage::DISPLAY, computeOnAdd,
                         shaderCompileCallback);
        setShaderProgram(shader, RenderStage::Z_PRE_PASS, computeOnAdd,
                         shaderCompileCallback);
        setShaderProgram(shader, RenderStage::SHADOW, computeOnAdd,
                         shaderCompileCallback);
        setShaderProgram(shader, RenderStage::REFLECTION, computeOnAdd,
                         shaderCompileCallback);
    }
    size_t setRenderStateBlock(const RenderStateBlockDescriptor& descriptor,
                               RenderStage renderStage);

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
    ShaderInfo& getShaderInfo(RenderStage renderStage = RenderStage::DISPLAY);

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
    bool computeShader(RenderStage renderStage,
                       const bool computeOnAdd,
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
                                  RenderStage renderStage,
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
    static I32 _invalidShaderKey;

    typedef std::tuple<U32, ResourceDescriptor, DELEGATE_CBK<>>
        ShaderQueueElement;
    std::deque<ShaderQueueElement> _shaderComputeQueue;
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
    std::array<ShaderInfo, to_const_uint(RenderStage::COUNT)> _shaderInfo;
    std::array<size_t, to_const_uint(RenderStage::COUNT)> _defaultRenderStates;

    bool _shaderThreadedLoad;

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