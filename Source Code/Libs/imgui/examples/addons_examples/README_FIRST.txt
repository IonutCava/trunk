===================================
HOW TO COMPILE THE ADDON EXAMPLES:
===================================

To compile these two examples (main.cpp and main2.cpp), no makefile is present.

Instead four project files are provided:
->	addons_examples.pro:			A Qt Creator project file that can be used in Ubuntu only and must be edited to set the paths and to choose the example to compile (handy for Linux and Mac users). This file can be tweaked to use the libraries of your choice (see below).
->	addons_examples_mingw.cbp:		A CodeBlocks project file that can be used to compile the first demo for Windows (to compile the second demo simply replace main.cpp with main2.cpp). This demo requires the glew library only to compile.
->	addons_examples_mingw_direct3d9.cbp: 	A CodeBlocks project file that can be used to compile the first demo for Windows (to compile the second demo simply replace main.cpp with main2.cpp). This demo requires the direct3d9 libraries to compile.
->	addons_example_monodevelop.cproj:	A Monodevelop C++ project file that contains both main*.cpp files: however only one has its "Build Action" set to "compile" (this makes it easier to switch between them). It requires the two C++ packages "gl" and "sdl2": their absolute paths are hardcoded for Ubuntu Linux 64bit, but they can be easily reconfigured by the user.
Even if you use another IDE, in case of compilation problems, it can still be useful to open these files with a text editor and see their content.

You can test the two examples (without compiling them) in your web browser by clicking on the two .html files inside the html subfolder (although not all the addons might be active in this build).
If you want to compile the examples to .html yourself, or you can't run the examples in your web browser, please read at the bottom of this page.

UPDATE: There's a third demo named main3.cpp, but it's currently used to develop imguicodeeditor (see below).

-----------------------------
NOTE FOR VISUAL STUDIO USERS:
-----------------------------
There's currently no Visual Studio project, and furthermore the code has been written/tested on a Linux machine (tested using 4 compilers: gcc, clang, mingw and emcc).
Unluckily cl.exe (the Visual Studio compiler) is not currently supported/tested.

If you want, you can try one of these command-lines at the Visual Studio Prompt (you can run vcvars32.bat or vcvarsall.bat to setup it)
and report the results here: https://github.com/Flix01/imgui/issues

[The current directory must be: imgui/examples/addons_examples]
# With GLUT + GLEW static: [GLEW is necessary for all Windows bindings except Direct3D9: here we use the static version to save one .dll]
cl /nologo /O2 /MT /I"../../" ../../imgui.cpp ../../imgui_draw.cpp ../../imgui_demo.cpp main.cpp /D"IMGUI_INCLUDE_IMGUI_USER_H" /D"IMGUI_INCLUDE_IMGUI_USER_INL" /D"IMGUI_USE_GLUT_BINDING" /D"IMGUI_USE_GLEW" /D"GLEW_STATIC" /D"WINVER=0x0501" /D"_WIN32_WINNT=0x0501" /link /out:imgui_addons_example1.exe glut32.lib glew32s.lib opengl32.lib gdi32.lib Shell32.lib user32.lib kernel32.lib
# With GLEW static only:
cl /nologo /O2 /MT /I"../../" ../../imgui.cpp ../../imgui_draw.cpp ../../imgui_demo.cpp main.cpp /D"IMGUI_INCLUDE_IMGUI_USER_H" /D"IMGUI_INCLUDE_IMGUI_USER_INL" /D"IMGUI_USE_WINAPI_BINDING" /D"WINVER=0x0501" /D"_WIN32_WINNT=0x0501" /D"IMGUI_USE_GLEW" /D"GLEW_STATIC" /link /out:imgui_addons_example1.exe glew32s.lib opengl32.lib gdi32.lib Shell32.lib user32.lib kernel32.lib
# With bundled GL3W only (and using shaders from GL3: /D"IMIMPL_SHADER_GL3"):	-> This should work with no dependencies <-
cl /O2 /MT /I"../../" /I"../libs/gl3w/" ../libs/gl3w/GL/gl3w.c ../../imgui.cpp ../../imgui_draw.cpp ../../imgui_demo.cpp main.cpp /D"IMGUI_INCLUDE_IMGUI_USER_H" /D"IMGUI_INCLUDE_IMGUI_USER_INL" /D"IMGUI_USE_WINAPI_BINDING" /D"WINVER=0x0501" /D"_WIN32_WINNT=0x0501" /D"IMGUI_USE_GL3W" /D"IMIMPL_SHADER_GL3" /link /out:imgui_addons_example1.exe opengl32.lib gdi32.lib Shell32.lib user32.lib kernel32.lib
# With DIRECT3D9:	-> This should work with no dependencies <-
cl /nologo /O2 /MT /I"../../" ../../imgui.cpp ../../imgui_draw.cpp ../../imgui_demo.cpp main.cpp /D"IMGUI_INCLUDE_IMGUI_USER_H" /D"IMGUI_INCLUDE_IMGUI_USER_INL" /D"IMGUI_USE_DIRECT3D9_BINDING" /D"WINVER=0x0501" /D"_WIN32_WINNT=0x0501" /link /out:imgui_addons_example1.exe d3d9.lib d3dx9.lib gdi32.lib Shell32.lib advapi32.lib user32.lib kernel32.lib
# With GLFW3 + GLEW static:
cl /nologo /O2 /MT /I"../../" ../../imgui.cpp ../../imgui_draw.cpp ../../imgui_demo.cpp main.cpp /D"IMGUI_INCLUDE_IMGUI_USER_H" /D"IMGUI_INCLUDE_IMGUI_USER_INL" /D"IMGUI_USE_GLEW_BINDING" /D"IMGUI_USE_GLEW" /D"GLEW_STATIC" /D"WINVER=0x0501" /D"_WIN32_WINNT=0x0501" /link /out:imgui_addons_example1.exe glfw3.lib glew32s.lib opengl32.lib gdi32.lib Shell32.lib user32.lib kernel32.lib
# TODO: Add the last command-line for SDL2

