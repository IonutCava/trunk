// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.


#ifndef IMGUIPDFVIEWER_H_
#define IMGUIPDFVIEWER_H_

#ifndef IMGUI_API
#include <imgui.h>
#endif //IMGUI_API

// Code ported from another one of my projects (used a different GUI and worked like a charm!).
// Now code is very dirty/buggy/terribly slow.
// Many features don't work (e.g. text selection) and other are buggy (e.g. find text),
// other are simply not implemented yet (e.g. 90% of the options in the context-menu).

// But It's too much work for me for now to make it work like it should (these two files could be a stand-alone git repository by themselves!).

namespace ImGui {

class PdfPagePanel;

class PdfViewer 
#if (defined(IMGUITABWINDOW_H_) && !defined(IMGUIPDFVIEWER_NO_TABLABEL))
: public TabWindow::TabLabel
#endif //IMGUITABWINDOW_H_
{
public:
    IMGUI_API PdfViewer();
#	if (defined(IMGUITABWINDOW_H_) && !defined(IMGUIPDFVIEWER_NO_TABLABEL))
    IMGUI_API virtual	~PdfViewer();
    bool saveAs(const char* savePath=NULL) {return true;}	// A viewer does not use this, nor the setModified(...) flag.
#	else  //IMGUITABWINDOW_H_
    IMGUI_API ~PdfViewer();
#	endif //IMGUITABWINDOW_H_
    

    bool isInited() const {return init;}

    IMGUI_API bool loadFromFile(const char* path);

    IMGUI_API void destroy();

    // returns true if some user action has been processed
    IMGUI_API bool render(const ImVec2 &size);    // to be called inside an ImGui::Window. Makes isInited() return true;
	void render() {render(ImVec2(0,0));} // changed the return value to void, so that it matches the TabLabel contract too

    static const double IMAGE_DPI;// = 150;	(moved definition to .cpp file)
    typedef void (*FreeTextureDelegate)(ImTextureID& texid);
    typedef void (*GenerateOrUpdateTextureDelegate)(ImTextureID& imtexid,int width,int height,int channels,const unsigned char* pixels,bool useMipmapsIfPossible,bool wraps,bool wrapt);
    static void SetFreeTextureCallback(FreeTextureDelegate freeTextureCb) {FreeTextureCb=freeTextureCb;}
    static void SetGenerateOrUpdateTextureCallback(GenerateOrUpdateTextureDelegate generateOrUpdateTextureCb) {GenerateOrUpdateTextureCb=generateOrUpdateTextureCb;}

protected:
    class PdfPagePanel* pagePanel;
    bool init;
    char* filePath;                             // owned = "";

    static FreeTextureDelegate FreeTextureCb;
    static GenerateOrUpdateTextureDelegate GenerateOrUpdateTextureCb;

    friend class PdfPagePanel;
};

} // namespace ImGui

#endif //IMGUIPDFVIEWER_H_

