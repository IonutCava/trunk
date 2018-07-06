#include "Headers/PreRenderBatch.h"
#include "Headers/PreRenderOperator.h"

#include "Core/Resources/Headers/ResourceCache.h"

#include "Rendering/PostFX/CustomOperators/Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/DofPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/PostAAPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/SSAOPreRenderOperator.h"

namespace Divide {

namespace {
    inline U32 nextPOW2(U32 n) {
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n++;
        return n;
    }
};

PreRenderBatch::PreRenderBatch() : _adaptiveExposureControl(true),
                                   _debugOperator(nullptr),
                                   _postFXOutput(nullptr),
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
    
    MemoryManager::DELETE(_postFXOutput);
    RemoveResource(_toneMap);
    RemoveResource(_luminanceCalc);
}

void PreRenderBatch::init(Framebuffer* renderTarget) {
    assert(_postFXOutput == nullptr);
    _renderTarget = renderTarget;

    _previousLuminance = GFX_DEVICE.newFB();
    _currentLuminance = GFX_DEVICE.newFB();
    _postFXOutput = GFX_DEVICE.newFB();
    SamplerDescriptor screenSampler;
    screenSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.setFilters(TextureFilter::LINEAR);
    screenSampler.toggleMipMaps(false);
    screenSampler.setAnisotropy(0);
    screenSampler.toggleSRGBColorSpace(true);
    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D,
                                       GFXImageFormat::RGBA8,
                                       GFXDataFormat::UNSIGNED_BYTE);
    outputDescriptor.setSampler(screenSampler);
    //Color0 holds the LDR screen texture
    _postFXOutput->addAttachment(outputDescriptor, TextureDescriptor::AttachmentType::Color0);

    SamplerDescriptor lumaSampler;
    lumaSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    lumaSampler.setMinFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);
    lumaSampler.toggleMipMaps(true);

    TextureDescriptor lumaDescriptor(TextureType::TEXTURE_2D,
                                     GFXImageFormat::RED16F,
                                     GFXDataFormat::FLOAT_16);
    lumaDescriptor.setSampler(lumaSampler);
    _currentLuminance->addAttachment(lumaDescriptor, TextureDescriptor::AttachmentType::Color0);

    lumaSampler.setFilters(TextureFilter::LINEAR);
    lumaSampler.toggleMipMaps(false);
    lumaDescriptor.setSampler(lumaSampler);
    _previousLuminance->addAttachment(lumaDescriptor, TextureDescriptor::AttachmentType::Color0);
    _previousLuminance->setClearColor(DefaultColors::BLACK());

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
        _postFXOutput->getAttachment()->Bind(slot);
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

    if (_adaptiveExposureControl) {
        // Compute Luminance
        // Step 1: Luminance calc
        _renderTarget->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0));
        _previousLuminance->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT1));

        _currentLuminance->begin(Framebuffer::defaultPolicy());
            GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _luminanceCalc);
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
    _renderTarget->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0));

    if (_adaptiveExposureControl) {
        _currentLuminance->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT1));
    }

    _postFXOutput->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true),
                                _adaptiveExposureControl ? _toneMapAdaptive : _toneMap);
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

    _toneMap->Uniform("luminanceMipLevel", _currentLuminance->getAttachment()->getMaxMipLevel());

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