P.S. currently GLEW is required to compile all the Windows bindings except Direct3D9.
UPDATE: Now GLEW can be optionally replaced by GLAD or by GL3W. The latter is already present in the repository,
so that we have another Windows binding that should work without additional libraries.

P.S.2: The order of the .cpp files is important! main.cpp must be the last of all the source files in the command line.

====================================
WHAT IS "IMGUI ADDONS" ?
====================================
It's a collection of "extra imgui widgets" together with an automatic way of "binding" ImGui to a specific openGL library (glfw, SDL2, glut and WinAPI), or to the Direct3D9 library, so that a single cpp "main" file can be used for all of them.

"ImGui Addons" does NOT modify the ImGui library itself in any way (i.e. imgui.cpp, imgui_draw.cpp and imgui_demo.cpp are untouched); it just adds:
-> the "addons" subfolder.
-> the two files "imgui_user.h" and "imgui_user.inl" (in the base ImGui folder).

Currently the extra imgui widgets that are available are:
-> imguistyleserializer: 	to load and save ImGuiStyle from/to file.
-> imguifilesystem:			this addon provides: chooseFileDialog, chooseFolderDialog, saveFileDialog
							together with plenty of handy methods to perform file system operations and
							an experimental support for browsing inside zip files (through an additional definition).
-> imguidatechooser:		a combobox-like datechooser.
-> imguilistview:			a list view widget with a lot of optional features (setting its height, row sorting through column header clicking, cell editing).
-> imguitoolbar:			a very flexible imagebutton-bar that can be used inside ImGui Windows (with dynamic layout) and outside (docked at the sides of the screen).
-> imguipanelmanager:		a mini dock panel layout. Basically it uses imguitoolbar and optionally assigns an ImGui Window to some buttons. Please see main2.cpp for an extensive example on how to use it.
-> imguitabwindow:			-> a self-partitioning ImGui::Window with TabLabels that can be dragged around. It's used in the central window of main2.cpp.
							-> this addon also provides ImGui::TabLabels(...) and ImGui::TabLabelsVertical(...), for adding generic tabs in normal imgui windows.
