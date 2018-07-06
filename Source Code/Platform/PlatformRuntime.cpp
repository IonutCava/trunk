#include "stdafx.h"

#include <thread>

namespace Divide {
namespace Runtime {

namespace detail {
    std::thread::id g_mainThreadID;
};

bool isMainThread() {
    return (detail::g_mainThreadID == std::this_thread::get_id());
}

const std::thread::id&  mainThreadID() {
    return detail::g_mainThreadID;
}

void mainThreadID(const std::thread::id& threadID) {
    static bool idSet = false;
    assert(!idSet);

    detail::g_mainThreadID = threadID;
    idSet = true;
}

}; //namespace Runtime
}; //namespace Divide