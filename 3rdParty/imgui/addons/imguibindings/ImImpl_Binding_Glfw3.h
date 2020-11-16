#ifndef IMIMPL_BINDING_H
#define IMIMPL_BINDING_H


#include "imguibindings.h"

#if (GLFW_VERSION_MAJOR>=3 && GLFW_VERSION_MINOR>0)
#define GLFW_HAS_MOUSE_CURSOR_SUPPORT
#endif //GLFW_VERSION

#if (defined(GLFW_HAS_MOUSE_CURSOR_SUPPORT) && defined(IMGUI_GLFW_NO_NATIVE_CURSORS))
#undef IMGUI_GLFW_NO_NATIVE_CURSORS
#warning IMGUI_GLFW_NO_NATIVE_CURSORS must be undefined globally, because your GLFW version is >= 3.1 and has embedded mouse cursor support.
#endif //IMGUI_GLFW_NO_NATIVE_CURSORS

#ifdef GLFW_HAS_MOUSE_CURSOR_SUPPORT
    static const int glfwCursorIds[ImGuiMouseCursor_COUNT+1] = {
        GLFW_ARROW_CURSOR,
        GLFW_IBEAM_CURSOR,
        GLFW_HAND_CURSOR,      //SDL_SYSTEM_CURSOR_HAND,    // or SDL_SYSTEM_CURSOR_SIZEALL  //ImGuiMouseCursor_ResizeAll,                  // Unused by ImGui
        GLFW_VRESIZE_CURSOR,       //ImGuiMouseCursor_ResizeNS,              // Unused by ImGui
        GLFW_HRESIZE_CURSOR,       //ImGuiMouseCursor_ResizeEW,              // Unused by ImGui
        GLFW_CROSSHAIR_CURSOR,     //ImGuiMouseCursor_ResizeNESW,
        GLFW_CROSSHAIR_CURSOR,       //ImGuiMouseCursor_ResizeNWSE,          // Unused by ImGui
        GLFW_HAND_CURSOR,       //ImGuiMouseCursor_Hand,          // Unused by ImGui
        GLFW_ARROW_CURSOR         //,ImGuiMouseCursor_Arrow
    };
    static GLFWcursor* glfwCursors[ImGuiMouseCursor_COUNT+1];

#else //GLFW_HAS_MOUSE_CURSOR_SUPPORT
#ifndef IMGUI_GLFW_NO_NATIVE_CURSORS
#   ifdef _WIN32
#       define IMGUI_USE_WIN32_CURSORS     // Optional, but needs at window creation: wc.hCursor = LoadCursor( NULL, NULL); // Now the window class is inside glfw3... Not sure how I can access it...
#       ifdef IMGUI_USE_WIN32_CURSORS
    static const LPCTSTR win32CursorIds[ImGuiMouseCursor_COUNT+1] = {
        IDC_ARROW,
        IDC_IBEAM,
        IDC_SIZEALL,      //SDL_SYSTEM_CURSOR_HAND,    // or SDL_SYSTEM_CURSOR_SIZEALL  //ImGuiMouseCursor_Move,                  // Unused by ImGui
        IDC_SIZENS,       //ImGuiMouseCursor_ResizeNS,              // Unused by ImGui
        IDC_SIZEWE,       //ImGuiMouseCursor_ResizeEW,              // Unused by ImGui
        IDC_SIZENESW,     //ImGuiMouseCursor_ResizeNESW,
        IDC_SIZENWSE,     //ImGuiMouseCursor_ResizeNWSE,          // Unused by ImGui
#       ifdef IDC_HAND    // winuser.h defines it if(WINVER >= 0x0500)
        IDC_HAND,         //ImGuiMouseCursor_Hand // Unused by ImGui
#       else
        IDC_ARROW,         //ImGuiMouseCursor_Hand // Unused by ImGui
#       endif
        IDC_ARROW         //,ImGuiMouseCursor_Arrow
    };
    static HCURSOR win32Cursors[ImGuiMouseCursor_COUNT+1];
#       endif //IMGUI_USE_WIN32_CURSORS
#   else //_WIN32
#       define IMGUI_USE_X11_CURSORS      // Optional (feel free to comment it out if you don't have X11)
#       ifdef IMGUI_USE_X11_CURSORS
    // Before the inclusion of @ref glfw3native.h, you must define exactly one
    // window API macro and exactly one context API macro.
#           define GLFW_EXPOSE_NATIVE_X11
#           define GLFW_EXPOSE_NATIVE_GLX
#           include <GLFW/glfw3native.h>        // GLFWAPI Display* glfwGetX11Display(void); //GLFWAPI Window glfwGetX11Window(GLFWwindow* window);
#           include <X11/cursorfont.h>
    // 52 (closed hand)   58 (hand with forefinger) 124 (spray)   86  (pencil)  150 (wait)
    static const unsigned int x11CursorIds[ImGuiMouseCursor_COUNT+1] = {
        2,//SDL_SYSTEM_CURSOR_ARROW,
        152,//SDL_SYSTEM_CURSOR_IBEAM,
        30,//SDL_SYSTEM_CURSOR_SIZEALL,      //SDL_SYSTEM_CURSOR_HAND,    // or SDL_SYSTEM_CURSOR_SIZEALL  //ImGuiMouseCursor_Move,                  // Unused by ImGui
        116,//42,//SDL_SYSTEM_CURSOR_SIZENS,       //ImGuiMouseCursor_ResizeNS,              // Unused by ImGui
        108,//SDL_SYSTEM_CURSOR_SIZEWE,       //ImGuiMouseCursor_ResizeEW,              // Unused by ImGui
        13,//SDL_SYSTEM_CURSOR_SIZENESW,     //ImGuiMouseCursor_ResizeNESW,
        15,//SDL_SYSTEM_CURSOR_SIZENWSE,     //ImGuiMouseCursor_ResizeNWSE,          // Unused by ImGui
        58,                                 //ImGuiMouseCursor_Hand
        2//SDL_SYSTEM_CURSOR_ARROW         //,ImGuiMouseCursor_Arrow
    };
    static Cursor x11Cursors[ImGuiMouseCursor_COUNT+1];
