#include "Headers/PreRenderBatch.h"
#include "Headers/PreRenderOperator.h"

#include "Core/Resources/Headers/ResourceCache.h"

#include "Rendering/PostFX/CustomOperators/Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/DofPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/PostAAPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/SSAOPreRenderOperator.h"

namespace Divide {

PreRenderBatch::PreRenderBatch() : _adaptiveExposureControl(true),
                                   _debugOperator(nullptr),
                                   _postFXOutput(nullptr),
                                   _renderTarget(nullptr),
                                   _toneMap(nullptr),
                                   _previousLuminance(nullptr),
                                   _currentLuminance(nullptr)
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
    
    MemoryManager::DELETE(_previousLuminance);
    MemoryManager::DELETE(_currentLuminance);
    MemoryManager::DELETE(_postFXOutput);
}

void PreRenderBatch::init(RenderTarget* renderTarget) {
    assert(_postFXOutput == nullptr);
    _renderTarget = renderTarget;

    _previousLuminance = GFX_DEVICE.newRT();
    _currentLuminance = GFX_DEVICE.newRT();
    _postFXOutput = GFX_DEVICE.newRT();
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
    _postFXOutput->addAttachment(outputDescriptor, RTAttachment::Type::Colour, 0);

    SamplerDescriptor lumaSampler;
    lumaSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    lumaSampler.setMinFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);
    lumaSampler.toggleMipMaps(true);

    TextureDescriptor lumaDescriptor(TextureType::TEXTURE_2D,
                                     GFXImageFormat::RED16F,
                                     GFXDataFormat::FLOAT_16);
    lumaDescriptor.setSampler(lumaSampler);
    _currentLuminance->addAttachment(lumaDescriptor, RTAttachment::Type::Colour, 0);

    lumaSampler.setFilters(TextureFilter::LINEAR);
    lumaSampler.toggleMipMaps(false);
    lumaDescriptor.setSampler(lumaSampler);
    _previousLuminance->addAttachment(lumaDescriptor, RTAttachment::Type::Colour, 0);
    _previousLuminance->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::BLACK());

    // Order is very important!
    OperatorBatch& hdrBatch = _operators[to_const_uint(FilterSpace::FILTER_SPACE_HDR)];
    hdrBatch.push_back(MemoryManager_NEW SSAOPreRenderOperator(_renderTarget, _postFXOutput));
    hdrBatch.push_back(MemoryManager_NEW DoFPreRenderOperator(_renderTarget, _postFXOutput));
    hdrBatch.push_back(MemoryManager_NEW BloomPreRenderOperator(_renderTarget, _postFXOutput));

    OperatorBatch& ldrBatch = _operators[to_const_uint(FilterSpace::FILTER_SPACE_LDR)];
    ldrBatch.push_back(MemoryManager_NEW PostAAPreRenderOperator(_renderTarget, _postFXOutput));

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
        _postFXOutput->bind(slot, RTAttachment::Type::Colour, 0);
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
        _previousLuminance->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT1), RTAttachment::Type::Colour, 0);

        _currentLuminance->begin(RenderTarget::defaultPolicy());
            triangleCmd.shaderProgram(_luminanceCalc);
            GFX_DEVICE.draw(triangleCmd);
        _currentLuminance->end();

        // Use previous luminance to control adaptive exposure
        _previousLuminance->blitFrom(_currentLuminance);
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
        _currentLuminance->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT1), RTAttachment::Type::Colour, 0);
    }

    _postFXOutput->begin(RenderTarget::defaultPolicy());
        triangleCmd.shaderProgram(_adaptiveExposureControl ? _toneMapAdaptive : _toneMap);
        GFX_DEVICE.draw(triangleCmd);
    _postFXOutput->end();

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
    _currentLuminance->create(lumaRez, lumaRez);
    _previousLuminance->create(1, 1);

    _toneMap->Uniform("luminanceMipLevel",
                      _currentLuminance
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

    _postFXOutput->create(width, height);
}
};