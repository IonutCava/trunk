#include "stdafx.h"

#if defined(_WIN32)

#if defined(_DEBUG)
//#include <float.h>
//unsigned int fp_control_state = _controlfp(_EM_INEXACT, _MCW_EM);
#endif

#include "Headers/PlatformDefinesWindows.h"
#include "Core/Headers/StringHelper.h"

#include "Platform/File/Headers/FileManagement.h"

#if defined(USE_VLD)
#include <vld.h>
#endif
#include <direct.h>

#pragma comment(lib, "OpenAL32.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#ifdef _DEBUG
#pragma comment(lib, "DbgHelp.lib")
#pragma comment(lib, "glbindingd.lib")
#pragma comment(lib, "glbinding-auxd.lib")
#pragma comment(lib, "assimp_d.lib")
#pragma comment(lib, "IL_d.lib")
#pragma comment(lib, "ILU_d.lib")
#pragma comment(lib, "libpng_d.lib")
#pragma comment(lib, "jpeg_d.lib")
#pragma comment(lib, "libmng_d.lib")
#pragma comment(lib, "zlib_d.lib")
#pragma comment(lib, "freetype_d.lib")
#pragma comment(lib, "FreeImage_d.lib")

#pragma comment(lib, "SDL2d.lib")
#pragma comment(lib, "SDL2_mixer_d.lib")

#pragma comment(lib, "CEGUIBase-0_d.lib")
#pragma comment(lib, "CEGUICommonDialogs-0_d.lib")
#pragma comment(lib, "CEGUICoreWindowRendererSet_d.lib")
#pragma comment(lib, "CEGUILuaScriptModule-0_d.lib")
#pragma comment(lib, "CEGUISTBImageCodec_d.lib")
#pragma comment(lib, "CEGUITinyXMLParser_d.lib")

#pragma comment(lib, "PhysX3CookingDEBUG_x64.lib")
#pragma comment(lib, "PhysX3DEBUG_x64.lib")
#pragma comment(lib, "PhysX3CommonDEBUG_x64.lib")
#pragma comment(lib, "PhysX3ExtensionsDEBUG.lib")
#pragma comment(lib, "PxPvdSDKDEBUG_x64.lib")
#pragma comment(lib, "PxFoundationDEBUG_x64.lib")

#else  //_DEBUG
#pragma comment(lib, "IL.lib")
#pragma comment(lib, "ILU.lib")
#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "jpeg.lib")
#pragma comment(lib, "libmng.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "freetype.lib")
#pragma comment(lib, "FreeImage.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2_mixer.lib")
#pragma comment(lib, "CEGUIBase-0.lib")
#pragma comment(lib, "CEGUICommonDialogs-0.lib")
#pragma comment(lib, "CEGUICoreWindowRendererSet.lib")
#pragma comment(lib, "CEGUILuaScriptModule-0.lib")
#pragma comment(lib, "CEGUISTBImageCodec.lib")
#pragma comment(lib, "CEGUITinyXMLParser.lib")
#pragma comment(lib, "assimp.lib")
#pragma comment(lib, "glbinding.lib")
#pragma comment(lib, "glbinding-aux.lib")

#if defined(_PROFILE)
    #pragma comment(lib, "PhysX3CookingPROFILE_x64.lib")
    #pragma comment(lib, "PhysX3PROFILE_x64.lib")
    #pragma comment(lib, "PhysX3CommonPROFILE_x64.lib")
    #pragma comment(lib, "PhysX3ExtensionsPROFILE.lib")
    #pragma comment(lib, "PxPvdSDKPROFILE_x64.lib")
    #pragma comment(lib, "PxFoundationPROFILE_x64.lib")
#else
    #pragma comment(lib, "PhysX3Cooking_x64.lib")
    #pragma comment(lib, "PhysX3_x64.lib")
    #pragma comment(lib, "PhysX3Common_x64.lib")
    #pragma comment(lib, "PhysX3Extensions.lib")
    #pragma comment(lib, "PxPvdSDK_x64.lib")
    #pragma comment(lib, "PxFoundation_x64.lib")
#endif

#endif  //_DEBUG

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
// SDL redefines WIN32_LEAN_AND_MEAN
#include <SDL_syswm.h>
#endif

#ifdef FORCE_HIGHPERFORMANCE_GPU
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#endif

void* malloc_aligned(const size_t size, size_t alignment) {
    return _aligned_malloc(size, alignment);
}

void  malloc_free(void*& ptr) {
    _aligned_free(ptr);
}

LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

namespace Divide {
    //https://msdn.microsoft.com/en-us/library/windows/desktop/ms679360%28v=vs.85%29.aspx
    static CHAR * getLastErrorText(CHAR *pBuf, LONG bufSize) {
        DWORD retSize;
        LPTSTR pTemp = NULL;

        if (bufSize < 16) {
            if (bufSize > 0) {
                pBuf[0] = '\0';
            }
            return(pBuf);
        }

        retSize = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                NULL,
                                GetLastError(),
                                LANG_NEUTRAL,
                                (LPTSTR)&pTemp,
                                0,
                                NULL);

        if (!retSize || pTemp == NULL) {
            pBuf[0] = '\0';
        } else {
            pTemp[strlen(pTemp) - 2] = '\0'; //remove cr and newline character
            sprintf(pBuf, "%0.*s (0x%x)", bufSize - 16, pTemp, GetLastError());
            LocalFree((HLOCAL)pTemp);
        }
        return(pBuf);
    }

    void getWindowHandle(void* window, WindowHandle& handleOut) {
        SDL_SysWMinfo wmInfo = {};
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(static_cast<SDL_Window*>(window), &wmInfo);
        handleOut._handle = wmInfo.info.win.window;
    }

    ErrorCode PlatformInitImpl(int argc, char** argv) {
        return ErrorCode::NO_ERR;
    }

    bool PlatformCloseImpl() {
        return true;
    }

    bool GetAvailableMemory(SysInfo& info) {
        MEMORYSTATUSEX status; 
        status.dwLength = sizeof(status);
        const BOOL infoStatus = GlobalMemoryStatusEx(&status);
        if (infoStatus != FALSE) {
            info._availableRam = status.ullAvailPhys;
            return true;
        } else {
            CHAR msgText[256];
            getLastErrorText(msgText,sizeof(msgText));
            std::cerr << msgText << std::endl;
        }
        return false;
    }


    const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)

    void setThreadName(U32 threadID, const char* threadName) {
        // DWORD dwThreadID = ::GetThreadId( static_cast<HANDLE>( t.native_handle() ) );

        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = threadName;
        info.dwThreadID = threadID;
        info.dwFlags = 0;

        __try
        {
            RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void setThreadName(const char* threadName) {
        setThreadName(GetCurrentThreadId(), threadName);
    }

    void setThreadName(std::thread* thread, const char* threadName) {
        const DWORD threadId = ::GetThreadId(static_cast<HANDLE>(thread->native_handle()));
        setThreadName(threadId, threadName);
    }

    bool createDirectory(const char* path) {
        const int ret = _mkdir(path);
        if (ret != 0) {
            return errno == EEXIST;
        }

        return true;
    }

    FileWithPath getExecutableLocation(I32 argc, char** argv) {
        ACKNOWLEDGE_UNUSED(argc);

        char buf[1024] = { 0 };
        const DWORD ret = GetModuleFileNameA(NULL, buf, sizeof(buf));
        if (ret == 0 || ret == sizeof(buf))
        {
            return getExecutableLocation(argv[0]);
        }
        return splitPathToNameAndLocation(buf);
    }

}; //namespace Divide

#endif //defined(_WIN32)