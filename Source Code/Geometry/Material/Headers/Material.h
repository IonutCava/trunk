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

#include "MaterialEnums.h"

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

constexpr F32 Specular_Glass = 0.5f;
constexpr F32 Specular_Plastic = 0.5f;
constexpr F32 Specular_Quarts = 0.57f;
constexpr F32 Specular_Ice = 0.224f;
constexpr F32 Specular_Water = 0.255f;
constexpr F32 Specular_Milk = 0.277f;
constexpr F32 Specular_Skin = 0.35f;
constexpr F32 PHONG_REFLECTIVITY_THRESHOLD = 100.0f;


class Material : public CachedResource {
   public:
    /// Since most variants come from different light sources, this seems like a good idea (famous last words ...)
    static const size_t g_maxVariantsPerPass = 3;

    /// ShaderData stores information needed by the shader code to properly shade objects
    struct ColourData {
        ColourData() noexcept
        {
            baseColour(FColour4(VECTOR3_UNIT / 1.5f, 1.0f));
            emissive(DefaultColours::BLACK.rgb());
            specular(DefaultColours::BLACK.rgb());
            shininess(0.0f);
        }

        inline void     baseColour(const FColour4& colour) { _data[0].set(colour); }
        inline FColour4 baseColour() const                 { return _data[0]; }

        inline void     emissive(const FColour3& colour)   { _data[2].rgb(colour); }
        inline FColour3 emissive() const                   { return _data[2].rgb(); }

        //Phong:
        inline void     specular(const FColour3& emissive) { _data[1].rgb(emissive); }
        inline FColour3 specular() const                   { return _data[1].rgb(); }

        inline void shininess(F32 value)                { _data[1].a = value; }
        inline F32  shininess() const                   { return _data[1].a; }

        //Pbr:
        inline void metallic(F32 value) { _data[1].r = CLAMPED_01(value); }
        inline F32  metallic()    const { return _data[1].r; }

        inline void reflectivity(F32 value) { _data[1].g = CLAMPED_01(value); } //specular
        inline F32  reflectivity() const    { return _data[1].g; }  //specular

        inline void roughness(F32 value) { _data[1].b = CLAMPED_01(value); }
        inline F32  roughness() const    { return _data[1].b; }

        vec4<F32> _data[3];
    };

    struct ShaderData {
        Str64 _depthShaderVertSource = "baseVertexShaders";
        Str32 _depthShaderVertVariant = "BasicLightData";
        Str32 _shadowShaderVertVariant = "BasicData";

        Str64 _colourShaderVertSource = "baseVertexShaders";
        Str32 _colourShaderVertVariant = "BasicLightData";

        Str64 _depthShaderFragSource = "depthPass";
        Str32 _depthShaderFragVariant = "";

        Str64 _colourShaderFragSource = "material";
        Str32 _colourShaderFragVariant = "";
    };

   public:
    explicit Material(GFXDevice& context, ResourceCache* parentCache, size_t descriptorHash, const Str128& name);
    ~Material() = default;

    static bool onStartup();
    static bool onShutdown();

    static void ApplyDefaultStateBlocks(Material& target);

    /// Return a new instance of this material with the name composed of the
    /// base material's name and the give name suffix.
    /// clone calls CreateResource internally!)
    Material_ptr clone(const Str128& nameSuffix);
    bool unload() noexcept override;
    /// Returns true if the material changed between update calls
    bool update(const U64 deltaTimeUS);

    void setColourData(const ColourData& other);
    void setHardwareSkinning(const bool state);
    void setShadingMode(const ShadingMode& mode);

    void setDoubleSided(const bool state);
    void setReceivesShadows(const bool state);
    void setReflective(const bool state);
    void setRefractive(const bool state);
    void setStatic(const bool state);

    void setTextureUseForDepth(ShaderProgram::TextureUsage slot, bool state);

    bool setTexture(ShaderProgram::TextureUsage textureUsageSlot,
                    const Texture_ptr& tex,
                    const TextureOperation& op = TextureOperation::NONE);

    /// Set the desired bump mapping method.
    void setBumpMethod(const BumpMethod& newBumpMethod);

    /// Add the specified shader to specific RenderStagePass parameters. Use "COUNT" or "0" for global options
    /// e.g. a RenderPassType::COUNT will use the shader in the specified stage+variant combo but for all of the passes
    void setShaderProgram(const ShaderProgram_ptr& shader, RenderStage stage, RenderPassType pass, U8 variant);

    /// Add the specified renderStateBlockHash to specific RenderStagePass parameters. Use "COUNT" or "0" for global options
    /// e.g. a RenderPassType::COUNT will use the block in the specified stage+variant combo but for all of the passes
    void setRenderStateBlock(size_t renderStateBlockHash, RenderStage stage, RenderPassType pass, U8 variant);

    void setParallaxFactor(F32 factor);

    void disableTranslucency();

    void getSortKeys(RenderStagePass renderStagePass, I64& shaderKey, I32& textureKey) const;

    void getMaterialMatrix(mat4<F32>& retMatrix) const;

    F32 getParallaxFactor() const;

    size_t getRenderStateBlock(RenderStagePass renderStagePass);

    I64 getProgramGUID(RenderStagePass renderStagePass) const;

    eastl::weak_ptr<Texture> getTexture(ShaderProgram::TextureUsage textureUsage) const;

