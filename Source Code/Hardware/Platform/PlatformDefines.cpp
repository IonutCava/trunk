#include "Headers/PlatformDefines.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIMessageBox.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/MemoryTracker.h"

namespace Divide {
    namespace MemoryManager {
        void log_new( void* p, size_t size, const char* zFile, I32 nLine ) {
#       if defined(_DEBUG)
            if ( MemoryTracker::Ready ) {
                AllocTracer.Add( p, size, zFile, nLine );
            }
#       endif
        }

        void log_delete( void* p ) {
#       if defined(_DEBUG)
            if ( MemoryTracker::Ready ) {
                AllocTracer.Remove( p );
            }
#       endif
        }
    }; //namespace MemoryManager
    bool preAssert( const bool expression, const char* failMessage ) {
        if ( expression ) {
            return false;
        }
        if ( Config::Assert::LOG_ASSERTS ) {
            ERROR_FN( "Assert: %s", failMessage );
        }
        if ( Config::Assert::SHOW_MESSAGE_BOX ) {
            if ( GUI::hasInstance() ) {
                GUIMessageBox* const msgBox = GUI::getInstance().getDefaultMessageBox();
                if ( msgBox ) {
                    msgBox->setTitle( "Assertion Failed!" );
                    msgBox->setMessage( stringImpl( "Assert: " ) + failMessage );
                    msgBox->setMessageType( GUIMessageBox::MESSAGE_ERROR );
                    msgBox->show();
                }
            }
        }
        return !Config::Assert::CONTINUE_ON_ASSERT;
    }
}; //namespace Divide

#if defined(_DEBUG)
void* operator new( size_t size ) {
	void* ptr = malloc( size );
	Divide::MemoryManager::log_new( ptr, size, " allocation outside of macro ", 0 );
	return ptr;
}

void  operator delete( void *ptr ) {
    Divide::MemoryManager::log_delete( ptr );
	free( ptr );
}

void* operator new[]( size_t size ) {
	void* ptr = malloc( size );
    Divide::MemoryManager::log_new( ptr, size, " array allocation outside of macro ", 0 );
	return ptr;
}

void operator delete[]( void *ptr ) {
    Divide::MemoryManager::log_delete( ptr );
	free( ptr );
}

void* operator new( size_t size, char* zFile, int nLine ){
	void* ptr = malloc( size );
    Divide::MemoryManager::log_new( ptr, size, zFile, nLine );
	return ptr;
}

void operator delete( void* ptr, char* zFile, Divide::I32 nLine ) {
    Divide::MemoryManager::log_delete( ptr );
	free( ptr );
}

void* operator new[]( size_t size, char* zFile, int nLine ) {
	void* ptr = malloc( size );
    Divide::MemoryManager::log_new( ptr, size, zFile, nLine );
	return ptr;
}

void operator delete[]( void* ptr, char* zFile, Divide::I32 nLine ) {
    Divide::MemoryManager::log_delete( ptr );
	free( ptr );
}
#endif

void* operator new[]( size_t size, const char* pName, Divide::I32 flags, Divide::U32 debugFlags, const char* file, Divide::I32 line ) {
	void* ptr = malloc( size );
    Divide::MemoryManager::log_new( ptr, size, file, line );
	return ptr;
}

void operator delete[]( void* ptr, const char* pName, Divide::I32 flags, Divide::U32 debugFlags, const char* file, Divide::I32 line ) {
    Divide::MemoryManager::log_delete( ptr );
	free( ptr );
}

void* operator new[]( size_t size, size_t alignment, size_t alignmentOffset, const char* pName, Divide::I32 flags, Divide::U32 debugFlags, const char* file, Divide::I32 line ) {
	// this allocator doesn't support alignment
	assert( alignment <= 8 );
	void* ptr = malloc( size );
    Divide::MemoryManager::log_new( ptr, size, file, line );
	return ptr;
}

void operator delete[]( void* ptr, size_t alignment, size_t alignmentOffset, const char* pName, Divide::I32 flags, Divide::U32 debugFlags, const char* file, Divide::I32 line ) {
    Divide::MemoryManager::log_delete( ptr );
	free( ptr );
}

// E
Divide::I32 Vsnprintf8( char* pDestination, size_t n, const char* pFormat, va_list arguments ) {
#ifdef _MSC_VER
	return _vsnprintf( pDestination, n, pFormat, arguments );
#else
	return vsnprintf( pDestination, n, pFormat, arguments );
#endif
}
