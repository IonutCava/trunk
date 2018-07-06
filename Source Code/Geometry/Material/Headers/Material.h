/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "Utility/Headers/XMLParser.h"
#include "Utility/Headers/StateTracker.h"

#include "Core/Resources/Headers/Resource.h"
#include "Core/Resources/Headers/ResourceDescriptor.h"

#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/Headers/RenderStagePass.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Geometry/Material/Headers/ShaderProgramInfo.h"

namespace Divide {

class RenderStateBlock;
class ResourceDescriptor;

enum class BlendProperty : U8;
enum class ReflectorType : U8;

class Material : public CachedResource {
   public:
    enum class BumpMethod : U8 {
        NONE = 0,    //<Use phong
        NORMAL = 1,  //<Normal mapping
        PARALLAX = 2,
        RELIEF = 3,
        COUNT
    };

    /// How should each texture be added
    enum class TextureOperation : U8 {
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

    enum class TranslucencySource : U8 {
        DIFFUSE = 0,
        DIFFUSE_MAP,
        OPACITY_MAP,
        COUNT
    };

    enum class TranslucencyType : U8 {
        FULL_TRANSPARENT = 0,
        TRANSLUCENT,
        COUNT
    };

    /// Not used yet but implemented for shading model selection in shaders
    /// This enum matches the ASSIMP one on a 1-to-1 basis
    enum class ShadingMode : U8 {
        FLAT = 0x1,
        PHONG = 0x2,
        BLINN_PHONG = 0x3,
        TOON = 0x4,
        // Use PBR for the following
        OREN_NAYAR = 0x5,
        COOK_TORRANCE = 0x6,
        COUNT
    };

    struct ExternalTexture {
        Texture_ptr _texture = nullptr;
        U8 _bindSlot = 0;
        bool _activeForDepth = false;
    };

    /// ShaderData stores information needed by the shader code to properly
    /// shade objects
    struct ColourData {
        FColour _diffuse;  /* diffuse component */
        FColour _specular; /* specular component*/
        FColour _emissive; /* emissive component*/
        F32 _shininess;    /* specular exponent */

        ColourData()
            : _diffuse(FColour(VECTOR3_UNIT / 1.5f, 1)),
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
    explicit Material(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name);
    ~Material();

    /// Return a new instance of this material with the name composed of the
    /// base material's name and the give name suffix.
    /// clone calls CreateResource internally!)
    Material_ptr clone(const stringImpl& nameSuffix);
    bool unload();
    void update(const U64 deltaTimeUS);

    void setColourData(const ColourData& other);
    void setDiffuse(const FColour& value);
    void setSpecular(const FColour& value);
    void setEmissive(const vec3<F32>& value);
    void setHardwareSkinning(const bool state);
    void setOpacity(F32 value);
    void setShininess(F32 value);
    void setShadingMode(const ShadingMode& mode);
    // Should the shaders be computed on add? Should reflections be always parsed? Etc
    void setHighPriority(const bool state);

    void setDoubleSided(const bool state);
    bool setTexture(ShaderProgram::TextureUsage textureUsageSlot,
                    const Texture_ptr& tex,
                    const TextureOperation& op = TextureOperation::REPLACE);
    /// Add a texture <-> bind slot pair to be bound with the default textures
    /// on each "bindTexture" call
    void addExternalTexture(const Texture_ptr& texture, U8 slot, bool activeForDepth = false);

    /// Remove the custom texture assigned to the specified offset
    bool removeCustomTexture(U8 bindslot);

    /// Set the desired bump mapping method.
    void setBumpMethod(const BumpMethod& newBumpMethod);
    /// Shader modifiers add tokens to the end of the shader name.
    /// Add as many tokens as needed but separate them with a ".". i.e:
    /// "Tree.NoWind.Glow"
    void addShaderModifier(const RenderStagePass& renderStagePass, const stringImpl& shaderModifier);

    void addShaderModifier(const stringImpl& shaderModifier);

    /// Shader defines, separated by commas, are added to the generated shader
    /// The shader generator appends "#define " to the start of each define
    /// For example, to define max light count and max shadow casters add this string:
    ///"MAX_LIGHT_COUNT 4, MAX_SHADOW_CASTERS 2"
    /// The above strings becomes, in the shader:
    ///#define MAX_LIGHT_COUNT 4
    ///#define MAX_SHADOW_CASTERS 2
    void setShaderDefines(RenderPassType passType, const stringImpl& shaderDefines);
    void setShaderDefines(RenderStage renderStage, const stringImpl& shaderDefines);
    void setShaderDefines(const RenderStagePass& renderStagePass, const stringImpl& shaderDefines);

    void setShaderDefines(const stringImpl& shaderDefines);

    /// toggle multi-threaded shader loading on or off for this material
    void setShaderLoadThreaded(const bool state);

    /// Add the specified shader to the specified Stage Pass (stage and pass type)
    void setShaderProgram(const stringImpl& shader,
                          const RenderStagePass& renderStagePass,
                          const bool computeOnAdd);

    void setShaderProgram(const ShaderProgram_ptr& shader,
                          const RenderStagePass& renderStagePass);

