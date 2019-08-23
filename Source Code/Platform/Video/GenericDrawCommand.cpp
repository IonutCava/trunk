#include "stdafx.h"

#include "Headers/GenericDrawCommand.h"

#include "Core/Math/Headers/MathHelper.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexDataInterface.h"

namespace Divide {


namespace GenericDrawCommandResults {
    hashMap<I64, QueryResult> g_queryResults;
};

bool compatible(const GenericDrawCommand& lhs, const GenericDrawCommand& rhs) {
    return
        (lhs._renderOptions & ~(to_base(CmdRenderOptions::CONVERT_TO_INDIRECT))) == (rhs._renderOptions & ~(to_base(CmdRenderOptions::CONVERT_TO_INDIRECT))) &&
        lhs._bufferIndex == rhs._bufferIndex &&
        lhs._primitiveType == rhs._primitiveType &&
        lhs._patchVertexCount == rhs._patchVertexCount &&
        lhs._sourceBuffer._data == rhs._sourceBuffer._data;
}

}; //namespace Divide
