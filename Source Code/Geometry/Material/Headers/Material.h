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
        ALBEDO,
        OPACITY_MAP_R, //single channel
        OPACITY_MAP_A, //rgba texture
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
        stringImpl _depthShaderVertSource = "baseVertexShaders";
        stringImpl _depthShaderVertVariant = "BasicLightData";

        stringImpl _colourShaderVertSource = "baseVertexShaders";
        stringImpl _colourShaderVertVariant = "BasicLightData";

        stringImpl _depthShaderFragSource = "depthPass";
        stringImpl _depthShaderFragVariant = "";

        stringImpl _colourShaderFragSource = "material";
        stringImpl _colourShaderFragVariant = "";
    };

   public:
    explicit Material(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name);
    ~Material();

    static bool onStartup();
    static bool onShutdown();

    static void ApplyDefaultStateBlocks(Material& target);

    /// Return a new instance of this material with the name composed of the
    /// base material's name and the give name suffix.
    /// clone calls CreateResource internally!)
    Material_ptr clone(const stringImpl& nameSuffix);
    bool unload() noexcept override;
    void update(const U64 deltaTimeUS);

    void setColourData(const ColourData& other);
    void setHardwareSkinning(const bool state);
    void setShadingMode(const ShadingMode& mode);

    void setDoubleSided(const bool state);
    void setReceivesShadows(const bool state);
    void setReflective(const bool state);
    void setRefractive(const bool state);

    void setTextureUseForDepth(ShaderProgram::TextureUsage slot, bool state);

    bool setTexture(ShaderProgram::TextureUsage textureUsageSlot,
                    const Texture_ptr& tex,
                    const TextureOperation& op = TextureOperation::NONE);

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

    void disableTranslucency();

    void getSortKeys(RenderStagePass renderStagePass, I64& shaderKey, I32& textureKey) const;

    void getMaterialMatrix(mat4<F32>& retMatrix) const;

    F32 getParallaxFactor() const;

    size_t getRenderStateBlock(RenderStagePass renderStagePass);

    I64 getProgramID(RenderStagePass renderStagePass) const;

    std::weak_ptr<Texture> getTexture(ShaderProgram::TextureUsage textureUsage) const;

    ShaderProgramInfo& getShaderInfo(RenderStagePass renderStagePass);

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

    // Checks if the shader needed for the current stage is already constructed.
    // Returns false if the shader was already ready.
    bool computeShader(RenderStagePass renderStagePass);

    bool canDraw(RenderStagePass renderStagePass);

    void ignoreXMLData(const bool state);
    bool ignoreXMLData() const;

    void saveToXML(const stringImpl& entryName, boost::property_tree::ptree& pt) const;
    void loadFromXML(const stringImpl& entryName, const boost::property_tree::ptree& pt);

    void setBaseShaderData(const ShaderData& data);
    const ShaderData& getBaseShaderData() const;

    // type == ShaderType::Count = add to all stages
    void addShaderDefine(ShaderType type, const stringImpl& define, bool addPrefix);
    const ModuleDefines& shaderDefines(ShaderType type) const;

   private:
    void addShaderDefineInternal(ShaderType type, const stringImpl& define, bool addPrefix);

    void updateTranslucency();

    bool getTextureDataFast(RenderStagePass renderStagePass, TextureDataContainer& textureData);
    bool getTextureData(ShaderProgram::TextureUsage slot, TextureDataContainer& container, bool force = false);

    void recomputeShaders();
    void setShaderProgramInternal(const ResourceDescriptor& shaderDescriptor,
                                  RenderStagePass renderStagePass,
                                  const bool computeOnAdd);

    void setShaderProgramInternal(const ShaderProgram_ptr& shader,
                                  RenderStagePass renderStagePass);

    ShaderProgramInfo& shaderInfo(RenderStagePass renderStagePass);

    const ShaderProgramInfo& shaderInfo(RenderStagePass renderStagePass) const;

    size_t& defaultRenderState(RenderStagePass renderStagePass);
    std::array<size_t, 3>& defaultRenderStates(RenderStagePass renderStagePass);

    void waitForShader(const ShaderProgram_ptr& shader, RenderStagePass stagePass, const char* newShader);

    const char* getResourceTypeName() const override { return "Material"; }

   private:
    GFXDevice& _context;
    ResourceCache& _parentCache;
    ShadingMode _shadingMode;
    TranslucencySource _translucencySource;
    /// parallax/relief factor (higher value > more pronounced effect)
    F32  _parallaxFactor;
    bool _needsNewShader;
    bool _doubleSided;
    bool _translucent;
    bool _translucencyDisabled;
    bool _receivesShadows;
    bool _isReflective;
    bool _isRefractive;
    bool _ignoreXMLData;
    /// Use shaders that have bone transforms implemented
    bool _hardwareSkinning;
    std::array<ShaderProgramInfo, to_base(RenderStagePass::count())> _shaderInfo;
    std::array<std::array<size_t, 3>,  to_base(RenderStagePass::count())> _defaultRenderStates;

    /// use this map to add textures to the material
    mutable SharedMutex _textureLock;
    std::array<Texture_ptr, to_base(ShaderProgram::TextureUsage::COUNT)> _textures;
    std::array<bool, to_base(ShaderProgram::TextureUsage::COUNT)> _textureExtenalFlag;
    std::array<bool, to_base(ShaderProgram::TextureUsage::COUNT)> _textureUseForDepth;
    ShaderData _baseShaderSources;

    I32 _textureKeyCache = -1;
    /// use the below map to define texture operation
    TextureOperation _operation;
    BumpMethod _bumpMethod;
    ColourData _colourData;
    std::array<ModuleDefines, to_base(ShaderType::COUNT)> _extraShaderDefines;

    static SharedMutex s_shaderDBLock;
    static hashMap<size_t, ShaderProgram_ptr> s_shaderDB;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Material);

const char* getShadingModeName(Material::ShadingMode shadingMode);
Material::ShadingMode getShadingModeByName(const stringImpl& name);

const char* getTexUsageName(ShaderProgram::TextureUsage texUsage);
ShaderProgram::TextureUsage getTexUsageByName(const stringImpl& name);

const char* getTextureOperationName(Material::TextureOperation textureOp);
Material::TextureOperation getTextureOperationByName(const stringImpl& operation);

const char* getBumpMethodName(Material::BumpMethod bumpMethod);
Material::BumpMethod getBumpMethodByName(const stringImpl& name);

const char* getWrapModeName(TextureWrap wrapMode);
TextureWrap getWrapModeByName(const stringImpl& wrapMode);

const char* getFilterName(TextureFilter filter);
TextureFilter getFilterByName(const stringImpl& filter);
};  // namespace Divide

#endif //_MATERIAL_H_

#include "Material.inl"