#       endif //IMGUI_USE_X11_CURSORS
#   endif //_WIN32
#endif //IMGUI_GLFW_NO_NATIVE_CURSORS
#endif //GLFW_HAS_MOUSE_CURSOR_SUPPORT




//static
GLFWwindow* window;
static bool g_MousePressed[5] = {false, false, false, false, false };
static ImVec2 mousePosScale(1.0f, 1.0f);


// NB: ImGui already provide OS clipboard support for Windows so this isn't needed if you are using Windows only.
static const char* ImImpl_GetClipboardTextFn(void*)
{
    return glfwGetClipboardString(window);
}

static void ImImpl_SetClipboardTextFn(void*,const char* text)
{
    glfwSetClipboardString(window, text);
}

#ifdef _WIN32
// Notify OS Input Method Editor of text input position (e.g. when using Japanese/Chinese inputs, otherwise this isn't needed)
static void ImImpl_ImeSetInputScreenPosFn(int x, int y)
{
    HWND hwnd = glfwGetWin32Window(window);
    if (HIMC himc = ImmGetContext(hwnd))
    {
        COMPOSITIONFORM cf;
        cf.ptCurrentPos.x = x;
        cf.ptCurrentPos.y = y;
        cf.dwStyle = CFS_FORCE_POSITION;
        ImmSetCompositionWindow(himc, &cf);
    }
}
#endif


// GLFW callbacks to get events
static void glfw_error_callback(int /*error*/, const char* description)	{
    fputs(description, stderr);
}
static bool gImGuiAppIsIconified = false;
static void glfw_window_iconify_callback(GLFWwindow* /*window*/,int iconified)    {
    gImGuiAppIsIconified = iconified == GL_TRUE;
}
static void glfw_framebuffer_size_callback(GLFWwindow* /*window*/,int fb_w,int fb_h)  {
    int w, h;glfwGetWindowSize(window, &w, &h);
    mousePosScale.x = (float)fb_w / w;                  // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
    mousePosScale.y = (float)fb_h / h;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);
}
static void glfw_window_size_callback(GLFWwindow* /*window*/,int w,int h)  {
    int fb_w, fb_h;glfwGetFramebufferSize(window, &fb_w, &fb_h);
    mousePosScale.x = (float)fb_w / w;                  // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
    mousePosScale.y = (float)fb_h / h;
    ResizeGL(w,h);
}
static void glfw_mouse_button_callback(GLFWwindow* /*window*/, int button, int action, int mods)	{
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < 5) {
        //io.MouseDown[button] = (action == GLFW_PRESS);
        // Manual double click handling:
        static double dblClickTimes[6]={-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX};  // seconds
        if (action == GLFW_PRESS)   {
            //g_MousePressed[button] = true;
            double time = glfwGetTime();
            double& oldTime = dblClickTimes[button];
            bool& mouseDoubleClicked = gImGuiBindingMouseDblClicked[button];
            if (time - oldTime < io.MouseDoubleClickTime) {
                mouseDoubleClicked = true;
                oldTime = -FLT_MAX;
                //fprintf(stderr,"Double Clicked button %d\n",button);
            }
            else {
                //fprintf(stderr,"Not Double Clicked button %d (%1.4f < %1.4f)\n",button,(float)time-(float)oldTime,io.MouseDoubleClickTime);
                mouseDoubleClicked = false;
                oldTime = time;
            }
        }
    }
    io.KeyCtrl = (mods & GLFW_MOD_CONTROL);
    io.KeyShift = (mods & GLFW_MOD_SHIFT);
    io.KeyAlt = (mods & GLFW_MOD_ALT);

}
static void glfw_scroll_callback(GLFWwindow* /*window*/, double xoffset, double yoffset)	{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel = (yoffset != 0.0f) ? (yoffset > 0.0f ? 1 : - 1) : 0;           // Mouse wheel: -1,0,+1
    io.MouseWheelH = (xoffset != 0.0f) ? (xoffset > 0.0f ? 1 : - 1) : 0;      // Mouse wheel: -1,0,+1
}
static bool gImGuiCapsLockDown = false;
static void glfw_key_callback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int mods)	{
    ImGuiIO& io = ImGui::GetIO();
    io.KeyCtrl = (mods & GLFW_MOD_CONTROL);
    io.KeyShift = (mods & GLFW_MOD_SHIFT);
    io.KeyAlt = (mods & GLFW_MOD_ALT);
    const bool down = (action!=GLFW_RELEASE);
    // (action == GLFW_PRESS);
    if (key==GLFW_KEY_LEFT_CONTROL || key==GLFW_KEY_RIGHT_CONTROL)  io.KeyCtrl = down;
    else if (key==GLFW_KEY_LEFT_SHIFT || key==GLFW_KEY_RIGHT_SHIFT) io.KeyShift = down;
    else if (key==GLFW_KEY_LEFT_ALT || key==GLFW_KEY_RIGHT_ALT)     io.KeyAlt = down;
    if (key == GLFW_KEY_CAPS_LOCK) gImGuiCapsLockDown = down;
    else if (key>=GLFW_KEY_A && key<=GLFW_KEY_Z && !io.KeyShift && !gImGuiCapsLockDown) {
        if (!(io.KeyCtrl && (key==GLFW_KEY_X || key==GLFW_KEY_C || key==GLFW_KEY_V ||
            key==GLFW_KEY_A || key==GLFW_KEY_Y || key==GLFW_KEY_Z)))    // Preserve copy/paste etc.
                key+= ((const int)'a'-GLFW_KEY_A);
    }
    if (key>=GLFW_KEY_F1 && key<=GLFW_KEY_F12) {
        const int i = key-GLFW_KEY_F1;
        const bool prevState = gImGuiFunctionKeyDown[i];
        gImGuiFunctionKeyDown[i] = down;
        if (down!=prevState)    {
            if (down) gImGuiFunctionKeyPressed[i] = true;
            else gImGuiFunctionKeyReleased[i] = true;
        }
        //fprintf(stderr,"%d) D:%d P:%d R:%d\n",i,(int)gImGuiFunctionKeyDown[i],(int)gImGuiFunctionKeyPressed[i],(int)gImGuiFunctionKeyReleased[i]);
    }
    else if (key>=0 && key<512)  io.KeysDown[key] = down;
}
static void glfw_char_callback(GLFWwindow* /*window*/, unsigned int c)	{
    if (c > 0 /*&& c < 0x10000*/ && !ImGui::GetIO().KeyCtrl) ImGui::GetIO().AddInputCharacter((unsigned int)c);
}
static void glfw_mouse_enter_leave_callback(GLFWwindow* /*window*/, int entered)	{
    if (entered==GL_FALSE) {
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos.x=io.MousePos.y=-FLT_MAX;
    }
}
static void glfw_mouse_move_callback(GLFWwindow* /*window*/, double x,double y)	{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x * mousePosScale.x, (float)y * mousePosScale.y);
}
static void InitImGui(const ImImpl_InitParams* pOptionalInitParams=NULL)	{
    int w, h;
    int fb_w, fb_h;
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    mousePosScale.x = (float)fb_w / w;                  // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
    mousePosScale.y = (float)fb_h / h;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);  // Display size, in pixels. For clamping windows positions.
    io.DeltaTime = 1.0f/60.0f;                          // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
    //io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels

    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;   // We can honor GetMouseCursor() values
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;    // We can honor io.WantSetMousePos requests (optional, rarely used)

    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;             // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
