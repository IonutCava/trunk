#include "stdafx.h"

#include "config.h"

#include "Headers/RenderingComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

void RenderingComponent::toggleRenderOption(RenderOptions option, bool state, bool recursive) {
    if (renderOptionEnabled(option) != state) {
        if (recursive) {
            _parentSGN.forEachChild([option, state, recursive](const SceneGraphNode* child, I32 /*childIdx*/) {
                RenderingComponent* const renderable = child->get<RenderingComponent>();
                if (renderable) {
                    renderable->toggleRenderOption(option, state, recursive);
                }
                return true;
            });
        }

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

void RenderingComponent::toggleBoundsDraw(bool drawAABB, bool drawBS, bool recursive) {
    if (recursive) {
        _parentSGN.forEachChild([drawAABB, drawBS, recursive](const SceneGraphNode* child, I32 /*childIdx*/) {
            RenderingComponent* const renderable = child->get<RenderingComponent>();
            if (renderable) {
                renderable->toggleBoundsDraw(drawAABB, drawBS, recursive);
            }
            return true;
        });
    }
    _drawAABB = drawAABB;
    _drawBS = drawBS;
}
}; //namespace Divide