#include "stdafx.h"

#include "Headers/DebugInterface.h"

#include "Core/Headers/Configuration.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Time/Headers/ProfileTimer.h"

#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

DebugInterface::DebugInterface(Kernel& parent) noexcept
    : KernelComponent(parent)
{
}

void DebugInterface::idle() {
    if_constexpr (!Config::Profile::BENCHMARK_PERFORMANCE && !Config::Profile::ENABLE_FUNCTION_PROFILING) {
        return;
    }

    if (!enabled()) {
        return;
    }

    OPTICK_EVENT();
    const LoopTimingData& timingData = Attorney::KernelDebugInterface::timingData(_parent);
    const GFXDevice& gfx = _parent.platformContext().gfx();
    const Application& app = _parent.platformContext().app();

    if (gfx.getFrameCount() % (Config::TARGET_FRAME_RATE / (Config::Build::IS_DEBUG_BUILD ? 4 : 2)) == 0)
    {
        _output = Util::StringFormat("Scene Update Loops: %d", timingData.updateLoops());

        if_constexpr (Config::Profile::BENCHMARK_PERFORMANCE) {
            const PerformanceMetrics perfMetrics = gfx.getPerformanceMetrics();

            _output.append("\n");
            _output.append(app.timer().benchmarkReport());
            _output.append("\n");
            _output.append(Util::StringFormat("GPU: [ %5.5f ms] [DrawCalls: %d] [Vertices: %zu] [Primitives: %zu]", 
                perfMetrics._gpuTimeInMS,
                gfx.getDrawCallCountLastFrame(),
                perfMetrics._verticesSubmitted,
                perfMetrics._primitivesGenerated));
        }
        if_constexpr (Config::Profile::ENABLE_FUNCTION_PROFILING) {
            _output.append("\n");
            _output.append(Time::ProfileTimer::printAll());
        }

        Arena::Statistics stats = gfx.getObjectAllocStats();
        const F32 gpuAllocatedKB = to_F32(stats.bytes_allocated_) / 1024.0f;

        _output.append("\n");
        _output.append(Util::StringFormat("GPU Objects: %5.2f Kb (%5.2f Mb),\n"
            "             %d allocs,\n"
            "             %d blocks,\n"
            "             %d destructors",
            gpuAllocatedKB,
            gpuAllocatedKB / 1024,
            stats.num_of_allocs_,
            stats.num_of_blocks_,
            stats.num_of_dtros_));
    }
}

void DebugInterface::toggle(const bool state) noexcept {
    _enabled = state;
    if (!_enabled) {
        _output.clear();
    }
}

bool DebugInterface::enabled() const noexcept {
    return _enabled;
}

const stringImpl& DebugInterface::output() const noexcept {
    return _output;
}

} //namespace Divide