    const TextureOperation& getTextureOperation() const;

          ColourData&  getColourData();
    const ColourData&  getColourData()  const;
    const ShadingMode& getShadingMode() const;
    const BumpMethod&  getBumpMethod()  const;

    bool getTextureData(RenderStagePass renderStagePass, TextureDataContainer& textureData);

    void rebuild();
    bool hasTransparency() const;
    bool hasTranslucency() const;

    bool isPBRMaterial() const;
    bool isDoubleSided() const;
    bool receivesShadows() const;
    bool isReflective() const;
    bool isRefractive() const;

    bool canDraw(RenderStagePass renderStagePass);

    void saveToXML(const stringImpl& entryName, boost::property_tree::ptree& pt) const;
    void loadFromXML(const stringImpl& entryName, const boost::property_tree::ptree& pt);

    // type == ShaderType::Count = add to all stages
    void addShaderDefine(ShaderType type, const Str128& define, bool addPrefix);
    const ModuleDefines& shaderDefines(ShaderType type) const;

   private:
    /// Checks if the shader needed for the current stage is already constructed.
    /// Returns false if the shader was already ready.
    bool computeShader(RenderStagePass renderStagePass);

    void addShaderDefineInternal(ShaderType type, const Str128& define, bool addPrefix);

    void updateTranslucency();

    bool getTextureDataFast(RenderStagePass renderStagePass, TextureDataContainer& textureData);
    bool getTextureData(ShaderProgram::TextureUsage slot, TextureDataContainer& container, bool force = false);

    void recomputeShaders();
    void setShaderProgramInternal(const ResourceDescriptor& shaderDescriptor,
                                  const RenderStagePass& stage,
                                  const bool computeOnAdd);

    void setShaderProgramInternal(const ShaderProgram_ptr& shader,
                                  ShaderProgramInfo& shaderInfo,
                                  RenderStage stage,
                                  RenderPassType pass);

    ShaderProgramInfo& shaderInfo(RenderStagePass renderStagePass);

    const ShaderProgramInfo& shaderInfo(RenderStagePass renderStagePass) const;

    const char* getResourceTypeName() const override { return "Material"; }

    PROPERTY_RW(bool, ignoreXMLData, false);
    PROPERTY_RW(ShaderData, baseShaderData);

   private:
    GFXDevice& _context;
    ResourceCache* _parentCache = nullptr;

    struct Properties {
        ColourData _colourData;
        /// parallax/relief factor (higher value > more pronounced effect)
        F32  _parallaxFactor = 1.0f;
        ShadingMode _shadingMode = ShadingMode::COUNT;
        TranslucencySource _translucencySource = TranslucencySource::COUNT;
        /// use the below map to define texture operation
        TextureOperation _operation = TextureOperation::NONE;
        BumpMethod _bumpMethod = BumpMethod::NONE;
        bool _doubleSided = false;
        bool _translucent = false;
        bool _translucencyDisabled = false;
        bool _receivesShadows = true;
        bool _isReflective = false;
        bool _isRefractive = false;
        bool _isStatic = false;
        /// Use shaders that have bone transforms implemented
        bool _hardwareSkinning = false;
    } _properties;

    bool _needsNewShader;

    using ShaderPerVariant = std::array<ShaderProgramInfo, g_maxVariantsPerPass>;
    using ShaderVariantsPerPass = eastl::array<ShaderPerVariant, to_base(RenderPassType::COUNT)>;
    using ShaderPassesPerStage = eastl::array<ShaderVariantsPerPass, to_base(RenderStage::COUNT)>;
    ShaderPassesPerStage _shaderInfo;

    using StatesPerVariant = std::array<size_t, g_maxVariantsPerPass>;
    using StateVariantsPerPass = eastl::array<StatesPerVariant, to_base(RenderPassType::COUNT)>;
    using StatePassesPerStage = eastl::array<StateVariantsPerPass, to_base(RenderStage::COUNT)>;
    StatePassesPerStage _defaultRenderStates;

    /// use this map to add textures to the material
    mutable SharedMutex _textureLock;
    std::array<Texture_ptr, to_base(ShaderProgram::TextureUsage::COUNT)> _textures;
    std::array<bool, to_base(ShaderProgram::TextureUsage::COUNT)> _textureUseForDepth;

    I32 _textureKeyCache = -1;
    std::array<ModuleDefines, to_base(ShaderType::COUNT)> _extraShaderDefines;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Material);

const char* getShadingModeName(ShadingMode shadingMode) noexcept;
ShadingMode getShadingModeByName(const stringImpl& name);

const char* getTexUsageName(ShaderProgram::TextureUsage texUsage) noexcept;
ShaderProgram::TextureUsage getTexUsageByName(const stringImpl& name);

const char* getTextureOperationName(TextureOperation textureOp) noexcept;
TextureOperation getTextureOperationByName(const stringImpl& operation);

const char* getBumpMethodName(BumpMethod bumpMethod) noexcept;
BumpMethod getBumpMethodByName(const stringImpl& name);

const char* getWrapModeName(TextureWrap wrapMode) noexcept;
TextureWrap getWrapModeByName(const stringImpl& wrapMode);

const char* getFilterName(TextureFilter filter) noexcept;
TextureFilter getFilterByName(const stringImpl& filter);
};  // namespace Divide

#endif //_MATERIAL_H_

#include "Material.inl"
