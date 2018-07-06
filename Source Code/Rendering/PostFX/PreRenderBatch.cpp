#include "Headers/PreRenderBatch.h"
#include "Headers/PreRenderOperator.h"

#include "Rendering/PostFX/CustomOperators/Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/DofPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/PostAAPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/SSAOPreRenderOperator.h"

namespace Divide {

PreRenderBatch::PreRenderBatch() : _debugOperator(nullptr),
                                   _postFXOutput(nullptr),
                                   _renderTarget(nullptr)
{
    _operators.fill(nullptr);
}

PreRenderBatch::~PreRenderBatch()
{
    destroy();
}

void PreRenderBatch::destroy() {
    for (PreRenderOperator*& op : _operators) {
        MemoryManager::DELETE_CHECK(op);
    }
    _operators.fill(nullptr);
    MemoryManager::DELETE(_postFXOutput);
}

void PreRenderBatch::init(Framebuffer* renderTarget) {
    assert(_postFXOutput == nullptr);
    _renderTarget = renderTarget;

    _postFXOutput = GFX_DEVICE.newFB();
    SamplerDescriptor screenSampler;
    screenSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.setFilters(TextureFilter::NEAREST);
    screenSampler.toggleMipMaps(false);
    screenSampler.setAnisotropy(0);
    TextureDescriptor outputDescriptor(TextureType::TEXTURE_2D,
                                       GFXImageFormat::RGB8,
                                       GFXDataFormat::UNSIGNED_BYTE);
    outputDescriptor.setSampler(screenSampler);
    //Color0 holds the LDR screen texture
    _postFXOutput->addAttachment(outputDescriptor, TextureDescriptor::AttachmentType::Color0);

    // HDR input -> HDR output
    _operators[getOperatorIndex(FilterType::FILTER_SS_ANTIALIASING)]
        = MemoryManager_NEW PostAAPreRenderOperator(_renderTarget);

    // HDR input -> HDR output
    _operators[getOperatorIndex(FilterType::FILTER_SS_AMBIENT_OCCLUSION)]
        = MemoryManager_NEW SSAOPreRenderOperator(_renderTarget);

    // HDR input -> HDR output
    _operators[getOperatorIndex(FilterType::FILTER_DEPTH_OF_FIELD)]
        = MemoryManager_NEW DoFPreRenderOperator(_renderTarget);

    // HDR input -> LDR output
    _operators[getOperatorIndex(FilterType::FILTER_BLOOM_TONEMAP)]
        = MemoryManager_NEW BloomPreRenderOperator(_renderTarget);

    //_debugOperator = _operators[getOperatorIndex(FilterType::FILTER_SS_AMBIENT_OCCLUSION)];
}

void PreRenderBatch::bindOutput(U8 slot) {
    if (_debugOperator != nullptr) {
        _debugOperator->debugPreview(slot);
    } else {
        _postFXOutput->getAttachment(TextureDescriptor::AttachmentType::Color0)->Bind(slot);
    }
}

void PreRenderBatch::idle() {
    for (PreRenderOperator* op : _operators) {
        if (op != nullptr) {
            op->idle();
        }
    }
}

void PreRenderBatch::execute(U32 filterMask) {
    for (PreRenderOperator* op : _operators) {
        if (op != nullptr && BitCompare(filterMask, to_uint(op->operatorType()))) {
            op->execute();
        }
    }
    _postFXOutput->blitFrom(_renderTarget);
}

void PreRenderBatch::reshape(U16 width, U16 height) {
    for (PreRenderOperator* op : _operators) {
        if (op != nullptr) {
            op->reshape(width, height);
        }
    }

    _postFXOutput->create(width, height);
}
};