-> imguidock:				Lumix Engine's docking system [from: https://github.com/nem0/LumixEngine/blob/master/src/editor/imgui/imgui_dock.h].
-> imguivariouscontrols:	a series of minor widgets, such as:
							-> ProgressBar.
							-> PopupMenuSimple	(a fast, single column, scrollable, popup menu).
							-> PopupMenu (a single column menu with image entries).
							-> ColorChooser and ColorCombo (based on the code from: https://github.com/benoitjacquier/imgui. Thank you benoitjacquier!).
							-> InputTextMultilineWithHorizontalScrolling (Roflraging posted it to the ImGui Issue Section here: https://github.com/ocornut/imgui/issues/383. Thank you Roflraging!).
							-> ImageButtonWithText and ImageWithZoomAndPan.
							-> The ImageAnimation struct, that can be used to display animated images or animated buttons from frames in a texture or from a .gif image.
							-> TreeView (a generic TreeView implementation).
							-> Other stuff (like an overloaded PlotHistogram, PlotCurve). Please see main.cpp.
-> imguinodegrapheditor:	-> Based on the code posted by Omar, the creator of ImGui.
-> imguicodeeditor (WIP, UNUSABLE)	this is an attempt to develop a code editor using ImGui only (without direct STL support).
					However, developing such a control is a huge challange, and I'm not sure when and if it will eventually be functional.
					In any case, if you need this kind of control, I suggest you try a more reliable solution, such as the Scintilla Editor,
					that some user has successfully managed to integrate with ImGui (please see: https://github.com/ocornut/imgui/issues/108 and https://github.com/ocornut/imgui/issues/200).
					The file main3.cpp is intended as a development playground for this control.
					UPDATE: It contains ImGui::InputTextWithSyntaxHighlighting(...), a cut-down version of the code editor that some users might find useful (see main.cpp).

And in addition:
-> imguistring:				this addon file contains some classes that can be used to replace some STL equivalents (STL is not accepted inside ImGui code):
							-> a string class based on ImVector<char>. It does not support iterators, but has many methods that std::string has: thus it can be used to replace std::string in many algorithms.
							-> ImVectorEx<T>. An extended version of ImVector<T> that supports C++ classes as items. The version of ImVector<T> that comes with ImGui is not a suitable replacement to std::vector<T>, because it does not call constructors and destructors on items.
							-> Other struct / classes like ImPair<T,U> and ImHashMap<T,...> mainly for internal usage (they're used by some other addon).
-> imguihelper:				this addon file contains: 
							-> ImGui::OpenWithDefaultApplication(...) that should work with urls, folders and files
							-> two serialization helper classes (mainly for internal usage, to provide serialization support to other addons) in a dedicated ImGuiHelper namespace.
							-> some Gz decompression helper methods, available if IMGUI_USE_ZLIB is defined at the project level (this definition requires linking to the zlib library).
							-> some Bz2 decompression helper methods, available if YES_IMGUIBZ2 is defined at the project level.


Tip: every single imgui "widget" addon listed above (and in some cases even some part of it) CAN BE EXCLUDED from the compilation by defining at the project level something like: NO_IMGUIFILESYSTEM, etc (and the first demo, main.cpp, should always compile).
The definitions needed to exclude all the addons are:
NO_IMGUISTYLESERIALIZER
NO_IMGUIFILESYSTEM
NO_IMGUIDATECHOOSER
NO_IMGUILISTVIEW
NO_IMGUITOOLBAR
NO_IMGUIPANELMANAGER
NO_IMGUITABWINDOW
NO_IMGUIDOCK
NO_IMGUIVARIOUSCONTROLS
NO_IMGUINODEGRAPHEDITOR
NO_IMGUICODEEDITOR
NO_IMGUISTRING
NO_IMGUIHELPER
Disabling unused addons greatly improves compilation times (any in many cases including <imgui.h> as a "precompiled header" can give a further speed-up).
Tip: you can use the definition NO_IMGUI_ADDONS to disable all the addons with a single definition! (More on this later)


And now there's a new kind of imgui addons, called "yes addons".
"imgui yes addons":
-> are all contained in the "./addons/imguiyesaddons/" subfolders.
-> each one must be explicitely enabled through a project definition (e.g. YES_IMGUIPDFVIEWER).
-> may depend on 3rd party libraries (that are NOT INCLUDED).
-> come with NO SUPPORT AT ALL (i.e. no demo [well, main.cpp supports a few of them], no snippet and no docs [if not present in their own header file]).

Currently "yes addons" are:
-> imguipdfviewer.h/cpp:	depends on -lpoppler-glib (that depends on glib-2.0 and cairo, and probably on STL as well).
-> imguisdf.h/cpp:		no dependencies, but needs OpenGL with Shaders.
-> imguisoloud.h/cpp:		depends on STL. License:  ZLib/LibPNG (and others more liberal) Link: [http://sol.gfxile.net/soloud/index.html]
-> imguitinyfiledialogs.h/cpp:	depends on Comdlg32.lib and Ole32.lib on Windows. Link: [http://tinyfiledialogs.sourceforge.net]
-> imguisqlite3.h/cpp:		depends on -lsqlite3. It's CppSQLite from [http://www.codeproject.com/KB/database/CppSQLite.aspx]
-> imguifreetype.h/cpp:		depends on freetype2 (-lfreetype). License: MIT. It allows better font hinting, and Bold and Oblique variations of the .ttf font. Link: [https://github.com/Vuhdo/imgui_freetype]
-> imguiminigames.h/cpp:	no dependencies. For a list of minigames and their own license, please read addons/yes_addons/imguiminigames.h.
-> imguibz2.h/cpp:		no dependencies. It's libbzip2 (http://www.bzip.org/), inlined and made a bit more ImGui-friendly. License: BSD-style. It allows loading .ttf.bz2 font files when using some imguibinding. [You can define BZ_DECOMPRESS_ONLY to reduce memory impact a bit].
-> imguistringifier.h/cpp:	no dependencies. It includes libb64 (libb64.sourceforge.net License: Public Domain). It makes easier to embed files inside source code (e.g. ttf fonts, shader source code, and so on).
-> imguiemscriptenpersistentfolder: no dependencies. It creates a "/persistent_folder" in the root of the emscripten filesystem (more on this in the emscripten section below).
-> imguiimageeditor:		no dependencies. License: Please see ./addons/yes_addons/imguiimageeditor.h

Tip: If you used the NO_IMGUI_ADDONS definition to disable all the "normal" addons, then all your addons behave like yes_addons!
     That means that you can define, for example, YES_IMGUISTYLESERIALIZER to re-enable the "normal" imguistyleserializer addon.

===========================================
HOW TO USE IMGUI ADDONS IN YOUR PROJECTS:
===========================================
Basically to use and compile projects that use the imgui addon framework you can follow these steps:

1 -> No ADDITIONAL .cpp file must be added to your project (i.e. DO NOT add any .cpp file inside the "addons" folder). 
	 Instead define at the project level: IMGUI_INCLUDE_IMGUI_USER_H and IMGUI_INCLUDE_IMGUI_USER_INL. 
	 This way the two files "imgui_user.h" and "imgui_user.inl" should include all the addons automatically.
	 Note: currently "imgui_user.h" defines IMGUI_INCLUDE_IMGUI_USER_INL if it is not already defined: so only
     IMGUI_INCLUDE_IMGUI_USER_H should be mandatory (TODO: test it).


2 -> OPTIONALLY define ONE (and only one) of the following at the project level (typically when you're using ImGui in a new demo project):
IMGUI_USE_GLUT_BINDING		# needs -lglut (or maybe -lGLUT)
IMGUI_USE_SDL2_BINDING		# needs SDL2 libraries
IMGUI_USE_GLFW_BINDING		# needs -lglfw (version3)
IMGUI_USE_WINAPI_BINDING	# needs the windows API	- Warning: this option might require glew (IMGUI_USE_GLEW) to include the opengl headers correctly (please read below)
IMGUI_USE_DIRECT3D9_BINDING	# needs the windows API	and Direct3D9
When one of these is defined, then in your main.cpp file (or somewhere else) you MUST define these 4 methods globally:
void InitGL() {}
void ResizeGL(int w,int h) {}
void DrawGL() {}
void DestroyGL() {}
And then in our main function you can simply call a method (for example ImImpl_Main(NULL,argc,argv);) to have the automatic binding with the library you've chosen (see the code below or in the examples for further info).
Optionally the main method allows the user to directly load ttf fonts (from files or embedded in the C++ code), and to specify various frame rate settings (that can be additionally accessed at runtime through some global variables).
Another benefit of using a binding is that you can access some helper methods such as ImImpl_LoadTexture(...) and ImImpl_LoadTextureFromMemory(...);


3 -> If you use an OpenGL binding, OPTIONALLY some of these definitions might work at the project level (depending on the binding you choose):
IMIMPL_SHADER_NONE 				# no shaders at all, and no vertex buffer object as well (minimal implementation).
IMIMPL_SHADER_GL3  				# shader uses openGL 3.3 (glsl #version 330)
IMIMPL_SHADER_GLES 				# shader uses gles (and if IMIMPL_SHADER_GL3 is defined glsl #version 300 es)
IMGUI_USE_GLEW     				# inits the glew library (needs -lGLEW). This definition might be mandatory for IMGUI_USE_WINAPI_BINDING. Tip: the glew library provides a static library alternative that can be used by defining GLEW_STATIC at the project level (see the glew docs for further info).
IMGUI_USE_GLAD     				# (experimental) inits the glad library (needs glad.c). Intended to be used as a possible ALTERNATIVE to IMGUI_USE_GLEW.
IMGUI_USE_GL3W     				# (experimental) inits the gl3w library (needs gl3w.c). Intended to be used as a possible ALTERNATIVE to IMGUI_USE_GLEW and IMGUI_USE_GLAD.
IMIMPL_GLUT_HAS_MOUSE_WHEEL_CALLBACK		# use this if you are using IMGUI_USE_GLUT_BINDING and mouse wheel does not work.

IMGUIBINDINGS_RESTORE_GL_STATE			# restores the glViewport (and most of other GL state settings) after the call to ImGui::Render().
						# It uses expensive glPop/Push(...) or glGet(...) calls that can be slow and/or deprecated in modern openGL (it's faster to tell openGL what to do than to retrieve something from it, unless the driver is smart enough to cache info or your're using a sofware openGL implementation).
						# Without it the user must specify its own viewport at the beginning of DrawGL() (if it's different from full screen),
						# and the openGL state is not fully restored, but it's just set to some "commonly used" values.

IMIMPL_USE_FONT_TEXTURE_LINEAR_FILTERING	# By default the font texture now uses GL_NEAREST filtering (so that scaled text looks better using the embedded imgui font).
						# This definition sets it to GL_LINEAR, that might be better with custom fonts.
						# That's why this definition is important, but often forgotten.

IMIMPL_USE_ALPHA_SHARPENER_SHADER		# shader uses a different fragment shader that improves the quality of zoomed fonts a bit. It forces GL_LINEAR filtering too (unless IMIMPL_USE_FONT_TEXTURE_NEAREST_FILTERING is defined by the user, and it shouldn't).
IMIMPL_USE_SDF_SHADER				# shader uses a more complex (= slower) fragment shader, that improves the quality of zoomed fonts a bit. It forces GL_LINEAR filtering too (unless IMIMPL_USE_FONT_TEXTURE_NEAREST_FILTERING is defined by the user, and it shouldn't).
						# Warning: ARTIFACTS will appear when displaying any user ImTextureID fragment with ALPHA inside the range (0,255) (edges excluded).
						# This also affects all the images displayed in the imguiimageeditor addon, when their ALPHA is not 0 or 255.
						# (This happens because we use a single shader to display everything).

IMIMPL_BUILD_SDF				# builds Signed Distance Fonts for ImGui. To display them correctly:
						# -> use an OpenGL binding with shader support (don't define IMIMPL_SHADER_NONE)
						# -> IMIMPL_BUILD_SDF defines IMIMPL_USE_SDF_SHADER internally, but IMIMPL_USE_SDF_OUTLINE_SHADER can be defined too. (They force GL_LINEAR filtering for the font texture, unless IMIMPL_USE_FONT_TEXTURE_NEAREST_FILTERING is defined by the user, and it shouldn't).
						# -> set the font's ImFontConfig::OversampleH==ImFontConfig::OversampleV (tested with 1 only. Note that this is NOT the default for custom fonts [it's 3,1]).
						# -> if you use the outline shader, don't use too thin fonts (like the default one).
						# -> you can tune the values in the shader using the global functions: const ImVec4* ImImpl_SdfShaderGetParams();bool ImImpl_SdfShaderSetParams(const ImVec4& sdfParams);bool ImImpl_EditSdfParams(). See main.cpp for further info.
						# Warning: ARTIFACTS will appear when displaying any user ImTextureID fragment with ALPHA inside the range (0,255) (edges excluded).
						# This also affects all the images displayed in the imguiimageeditor addon, when their ALPHA is not 0 or 255.
						# (This happens because we use a single shader to display everything).
						# Warning: I'm not sure this works with the imguifreetype addon (YES_IMGUIFREETYPE).

The following definitions can be defined with any binding:

IMGUIBINDINGS_DONT_CLEAR_INPUT_DATA_SOON:	# Normally ImGui::GetIO()->Fonts->ClearInputData() and ImGui::GetIO()->Fonts->ClearTexData() are called as soon as possible to save some memory.
						# However this prevents you from appending new fonts later (e.g. in InitGL() instead of in the main() method).
						# And this also prevents you from using SOFTWARE CURSORS.
						# If you want to use software cursors or to append fonts later, please define this definition.
						# Possible future ImGui dynamic atlas support will require input data anyway, so this definition will be made enabled by default.
						# The input data is cleaned up before the delation of the font texture if IMGUIBINDINGS_DONT_CLEAR_INPUT_DATA_SOON is defined.

IMGUIBINDINGS_FONTATLAS_NOMOUSECURSORS		# Excludes SOFTWARE CURSORS from the font texture (it's implicit when IMGUIBINDINGS_DONT_CLEAR_INPUT_DATA_SOON is NOT defined).

IMGUIBINDINGS_FONTATLAS_NOPOWEROFTWOHEIGHT	# If you define it the font texture height is not bound to be a power of two.


4 -> OPTIONALLY you can use other definitions at the project level:
IMGUI_USE_ZLIB					# requires the library zlib. It currently enables loading ttf.gz fonts (from file or embedded in C++ code) through the ImImpl_Main(...) method (only if you use one of the "bindings" above), 
						# and two generic Gz decompression methods in imguihelper.
IMGUIFILESYSTEM_USE_ASCII_SHORT_PATHS_ON_WINDOWS# affects imguifilesystem on Windows OS only. When not defined it should display UTF8 paths instead of Ascii paths. It's not defined by default but short ascii paths are better tested (i.e. try this definition if something goes wrong).
IMGUI_USE_MINIZIP				# EXPERIMENTAL. requires the library zlib. It currently affects imguifilesystem only and allows browsing inside zip files (not recursively).
						# Currently when it's enabled it's not possible to disable this feature, but if this is required by some user, we can implement/fix it.
NO_ADDONNAMEHERE				# Addon exclusion definitions (e.g. NO_IMGUIFILESYSTEM). I've already addressed this topic above. They can be handy to speed up compilation and to resolve compilation problems.


As you can see the only mandatory step is the first one.

If you don't want to add all the required definitions at the project level (expecially when you deploy a project), 
you can try adding them to the file imconfig.h and see if it works.

Please DO NOT ADD these definitions in your .cpp files: it won't work! 
("At the project level" means in the "Project Options").

----------------------------------------------------------------------------------------------------------------------------------
DEPLOYING TIPS:
---------------------------
If you need to release source code packages containing "ImGui Addons" you should:
-> COPY the imgui folder, stripping it from all the unused stuff (all the subfolders except "addons").
-> Remove imgui_demo.cpp (if not used).
-> Move all your imgui definitions from your Project Settings to "imconfig.h".
-> Delete from the "addons" subfolder everything you don't use.
       For example, if you define NO_IMGUIFILESYSTEM, you can probably delete the whole "addons/imguifilesystem" subfolder.
       If you don't use any yes_addon, you can probably delete the whole "addons/yes_addons" subfolder; otherwise you can probably
       just delete the files you don't use inside the "addons/yes_addons" subfolder.
       You can probably delete the headers of the bindings you don't use too (TODO: test it), e.g "addons/bindings/ImImpl_Binding_Direct3D9.h",...
Following these steps "ImGui Addons" becomes lighter, more flexible and easier to use in bigger projects: you just include what you actually use, and you're free from all the imgui definitions!
-> Just remember NOT to add any of the .c/.cpp files inside the "imgui/addons" folder (and its subfolders) to the list of your project source files directly: THIS IS IMPORTANT!

----------------------------------------------------------------------------------------------------------------------------------
SPARE SINGLE ADDON USAGE
---------------------------
If you just want to add a single addon (a pair of addonName.h/.cpp files) to an existing project WITHOUT following the steps above, you can probably just include its header file,
add the include folders: $IMGUI_HOME and $IMGUI_HOME/addons/addonName to your project (where $IMGUI_HOME is the path where imgui.h/.cpp are located)
and compile the file $IMGUI_HOME/addons/addonName/addonName.cpp, where "addonName" is the name of the addon you want to use.
Be warned that some addons might depend on others: e.g. imguipanelmanager depends on imguitoolbar: so you may need to include both addons.

However I'm not sure this approach works for all the addons, since some .cpp files (like imguicodeeditor.cpp and imguipanelmanager.cpp) need to be included after imgui.cpp to access its internals.
There's no guarantee this approach will work for all the addons, but it should in most cases.

if you want to extract some addons like imguicodeeditor and imguipanelmanager, then you're forced to implement
some kind of  IMGUI_INCLUDE_IMGUI_USER_H / IMGUI_INCLUDE_IMGUI_USER_INL mechanism in your code (without compiling the .cpp files),
because they use some methods that are static inside imgui.cpp, and not exposed by imgui_internal.h.

P.S. imguibindings is NOT considered an addon that you can easily extract and use inside your code!

-----------------------------------------------------------------------------------------------------------------------------------
A MINIMAL EXAMPLE: mainBasic.cpp
---------------------------------

#include <imgui.h>	

// Mandatory methods
void InitGL() {}
void ResizeGL(int w,int h) {}
void DestroyGL() {}
void DrawGL()	
{
        ImImpl_ClearColorBuffer(ImVec4(0.6f, 0.6f, 0.6f, 1.0f));    // Warning: it does not clear the depth buffer

        static bool open = true;
        ImGui::Begin("Debug", &open, ImVec2(300,300)); 
        ImGui::Text("Hello, world!");
		ImGui::End();	

		// However I get access to all addons from here now
}

#ifndef IMGUI_USE_AUTO_BINDING_WINDOWS  // IMGUI_USE_AUTO_ definitions get defined automatically (e.g. do NOT touch them!)
int main(int argc, char** argv)	{
	ImImpl_Main(NULL,argc,argv);
	return 0;
}
#else // IMGUI_USE_AUTO_BINDING_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int iCmdShow)	{
	 ImImpl_WinMain(NULL,hInstance,hPrevInstance,lpCmdLine,iCmdShow);
}
#endif //IMGUI_USE_AUTO_BINDING_WINDOWS


On Ubuntu, I can compile it with the following command line (provided that imgui.h is two folders up, and that I want to use glfw):
gcc  -o basicExample -I"../../" ../../imgui.cpp ../../imgui_draw.cpp mainBasic.cpp -D"IMGUI_INCLUDE_IMGUI_USER_H" -D"IMGUI_INCLUDE_IMGUI_USER_INL" -I"/usr/include/GLFW" -D"IMGUI_USE_GLFW_BINDING" -L"/usr/lib/x86_64-linux-gnu" -lglfw -lX11 -lm -lGL -lstdc++ -s


P.S. Please note that you CAN still use OpenGL/Direct3D functions/commands from within mainBasic.cpp, 
and convert ImTextureIDs to native textures (e.g. GLuint in OpenGL): this possibility makes using 
the complete "ImGui Addons Framework" convenient not only for programming Dear ImGui Demos, but for 
general-purpose new projects too.

======================================================================================================================================
EXTRA: COMPILING TO HTML USING EMSCRIPTEN:
===========================================
Follow these steps:
1) Using a terminal (=command line), make sure you have a working emcc setup (try: emcc -v).
2) Navigate (cd) to this folder (the folder where README_FIRST.txt is located).
3) To compile the first example try:
em++ -O2 -o main.html -I"../../" ../../imgui.cpp ../../imgui_draw.cpp  ../../imgui_demo.cpp main.cpp --preload-file myNumbersTexture.png --preload-file Tile8x8.png -D"IMGUI_INCLUDE_IMGUI_USER_H" -D"IMGUI_INCLUDE_IMGUI_USER_INL" -D"IMGUI_USE_SDL2_BINDING" -s USE_SDL=2 -s LEGACY_GL_EMULATION=0 -s ALLOW_MEMORY_GROWTH=1 -lm -lGL
4) To compile the second example try:
em++ -O2 -o main2.html -I"../../" ../../imgui.cpp ../../imgui_draw.cpp main2.cpp --preload-file myNumbersTexture.png  --preload-file Tile8x8.png -D"IMGUI_INCLUDE_IMGUI_USER_H" -D"IMGUI_INCLUDE_IMGUI_USER_INL" -D"IMGUI_USE_SDL2_BINDING" -s USE_SDL=2 -s LEGACY_GL_EMULATION=0 -lm -lGL

Some notes:
->  The order of the .cpp files matters: main.cpp (or main2.cpp) must be the last in the command line.
->	As you can see we have used the SDL2 binding (-D"IMGUI_USE_SDL2_BINDING"). 
	The GLFW3 and GLUT bindings are compatible with emscripten too [see (*) below]. 

->	When you pass files (or folders) to em++ using --preload-file, you can actually load the file and save it at runtime, but any modification is lost when you exit the program.
	I'm pretty new to emscripten and I don't know if there's some easy workaround to persist file changes when you close the browser (but read next point please).
	Note: Files passed with --preload-file are copied and grouped together in a blob with the .data extension next to the .html file.

-> There is an imgui yes_addon named imguiemscriptenpersistentfolder: it's in charge of adding "/persistent_folder" to the emscripten file system.
   File saved in this folder should persist somewhere in your browser cache (in form of what is called an "IndexedDB", please search your browser's docs for further info).
   Emscripten users can define YES_IMGUIEMSCRIPTENPERSISTENTFOLDER to enable the creation of the persistent folder.
   ATM I have some issue with the persistent folder (basically I don't seem to be able to load something back at init time).
   UPDATE: I strongly suggest you do not define YES_IMGUIEMSCRIPTENPERSISTENTFOLDER for emscripten builds.
   It can lead to problems: you can experience errors that Firefox "Tools->Web Developer" tells you that are related to IFSDB (or something like that).

(*): To compile the first demo using the GLUT binding, please try:
em++ -O2 -o main.html -I"../../" ../../imgui.cpp ../../imgui_draw.cpp ../../imgui_demo.cpp main.cpp --preload-file myNumbersTexture.png --preload-file Tile8x8.png -D"IMGUI_INCLUDE_IMGUI_USER_H" -D"IMGUI_INCLUDE_IMGUI_USER_INL" -D"IMGUI_USE_GLUT_BINDING" -s LEGACY_GL_EMULATION=0 -s ALLOW_MEMORY_GROWTH=1 -lm -lGL
	To compile it using GLFW3 try: 
em++ -O2 -o main.html -I"../../" ../../imgui.cpp ../../imgui_draw.cpp ../../imgui_demo.cpp main.cpp --preload-file myNumbersTexture.png --preload-file Tile8x8.png  -D"IMGUI_INCLUDE_IMGUI_USER_H" -D"IMGUI_INCLUDE_IMGUI_USER_INL" -D"IMGUI_USE_GLFW_BINDING" -D"IMGUI_GLFW_NO_NATIVE_CURSORS" -s USE_GLFW=3 -s LEGACY_GL_EMULATION=0 -s ALLOW_MEMORY_GROWTH=1 -lm -lGL
	As you can see, due to the lack of cursor support inside the glfw library, in the cpp code we had to use header file <GLFW/glfwnative.h>, that seems to be missing in emscripten.
	Thus native cursors have been disabled with: -D"IMGUI_GLFW_NO_NATIVE_CURSORS" (in InitGL() we can use: ImGui::GetIO().MouseDrawCursor = true; to use ImGui cursors instead).
	UPDATE: Mouse cursor support has been added to GLFW version 3.1. 
	-D"IMGUI_GLFW_NO_NATIVE_CURSORS" is DEPRECATED for GLFW versions >= 3.1.
	If your version of GLFW is >=3.1 you should NOT define -D"IMGUI_GLFW_NO_NATIVE_CURSORS" and the code should compile without problems; otherwise you will probably face a missing file <GLFW/glfwnative.h>,
	and you should compile with -D"IMGUI_GLFW_NO_NATIVE_CURSORS" again.

In short I suggest you use the SDL2 binding when building with the emscripten compiler, because you can use native cursors (that are missing from GLFW < 3.1), and unicode support (that is missing from GLUT).
GLFW can be a possible alternative only for version 3.1 or above (but of course the GLFW version is the one that em++ downloads on the fly, not yours).

P.S. In the C++ code you can check emscripten builds using the __EMSCRIPTEN__ definition.

Using predefined shells:
------------------------
In the examples/addons_examples/html folder, together with the .html (and .js) files that are necessary to run the emscripten version,
some files with the .shell extensions are also present. 
These files are NOT necessary when RUNNING the .html files, but can be used in the
building process to make the .html files have a different look and functionality.
-> A shell file is a kind of 'template .html file' that can be used by emcc/em++ with the command-line option: --shell-file.
Currently the available shells are:
a) --shell-file html/emscripten_default.shell 	-> very similiar to the normal shell, except that it lacks a few options that didn't work as expected in my tests.
b) --shell-file html/emscripten_load.shell		-> same as above, except that it lacks the emscripten logo and link and it exposes a "Browse" button that can be used
												   to "upload" files from the user local filesystem to the root folder of the emscripten/browser filesystem, that is the only one accessible from C/C++ code.
												   (there is currently no notification to the C++ code that files have been added, so that we don't need any modification to our existing C++ code: however it's very easy to modify the shell code to achieve it). 
c) --shell-file html/emscripten_load_save.shell	-> same as the 'load' shell, except that it allows the C++ code to 'download' files from the emscripten/browser filesystem to the user local filesystem,
												   through a Javascript method called "saveFileFromMemoryFSToDisk(memoryFSname,localFSname)" [Tip: 'localFSname' shouldn't contain folder paths: it's up to the browser to decide where to put it].
												   This shell REQUIRES "FileSaver.js" to be present in the output (html/) folder at RUNTIME (when this shell [or shell d)] is not used "FileSaver.js" can be safely removed).
												   (all this requires no modifications to existing C++ code, but of course it would be useless if "saveFileFromMemoryFSToDisk" is never called). 
												   P.S. to call Javascript code from C++, we must include <emscripten.h> and then we can write something like: emscripten_run_script("saveFileFromMemoryFSToDisk('images/image.jpg','image.jpg')");
												   P.S.2. There's more than this: we can call the Javascript methods exposed by "FileSaver.js" too, if we just need to create a file and download it without first saving it to the browser FS [Never tested].
d) --shell-file html/emscripten_save.shell		-> c) - b). [This shell REQUIRES "FileSaver.js" to be present in the output (html/) folder at RUNTIME too].