#   ifndef __EMSCRIPTEN__  // emscripten doesn't like it (and triggers a 'NewFrameSanityCheck' or something like that [But tested only with SDL2 binding, so it might work])
    io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
#   endif

    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    //io.RenderDrawListsFn = ImImpl_RenderDrawLists;
    io.SetClipboardTextFn = ImImpl_SetClipboardTextFn;
    io.GetClipboardTextFn = ImImpl_GetClipboardTextFn;
#ifdef _WIN32
    io.ImeSetInputScreenPosFn = ImImpl_ImeSetInputScreenPosFn;
#endif

    // 3 common init steps
    InitImGuiFontTexture(pOptionalInitParams);
    InitImGuiProgram();
    InitImGuiBuffer();
}


static bool InitBinding(const ImImpl_InitParams* pOptionalInitParams=NULL,int argc=0, char** argv=NULL)	{
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())    {
        fprintf(stderr, "Could not call glfwInit(...) successfully.\n");
        return false;
    }

//-ENABLE-OPENGLES COMPATIBILITY PROFILES----------------------------------
#ifndef IMIMPL_SHADER_NONE
#ifdef IMIMPL_SHADER_GLES
#   ifndef IMIMPL_SHADER_GL3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);          // 1 => ES1.1   2 => ES2.0  3 => ES3.0
#   else //IMIMPL_SHADER_GL3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
#   endif //MIMPL_SHADER_GL3
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#endif // IMIMPL_SHADER_GLES
#endif //IMIMPL_SHADER_NONE
//--------------------------------------------------------------------------
    //glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

#ifdef IMIMPL_FORCE_DEBUG_CONTEXT
glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,GL_TRUE);
#endif //IMIMPL_FORCE_DEBUG_CONTEXT

