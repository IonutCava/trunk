#include "stdafx.h"

#include "Headers/ImWindowDivide.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Rendering/Camera/Headers/Camera.h"


namespace Divide {

U32 ImwWindowDivide::m_windowCount = 0;

ImwWindowDivide::ImwWindowDivide(PlatformContext& context, ImWindow::EPlatformWindowType eType, bool bCreateState)
    : ImwPlatformWindow(eType, bCreateState)
    , m_context(context)
    , m_pWindow(nullptr)
    , m_isMainWindow(false)
{
    ++m_windowCount;
    ResourceDescriptor shaderDescriptor("IMGUI");
    shaderDescriptor.setThreadedLoading(false);
    m_imWindowProgram = CreateResource<ShaderProgram>(context.gfx().parent().resourceCache(), shaderDescriptor);
}

ImwWindowDivide::~ImwWindowDivide()
{
    if (!m_isMainWindow) {
        m_context.app().windowManager().destroyWindow(m_pWindow);
    }

    --m_windowCount;
}

bool ImwWindowDivide::Init(ImwPlatformWindow* parent)
{
    ImwWindowDivide* pMainDivide= ((ImwWindowDivide*)parent);

    SetState();
    ImGuiIO& io = ImGui::GetIO();

    if (pMainDivide != nullptr) {
        //Copy texture reference
        m_texture = pMainDivide->m_texture;

        WindowDescriptor descriptor;
        descriptor.fullscreen = false;
        descriptor.title = "ImWindow";
        descriptor.targetDisplay = 0;
        descriptor.dimensions.set(m_context.config().runtime.resolution);

        ErrorCode err = ErrorCode::NO_ERR;
        U32 idx = m_context.app().windowManager().createWindow(descriptor, err);
        if (err != ErrorCode::NO_ERR) {
            RestoreState();
            return false;
        }
        m_pWindow = &m_context.app().windowManager().getWindow(idx);
    } else {
        unsigned char* pPixels;
        int iWidth;
        int iHeight;
        io.Fonts->AddFontDefault();
        io.Fonts->GetTexDataAsRGBA32(&pPixels, &iWidth, &iHeight);

        SamplerDescriptor sampler;
        sampler.setFilters(TextureFilter::LINEAR);

        TextureDescriptor descriptor(TextureType::TEXTURE_2D,
                                     GFXImageFormat::RGBA8,
                                     GFXDataFormat::UNSIGNED_BYTE);
        descriptor.setSampler(sampler);

        ResourceDescriptor textureDescriptor(Util::StringFormat("ImWindow_%d", m_windowCount));
        textureDescriptor.setThreadedLoading(false);
        textureDescriptor.setFlag(true);
        textureDescriptor.setPropertyDescriptor(descriptor);
        
        ResourceCache& parentCache = m_context.gfx().parent().resourceCache();
        m_texture = CreateResource<Texture>(parentCache, textureDescriptor);
        assert(m_texture);

        Texture::TextureLoadInfo info;

        m_texture->loadData(info, (bufferPtr)pPixels, vec2<U16>(iWidth, iHeight));

        // Store our identifier
        io.Fonts->TexID = (void *)(intptr_t)m_texture->getHandle();
        io.Fonts->ClearInputData();
        io.Fonts->ClearTexData();

        m_pWindow = &m_context.app().windowManager().getWindow(0u);
        m_isMainWindow = true;
    }

    m_pWindow->addEventListener(WindowEvent::CLOSE_REQUESTED, [this](const DisplayWindow::WindowEventArgs& args) { ACKNOWLEDGE_UNUSED(args); OnClose();});
    m_pWindow->addEventListener(WindowEvent::GAINED_FOCUS, [this](const DisplayWindow::WindowEventArgs& args) { OnFocus(args._flag);});
    m_pWindow->addEventListener(WindowEvent::RESIZED_EXTERNAL, [this](const DisplayWindow::WindowEventArgs& args) { OnSize(args.x, args.y);});
    m_pWindow->addEventListener(WindowEvent::MOUSE_BUTTON, [this](const DisplayWindow::WindowEventArgs& args) { OnMouseButton(args.id, args._flag);});
    m_pWindow->addEventListener(WindowEvent::MOUSE_MOVE, [this](const DisplayWindow::WindowEventArgs& args) { OnMouseMove(args.x, args.y);});
    m_pWindow->addEventListener(WindowEvent::MOUSE_WHEEL, [this](const DisplayWindow::WindowEventArgs& args) { OnMouseWheel(args.y);});
    m_pWindow->addEventListener(WindowEvent::KEY_PRESS, [this](const DisplayWindow::WindowEventArgs& args) { OnKey(args._key, args._flag);});
    m_pWindow->addEventListener(WindowEvent::CHAR, [this](const DisplayWindow::WindowEventArgs& args) { OnChar(args._char);});
    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.

    io.KeyMap[ImGuiKey_Tab] = Input::KeyCode::KC_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = Input::KeyCode::KC_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = Input::KeyCode::KC_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = Input::KeyCode::KC_UP;
    io.KeyMap[ImGuiKey_DownArrow] = Input::KeyCode::KC_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = Input::KeyCode::KC_PGUP;
    io.KeyMap[ImGuiKey_PageDown] = Input::KeyCode::KC_PGDOWN;
    io.KeyMap[ImGuiKey_Home] = Input::KeyCode::KC_HOME;
    io.KeyMap[ImGuiKey_End] = Input::KeyCode::KC_END;
    io.KeyMap[ImGuiKey_Delete] = Input::KeyCode::KC_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = Input::KeyCode::KC_BACK;
    io.KeyMap[ImGuiKey_Enter] = Input::KeyCode::KC_RETURN;
    io.KeyMap[ImGuiKey_Escape] = Input::KeyCode::KC_ESCAPE;
    io.KeyMap[ImGuiKey_A] = Input::KeyCode::KC_A;
    io.KeyMap[ImGuiKey_C] = Input::KeyCode::KC_C;
    io.KeyMap[ImGuiKey_V] = Input::KeyCode::KC_V;
    io.KeyMap[ImGuiKey_X] = Input::KeyCode::KC_X;
    io.KeyMap[ImGuiKey_Y] = Input::KeyCode::KC_Y;
    io.KeyMap[ImGuiKey_Z] = Input::KeyCode::KC_Z;

    io.RenderDrawListsFn = nullptr;

    io.ImeWindowHandle = (void*)m_pWindow->handle()._handle;

    RestoreState();

    return true;
}

ImVec2 ImwWindowDivide::GetPosition() const
{
    return ImVec2(m_pWindow->getPosition());
}

ImVec2 ImwWindowDivide::GetSize() const
{
    return ImVec2(m_pWindow->getDimensions());
}

bool ImwWindowDivide::IsWindowMaximized() const
{
    return m_pWindow->maximized();
}

bool ImwWindowDivide::IsWindowMinimized() const {
    return m_pWindow->minimized();
}

void ImwWindowDivide::Show(bool bShow) {
    m_pWindow->hidden(!bShow);
}

void ImwWindowDivide::SetSize(int iWidth, int iHeight)
{
    m_pWindow->setDimensions(to_U16(iWidth), to_U16(iHeight));
}

void ImwWindowDivide::SetPosition(int iX, int iY)
{
    m_pWindow->setPosition(iX, iY);
}

void ImwWindowDivide::SetWindowMaximized(bool bMaximized)
{
    m_pWindow->maximized(bMaximized);
}

void ImwWindowDivide::SetWindowMinimized()
{
    m_pWindow->minimized(true);
}

void ImwWindowDivide::SetTitle(const char* pTitle)
{
    m_pWindow->title(pTitle);
}

void ImwWindowDivide::PreUpdate()
{

    ImGuiIO& oIO = ((ImGuiContext*)m_pState)->IO;

    //m_pWindow->update(oIO.DeltaTime);

    oIO.KeyCtrl = m_context.input().getKeyState(0, Input::KeyCode::KC_LCONTROL) == Input::InputState::PRESSED ||
                  m_context.input().getKeyState(0, Input::KeyCode::KC_RCONTROL) == Input::InputState::PRESSED;

    oIO.KeyShift = m_context.input().getKeyState(0, Input::KeyCode::KC_LSHIFT) == Input::InputState::PRESSED ||
                   m_context.input().getKeyState(0, Input::KeyCode::KC_RSHIFT) == Input::InputState::PRESSED;

    oIO.KeyAlt = m_context.input().getKeyState(0, Input::KeyCode::KC_LMENU) == Input::InputState::PRESSED ||
                 m_context.input().getKeyState(0, Input::KeyCode::KC_RMENU) == Input::InputState::PRESSED;
    oIO.KeySuper = false;

    if (oIO.MouseDrawCursor)
    {
        m_pWindow->setCursorStyle(CursorStyle::NONE);
    } else if (oIO.MousePos.x != -1.f && oIO.MousePos.y != -1.f)
    {
        switch (((ImGuiContext*)m_pState)->MouseCursor)
        {
            case ImGuiMouseCursor_Arrow:
                m_pWindow->setCursorStyle(CursorStyle::ARROW);
                break;
            case ImGuiMouseCursor_TextInput:         // When hovering over InputText, etc.
                m_pWindow->setCursorStyle(CursorStyle::TEXT_INPUT);
                break;
            case ImGuiMouseCursor_Move:              // Unused
                m_pWindow->setCursorStyle(CursorStyle::HAND);
                break;
            case ImGuiMouseCursor_ResizeNS:          // Unused
                m_pWindow->setCursorStyle(CursorStyle::RESIZE_NS);
                break;
            case ImGuiMouseCursor_ResizeEW:          // When hovering over a column
                m_pWindow->setCursorStyle(CursorStyle::RESIZE_EW);
                break;
            case ImGuiMouseCursor_ResizeNESW:        // Unused
                m_pWindow->setCursorStyle(CursorStyle::RESIZE_NESW);
                break;
            case ImGuiMouseCursor_ResizeNWSE:        // When hovering over the bottom-right corner of a window
                m_pWindow->setCursorStyle(CursorStyle::RESIZE_NWSE);
                break;
        }
    }
}

void ImwWindowDivide::Render()
{
    if (!m_bNeedRender) {
        return;
    }

    SetState();
    ImGui::GetIO().DisplaySize = GetSize();

    ImGui::Render();
    RenderDrawList(ImGui::GetDrawData());
    
    RestoreState();
}

bool ImwWindowDivide::OnClose() {
    ImwPlatformWindow::OnClose();
    return true;
}

void ImwWindowDivide::OnFocus(bool bHasFocus)
{
    if (!bHasFocus) {
        OnLoseFocus();
    }
}

void ImwWindowDivide::OnSize(int iWidth, int iHeight)
{
    ACKNOWLEDGE_UNUSED(iWidth);
    ACKNOWLEDGE_UNUSED(iHeight);
}

void ImwWindowDivide::OnMouseButton(int iButton, bool bDown)
{
    ((ImGuiContext*)m_pState)->IO.MouseDown[iButton] = bDown;
}

void ImwWindowDivide::OnMouseMove(int iX, int iY)
{
    ((ImGuiContext*)m_pState)->IO.MousePos = ImVec2((float)iX, (float)iY);
}

void ImwWindowDivide::OnMouseWheel(int iStep)
{
    ((ImGuiContext*)m_pState)->IO.MouseWheel += iStep;
}

void ImwWindowDivide::OnKey(Input::KeyCode eKey, bool bDown)
{
    ((ImGuiContext*)m_pState)->IO.KeysDown[eKey] = bDown;
}

void ImwWindowDivide::OnChar(int iChar)
{
    ((ImGuiContext*)m_pState)->IO.AddInputCharacter((ImwChar)iChar);
}

void ImwWindowDivide::RenderDrawList(ImDrawData* pDrawData)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0) {
        return;
    }

    pDrawData->ScaleClipRects(io.DisplayFramebufferScale);

    GFX::ScopedCommandBuffer sBuffer = GFX::allocateScopedCommandBuffer();
    GFX::CommandBuffer& buffer = sBuffer();

    RenderStateBlock state;
    state.setCullMode(CullMode::NONE);
    state.setZRead(false);
    state.setScissorTest(true);

    PipelineDescriptor pipelineDesc;
    pipelineDesc._stateHash = state.getHash();
    pipelineDesc._shaderProgram = m_imWindowProgram;

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = std::numeric_limits<U16>::max();
    beginDebugScopeCmd._scopeName = "Render IMGUI";
    GFX::BeginDebugScope(buffer, beginDebugScopeCmd);

    GFX::SetBlendCommand blendCmd;
    blendCmd._enabled = true;
    blendCmd._blendProperties = BlendingProperties{
        BlendProperty::SRC_ALPHA,
        BlendProperty::INV_SRC_ALPHA
    };
    GFX::SetBlend(buffer, blendCmd);

    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = &m_context.gfx().newPipeline(pipelineDesc);
    GFX::BindPipeline(buffer, pipelineCmd);

    GFX::SetViewportCommand viewportCmd;
    viewportCmd._viewport.set(0, 0, fb_width, fb_height);
    GFX::SetViewPort(buffer, viewportCmd);

    GFX::SetCameraCommand cameraCmd;
    cameraCmd._camera = Camera::utilityCamera(Camera::UtilityCamera::_2D);
    GFX::SetCamera(buffer, cameraCmd);

    GFX::DrawIMGUICommand drawIMGUI;
    drawIMGUI._data = pDrawData;
    GFX::AddDrawIMGUICommand(buffer, drawIMGUI);

    GFX::EndDebugScopeCommand endDebugScope;
    GFX::EndDebugScope(buffer, endDebugScope);

    m_context.gfx().flushCommandBuffer(buffer);
}
}; //namespace Divide