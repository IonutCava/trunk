#include "config.h"

#include "Headers/PlatformDefines.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIMessageBox.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"

#if defined(USE_CUSTOM_MEMORY_ALLOCATORS)
#include <Allocator/xallocator.h>
#endif

#include "Utility/Headers/MemoryTracker.h"

namespace Divide {
namespace MemoryManager {
void log_new(void* p, size_t size, const char* zFile, size_t nLine) {
    if (Config::Build::IS_DEBUG_BUILD) {
        if (MemoryTracker::Ready) {
             AllocTracer.Add( p, size, zFile, nLine );
        }
    }
}

void log_delete(void* p) {
    if (Config::Build::IS_DEBUG_BUILD) {
        if (MemoryTracker::Ready) {
            AllocTracer.Remove( p );
        }
    }
}
};  // namespace MemoryManager

bool PlatformPostInit() {
    SeedRandom();
    return true;
}

U32 HARDWARE_THREAD_COUNT() {
    return std::max(std::thread::hardware_concurrency(), 2u);
}

bool preAssert(const bool expression, const char* failMessage) {
    if (expression) {
        return false;
    }
    if (Config::Assert::LOG_ASSERTS) {
        Console::errorfn("Assert: %s", failMessage);
    }
    /// Message boxes without continue on assert don't render!
    if (Config::Assert::SHOW_MESSAGE_BOX && Config::Assert::CONTINUE_ON_ASSERT) {
        GUIMessageBox* const msgBox = GUI::instance().getDefaultMessageBox();
        if (msgBox) {
            msgBox->setTitle("Assertion Failed!");
            msgBox->setMessage(stringImpl("Assert: ") + failMessage);
            msgBox->setMessageType(GUIMessageBox::MessageType::MESSAGE_ERROR);
            msgBox->show();
        }
    }

    return !Config::Assert::CONTINUE_ON_ASSERT;
}

};  // namespace Divide

#if defined(_DEBUG)
#if defined(DEBUG_EXTERNAL_ALLOCATIONS)
void* operator new(size_t size) {
    static thread_local bool logged = false;
    void* ptr = malloc(size);
    if (!logged) {
        Divide::MemoryManager::log_new(ptr, size, " allocation outside of macro ", 0);
        if (Divide::MemoryManager::MemoryTracker::Ready) {
            logged = true;
        }
    }
    return ptr;
}

void operator delete(void* ptr) noexcept {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}

void* operator new[](size_t size) {
    static thread_local bool logged = false;
    void* ptr = malloc(size);
    if (!logged) {
        Divide::MemoryManager::log_new(ptr, size, " array allocation outside of macro ", 0);
        if (Divide::MemoryManager::MemoryTracker::Ready) {
            logged = true;
        }
    }
    return ptr;
}

void operator delete[](void* ptr) noexcept {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}
#endif

void* operator new(size_t size, const char* zFile, size_t nLine) {
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size, zFile, nLine);
    return ptr;
}

void operator delete(void* ptr, const char* zFile, size_t nLine) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}

void* operator new[](size_t size, const char* zFile, size_t nLine) {
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size, zFile, nLine);
    return ptr;
}

void operator delete[](void* ptr, const char* zFile, size_t nLine) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}
#else
#endif