/*
//-ENABLE-OPENGLES COMPATIBILITY PROFILES----------------------------------
#ifndef IMIMPL_SHADER_NONE
#ifdef IMIMPL_SHADER_GLES
#   ifndef IMIMPL_SHADER_GL3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);          // 1 => ES1.1   2 => ES2.0  3 => ES3.0
#   ifdef IMIMPL_SHADER_GLES_MINOR_VERSION
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, IMIMPL_SHADER_GLES_MINOR_VERSION);
#   endif //IMIMPL_SHADER_GLES_MINOR_VERSION
#   else //IMIMPL_SHADER_GL3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
#   ifdef IMIMPL_SHADER_GLES_MINOR_VERSION
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, IMIMPL_SHADER_GLES_MINOR_VERSION);
#   endif //IMIMPL_SHADER_GLES_MINOR_VERSION
#   endif //MIMPL_SHADER_GL3
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#endif // IMIMPL_SHADER_GLES
#endif //IMIMPL_SHADER_NONE

#ifndef IMIMPL_SHADER_GLES
#ifdef IMIMPL_FORWARD_COMPAT
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif
#ifdef IMIMPL_CORE_PROFILE
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#endif
#endif

#ifdef IMIMPL_SRGB_CAPABLE
    glfwWindowHint(GLFW_SRGB_CAPABLE,GL_TRUE);
#endif
if (pOptionalInitParams && pOptionalInitParams->useOpenGLDebugContext) glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,GL_TRUE);
//--------------------------------------------------------------------------
*/

    if (pOptionalInitParams && pOptionalInitParams->gWindowTitle[0]!='\0')  window = glfwCreateWindow(pOptionalInitParams ? pOptionalInitParams->gWindowSize.x : 1270, pOptionalInitParams ? pOptionalInitParams->gWindowSize.y : 720,(const char*) &pOptionalInitParams->gWindowTitle[0], NULL, NULL);
    else		window = glfwCreateWindow(pOptionalInitParams ? pOptionalInitParams->gWindowSize.x : 1270, pOptionalInitParams ? pOptionalInitParams->gWindowSize.y : 720, "ImGui Glfw3 OpenGL example", NULL, NULL);
    if (!window)    {
        fprintf(stderr, "Could not call glfwCreateWindow(...) successfully.\n");
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    //glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1);
    glfwSetScrollCallback(window, glfw_scroll_callback);
    glfwSetCharCallback(window, glfw_char_callback);
    glfwSetCursorPosCallback(window, glfw_mouse_move_callback);
    glfwSetCursorEnterCallback(window, glfw_mouse_enter_leave_callback);

    glfwSetWindowSizeCallback(window, glfw_window_size_callback);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
    glfwSetWindowIconifyCallback(window, glfw_window_iconify_callback);

#ifdef IMGUI_USE_GLAD
   if(!gladLoadGL()) {
        fprintf(stderr,"Error initializing GLAD!\n");
        return false;
    }
    // gladLoadGLLoader(&glfwGetProcAddress);
#endif //IMGUI_USE_GLAD
#ifdef IMGUI_USE_GL3W
   if (gl3wInit()) {
       fprintf(stderr, "Error initializing GL3W!\n");
       return false;
   }
#endif //IMGUI_USE_GL3W

        //OpenGL info
    {
        printf("GLFW Version: %d.%d.%d\n",GLFW_VERSION_MAJOR,GLFW_VERSION_MINOR,GLFW_VERSION_REVISION);
        printf("GL Vendor: %s\n", glGetString( GL_VENDOR ));
        printf("GL Renderer : %s\n", glGetString( GL_RENDERER ));
        printf("GL Version (string) : %s\n",  glGetString( GL_VERSION ));
#       ifndef IMIMPL_SHADER_NONE
        printf("GLSL Version : %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ));
#       endif //IMIMPL_SHADER_NONE
        //printf("GL Extensions:\n%s\n",(char *) glGetString(GL_EXTENSIONS));
    }

#ifdef IMGUI_USE_GLEW
    GLenum err = glewInit();
    if( GLEW_OK != err )
    {
        fprintf(stderr, "Error initializing GLEW: %s\n",
                glewGetErrorString(err) );
        return false;
    }
#endif //IMGUI_USE_GLEW

#ifdef IMIMPL_FORCE_DEBUG_CONTEXT
    glDebugMessageCallback(GLDebugMessageCallback,NULL);    // last is userParam
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_DEBUG_OUTPUT);   // use glDisable(GL_DEBUG_OUTPUT); at runtime to disable it

    // Test:
    //glEnable(GL_DEPTH); // instead of glEnable(GL_DEPTH_TEST); => Produces: GL_INVALID_ENUM error generated. <cap> enum is invalid; expected GL_ALPHA_TEST, GL_BLEND, GL_COLOR_MATERIAL, GL_CULL_FACE, GL_DEPTH_TEST, GL_DITHER, GL_FOG, etc. (136 others).
#endif //IMIMPL_FORCE_DEBUG_CONTEXT

	return true;
}

struct ImImplMainLoopFrameStruct {
int done;
#ifndef GLFW_HAS_MOUSE_CURSOR_SUPPORT
#ifndef IMGUI_GLFW_NO_NATIVE_CURSORS
#if (!defined(IMGUI_USE_WIN32_CURSORS) && defined(IMGUI_USE_X11_CURSORS))
Display* x11Display;
Window x11Window;
#endif //IMGUI_USE_CURSORS
#endif //IMGUI_GLFW_NO_NATIVE_CURSORS
#endif //GLFW_HAS_MOUSE_CURSOR_SUPPORT
ImImplMainLoopFrameStruct() : done(false) {}
};



