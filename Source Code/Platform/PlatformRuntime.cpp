#include "stdafx.h"

#include <thread>

namespace Divide {
namespace Runtime {

namespace detail {
    bool g_idSet = false;
    std::thread::id g_mainThreadID;
};

const std::thread::id&  mainThreadID() {
    return detail::g_mainThreadID;
}

void mainThreadID(const std::thread::id& threadID) {
    assert(!detail::g_idSet);

    detail::g_mainThreadID = threadID;
    detail::g_idSet = true;
}

bool resetMainThreadID() {
    detail::g_idSet = false;
    return true;
}

}; //namespace Runtime
}; //namespace Divide