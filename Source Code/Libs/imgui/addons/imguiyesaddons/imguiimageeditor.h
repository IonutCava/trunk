/*
MIT License

Copyright (c) 2017 by Flix (https://github.com/Flix01)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef IMGUIIMAGEEDITOR_H_
#define IMGUIIMAGEEDITOR_H_


#ifndef IMGUI_API
#include <imgui.h>	
#endif //IMGUI_API

// OVERVIEW:
/* It's a raw and very basic image editor (or better an icon editor) for Dear ImGui in a single .cpp file (well.., compilation unit).
 * It has no dependency other than Dear ImGui.
 *
 * It uses:
 * <stb_image.h> to load all image formats-
 * <stb_image_write.h> to save .png, .tga, .bmp and .jpg formats (.png anf .jpg encoders can be overridden)-
 * <stb_image_resize.h> to resize images (it can be optionally disabled)-
 * Other non-stb plugins can be optionally included (see below).
*/

// USAGE:
/*
1) First of all, before doing any operation with the ImGui::ImageEditor class, you must set its static callbacks:
     ImGui::ImageEditor::SetGenerateOrUpdateTextureCallback(...);   // This will be called only with channels=3 or channels=4
     ImGui::ImageEditor::SetFreeTextureCallback(...);

2a)  // Globally:
     ImGui::ImageEditor imageEditor;
     // In initGL() or similiar:
     if (!imageEditor.loadFromFile("myImage.png")) printf("Loading Failed.\n");
     // In drawGL() or similiar, inside a ImGuiWindow:
     imageEditor.render();
     // In destroyGL() or similiar, to destroy the textures: (*)
     imageEditor.destroy();         // (*) Frees instance textures

2b) //Easier way (that doesn't delete instance textures other than in the destructor):
    // In drawGL() or similiar, inside a ImGuiWindow:
    static ImGui::ImageEditor imageEditor;
    if (!imageEditor.isInited()) if (!imageEditor.loadFromFile("myImage.png")) printf("Loading Failed.\n");
    imageEditor.render();   // makes isInited() return false

3)  // In destroyGL() or similiar
    ImGui::ImageEditor::Destroy(); // Frees shared textures

NOTES:
     -> Steps 1) and 3) are done for you if you use a IMGUI_USE_XXX_BINDING definition of the imgui_addon framework.

     -> (*) Of course the ImageEditor::~ImageEditor() calls destroy(), but it can be too late to destroy the textures.
        Note that you're free to reuse the instance after the destroy() call, but then
        you need to call again: imageEditor.destroy()
*/

// Project Options Optional Definitions:
/*
 * IMGUIIMAGEEDITOR_STBIMAGE_PATH           - (default:"./addons/imguibindings/stb_image.h")
 *
 * IMGUIIMAGEEDITOR_ENABLE_NON_STB_PLUGINS  - Enables support for saving .gif and for loading/saving .ico files
 *                                          - Overrides the .png and .jpg encoders from <stb_image_write.h> to
 *                                            lodePng.h and tiny_jpeg.h (Each can be reverted by defining
 *                                            IMGUIIMAGEEDITOR_NO_LODEPNG_PLUGIN and/or
 *                                            IMGUIIMAGEEDITOR_NO_TINY_JPEG_PLUGIN).
 *
 *                                            ===>> WARNING <<==
 *                                            These are all proprietary formats that MIGHT contain PATENTED algorithms.
 *                                            ====>>> USE THEM AT YOUR OWN RISK!!! <<<====
 *                                            You can further filter the non-stb plugins by using:
 *                                            IMGUIIMAGEEDITOR_NO_LODEPNG_PLUGIN        // <stb_image_write.h> fallback
 *                                            IMGUIIMAGEEDITOR_NO_TINY_JPEG_PLUGIN      // <stb_image_write.h> fallback
 *                                            IMGUIIMAGEEDITOR_NO_JO_GIF_PLUGIN
 *                                            IMGUIIMAGEEDITOR_NO_TINY_ICO_PLUGIN
 *                                            When not used, these plugins can be safely deleted from the plugin folder.
 *
 *                                            [Experimental]: when IMGUIIMAGEEDITOR_ENABLE_NON_STB_PLUGINS is defined,
 *                                            you can further define IMGUI_USE_LIBTIFF
 *                                            (it requires libtiff to be installed in your system) to load/save
 *                                            .tiff files. Still: check out libtiff license and be warned that this
 *                                            format might contain patented algorithms.
 *                                            [Experimental]: when IMGUIIMAGEEDITOR_ENABLE_NON_STB_PLUGINS is defined,
 *                                            you can further define IMGUI_USE_LIBWEBP
 *                                            (it requires libwebp to be installed in your system) to load/save
 *                                            .webp files. Still: check out libwebp license and be warned that this
 *                                            format might contain patented algorithms.
 *
 *                                            Since version 0.31, when IMGUIIMAGEEDITOR_ENABLE_NON_STB_PLUGINS is defined,
 *                                            it's even possible to disable <stb_image_write.h> completely by defining:
 *                                            IMGUIIMAGEEDITOR_NO_STB_IMAGE_WRITE_PLUGIN (if you don't need .tga and .bmp support,
 *                                            and you use other plugins for .png and .jpg). However, since tiny_ico.h needs a png
 *                                            encoder, and it currently supports only <stb_image_write.h>, this is currently possible
 *                                            only if you disable .ico support with IMGUIIMAGEEDITOR_NO_TINY_ICON_PLUGIN too.
 *                                            A modification of tiny_ico.h that use the lodePng encoder can be added, if somebody requests it.
 *
 * IMGUIIMAGEEDITOR_LOAD_ONLY_SAVABLE_FORMATS - does what it says
 *
 * stb_image definitions can be used too (for example STBI_NO_GIF,etc.), but please make sure that you can load back
 * all the savable image formats.
 *
 * UPDATE: Added ability to load and browse (but not to save) images inside zip folders,
 * when IMGUI_USE_MINIZIP is defined and imguifilesystem is used (this needs linking to zlib).
*/

