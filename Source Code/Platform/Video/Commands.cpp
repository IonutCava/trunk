#include "stdafx.h"
#include "Headers/Commands.h"

#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {
namespace GFX {
stringImpl DrawCommand::toString(U16 indent) const {
    stringImpl ret = CommandBase::toString(indent);
    ret.append("\n");
    size_t i = 0;
    for (const GenericDrawCommand& cmd : _drawCommands) {
        ret.append("    ");
        for (U16 j = 0; j < indent; ++j) {
            ret.append("    ");
        }
        ret.append(Util::StringFormat("%d: Draw count: %d Base instance: %d Instance count: %d\n", i++, cmd._drawCount, cmd._cmd.baseInstance, cmd._cmd.primCount));
    }

    return ret;
}

stringImpl SetViewportCommand::toString(U16 indent) const {
    return CommandBase::toString(indent) + Util::StringFormat(" [%d, %d, %d, %d]", _viewport.x, _viewport.y, _viewport.z, _viewport.w);
}

stringImpl BeginRenderPassCommand::toString(U16 indent) const {
    return CommandBase::toString(indent) + " [ " + stringImpl(_name.c_str()) + " ]";
}
stringImpl SetScissorCommand::toString(U16 indent) const {
    return CommandBase::toString(indent) + Util::StringFormat(" [%d, %d, %d, %d]", _rect.x, _rect.y, _rect.z, _rect.w);
}

stringImpl BindDescriptorSetsCommand::toString(U16 indent) const {
    stringImpl ret = CommandBase::toString(indent);

    ret.append(Util::StringFormat(" [ Buffers: %d, Textures: %d ]\n", _set._shaderBuffers.size(), _set._textureData.textures().size()));
    for (auto it : _set._shaderBuffers) {
        ret.append("    ");
        for (U16 j = 0; j < indent; ++j) {
            ret.append("    ");
        }
        ret.append(Util::StringFormat("Buffer [ %d - %d ]\n", to_base(it._binding), it._buffer->getGUID()));
    }
    for (auto it : _set._textureData.textures()) {
        ret.append("    ");
        for (U16 j = 0; j < indent; ++j) {
            ret.append("    ");
        }
        ret.append(Util::StringFormat("Texture [ %d - %d ]\n", it.second, it.first.getHandle()));
    }

    return ret;
}

stringImpl BeginDebugScopeCommand::toString(U16 indent) const {
    return CommandBase::toString(indent) + " [ " + stringImpl(_scopeName.c_str()) + " ]";
}

stringImpl DrawTextCommand::toString(U16 indent) const {
    stringImpl ret = CommandBase::toString(indent);
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

stringImpl MemoryBarrierCommand::toString(U16 indent) const {
    return CommandBase::toString(indent) + Util::StringFormat(" [ Mask: %d ]", _barrierMask);
}


}; //namespace GFX
}; //namespace Divide