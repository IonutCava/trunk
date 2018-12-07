#include "stdafx.h"

#include "Headers/GenericDrawCommand.h"

#include "Core/Math/Headers/MathHelper.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

namespace Divide {

namespace GenericDrawCommandResults {
    hashMap<I64, QueryResult> g_queryResults;
};

GenericDrawCommand::GenericDrawCommand()
    : GenericDrawCommand(PrimitiveType::TRIANGLE_STRIP, 0, 0)
{
}

GenericDrawCommand::GenericDrawCommand(PrimitiveType type,
                                       U32 firstIndex,
                                       U32 indexCount,
                                       U32 primCount)
  : _drawCount(1),
    _bufferIndex(0),
    _primitiveType(type),
    _commandOffset(0),
    _patchVertexCount(3),
    _sourceBuffer(nullptr),
    _renderOptions(to_base(CmdRenderOptions::RENDER_GEOMETRY))
{
    _cmd.indexCount = indexCount;
    _cmd.firstIndex = firstIndex;
    _cmd.primCount = primCount;
}

bool compatible(const GenericDrawCommand& lhs, const GenericDrawCommand& rhs) {
    // Instancing is not compatible with MDI. Well, it might be, but I can't be bothered a.t.m. to implement it -Ionut
    if (lhs._cmd.primCount > 1 || rhs._cmd.primCount > 1)
        return false;

    return
        lhs._bufferIndex == rhs._bufferIndex &&
        lhs._renderOptions == rhs._renderOptions &&
        lhs._primitiveType == rhs._primitiveType &&
        lhs._patchVertexCount == rhs._patchVertexCount &&
        (lhs._sourceBuffer != nullptr) == (rhs._sourceBuffer != nullptr);
}

}; //namespace Divide