WARNING: when using a shell b), c) or d) the command-line: --closure 1 should be avoided.
TIP: Currently if you use the imguifilesystem addon together with shell c) or d), you can define EMSCRIPTEN_SAVE_SHELL globally,
so that when a filesystem dialog is open, you can hold CTRL down and right-click on a file to download it locally.

================================================
FAQ: HOW TO RUN THE (LOCAL) HTML DEMOS
================================================
If the html demos (in the html subfolder) don't run in your browser:
->  see if the demos work when they are hosted on a web server. Try these links:
    https://rawgit.com/Flix01/imgui/2015-10-Addons/examples/addons_examples/html/main.html
    https://rawgit.com/Flix01/imgui/2015-10-Addons/examples/addons_examples/html/main2.html
    https://rawgit.com/Flix01/imgui/2015-10-Addons/examples/addons_examples/html/main3.html
    If they work they should work locally if you use the Firefox web browser.
    On other browsers you may get exceptions that, in the Javascript console, look like: "XMLHttpRequest cannot load".
    Here is how to solve them (from the emscripten docs):
    "Unfortunately Chrome and Internet Explorer do not support file:// XHR requests, and can’t directly load the local
    file in which preloaded data is stored. For these browsers you’ll need to serve the files using a webserver.
    The easiest way to do this is to use the python SimpleHTTPServer"
    (in a console/terminal opened in the html subfolder type: python -m SimpleHTTPServer 8080 
	and then open http://localhost:8080/main.html from your browser).
	Another way of making it work locally is to use the emrun program from a console/terminal opened 
	in the html subfolder: emrun main.html	[Never tested, but it should work better.]

->  otherwise it must be something related to your system or the configuration of your browser.


