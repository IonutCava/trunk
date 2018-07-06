/*
   Copyright (c) 2017 DIVIDE-Studio
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

#include "Geometry/Material/Headers/ShaderProgramInfo.h"

namespace Divide {

class RenderStateBlock;
class ResourceDescriptor;
class RenderStateBlock;
enum class RenderStage : U32;
enum class BlendProperty : U32;
enum class ReflectorType : U32;

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
        NONE = 0,
        MULTIPLY = 1,
        ADD = 2,
        SUBTRACT = 3,
        DIVIDE = 4,
        SMOOTH_ADD = 5,
        SIGNED_ADD = 6,
        DECAL = 7,
        REPLACE = 8,
        COUNT
    };

    enum class TranslucencySource : U32 {
        DIFFUSE = 0,
        DIFFUSE_MAP,
        OPACITY_MAP,
        COUNT
    };
    /// Not used yet but implemented for shading model selection in shaders
    /// This enum matches the ASSIMP one on a 1-to-1 basis
    enum class ShadingMode : U32 {
        FLAT = 0x1,
        PHONG = 0x2,
        BLINN_PHONG = 0x3,
        TOON = 0x4,
        // Use PBR for the following
        OREN_NAYAR = 0x5,
        COOK_TORRANCE = 0x6,
        COUNT
    };

    /// ShaderData stores information needed by the shader code to properly
    /// shade objects
    struct ColourData {
        vec4<F32> _diffuse;  /* diffuse component */
        vec4<F32> _specular; /* specular component*/
        vec4<F32> _emissive; /* emissive component*/
        F32 _shininess;      /* specular exponent */

        ColourData()
            : _diffuse(vec4<F32>(VECTOR3_UNIT / 1.5f, 1)),
              _specular(0.8f, 0.8f, 0.8f, 1.0f),
              _emissive(0.0f, 0.0f, 0.0f, 1.0f),
              _shininess(5)
        {
        }

        ColourData(const ColourData& other)
            : _diffuse(other._diffuse),
              _specular(other._specular),
              _emissive(other._emissive),
              _shininess(other._shininess)
        {
        }

        ColourData& operator=(const ColourData& other) {
            _diffuse.set(other._diffuse);
            _specular.set(other._specular);
            _emissive.set(other._emissive);
            _shininess = other._shininess;
            return *this;
        }
    };

   public:
    explicit Material(GFXDevice& context, ResourceCache& parentCache, const stringImpl& name);
    ~Material();

    /// Return a new instance of this material with the name composed of the
    /// base material's name and the give name suffix.
    /// clone calls CreateResource internally!)
    Material_ptr clone(const stringImpl& nameSuffix);
    bool unload();
    void update(const U64 deltaTime);

    inline void setColourData(const ColourData& other) {
        _dirty = true;
        _colourData = other;
        _translucencyCheck = true;
    }

    inline void setDiffuse(const vec4<F32>& value) {
        _dirty = true;
        _colourData._diffuse = value;
        if (value.a < 0.95f) {
            _translucencyCheck = true;
        }
    }

    inline void setSpecular(const vec4<F32>& value) {
        _dirty = true;
        _colourData._specular = value;
    }

    inline void setEmissive(const vec3<F32>& value) {
        _dirty = true;
        _colourData._emissive = value;
    }

    inline void setHardwareSkinning(const bool state) {
        _dirty = true;
        _hardwareSkinning = state;
    }

    inline void setOpacity(F32 value) {
        _dirty = true;
        _colourData._diffuse.a = value;
        _translucencyCheck = true;
    }

    inline void setShininess(F32 value) {
        _dirty = true;
        _colourData._shininess = value;
    }

    void setShadingMode(const ShadingMode& mode);

    inline void useAlphaTest(const bool state) { _useAlphaTest = state; }
    
    // Should the shaders be computed on add? Should reflections be always parsed? Etc
    inline void setHighPriority(const bool state) { _highPriority = state; }

    void setDoubleSided(const bool state, const bool useAlphaTest = true);
    bool setTexture(ShaderProgram::TextureUsage textureUsageSlot,
                    const Texture_ptr& tex,
                    const TextureOperation& op = TextureOperation::REPLACE);
    /// Add a texture <-> bind slot pair to be bound with the default textures
    /// on each "bindTexture" call
    void addCustomTexture(const Texture_ptr& texture, U8 offset);

    /// Remove the custom texture assigned to the specified offset
    bool removeCustomTexture(U8 index);

    /// Set the desired bump mapping method.
    void setBumpMethod(const BumpMethod& newBumpMethod);
    /// Shader modifiers add tokens to the end of the shader name.
    /// Add as many tokens as needed but separate them with a ".". i.e:
    /// "Tree.NoWind.Glow"
    inline void addShaderModifier(RenderStage renderStage, const stringImpl& shaderModifier) {
        _shaderModifier[to_uint(renderStage)] = shaderModifier;
    }
    inline void addShaderModifier(const stringImpl& shaderModifier) {
        for (U32 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
            addShaderModifier(static_cast<RenderStage>(i), shaderModifier);
        }
    }

    /// Shader defines, separated by commas, are added to the generated shader
    /// The shader generator appends "#define " to the start of each define
    /// For example, to define max light count and max shadow casters add this string:
    ///"MAX_LIGHT_COUNT 4, MAX_SHADOW_CASTERS 2"
    /// The above strings becomes, in the shader:
    ///#define MAX_LIGHT_COUNT 4
    ///#define MAX_SHADOW_CASTERS 2
    inline void setShaderDefines(RenderStage renderStage,  const stringImpl& shaderDefines) {
        vectorImpl<stringImpl>& defines =  _shaderInfo[to_uint(renderStage)]._shaderDefines;
        if (std::find(std::cbegin(defines), std::cend(defines), shaderDefines) == std::cend(defines)) {
            defines.push_back(shaderDefines);
        }
    }

    inline void setShaderDefines(const stringImpl& shaderDefines) {
        for (U32 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
            setShaderDefines(static_cast<RenderStage>(i), shaderDefines);
        }
    }

    /// toggle multi-threaded shader loading on or off for this material
    inline void setShaderLoadThreaded(const bool state) {
        _shaderThreadedLoad = state;
    }

    void setShaderProgram(const stringImpl& shader,
                          RenderStage renderStage,
                          const bool computeOnAdd);

    inline void setShaderProgram(const stringImpl& shader,
                                 const bool computeOnAdd) {
        for (U32 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
            setShaderProgram(shader, static_cast<RenderStage>(i), computeOnAdd);
        }
    }

    inline void setRenderStateBlock(size_t renderStateBlockHash,
                                    I32 variant = -1) {
        for (U32 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
            setRenderStateBlock(renderStateBlockHash, static_cast<RenderStage>(i), variant);
        }
    }

    inline void setRenderStateBlock(size_t renderStateBlockHash,
                                    RenderStage renderStage,
                                    I32 variant = -1) {
        if (variant < 0 || variant >= _defaultRenderStates[to_uint(renderStage)].size()) {
            for (size_t& state : _defaultRenderStates[to_uint(renderStage)]) {
              state  = renderStateBlockHash;
            }
        } else {
            _defaultRenderStates[to_uint(renderStage)][variant] = renderStateBlockHash;
        }
    }

    inline void setParallaxFactor(F32 factor) {
        _parallaxFactor = std::min(0.01f, factor);
    }

    void getSortKeys(I32& shaderKey, I32& textureKey) const;

    void getMaterialMatrix(mat4<F32>& retMatrix) const;

    inline F32 getParallaxFactor() const { return _parallaxFactor; }

    size_t getRenderStateBlock(RenderStage currentStage, I32 variant = 0);
    inline std::weak_ptr<Texture> getTexture(ShaderProgram::TextureUsage textureUsage) const {
        return _textures[to_uint(textureUsage)];
    }

    ShaderProgramInfo& getShaderInfo(RenderStage renderStage = RenderStage::DISPLAY);

    inline const TextureOperation& getTextureOperation() const { return _operation; }

    inline const ColourData&  getColourData()  const { return _colourData; }
    inline const ShadingMode& getShadingMode() const { return _shadingMode; }
    inline const BumpMethod&  getBumpMethod()  const { return _bumpMethod; }

    void getTextureData(TextureDataContainer& textureData);

    void rebuild();
    void clean();
    bool updateTranslucency();
    inline bool isTranslucent() const { return _translucencySource != TranslucencySource::COUNT; }

    inline void dumpToFile(bool state) { _dumpToFile = state; }
    inline bool isDirty() const { return _dirty; }
    inline bool isDoubleSided() const { return _doubleSided; }
    inline bool useAlphaTest() const { return _useAlphaTest; }
    // Checks if the shader needed for the current stage is already constructed.
    // Returns false if the shader was already ready.
    bool computeShader(RenderStage renderStage, const bool computeOnAdd);

    bool canDraw(RenderStage renderStage);

    void updateReflectionIndex(ReflectorType type, I32 index);
    void updateRefractionIndex(ReflectorType type, I32 index);

    void defaultReflectionTexture(const Texture_ptr& reflectionPtr, U32 arrayIndex);
    void defaultRefractionTexture(const Texture_ptr& reflectionPtr, U32 arrayIndex);

    inline U32 defaultReflectionTextureIndex() const { return _reflectionIndex > -1 ? to_uint(_reflectionIndex) : _defaultReflection.second; }
    inline U32 defaultRefractionTextureIndex() const { return _refractionIndex > -1 ? to_uint(_refractionIndex) : _defaultRefraction.second; }

   private:
    void getTextureData(ShaderProgram::TextureUsage slot,
                        TextureDataContainer& container);

    void recomputeShaders();
    void setShaderProgramInternal(const stringImpl& shader,
                                  RenderStage renderStage,
                                  const bool computeOnAdd);

    inline bool isExternalTexture(ShaderProgram::TextureUsage slot) const {
        return _textureExtenalFlag[to_uint(slot)];
    }

   private:
    GFXDevice& _context;
    ResourceCache& _parentCache;
    ShadingMode _shadingMode;
    /// use for special shader tokens, such as "Tree"
    std::array<stringImpl, to_const_uint(RenderStage::COUNT)> _shaderModifier;
    TranslucencySource _translucencySource;
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
    std::array<ShaderProgramInfo, to_const_uint(RenderStage::COUNT)> _shaderInfo;
    std::array<std::array<size_t, 3>,  to_const_uint(RenderStage::COUNT)> _defaultRenderStates;

    bool _shaderThreadedLoad;
    bool _highPriority;
    /// use this map to add textures to the material
    std::array<Texture_ptr, to_const_uint(ShaderProgram::TextureUsage::COUNT)> _textures;
    std::array<bool, to_const_uint(ShaderProgram::TextureUsage::COUNT)> _textureExtenalFlag;
    vectorImpl<std::pair<Texture_ptr, U8>> _customTextures;

    /// use the below map to define texture operation
    TextureOperation _operation;
    BumpMethod _bumpMethod;
    ColourData _colourData;

    /// used to keep track of what GFXDevice::reflectionTarget we are using for this rendering pass
    I32 _reflectionIndex;
    I32 _refractionIndex;
    std::pair<Texture_ptr, U32> _defaultReflection;
    std::pair<Texture_ptr, U32> _defaultRefraction;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(Material);

};  // namespace Divide

#endif