static void ImImplMainLoopFrame(void* userPtr)	{
    ImImplMainLoopFrameStruct& mainLoopFrameStruct = *((ImImplMainLoopFrameStruct*) userPtr);

    static double time = 0.0;
    ImGuiIO& io = ImGui::GetIO();

    for (size_t i = 0; i < 5; i++) gImGuiBindingMouseDblClicked[i] = false;   // We manually set it (otherwise it won't work with low frame rates)
    memset(io.NavInputs, 0, sizeof(io.NavInputs));
    if (!gImGuiPaused)	{
        static ImGuiMouseCursor oldCursor = ImGuiMouseCursor_Arrow;
        static bool oldMustHideCursor = io.MouseDrawCursor;
        if (oldMustHideCursor!=io.MouseDrawCursor) {
            glfwSetInputMode(window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
            oldMustHideCursor = io.MouseDrawCursor;
            oldCursor = ImGuiMouseCursor_COUNT;
        }
        if (!io.MouseDrawCursor) {
            if (oldCursor!=ImGui::GetMouseCursor()) {
                oldCursor=ImGui::GetMouseCursor();
#               ifdef GLFW_HAS_MOUSE_CURSOR_SUPPORT
                glfwSetCursor(window,glfwCursors[oldCursor]);
#               else //GLFW_HAS_MOUSE_CURSOR_SUPPORT
#                   ifndef IMGUI_GLFW_NO_NATIVE_CURSORS
#                       ifdef IMGUI_USE_WIN32_CURSORS
                            SetCursor(win32Cursors[oldCursor]);           // If this does not work, it's bacause the native Window must be created with a NULL cursor (but how to tell glfw about it?)
#                       elif defined IMGUI_USE_X11_CURSORS
                            XDefineCursor(mainLoopFrameStruct.x11Display,mainLoopFrameStruct.x11Window,x11Cursors[oldCursor]);
#                       endif
#                   endif //IMGUI_GLFW_NO_NATIVE_CURSORS
#               endif //GLFW_HAS_MOUSE_CURSOR_SUPPORT
            }
        }
    }

    static const int numFramesDelay = 12;
    static int curFramesDelay = -1;
    // What about using: if (gImGuiAppIsIconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED)) instead of listening to glfw_window_iconify_callback ?
    // (some users suggest using !glfwGetWindowAttrib(window, GLFW_FOCUSED) instead..., some other: !glfwGetWindowAttrib(window, GLFW_VISIBLE) )
    if (gImGuiAppIsIconified || (gImGuiWereOutsideImGui ? (gImGuiInverseFPSClampOutsideImGui==0) : (gImGuiInverseFPSClampInsideImGui==0))) {
        //fprintf(stderr,"glfwWaitEvents() Start %1.4f\n",glfwGetTime());
        glfwWaitEvents();
        //fprintf(stderr,"glfwWaitEvents() End %1.4f\n",glfwGetTime());
    }
    else {
        glfwPollEvents();
        //----------------------------------------------------------------------
        // New: Gamepad navigation mapping [BETA]
        // New: Added a Linux version (see GamePadMapping ctr)
        // New: Added gEmulateGamepadWithKeyboard (intended mainly for __EMSCRIPTEN__ to replace NavEnableKeyboard), hard-coded to false
        // Ideally we should make gGamePadMapping and gEmulateGamepadWithKeyboard global variables, so that users can change them.
        // But I don't know what to do with this code snippet, because it's not easy to port it to other bindings.
        // Moreover I don't own any gamepad (I borrowed one the day I made it work on Linux), so there's no
        // chance I'll improve this code... :(
        if (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad)  // TODO: Ideally we'd like to always fill io.NavInputs[] so that we can have a high-level Gamepad API
        {
            struct GamePadMapping {
                int leftStickButtonsOrAxisIndices[4];
                bool leftStickIsAnalog;             // If true, in the above field: [0]=[1]=hor_axis and [2]=[3]=ver_axis

                int dpadButtonsOrAxisIndices[4];
                bool dpadIsAnalog;                  // If true, in the above field: [0]=[1]=hor_axis and [2]=[3]=ver_axis

                int mainButtons[4];                 // Cross/A, Circle/B, Square/X, Triangle/Y
                int lrButtons[4];                   // L1,R1,LB,RB

                // These control the analog sensitivity and direction:
                float analogValuesX[4];             // 2 FOR LEFT AND TWO FOR RIGHT (default: {-0.3f,-0.9f,0.3f,0.9f})
                float analogValuesY[4];             // 2 FOR UP AND TWO FOR DOWN    (handy if we must flip the vertical axis)

                void flipAnalogAxisY() {
                    float tmp=analogValuesY[0];analogValuesY[0]=analogValuesY[2];analogValuesY[2]=tmp;
                    tmp=analogValuesY[1];analogValuesY[1]=analogValuesY[3];analogValuesY[3]=tmp;
                }

                GamePadMapping() {
                    leftStickIsAnalog = true;
                    leftStickButtonsOrAxisIndices[0]=leftStickButtonsOrAxisIndices[1]=0;
                    leftStickButtonsOrAxisIndices[2]=leftStickButtonsOrAxisIndices[3]=1;

                    lrButtons[0]=lrButtons[2]=4;lrButtons[1]=lrButtons[3]=5;

                    analogValuesX[0]=-0.3f;analogValuesX[1]=-0.9f;
                    analogValuesX[2]= 0.3f;analogValuesX[3]= 0.9f;
                    for (int i=0;i<4;i++) analogValuesY[i]=analogValuesX[i];

 #                  ifndef __linux__
                    // This should be the default setting used by Dear ImGui
                    mainButtons[0]=0;mainButtons[1]=1;mainButtons[2]=2;mainButtons[3]=3;
                    dpadIsAnalog = false;
                    dpadButtonsOrAxisIndices[0]=13; dpadButtonsOrAxisIndices[1]=11;
                    dpadButtonsOrAxisIndices[2]=1;  dpadButtonsOrAxisIndices[3]=12;
#                   else //__linux__
                    // This is what I got from a Sony joypad
                    // (that looks like the one used for the default settings)
                    // on Ubuntu Linux (without any sort of global config mapping applied)
                    mainButtons[0]=1;mainButtons[1]=0;mainButtons[2]=2;mainButtons[3]=3;    // Buttons 0 and 1 are inverted
                    dpadIsAnalog = true;                                                    // D-Pad is detected as analog (why?)
                    dpadButtonsOrAxisIndices[0]=dpadButtonsOrAxisIndices[1]=6;
                    dpadButtonsOrAxisIndices[2]=dpadButtonsOrAxisIndices[3]=7;
                    flipAnalogAxisY();                                                      // Analog Up and Down are inverted
#                   endif //__linux__
                }
            };
            static GamePadMapping gGamePadMapping;
            static bool gEmulateGamepadWithKeyboard =   // Intended mainly for __EMSCRIPTEN__ to replace NavEnableKeyboard, that does not work on browsers
                    false;
                    //true;

            if (!gEmulateGamepadWithKeyboard)   {
            // Update gamepad inputs
#           define MAP_BUTTON(NAV_NO, BUTTON_NO)       { if (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS) io.NavInputs[NAV_NO] = 1.0f; }
#           define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; v = (v - V0) / (V1 - V0); if (v > 1.0f) v = 1.0f; if (io.NavInputs[NAV_NO] < v) io.NavInputs[NAV_NO] = v; }
            int axes_count = 0, buttons_count = 0;
            const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
            const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
            GamePadMapping& GP = gGamePadMapping;
            MAP_BUTTON(ImGuiNavInput_Activate,   GP.mainButtons[0]);     // Cross / A
            MAP_BUTTON(ImGuiNavInput_Cancel,     GP.mainButtons[1]);     // Circle / B
            MAP_BUTTON(ImGuiNavInput_Menu,       GP.mainButtons[2]);     // Square / X
            MAP_BUTTON(ImGuiNavInput_Input,      GP.mainButtons[3]);     // Triangle / Y
            if (GP.dpadIsAnalog) {
                MAP_ANALOG(ImGuiNavInput_DpadLeft,   GP.dpadButtonsOrAxisIndices[0], GP.analogValuesX[0],  GP.analogValuesX[1]);    // D-Pad Left
                MAP_ANALOG(ImGuiNavInput_DpadRight,  GP.dpadButtonsOrAxisIndices[1], GP.analogValuesX[2],  GP.analogValuesX[3]);    // D-Pad Right
                MAP_ANALOG(ImGuiNavInput_DpadUp,     GP.dpadButtonsOrAxisIndices[2], GP.analogValuesY[0],  GP.analogValuesY[1]);    // D-Pad Up
                MAP_ANALOG(ImGuiNavInput_DpadDown,   GP.dpadButtonsOrAxisIndices[3], GP.analogValuesY[2],  GP.analogValuesY[3]);    // D-Pad Down
            }
            else    {
                MAP_BUTTON(ImGuiNavInput_DpadLeft,   GP.dpadButtonsOrAxisIndices[0]);    // D-Pad Left
                MAP_BUTTON(ImGuiNavInput_DpadRight,  GP.dpadButtonsOrAxisIndices[1]);    // D-Pad Right
                MAP_BUTTON(ImGuiNavInput_DpadUp,     GP.dpadButtonsOrAxisIndices[2]);    // D-Pad Up
                MAP_BUTTON(ImGuiNavInput_DpadDown,   GP.dpadButtonsOrAxisIndices[3]);    // D-Pad Down
            }
            MAP_BUTTON(ImGuiNavInput_FocusPrev,  GP.lrButtons[0]);     // L1 / LB
            MAP_BUTTON(ImGuiNavInput_FocusNext,  GP.lrButtons[1]);     // R1 / RB
            MAP_BUTTON(ImGuiNavInput_TweakSlow,  GP.lrButtons[2]);     // L1 / LB
            MAP_BUTTON(ImGuiNavInput_TweakFast,  GP.lrButtons[3]);     // R1 / RB
            if (GP.leftStickIsAnalog)  {
                MAP_ANALOG(ImGuiNavInput_LStickLeft, GP.leftStickButtonsOrAxisIndices[0],  GP.analogValuesX[0],  GP.analogValuesX[1]);
                MAP_ANALOG(ImGuiNavInput_LStickRight,GP.leftStickButtonsOrAxisIndices[1],  GP.analogValuesX[2],  GP.analogValuesX[3]);
                MAP_ANALOG(ImGuiNavInput_LStickUp,   GP.leftStickButtonsOrAxisIndices[2],  GP.analogValuesY[0],  GP.analogValuesY[1]);
                MAP_ANALOG(ImGuiNavInput_LStickDown, GP.leftStickButtonsOrAxisIndices[3],  GP.analogValuesY[2],  GP.analogValuesY[3]);
            }
            else    {
                MAP_BUTTON(ImGuiNavInput_LStickLeft,   GP.leftStickButtonsOrAxisIndices[0]);
                MAP_BUTTON(ImGuiNavInput_LStickRight,  GP.leftStickButtonsOrAxisIndices[1]);
                MAP_BUTTON(ImGuiNavInput_LStickUp,     GP.leftStickButtonsOrAxisIndices[2]);
                MAP_BUTTON(ImGuiNavInput_LStickDown,   GP.leftStickButtonsOrAxisIndices[3]);
            }
#           undef MAP_BUTTON
#           undef MAP_ANALOG
            if (axes_count > 0 && buttons_count > 0) io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
            else io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
            }
            else {
                // gEmulateGamepadWithKeyboard
                float analogStep = 0.1f; // Bad stuff
                if (io.Framerate>65.f) analogStep*= 60.f/io.Framerate;    // Never tested
#               define MAP_BUTTON_PRESSED(NAV_NO, IMGUI_KEY_NO, MOD_BOOL)  {io.NavInputs[NAV_NO] = (MOD_BOOL && ImGui::IsKeyPressed(IMGUI_KEY_NO)) ? 1.0f : 0.f;}
#               define MAP_BUTTON_DOWN(NAV_NO, GLFW_KEY_NO, MOD_BOOL)  { io.NavInputs[NAV_NO] = (MOD_BOOL && glfwGetKey(window,GLFW_KEY_NO)==GLFW_PRESS) ? 1.0f : 0.f;}
#               define MAP_ANALOG(NAV_NO, GLFW_KEY_NO,MOD_BOOL,MOD_BOOL_HALF) {float v =((MOD_BOOL && glfwGetKey(window,GLFW_KEY_NO)==GLFW_PRESS) ? analogStep : 0.f)*(MOD_BOOL_HALF?0.5f:1.f); if (v > 1.0f) v = 1.0f; if (io.NavInputs[NAV_NO] < v) io.NavInputs[NAV_NO] = v;}

                const bool noMods = (!io.KeyCtrl && !io.KeyShift);

                // These work.
                MAP_BUTTON_PRESSED(ImGuiNavInput_Activate,  io.KeyMap[ImGuiKey_Space],      noMods);
                MAP_BUTTON_PRESSED(ImGuiNavInput_Cancel,    io.KeyMap[ImGuiKey_Delete],     noMods);
                MAP_BUTTON_PRESSED(ImGuiNavInput_Input,     io.KeyMap[ImGuiKey_Enter],      noMods);
                MAP_BUTTON_DOWN(ImGuiNavInput_Menu,      GLFW_KEY_KP_0,    noMods);

                MAP_BUTTON_DOWN(ImGuiNavInput_DpadLeft, GLFW_KEY_LEFT,   noMods);
                MAP_BUTTON_DOWN(ImGuiNavInput_DpadRight,GLFW_KEY_RIGHT,  noMods);
                MAP_BUTTON_DOWN(ImGuiNavInput_DpadUp,   GLFW_KEY_UP,     noMods);
                MAP_BUTTON_DOWN(ImGuiNavInput_DpadDown, GLFW_KEY_DOWN,   noMods);

                MAP_BUTTON_DOWN(ImGuiNavInput_FocusPrev, GLFW_KEY_KP_7,     noMods);
                MAP_BUTTON_DOWN(ImGuiNavInput_FocusNext, GLFW_KEY_KP_9,     noMods);
                MAP_BUTTON_DOWN(ImGuiNavInput_TweakSlow, GLFW_KEY_KP_7,     noMods);
                MAP_BUTTON_DOWN(ImGuiNavInput_TweakFast, GLFW_KEY_KP_9,     noMods);

                // These work (partially: io.KeyCtrl does nothing and there's the intimidating 'analogStep' above).
                MAP_ANALOG(ImGuiNavInput_LStickLeft,    GLFW_KEY_KP_4,  noMods,  io.KeyCtrl);
                MAP_ANALOG(ImGuiNavInput_LStickRight,   GLFW_KEY_KP_6,  noMods,  io.KeyCtrl);
                MAP_ANALOG(ImGuiNavInput_LStickUp,      GLFW_KEY_KP_8,  noMods,  io.KeyCtrl);
                MAP_ANALOG(ImGuiNavInput_LStickDown,    GLFW_KEY_KP_2,  noMods,  io.KeyCtrl);

#               undef MAP_BUTTON_PRESSED
#               undef MAP_BUTTON_DOWN
#               undef MAP_ANALOG
                io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
            }
        //------------------------------------------------------------------------
        }
    }

    // Setup timestep
    const double current_time =  glfwGetTime();
    static float deltaTime = (float)(current_time -time);
    deltaTime = (float) (current_time - time);
    time = current_time;

    // Start the frame
    {
        io.DeltaTime = deltaTime;
        for (size_t i = 0; i < 5; i++) {
            io.MouseDown[i]= g_MousePressed[i] || glfwGetMouseButton(window, i); // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
            g_MousePressed[i] = false;
        }
        if (!gImGuiPaused) {
            if (io.WantSetMousePos)  {
                // Set mouse position if requested by io.WantSetMousePos flag (used when io.NavMovesTrue is enabled by user and using directional navigation)
                glfwSetCursorPos(window, (double)io.MousePos.x, (double)io.MousePos.y);
            }
            /*else    {
                // Get mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
                double mouse_x, mouse_y;
                glfwGetCursorPos(window, &mouse_x, &mouse_y);
                io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
            }*/

            ImGui::NewFrame();
        }
        else {
            ImImpl_NewFramePaused();    // Enables some ImGui queries regardless ImGui::NewFrame() not being called.
            gImGuiCapturesInput = false;
        }
        for (size_t i = 0; i < 5; i++) io.MouseDoubleClicked[i]=gImGuiBindingMouseDblClicked[i];   // We manually set it (otherwise it won't work with low frame rates)
    }

    if (gImGuiPreDrawGLCallback) gImGuiPreDrawGLCallback();
    DrawGL();


    if (!gImGuiPaused)	{
        gImGuiWereOutsideImGui = !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !ImGui::IsAnyItemActive();
        const bool imguiNeedsInputNow = !gImGuiWereOutsideImGui && (io.WantTextInput || io.MouseDelta.x!=0 || io.MouseDelta.y!=0 || io.MouseWheel!=0  || io.MouseWheelH!=0);// || io.MouseDownOwned[0] || io.MouseDownOwned[1] || io.MouseDownOwned[2]);
        if (gImGuiCapturesInput != imguiNeedsInputNow) {
            gImGuiCapturesInput = imguiNeedsInputNow;
             if (gImGuiDynamicFPSInsideImGui) {
                if (!gImGuiCapturesInput && !gImGuiWereOutsideImGui) curFramesDelay = 0;
                else curFramesDelay = -1;                
            }            
        }
        if (gImGuiWereOutsideImGui) curFramesDelay = -1;
        //fprintf(stderr,"gImGuiCapturesInput=%s curFramesDelay=%d wereOutsideImGui=%s\n",gImGuiCapturesInput?"true":"false",curFramesDelay,wereOutsideImGui?"true":"false");

        // Rendering
        ImGui::Render();
        ImImpl_RenderDrawLists(ImGui::GetDrawData());
    }
    else {gImGuiWereOutsideImGui=true;curFramesDelay = -1;}

    if (gImGuiPreDrawGLSwapBuffersCallback) gImGuiPreDrawGLSwapBuffersCallback();
    glfwSwapBuffers(window);
    if (gImGuiPostDrawGLSwapBuffersCallback) gImGuiPostDrawGLSwapBuffersCallback();

    // Reset additional special keys composed states (mandatory):
    for (int i=0;i<12;i++) {gImGuiFunctionKeyPressed[i] = gImGuiFunctionKeyReleased[i]= false;}

    // Handle clamped FPS:
    if (curFramesDelay>=0 && ++curFramesDelay>numFramesDelay) WaitFor(200);     // 200 = 5 FPS - frame rate when ImGui is inactive
    else {
        const float& inverseFPSClamp = gImGuiWereOutsideImGui ? gImGuiInverseFPSClampOutsideImGui : gImGuiInverseFPSClampInsideImGui;
        if (inverseFPSClamp==0.f) WaitFor(500);
        // If needed we must wait (inverseFPSClamp-deltaTime) seconds (=> honestly I shouldn't add the * 2.0f factor at the end, but ImGui tells me the wrong FPS otherwise... why? <=)
        else if (inverseFPSClamp>0 && deltaTime < inverseFPSClamp)  WaitFor((unsigned int) ((inverseFPSClamp-deltaTime)*1000.f * 2.0f) );
    }

#	ifdef __EMSCRIPTEN__
    if ((mainLoopFrameStruct.done=!glfwWindowShouldClose(window))==0) emscripten_cancel_main_loop();
#   endif //__EMSCRIPTEN__
}


// Application code
int ImImpl_Main(const ImImpl_InitParams* pOptionalInitParams,int argc, char** argv)
{
    if (!InitBinding(pOptionalInitParams,argc,argv)) return -1;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    InitImGui(pOptionalInitParams);
    ImGuiIO& io = ImGui::GetIO();

ImImplMainLoopFrameStruct mainLoopFrameStruct;
    // New: create cursors-------------------------------------------
#ifdef GLFW_HAS_MOUSE_CURSOR_SUPPORT
        for (int i=0,isz=ImGuiMouseCursor_COUNT+1;i<isz;i++) {
            glfwCursors[i] = glfwCreateStandardCursor(glfwCursorIds[i]);
            if (i==0) glfwSetCursor(window,glfwCursors[i]);
        }
#else //GLFW_HAS_MOUSE_CURSOR_SUPPORT
#   ifndef IMGUI_GLFW_NO_NATIVE_CURSORS
#       ifdef IMGUI_USE_WIN32_CURSORS
        for (int i=0,isz=ImGuiMouseCursor_COUNT+1;i<isz;i++) {
            win32Cursors[i] = LoadCursor(NULL,(LPCTSTR) win32CursorIds[i]);
            if (i==0) SetCursor(win32Cursors[i]);
        }
#       elif defined IMGUI_USE_X11_CURSORS
        mainLoopFrameStruct.x11Display = glfwGetX11Display();
        mainLoopFrameStruct.x11Window = glfwGetX11Window(window);
        //XColor white;white.red=white.green=white.blue=255;
        //XColor black;black.red=black.green=black.blue=0;
        for (int i=0,isz=ImGuiMouseCursor_COUNT+1;i<isz;i++) {
            x11Cursors[i] = XCreateFontCursor(mainLoopFrameStruct.x11Display,x11CursorIds[i]);
            //XRecolorCursor(x11Display, x11Cursors[i], &white,&black);
            if (i==0) XDefineCursor(mainLoopFrameStruct.x11Display,mainLoopFrameStruct.x11Window,x11Cursors[i]);
        }
#       endif
#   endif //IMGUI_GLFW_NO_NATIVE_CURSORS
#endif //GLFW_HAS_MOUSE_CURSOR_SUPPORT
    //---------------------------------------------------------------

    InitGL();
    if (gImGuiPostInitGLCallback) gImGuiPostInitGLCallback();
 	ResizeGL(io.DisplaySize.x,io.DisplaySize.y);
	
    gImGuiInverseFPSClampInsideImGui = pOptionalInitParams ? ((pOptionalInitParams->gFpsClampInsideImGui!=0) ? (1.0f/pOptionalInitParams->gFpsClampInsideImGui) : 1.0f) : -1.0f;
    gImGuiInverseFPSClampOutsideImGui = pOptionalInitParams ? ((pOptionalInitParams->gFpsClampOutsideImGui!=0) ? (1.0f/pOptionalInitParams->gFpsClampOutsideImGui) : 1.0f) : -1.0f;
    gImGuiDynamicFPSInsideImGui = pOptionalInitParams ? pOptionalInitParams->gFpsDynamicInsideImGui : false;

    mainLoopFrameStruct.done = 0;
#	ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(ImImplMainLoopFrame,&mainLoopFrameStruct, 0, 1);
#	else
    while ((mainLoopFrameStruct.done=!glfwWindowShouldClose(window)))   {
        ImImplMainLoopFrame((void*)&mainLoopFrameStruct);
    }
#	endif //__EMSCRIPTEN__


    DestroyGL();
    ImGui::DestroyContext();
    DestroyImGuiFontTexture();
    DestroyImGuiProgram();
    DestroyImGuiBuffer();

    // New: delete cursors-------------------------------------------
#   ifdef  GLFW_HAS_MOUSE_CURSOR_SUPPORT
    for (int i=0,isz=ImGuiMouseCursor_COUNT+1;i<isz;i++) {
        glfwDestroyCursor(glfwCursors[i]);
    }
#   else   //GLFW_HAS_MOUSE_CURSOR_SUPPORT
#       ifndef IMGUI_GLFW_NO_NATIVE_CURSORS
#           ifdef IMGUI_USE_WIN32_CURSORS
                // Nothing to do
#           elif defined IMGUI_USE_X11_CURSORS
                XUndefineCursor(mainLoopFrameStruct.x11Display,mainLoopFrameStruct.x11Window);
                for (int i=0,isz=ImGuiMouseCursor_COUNT+1;i<isz;i++) {
                    XFreeCursor(mainLoopFrameStruct.x11Display,x11Cursors[i]);
                }
#           endif
#       endif //IMGUI_GLFW_NO_NATIVE_CURSORS
#   endif //GLFW_HAS_MOUSE_CURSOR_SUPPORT
    //---------------------------------------------------------------

    glfwTerminate();
    return 0;
}

#endif //#ifndef IMIMPL_BINDING_H

