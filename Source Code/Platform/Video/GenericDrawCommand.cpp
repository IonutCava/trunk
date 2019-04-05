#include "stdafx.h"

#include "Headers/GenericDrawCommand.h"

#include "Core/Math/Headers/MathHelper.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexDataInterface.h"

namespace Divide {

namespace {
    inline bool IsSameBuffer(VertexDataInterface* lhs, VertexDataInterface* rhs) {
        return  (lhs == nullptr && rhs == nullptr) || 
                ((lhs == nullptr ? 0 : lhs->getGUID()) == (rhs == nullptr ? 0 : rhs->getGUID()));
    }
};

namespace GenericDrawCommandResults {
    hashMap<I64, QueryResult> g_queryResults;
};

bool compatible(const GenericDrawCommand& lhs, const GenericDrawCommand& rhs) {
    if (lhs._cmd.primCount > 1 || rhs._cmd.primCount > 1) {
        STUBBED("ToDo:: MDI with instancing is busted a.t.m -Ionut");
        return false;
    }
    return
        (lhs._renderOptions & ~(to_base(CmdRenderOptions::CONVERT_TO_INDIRECT))) == (rhs._renderOptions & ~(to_base(CmdRenderOptions::CONVERT_TO_INDIRECT))) &&
        lhs._bufferIndex == rhs._bufferIndex &&
        lhs._primitiveType == rhs._primitiveType &&
        lhs._patchVertexCount == rhs._patchVertexCount &&
        IsSameBuffer(lhs._sourceBuffer, rhs._sourceBuffer);
}

}; //namespace Divide
