#include "Headers/PlatformDefines.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIMessageBox.h"

#include "Core/Headers/Console.h"

#if defined(_DEBUG)
#include "Utility/Headers/MemoryTracker.h"
#endif

namespace Divide {
namespace MemoryManager {
void log_new(void* p, size_t size, const char* zFile, I32 nLine) {
#if defined(_DEBUG)
    if (MemoryTracker::Ready) {
         AllocTracer.Add( p, size, zFile, nLine );
    }
#endif
}

void log_delete(void* p) {
#if defined(_DEBUG)
    if (MemoryTracker::Ready) {
        AllocTracer.Remove( p );
    }
#endif
}
};  // namespace MemoryManager

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

void* operator new(size_t size, const char* zFile, Divide::I32 nLine) {
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size, zFile, nLine);
    return ptr;
}

void operator delete(void* ptr, const char* zFile, Divide::I32 nLine) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}

void* operator new[](size_t size, const char* zFile, Divide::I32 nLine) {
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size, zFile, nLine);
    return ptr;
}

void operator delete[](void* ptr, const char* zFile, Divide::I32 nLine) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}
#endif