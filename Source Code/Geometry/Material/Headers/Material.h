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

        ColourData() noexcept
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

    static bool onStartup();
    static bool onShutdown();

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

    void useTriangleStrip(const bool state);
    void setDoubleSided(const bool state);
    void setReflective(const bool state);
    void setRefractive(const bool state);

    bool setTexture(ShaderProgram::TextureUsage textureUsageSlot,
                    const Texture_ptr& tex,
                    const TextureOperation& op = TextureOperation::NONE);
    /// Add a texture <-> bind slot pair to be bound with the default textures
    /// on each "bindTexture" call
    void addExternalTexture(const Texture_ptr& texture, U8 slot, bool activeForDepth = false);

    /// Remove the custom texture assigned to the specified offset
    bool removeCustomTexture(U8 bindslot);

    /// Set the desired bump mapping method.
    void setBumpMethod(const BumpMethod& newBumpMethod);

    /// Add the specified shader to all of the available stage passes
    void setShaderProgram(const ShaderProgram_ptr& shader);
    /// Add the specified shader to the specified Stage Pass (stage and pass type)
    void setShaderProgram(const ShaderProgram_ptr& shader, RenderStagePass renderStagePass);
    /// Add the specified shader to the specified render stage (for all pass types)
    void setShaderProgram(const ShaderProgram_ptr& shader, RenderStage stage);
    /// Add the specified shader to the specified pass type (for all render stages)
    void setShaderProgram(const ShaderProgram_ptr& shader, RenderPassType passType);

    void setRenderStateBlock(size_t renderStateBlockHash, I32 variant = -1);
    void setRenderStateBlock(size_t renderStateBlockHash, RenderStage renderStage, I32 variant = -1);
    void setRenderStateBlock(size_t renderStateBlockHash, RenderPassType renderPassType, I32 variant = -1);
    void setRenderStateBlock(size_t renderStateBlockHash, RenderStagePass renderStagePass, I32 variant = -1);

    void setParallaxFactor(F32 factor);

    void getSortKeys(RenderStagePass renderStagePass, I32& shaderKey, I32& textureKey) const;

    void getMaterialMatrix(mat4<F32>& retMatrix) const;

    F32 getParallaxFactor() const;

    size_t getRenderStateBlock(RenderStagePass renderStagePass);

    U32 getProgramID(RenderStagePass renderStagePass) const;

    std::weak_ptr<Texture> getTexture(ShaderProgram::TextureUsage textureUsage) const;

    ShaderProgramInfo& getShaderInfo(RenderStagePass renderStagePass);

    const TextureOperation& getTextureOperation() const;

    const ColourData&  getColourData()  const;
    const ShadingMode& getShadingMode() const;
    const BumpMethod&  getBumpMethod()  const;

    bool getTextureData(RenderStagePass renderStagePass, TextureDataContainer& textureData);

    void rebuild();
    bool hasTransparency() const;

    bool isDoubleSided() const;
    bool isReflective() const;
    bool isRefractive() const;
    bool useTriangleStrip() const;

    // Checks if the shader needed for the current stage is already constructed.
    // Returns false if the shader was already ready.
    bool computeShader(RenderStagePass renderStagePass);

    bool canDraw(RenderStagePass renderStagePass);

    void updateReflectionIndex(ReflectorType type, I32 index);
    void updateRefractionIndex(ReflectorType type, I32 index);

    void defaultReflectionTexture(const Texture_ptr& reflectionPtr, U32 arrayIndex);
    void defaultRefractionTexture(const Texture_ptr& reflectionPtr, U32 arrayIndex);

    U32 defaultReflectionTextureIndex() const;
    U32 defaultRefractionTextureIndex() const;


    void ignoreXMLData(const bool state);
    bool ignoreXMLData() const;

    void saveToXML(const stringImpl& entryName, boost::property_tree::ptree& pt) const;
    void loadFromXML(const stringImpl& entryName, const boost::property_tree::ptree& pt);

   private:
    void updateTranslucency();

    bool getTextureData(ShaderProgram::TextureUsage slot, TextureDataContainer& container);

    void recomputeShaders();
    void setShaderProgramInternal(const ResourceDescriptor& shaderDescriptor,
                                  RenderStagePass renderStagePass,
                                  const bool computeOnAdd);

    void setShaderProgramInternal(const ShaderProgram_ptr& shader,
                                  RenderStagePass renderStagePass);

    bool isExternalTexture(ShaderProgram::TextureUsage slot) const;

    ShaderProgramInfo& shaderInfo(RenderStagePass renderStagePass);

    const ShaderProgramInfo& shaderInfo(RenderStagePass renderStagePass) const;

    size_t& defaultRenderState(RenderStagePass renderStagePass);
    std::array<size_t, 3>& defaultRenderStates(RenderStagePass renderStagePass);

    void waitForShader(const ShaderProgram_ptr& shader, RenderStagePass stagePass, const char* newShader);
   private:
    GFXDevice& _context;
    ResourceCache& _parentCache;
    ShadingMode _shadingMode;
    TranslucencySource _translucencySource;
    /// parallax/relief factor (higher value > more pronounced effect)
    F32 _parallaxFactor;
    bool _useTriangleStrip;
    bool _needsNewShader;
    bool _doubleSided;
    bool _isReflective;
    bool _isRefractive;
    bool _ignoreXMLData;
    /// Use shaders that have bone transforms implemented
    bool _hardwareSkinning;
    std::array<ShaderProgramInfo, to_base(RenderStagePass::count())> _shaderInfo;
    std::array<std::array<size_t, 3>,  to_base(RenderStagePass::count())> _defaultRenderStates;

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

    static SharedMutex s_shaderDBLock;
    static hashMap<size_t, ShaderProgram_ptr> s_shaderDB;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Material);

};  // namespace Divide

#endif //_MATERIAL_H_

#include "Material.inl"