// Please post bugs here: https://github.com/Flix01/imgui/issues

/* TODO:
 * Fix bugs
 * Add more tooltips
*/

/* CHANGELOG:
 IMGUIIMAGEEDITOR_VERSION 0.31
 * Updated stb_image.h, stb_image_write.h, stb_image_resize.h. Added lodepng.h/cpp.
 *
 *

 IMGUIIMAGEEDITOR_VERSION 0.3
 * Added optional support for the WebP format. Issues:
 * -> In an animated webp, no frame is loaded.
 * -> Saving quality is hardcoded at 80 for RGB and LossLess for RGBA images.
 * -> It's not possible to save a RGB webp image as RGBA, because the webp encoder strips the alpha channel when it's not used.

 IMGUIIMAGEEDITOR_VERSION 0.25
 * Changed the license to the default MIT license (it's actually shorter than before)
 * Added a (slow and buggy) normal map filter
 * Added a (even slower and buggier) ambient occlusion / bent normals generator, based on:
 *  (1) https://blenderartists.org/forum/showthread.php?330268-Self-Shadowing-Normal-Maps
 *  (2) https://www.youtube.com/watch?v=e1e6o7KlhEM
 *  (3) https://www.gamedev.net/topic/557465-self-shadowing-normal-maps/
 *  (4) https://www.gamedev.net/topic/469124-bent-normals--ambient-occlusion-for-heightmaps/        by lonesock
 *      Bent Normals work on the same images as (1) and (2) almost correctly, but it doesn't on 80% of all the other images I've tried so far.
 *      Ambient Occlusion seems to work always correctly (and that was my main goal actually).
 * When saving as RGB or RGBA from A, put A into RGB channels
 * Now the ET_IMAGE_LOADED event is fired only when an image is loaded/reloaded, not after saving an image (this is what every user expects)
 * Now "undo" removes the "modified flag" when necessary
 * Now the preview image of the Brightness and Constrast dialog has better resolution (max w*h = 256x256, previously 128x128)
 * Previously the file name changed after extracting a selection to a new image (and there was an option to keep the existing name):
 *      now this is no more possible (reason: "Undo" can't remember old file names and restore them).

 IMGUIIMAGEEDITOR_VERSION 0.20
 * Added a Gaussian Blur filter
 * Made some collapsable headers closed by default

 IMGUIIMAGEEDITOR_VERSION 0.15
 * Now Paste blends the copied image's alpha with the background, or just copies its alpha value (according to the image channels).
 * Removed a hard-coded definition and added an "Optimize RGBA" check button.
 * Now "Save As ALPHA", when called on a RGBA image, should do better what the user expects (= save the alpha channel only if it's meaningful, otherwise save luminance).
 * Basically with this version it should be possible to merge an RGB image with an ALPHA image and split back an RGBA image into an RGB image and an ALPHA image.

 IMGUIIMAGEEDITOR_VERSION 0.12
 * Added ability to show/hide the image name panel by pressing key S
 * Added getImageFileName() and getImageFileFileExtension()
 * Added a proper empty ctr
 * Added round pen
 * Added pen in overlay mode (preview is wrong for RGBA images, but I don't care)
 * Added fill in overlay mode (it doesn't seem 100% correct to me, but this is what I got so far). <=== THIS IS PROBABLY BUGGY!

 IMGUIIMAGEEDITOR_VERSION 0.11
 * Added a bunch of booleans in the ImageEditor::ctr()
 * Added some methods to retrieve some image properties
 * Added an optional callback SetImageEditorEventCallback(...)
*/

#define IMGUIIMAGEEDITOR_VERSION 0.31

