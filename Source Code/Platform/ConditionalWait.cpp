#include "stdafx.h"

#include "Headers/ConditionalWait.h"

#include "Core/Headers/PlatformContext.h"
#include "Headers/PlatformRuntime.h"

namespace Divide {
namespace {
    PlatformContext* g_ctx = nullptr;
}

void InitConditionalWait(PlatformContext& context) {
    g_ctx = &context;
}

void PlatformContextIdleCall() {
    assert(g_ctx != nullptr);
    if (Runtime::isMainThread()) {
        g_ctx->idle(true, 0u);
    }
}
} //namespace Divide