    /// Add the specified shader to the specified render stage (for all pass types)
    void setShaderProgram(const stringImpl& shader,
                          RenderStage stage,
                          const bool computeOnAdd);

    void setShaderProgram(const ShaderProgram_ptr& shader,
                          RenderStage stage);

    /// Add the specified shader to the specified pass type (for all render stages)
    void setShaderProgram(const stringImpl& shader,
                          RenderPassType passType,
                          const bool computeOnAdd);

    void setShaderProgram(const ShaderProgram_ptr& shader,
                          RenderPassType passType);

    void setShaderProgram(const stringImpl& shader,
                          const bool computeOnAdd);

    void setShaderProgram(const ShaderProgram_ptr& shader);

    void setRenderStateBlock(size_t renderStateBlockHash,
                             I32 variant = -1);

    void setRenderStateBlock(size_t renderStateBlockHash,
                             RenderStage renderStage,
                             I32 variant = -1);

    void setRenderStateBlock(size_t renderStateBlockHash,
                             RenderPassType renderPassType,
                             I32 variant = -1);

    void setRenderStateBlock(size_t renderStateBlockHash,
                             const RenderStagePass& renderStagePass,
                              I32 variant = -1);

    void setParallaxFactor(F32 factor);

    void getSortKeys(const RenderStagePass& renderStagePass, I32& shaderKey, I32& textureKey) const;

    void getMaterialMatrix(mat4<F32>& retMatrix) const;

    F32 getParallaxFactor() const;

    size_t getRenderStateBlock(const RenderStagePass& renderStagePass, I32 variant = 0);

    std::weak_ptr<Texture> getTexture(ShaderProgram::TextureUsage textureUsage) const;

    ShaderProgramInfo& getShaderInfo(const RenderStagePass& renderStagePass);

    const TextureOperation& getTextureOperation() const;

    const ColourData&  getColourData()  const;
    const ShadingMode& getShadingMode() const;
    const BumpMethod&  getBumpMethod()  const;

    void getTextureData(TextureDataContainer& textureData);

    void rebuild();
    void clean();
    void updateTranslucency(bool requestRecomputeShaders = true);
    bool hasTranslucency() const;
    bool hasTransparency() const;

    void dumpToFile(bool state);
    bool isDirty() const;
    bool isDoubleSided() const;
    // Checks if the shader needed for the current stage is already constructed.
    // Returns false if the shader was already ready.
    bool computeShader(const RenderStagePass& renderStagePass, const bool computeOnAdd);

    bool canDraw(const RenderStagePass& renderStagePass);

    void updateReflectionIndex(ReflectorType type, I32 index);
    void updateRefractionIndex(ReflectorType type, I32 index);

    void defaultReflectionTexture(const Texture_ptr& reflectionPtr, U32 arrayIndex);
    void defaultRefractionTexture(const Texture_ptr& reflectionPtr, U32 arrayIndex);

    U32 defaultReflectionTextureIndex() const;
    U32 defaultRefractionTextureIndex() const;

   private:
    void getTextureData(ShaderProgram::TextureUsage slot,
                        TextureDataContainer& container);

    void recomputeShaders();
    void setShaderProgramInternal(const stringImpl& shader,
                                  const RenderStagePass& renderStagePass,
                                  const bool computeOnAdd);

    void setShaderProgramInternal(const ShaderProgram_ptr& shader,
                                  const RenderStagePass& renderStagePass);

    bool isExternalTexture(ShaderProgram::TextureUsage slot) const;

    ShaderProgramInfo& shaderInfo(const RenderStagePass& renderStagePass);

    const ShaderProgramInfo& shaderInfo(const RenderStagePass& renderStagePass) const;

    std::array<size_t, 3>& defaultRenderStates(const RenderStagePass& renderStagePass);

   private:
    GFXDevice& _context;
    ResourceCache& _parentCache;
    ShadingMode _shadingMode;
    /// use for special shader tokens, such as "Tree"
    std::array<stringImpl, to_base(RenderStagePass::count())> _shaderModifier;
    TranslucencySource _translucencySource;
    TranslucencyType _translucencyType;
    /// parallax/relief factor (higher value > more pronounced effect)
    F32 _parallaxFactor;
    bool _dirty;
    bool _dumpToFile;
    bool _translucencyCheck;
    bool _doubleSided;
    /// Use shaders that have bone transforms implemented
    bool _hardwareSkinning;
    std::array<ShaderProgramInfo, to_base(RenderStagePass::count())> _shaderInfo;
    std::array<std::array<size_t, 3>,  to_base(RenderStagePass::count())> _defaultRenderStates;

    bool _shaderThreadedLoad;
    bool _highPriority;
    /// use this map to add textures to the material
    std::array<Texture_ptr, to_base(ShaderProgram::TextureUsage::COUNT)> _textures;
    std::array<bool, to_base(ShaderProgram::TextureUsage::COUNT)> _textureExtenalFlag;

    vector<ExternalTexture> _externalTextures;

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

#endif //_MATERIAL_H_

#include "Material.inl"
