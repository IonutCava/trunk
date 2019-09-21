#include "stdafx.h"

#include "config.h"

#include "Headers/RenderingComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

void RenderingComponent::toggleRenderOption(RenderOptions option, bool state) {
    if (renderOptionEnabled(option) != state) {
        _parentSGN.forEachChild([option, state](const SceneGraphNode* child) {
            RenderingComponent* const renderable = child->get<RenderingComponent>();
            if (renderable) {
                renderable->toggleRenderOption(option, state);
            }
        });

        if (state) {
            SetBit(_renderMask, to_U32(option));
        } else {
            ClearBit(_renderMask, to_U32(option));
        }
    }
}

bool RenderingComponent::renderOptionEnabled(RenderOptions option) const {
    return renderOptionsEnabled(to_U32(option));
}

bool RenderingComponent::renderOptionsEnabled(U32 mask) const{
    return BitCompare(_renderMask, mask);
}

}; //namespace Divide