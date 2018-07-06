#include "Headers/PlatformDefines.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIMessageBox.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/MemoryTracker.h"

namespace Divide {
namespace MemoryManager {
void log_new(void* p, size_t size, const char* zFile, I32 nLine) {
#if defined(_DEBUG)
    if (MemoryTracker::Ready) {
        // AllocTracer.Add( p, size, zFile, nLine );
    }
#endif
}

void log_delete(void* p) {
#if defined(_DEBUG)
    if (MemoryTracker::Ready) {
        // AllocTracer.Remove( p );
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
        GUIMessageBox* const msgBox = GUI::getInstance().getDefaultMessageBox();
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
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size, " allocation outside of macro ",
                                   0);
    return ptr;
}

void operator delete(void* ptr) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}

void* operator new[](size_t size) {
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size,
                                   " array allocation outside of macro ", 0);
    return ptr;
}

void operator delete[](void* ptr) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}

void* operator new(size_t size, char* zFile, Divide::I32 nLine) {
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size, zFile, nLine);
    return ptr;
}

void operator delete(void* ptr, char* zFile, Divide::I32 nLine) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}

void* operator new[](size_t size, char* zFile, Divide::I32 nLine) {
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size, zFile, nLine);
    return ptr;
}

void operator delete[](void* ptr, char* zFile, Divide::I32 nLine) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}
#endif

void* operator new[](size_t size, const char* pName, Divide::I32 flags,
                     Divide::U32 debugFlags, const char* file,
                     Divide::I32 line) {
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size, file, line);
    return ptr;
}

void operator delete[](void* ptr, const char* pName, Divide::I32 flags,
                       Divide::U32 debugFlags, const char* file,
                       Divide::I32 line) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char* pName, Divide::I32 flags,
                     Divide::U32 debugFlags, const char* file,
                     Divide::I32 line) {
    // this allocator doesn't support alignment
    assert(alignment <= 8);
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size, file, line);
    return ptr;
}

void operator delete[](void* ptr, size_t alignment, size_t alignmentOffset,
                       const char* pName, Divide::I32 flags,
                       Divide::U32 debugFlags, const char* file,
                       Divide::I32 line) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}

Divide::I32 Vsnprintf8(char* pDestination, size_t n, const char* pFormat,
                       va_list arguments) {
    return vsnprintf(pDestination, n, pFormat, arguments);
}