namespace ImGui {

struct StbImage;
class ImageEditor
#if (defined(IMGUITABWINDOW_H_) && !defined(IMGUIIMAGEEDITOR_NO_TABLABEL))
: public TabWindow::TabLabel
#endif //IMGUITABWINDOW_H_
{
public:

    enum EventType {
        ET_IMAGE_UPDATED=0, // This gets called very frequently, every time the displayed image gets updated
        ET_IMAGE_LOADED,    // Called when an image is loaded/reloaded
        ET_IMAGE_SAVED      // Called when an image is saved
    };

    IMGUI_API ImageEditor();
    IMGUI_API ImageEditor(bool hideImageNamePanel,bool forbidLoadingNewImagesIfAvailable=false,bool forbidBrowsingInsideFolderIfAvailable=false,bool forbidSaveAsIfAvailable=false);
#if (defined(IMGUITABWINDOW_H_) && !defined(IMGUIIMAGEEDITOR_NO_TABLABEL))
    IMGUI_API virtual ~ImageEditor();
    #else //IMGUITABWINDOW_H_
    IMGUI_API ~ImageEditor();
    IMGUI_API bool getModified() const;       // so that this method is always available
#   endif //IMGUITABWINDOW_H_

    bool isInited() const {return init;}

    IMGUI_API bool loadFromFile(const char* path);
    IMGUI_API bool saveAs(const char* path=NULL);

    IMGUI_API void render(const ImVec2 &size);    // to be called inside an ImGui::Window. Makes isInited() return true;
    void render() {render(ImVec2(0,0));}

    IMGUI_API void destroy();         // clears instance textures and frees memory
    IMGUI_API static void Destroy();  // clears shared textures and frees shared memory

    typedef void (*FreeTextureDelegate)(ImTextureID& texid);
    typedef void (*GenerateOrUpdateTextureDelegate)(ImTextureID& imtexid,int width,int height,int channels,const unsigned char* pixels,bool useMipmapsIfPossible,bool wraps,bool wrapt,bool minFilterNearest, bool magFilterNearest);
    static void SetFreeTextureCallback(FreeTextureDelegate freeTextureCb) {FreeTextureCb=freeTextureCb;}
    static void SetGenerateOrUpdateTextureCallback(GenerateOrUpdateTextureDelegate generateOrUpdateTextureCb) {GenerateOrUpdateTextureCb=generateOrUpdateTextureCb;}

    typedef void (*ImageEditorEventDelegate)(ImageEditor& ie,EventType event);
    static void SetImageEditorEventCallback(ImageEditorEventDelegate imageEditorEventCb) {ImageEditorEventCb=imageEditorEventCb;}

    class Style {
        public:
        float splitterSize;      // default: -1
        ImVec4 splitterColor;    // default: .x<0
        char arrowsChars[4][5];   // default: "<",">","^","v"
        int keySave;             // default: (int)'s'. The ImGui key index for saving the image when CTRL is down
        IMGUI_API Style();
        static Style style;
        inline static Style& Get() {return style;}        
    };

    // The image name panel on the top sometimes is a waste of space, so we can hide it
    bool getShowImageNamePanel() const {return showImageNamePanel;}
    void setShowImageNamePanel(bool flag) {showImageNamePanel=flag;}

    IMGUI_API const char* getImageFilePath() const;           // gets the current image file path [e.g. "./myImage.png"]. It's absolute when imguifilesystem is available.
    IMGUI_API const char* getImageFileName() const;           // gets the current image file name [e.g. "myImage.png"]. It's the path suffix withouth slashes.
    IMGUI_API const char* getImageFileFileExtension() const;  // gets the current image file extension [e.g. ".png"]. It's always lowercase.
    IMGUI_API void getImageInfo(int* w,int* h,int* c) const;  // gets image width, height and num channels (1,3 or 4)
    IMGUI_API const unsigned char* getImagePixels() const;    // get the internal data [size is w*h*c]
    IMGUI_API const ImTextureID* getImageTexture() const;     // get a pointer to the texture that is used and owned by the ImageEditor (a bit dangerous...)

    // user must free the returned texture
    // Warning: this is not usually what user expects (for example when c=1 we get a RGB grayscale texture):
    // manually creating it using getImagePixels() is usually better
    IMGUI_API ImTextureID getClonedImageTexID(bool useMipmapsIfPossible=false,bool wraps=false,bool wrapt=false,bool minFilterNearest=false, bool magFilterNearest=false) const;

    void* userPtr;                                  // yours

protected:
	struct StbImage* is;

    bool init,showImageNamePanel,allowLoadingNewImages,allowBrowsingInsideFolder,allowSaveAs;

    friend struct StbImage;

#   ifdef _MSC_VER   // old compilers might complain
public:
#   endif //_MSC_VER

    static FreeTextureDelegate FreeTextureCb;
    static GenerateOrUpdateTextureDelegate GenerateOrUpdateTextureCb;
    static ImageEditorEventDelegate ImageEditorEventCb;
};

} // namespace ImGui

#endif //IMGUIIMAGEEDITOR_H_

