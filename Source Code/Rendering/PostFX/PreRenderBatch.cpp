#include "Headers/PreRenderBatch.h"
#include "Headers/PreRenderOperator.h"

#include "Platform/Video/Headers/GFXDevice.h"

#include "Core/Resources/Headers/ResourceCache.h"

#include "Rendering/PostFX/CustomOperators/Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/DofPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/PostAAPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/SSAOPreRenderOperator.h"

namespace Divide {

PreRenderBatch::PreRenderBatch() : _adaptiveExposureControl(true),
                                   _debugOperator(nullptr),
                                   _renderTarget(nullptr),
                                   _toneMap(nullptr)
{
}

PreRenderBatch::~PreRenderBatch()
{
    destroy();
}

void PreRenderBatch::destroy() {
    for (OperatorBatch& batch : _operators) {
        MemoryManager::DELETE_VECTOR(batch);
        batch.clear();
    }
    
    GFX_DEVICE.deallocateRT(_previousLuminance);
    GFX_DEVICE.deallocateRT(_currentLuminance);
    GFX_DEVICE.deallocateRT(_postFXOutput);
}

void PreRenderBatch::init(RenderTarget* renderTarget) {
    assert(_postFXOutput._targetID._usage == RenderTargetUsage::COUNT);
    _renderTarget = renderTarget;

    _previousLuminance = GFX_DEVICE.allocateRT();
    _currentLuminance = GFX_DEVICE.allocateRT();
    _postFXOutput = GFX_DEVICE.allocateRT();
    SamplerDescriptor screenSampler;
    screenSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.setFilters(TextureFilter::LINEAR);
    screenSampler.toggleMipMaps(false);
    screenSampler.setAnisotropy(0);
    screenSampler.toggleSRGBColourSpace(true);
    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D,
                                       GFXImageFormat::RGBA8,
                                       GFXDataFormat::UNSIGNED_BYTE);
    outputDescriptor.setSampler(screenSampler);
    //Colour0 holds the LDR screen texture
    _postFXOutput._rt->addAttachment(outputDescriptor, RTAttachment::Type::Colour, 0);

    SamplerDescriptor lumaSampler;
    lumaSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    lumaSampler.setMinFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);
    lumaSampler.toggleMipMaps(true);
    TextureDescriptor lumaDescriptor(TextureType::TEXTURE_2D,
                                     GFXImageFormat::RED16F,
                                     GFXDataFormat::FLOAT_16);
    lumaDescriptor.setSampler(lumaSampler);
    lumaDescriptor.toggleAutomaticMipMapGeneration(false);
    _currentLuminance._rt->addAttachment(lumaDescriptor, RTAttachment::Type::Colour, 0);

    lumaSampler.setFilters(TextureFilter::LINEAR);
    lumaSampler.toggleMipMaps(false);
    lumaDescriptor.setSampler(lumaSampler);
    _previousLuminance._rt->addAttachment(lumaDescriptor, RTAttachment::Type::Colour, 0);
    _previousLuminance._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::BLACK());

    // Order is very important!
    OperatorBatch& hdrBatch = _operators[to_const_uint(FilterSpace::FILTER_SPACE_HDR)];
    hdrBatch.push_back(MemoryManager_NEW SSAOPreRenderOperator(_renderTarget, _postFXOutput._rt));
    hdrBatch.push_back(MemoryManager_NEW DoFPreRenderOperator(_renderTarget, _postFXOutput._rt));
    hdrBatch.push_back(MemoryManager_NEW BloomPreRenderOperator(_renderTarget, _postFXOutput._rt));

    OperatorBatch& ldrBatch = _operators[to_const_uint(FilterSpace::FILTER_SPACE_LDR)];
    ldrBatch.push_back(MemoryManager_NEW PostAAPreRenderOperator(_renderTarget, _postFXOutput._rt));

    ResourceDescriptor toneMap("bloom.ToneMap");
    toneMap.setThreadedLoading(false);
    _toneMap = CreateResource<ShaderProgram>(toneMap);

    ResourceDescriptor toneMapAdaptive("bloom.ToneMap.Adaptive");
    toneMapAdaptive.setThreadedLoading(false);
    toneMapAdaptive.setPropertyList("USE_ADAPTIVE_LUMINANCE");
    _toneMapAdaptive = CreateResource<ShaderProgram>(toneMapAdaptive);

    ResourceDescriptor luminanceCalc("bloom.LuminanceCalc");
    luminanceCalc.setThreadedLoading(false);
    _luminanceCalc = CreateResource<ShaderProgram>(luminanceCalc);

    //_debugOperator = hdrBatch[0];
}

