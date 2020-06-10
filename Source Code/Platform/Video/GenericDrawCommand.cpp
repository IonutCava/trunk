#include "stdafx.h"

#include "Headers/GenericDrawCommand.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexDataInterface.h"

namespace Divide {

namespace GenericDrawCommandResults {
    hashMap<I64, QueryResult> g_queryResults;
};

bool compatible(const GenericDrawCommand& lhs, const GenericDrawCommand& rhs) noexcept {
    return lhs._primitiveType == rhs._primitiveType &&
           lhs._bufferIndex == rhs._bufferIndex &&
           lhs._sourceBuffer == rhs._sourceBuffer;
}

}; //namespace Divide
