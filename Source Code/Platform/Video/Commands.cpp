#include "stdafx.h"
#include "Headers/Commands.h"

#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {
namespace GFX {

namespace {
    const char* primitiveTypeToString(PrimitiveType type) {
        switch (type) {
            case PrimitiveType::API_POINTS: return "POINTS";
            case PrimitiveType::LINES: return "LINES";
            case PrimitiveType::LINE_LOOP: return "LINE_LOOP";
            case PrimitiveType::LINE_STRIP: return "LINE_STRIP";
            case PrimitiveType::PATCH: return "PATCH";
            case PrimitiveType::POLYGON: return "POLYGON";
            case PrimitiveType::QUAD_STRIP: return "QUAD_STRIP";
            case PrimitiveType::TRIANGLES: return "TRIANGLES";
            case PrimitiveType::TRIANGLE_FAN: return "TRIANGLE_FAN";
            case PrimitiveType::TRIANGLE_STRIP: return "TRIANGLE_STRIP";
        };

        return "ERROR";
    }
};

stringImpl BindPipelineCommand::toString(U16 indent) const {
    assert(_pipeline != nullptr);

    stringImpl ret = Base::toString(indent) + "\n";
    ret.append("    ");
    for (U16 j = 0; j < indent; ++j) {
        ret.append("    ");
    }
    ret.append(Util::StringFormat("Shader handle : %d\n",_pipeline->shaderProgramHandle()));
    ret.append("    ");
    for (U16 j = 0; j < indent; ++j) {
        ret.append("    ");
    }
    ret.append(Util::StringFormat("State hash : %zu\n", _pipeline->stateHash()));

    return ret;
}

stringImpl SendPushConstantsCommand::toString(U16 indent) const {
    stringImpl ret = Base::toString(indent) + "\n";

    for (auto it : _constants.data()) {
        ret.append("    ");
        for (U16 j = 0; j < indent; ++j) {
            ret.append("    ");
        }
        ret.append(Util::StringFormat("Constant binding: %s\n", it._binding.c_str()));
    }

    return ret;
}

stringImpl DrawCommand::toString(U16 indent) const {
    stringImpl ret = Base::toString(indent);
    ret.append("\n");
    size_t i = 0;
    for (const GenericDrawCommand& cmd : _drawCommands) {
        ret.append("    ");
        for (U16 j = 0; j < indent; ++j) {
            ret.append("    ");
        }
        ret.append(Util::StringFormat("%d: Draw count: %d Type: %s Base instance: %d Instance count: %d Index count: %d\n", i++, cmd._drawCount, primitiveTypeToString(cmd._primitiveType), cmd._cmd.baseInstance, cmd._cmd.primCount, cmd._cmd.indexCount));
    }

    return ret;
}

stringImpl SetViewportCommand::toString(U16 indent) const {
    return Base::toString(indent) + Util::StringFormat(" [%d, %d, %d, %d]", _viewport.x, _viewport.y, _viewport.z, _viewport.w);
}

stringImpl BeginRenderPassCommand::toString(U16 indent) const {
    return Base::toString(indent) + " [ " + stringImpl(_name.c_str()) + " ]";
}
stringImpl SetScissorCommand::toString(U16 indent) const {
    return Base::toString(indent) + Util::StringFormat(" [%d, %d, %d, %d]", _rect.x, _rect.y, _rect.z, _rect.w);
}

stringImpl SetClipPlanesCommand::toString(U16 indent) const {
    stringImpl ret = Base::toString(indent) + "\n";
    for (U8 i = 0; i < _clippingPlanes._planes.size(); ++i) {
        ret.append("    ");
        for (U16 j = 0; j < indent; ++j) {
            ret.append("    ");
        }

        const vec4<F32>& eq = _clippingPlanes._planes[i].getEquation();

        ret.append(Util::StringFormat("Plane [%d] [ %5.2f %5.2f %5.2f - %5.2f ]\n", i, eq.x, eq.y, eq.z, eq.w));
    }

    return ret;
}

stringImpl BindDescriptorSetsCommand::toString(U16 indent) const {
    stringImpl ret = Base::toString(indent);

    ret.append(Util::StringFormat(" [ Buffers: %d, Textures: %d ]\n", _set._shaderBuffers.size(), _set._textureData.textures().size()));
    for (auto it : _set._shaderBuffers) {
        ret.append("    ");
        for (U16 j = 0; j < indent; ++j) {
            ret.append("    ");
        }
        ret.append(Util::StringFormat("Buffer [ %d - %d ] Range [%d - %d] ]\n", to_base(it._binding), it._buffer->getGUID(), it._elementRange.x, it._elementRange.y));
    }
    for (auto it : _set._textureData.textures()) {
        ret.append("    ");
        for (U16 j = 0; j < indent; ++j) {
            ret.append("    ");
        }
        ret.append(Util::StringFormat("Texture [ %d - %d ]\n", it.first, it.second._textureHandle));
    }

    for (auto it : _set._textureViews) {
        ret.append("    ");
        for (U16 j = 0; j < indent; ++j) {
            ret.append("    ");
        }
        ret.append(Util::StringFormat("Texture layers [ %d - [%d - %d ]]\n", it._binding, it._view._layerRange.min, it._view._layerRange.max));
    }

    for (auto it : _set._images) {
        ret.append("    ");
        for (U16 j = 0; j < indent; ++j) {
            ret.append("    ");
        }
        ret.append(Util::StringFormat("Image binds: [ %d - [%d - %d - %s]", it._binding, it._layer, it._level, it._flag == Image::Flag::READ ? "READ" : it._flag == Image::Flag::WRITE ? "WRITE" : "READ_WRITE"));
    }
    return ret;
}

stringImpl BeginDebugScopeCommand::toString(U16 indent) const {
    return Base::toString(indent) + " [ " + stringImpl(_scopeName.c_str()) + " ]";
}

stringImpl DrawTextCommand::toString(U16 indent) const {
    stringImpl ret = Base::toString(indent);
    ret.append("\n");
    size_t i = 0;
    for (const TextElement& element : _batch()) {
        ret.append("    ");
        for (U16 j = 0; j < indent; ++j) {
            ret.append("    ");
        }
        stringImpl string = "";
        for (auto it : element.text()) {
            string.append(it.c_str());
            string.append("\n");
        }
        ret.append(Util::StringFormat("%d: Text: [ %s ]", i++, string.c_str()));
    }
    return ret;
}

stringImpl DispatchComputeCommand::toString(U16 indent) const {
    return Base::toString(indent) + Util::StringFormat(" [ Group sizes: %d %d %d]", _computeGroupSize.x, _computeGroupSize.y, _computeGroupSize.z);
}

stringImpl MemoryBarrierCommand::toString(U16 indent) const {
    return Base::toString(indent) + Util::StringFormat(" [ Mask: %d ]", _barrierMask);
}


}; //namespace GFX
}; //namespace Divide