void PreRenderBatch::bindOutput(U8 slot) {
    if (_debugOperator != nullptr) {
        _debugOperator->debugPreview(slot);
    } else {
        _postFXOutput._rt->bind(slot, RTAttachment::Type::Colour, 0);
    }
}

void PreRenderBatch::idle() {
    for (OperatorBatch& batch : _operators) {
        for (PreRenderOperator* op : batch) {
            if (op != nullptr) {
                op->idle();
            }
        }
    }
}

void PreRenderBatch::execute(U32 filterMask) {
    OperatorBatch& hdrBatch = _operators[to_const_uint(FilterSpace::FILTER_SPACE_HDR)];
    OperatorBatch& ldrBatch = _operators[to_const_uint(FilterSpace::FILTER_SPACE_LDR)];

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);
    triangleCmd.stateHash(GFX_DEVICE.getDefaultStateBlock(true));

    if (_adaptiveExposureControl) {
        // Compute Luminance
        // Step 1: Luminance calc
        _renderTarget->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);
        _previousLuminance._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT1), RTAttachment::Type::Colour, 0);

        _currentLuminance._rt->begin(RenderTarget::defaultPolicy());
            triangleCmd.shaderProgram(_luminanceCalc);
            GFX_DEVICE.draw(triangleCmd);
        _currentLuminance._rt->end();

        // Use previous luminance to control adaptive exposure
        _previousLuminance._rt->blitFrom(_currentLuminance._rt);
    }

    // Execute all HDR based operators
    for (PreRenderOperator* op : hdrBatch) {
        if (op != nullptr && BitCompare(filterMask, to_uint(op->operatorType()))) {
            op->execute();
        }
    }

    // ToneMap and generate LDR render target (Alpha channel contains pre-toneMapped luminance value)
    _renderTarget->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, 0);

    if (_adaptiveExposureControl) {
        _currentLuminance._rt->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT1), RTAttachment::Type::Colour, 0);
    }

    _postFXOutput._rt->begin(RenderTarget::defaultPolicy());
        triangleCmd.shaderProgram(_adaptiveExposureControl ? _toneMapAdaptive : _toneMap);
        GFX_DEVICE.draw(triangleCmd);
    _postFXOutput._rt->end();

    // Execute all LDR based operators
    for (PreRenderOperator* op : ldrBatch) {
        if (op != nullptr && BitCompare(filterMask, to_uint(op->operatorType()))) {
            op->execute();
        }
    }
}

void PreRenderBatch::reshape(U16 width, U16 height) {
    U16 lumaRez = to_ushort(nextPOW2(to_uint(width / 3.0f)));
    // make the texture square sized and power of two
    _currentLuminance._rt->create(lumaRez, lumaRez);
    _previousLuminance._rt->create(1, 1);

    _toneMap->Uniform("luminanceMipLevel",
                      _currentLuminance
                      ._rt
                      ->getAttachment(RTAttachment::Type::Colour, 0)
                      .asTexture()
                      ->getMaxMipLevel());

    for (OperatorBatch& batch : _operators) {
        for (PreRenderOperator* op : batch) {
            if (op != nullptr) {
                op->reshape(width, height);
            }
        }
    }

    _postFXOutput._rt->create(width, height);
}
};