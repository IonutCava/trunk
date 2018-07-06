#include "Headers/Reflector.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

Reflector::Reflector(ReflectorType type, const vec2<U16>& resolution)
    : FrameListener(),
      _type(type),
      _resolution(resolution),
      _updateTimer(0),
      _updateInterval(45),  /// 45 milliseconds?
      _reflectedTexture(nullptr),
      _createdFB(false),
      _updateSelf(false),
      _excludeSelfReflection(true),
      _previewReflection(false) {
    REGISTER_FRAME_LISTENER(this, 3);
    ResourceDescriptor reflectionPreviewShader("fbPreview");
    reflectionPreviewShader.setThreadedLoading(false);
    _previewReflectionShader =
        CreateResource<ShaderProgram>(reflectionPreviewShader);

    GFX_DEVICE.add2DRenderFunction(
        DELEGATE_BIND(&Reflector::previewReflection, this), 2);
}

Reflector::~Reflector() {
    UNREGISTER_FRAME_LISTENER(this);
    MemoryManager::DELETE(_reflectedTexture);
    RemoveResource(_previewReflectionShader);
}

/// Returning false in any of the FrameListener methods will exit the entire
/// application!
bool Reflector::framePreRenderEnded(const FrameEvent& evt) {
    // Do not update the reflection too often so that we can improve speed
    if (evt._currentTime - _updateTimer < _updateInterval) {
        return true;
    }
    _updateTimer += _updateInterval;
    if (!_createdFB) {
        if (!build()) {
            // Something wrong. Exit application!
            Console::errorfn(Locale::get("ERROR_REFLECTOR_INIT_FB"));
            return false;
        }
    }
    // We should never have an invalid FB
    assert(_reflectedTexture != nullptr);
    // mark ourselves as reflection target only if we do not wish to reflect
    // ourself back
    _updateSelf = !_excludeSelfReflection;
    // generate reflection texture
    updateReflection();
    // unmark from reflection target
    _updateSelf = true;
    return true;
}

bool Reflector::build() {
    Console::printfn(Locale::get("REFLECTOR_INIT_FB"), _resolution.x,
                     _resolution.y);
    SamplerDescriptor reflectionSampler;
    reflectionSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    reflectionSampler.toggleMipMaps(false);
    // Less precision for reflections
    TextureDescriptor reflectionDescriptor(TextureType::TEXTURE_2D,
                                           GFXImageFormat::RGBA8,
                                           GFXDataFormat::UNSIGNED_BYTE);

    reflectionDescriptor.setSampler(reflectionSampler);

    _reflectedTexture = GFX_DEVICE.newFB();
    _reflectedTexture->AddAttachment(reflectionDescriptor,
                                     TextureDescriptor::AttachmentType::Color0);
    _reflectedTexture->toggleDepthBuffer(true);
    _createdFB = _reflectedTexture->Create(_resolution.x, _resolution.y);

    return _createdFB;
}

void Reflector::previewReflection() {
#ifdef _DEBUG
    if (_previewReflection) {
        F32 height = _resolution.y * 0.333f;
        _reflectedTexture->Bind(static_cast<U8>(ShaderProgram::TextureUsage::UNIT0));
        GFX::ScopedViewport viewport(0,
                                     to_int(Application::getInstance().getResolution().y - height),
                                     to_int(_resolution.x * 0.333f),
                                     to_int(height));
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true), _previewReflectionShader);
    }
#endif
}
};