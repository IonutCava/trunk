#include "stdafx.h"

#include "Headers/SceneEnvironmentProbePool.h"
#include "Scenes/Headers/Scene.h"
#include "Core/Headers/PlatformContext.h"

#include "ECS/Components/Headers/EnvironmentProbeComponent.h"

namespace Divide {

vectorEASTL<DebugView_ptr> SceneEnvironmentProbePool::s_debugViews;

SceneEnvironmentProbePool::SceneEnvironmentProbePool(Scene& parentScene)
    : SceneComponent(parentScene)
{
}

const EnvironmentProbeList& SceneEnvironmentProbePool::sortAndGetLocked(const vec3<F32>& position) {
    eastl::sort(eastl::begin(_envProbes),
                eastl::end(_envProbes),
                [&position](const auto& a, const auto& b) -> bool {
                    return a->distanceSqTo(position) < b->distanceSqTo(position);
                });

    return _envProbes;
}

void SceneEnvironmentProbePool::prepare(GFX::CommandBuffer& bufferInOut) {
    EnvironmentProbeComponent::prepare(bufferInOut);
}

void SceneEnvironmentProbePool::lockProbeList() {
    _probeLock.lock();
}

void SceneEnvironmentProbePool::unlockProbeList() {
    _probeLock.unlock();
}

const EnvironmentProbeList& SceneEnvironmentProbePool::getLocked() {
    return _envProbes;
}

EnvironmentProbeComponent* SceneEnvironmentProbePool::registerProbe(EnvironmentProbeComponent* probe) {
    UniqueLock<SharedMutex> w_lock(_probeLock);
    _envProbes.emplace_back(probe);
    return probe;
}

void SceneEnvironmentProbePool::unregisterProbe(EnvironmentProbeComponent* probe) {
    if (probe != nullptr) {
        UniqueLock<SharedMutex> w_lock(_probeLock);
        I64 probeGUID = probe->getGUID();
        _envProbes.erase(eastl::remove_if(eastl::begin(_envProbes), eastl::end(_envProbes),
                                       [&probeGUID](const auto& probe)
                                            -> bool { return probe->getGUID() == probeGUID; }),
                         eastl::end(_envProbes));
    }
    if (probe == _debugProbe) {
        debugProbe(nullptr);
    }
}

void SceneEnvironmentProbePool::debugProbe(EnvironmentProbeComponent* probe) {
    _debugProbe = probe;
    // Remove old views
    if (!s_debugViews.empty()) {
        for (const DebugView_ptr& view : s_debugViews) {
            parentScene().context().gfx().removeDebugView(view.get());
        }
        s_debugViews.clear();
    }

    // Add new views if needed
    if (probe != nullptr) {
        ShaderModuleDescriptor vertModule = {};
        vertModule._moduleType = ShaderType::VERTEX;
        vertModule._sourceFile = "baseVertexShaders.glsl";
        vertModule._variant = "FullScreenQuad";

        ShaderModuleDescriptor fragModule = {};
        fragModule._moduleType = ShaderType::FRAGMENT;
        fragModule._sourceFile = "fbPreview.glsl";
        fragModule._variant = "Cube";

        ShaderProgramDescriptor shaderDescriptor = {};
        shaderDescriptor._modules.push_back(vertModule);
        shaderDescriptor._modules.push_back(fragModule);

        ResourceDescriptor shadowPreviewShader("fbPreview.Cube");
        shadowPreviewShader.propertyDescriptor(shaderDescriptor);
        shadowPreviewShader.threaded(false);
        ShaderProgram_ptr previewShader = CreateResource<ShaderProgram>(parentScene().resourceCache(), shadowPreviewShader);
        for (U32 i = 0; i < 6; ++i) {
            DebugView_ptr shadow = std::make_shared<DebugView>(to_I16((std::numeric_limits<I16>::max() - 1) - 6 + i));
            shadow->_texture = probe->reflectionTarget()._rt->getAttachment(RTAttachmentType::Colour, 0).texture();
            shadow->_shader = previewShader;
            shadow->_shaderData.set(_ID("layer"), GFX::PushConstantType::INT, probe->rtLayerIndex());
            shadow->_shaderData.set(_ID("face"), GFX::PushConstantType::INT, i);
            shadow->_name = Util::StringFormat("CubeProbe_%d_face_%d", probe->rtLayerIndex(), i);
            s_debugViews.push_back(shadow);
        }
    };
    for (const DebugView_ptr& view : s_debugViews) {
        parentScene().context().gfx().addDebugView(view);
    }

}
}; //namespace Divide