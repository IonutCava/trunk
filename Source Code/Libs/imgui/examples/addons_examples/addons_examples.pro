TEMPLATE = app
CONFIG -= qt
CONFIG += link_pkgconfig
TARGET = imgui_addons_examples
DESTDIR = ./
#CONFIG += link_pkgconfig
#PKGCONFIG += ogg dbus-1

# START USER EDITABLE AREA -----------------------------------------------------------------------

# When commented out, main.cpp is built, otherwise main2.cpp is built
#CONFIG+= use_main2
#CONFIG+= use_main3  # dev only
#CONFIG+= use_main4  # dev only
#CONFIG+= use_main5  # dev only
#CONFIG+= use_main6  # dev only

# Only one of these must be active:
CONFIG+= use_glfw3
#CONFIG+= use_glut
#CONFIG+= use_sdl2
#CONFIG+= use_winapi
#CONFIG+= use_direct3d9
# Optional, but it might be mandatory for Windows: only ONE of them is allowed
#CONFIG+= use_glew
#CONFIG+= use_glad	# experimental
#CONFIG+= use_gl3w	# experimental
#------------------------------------------------------------------------------

IMGUI_BASE_PATH=../..

# Experimental, needs zlib (here we link to -lz, some might have -lzip)
#CONFIG+= imguifilesystem_supports_zip_files

# These are optional definitions that nobody will ever use (all undefined by default):
#DEFINES+=IMIMPL_SHADER_NONE			    # no shaders at all, and no vertex buffer object as well (minimal implementation).
#DEFINES+=IMIMPL_SHADER_GL3                         # shader uses openGL 3.3 (glsl #version 330)
#DEFINES+=IMIMPL_SHADER_GLES                        # shader uses gles (and if IMIMPL_SHADER_GL3 is defined glsl #version 300 es)
#DEPRECATED: Not supported by new imgui: DEFINES+=IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS     # undefined (=default) => 1. Memory usage vs performance. When defined MUST BE A VALUE >=1.

# Experimental:
#DEFINES+=IMIMPL_FORCE_DEBUG_CONTEXT  # To remove! Implemented only in glfw3

#DEFINES+= IMGUI_WIP	    # to remove (dev only)
#DEFINES+=IMGUIBINDINGS_RESTORE_GL_STATE

#DEFINES+= YES_IMGUISDF
#DEFINES+= YES_IMGUISOLOUD_ALL #YES_IMGUISOLOUD_MODPLUG #YES_IMGUISOLOUD_SPEECH		# Testing only (with use_sdl2)! To remove
#DEFINES+= YES_IMGUITINYFILEDIALOGS
#DEFINES+= YES_IMGUISQLITE   # To remove
#LIBS+= -lsqlite3	    # To remove
#PKGCONFIG += alsa
#DEFINES+= YES_IMGUIFREETYPE
#PKGCONFIG += freetype2
#DEFINES += IMGUIFS_NO_EXTRA_METHODS
#DEFINES += IMGUI_USE_MINIZIP
#LIBS+= -lz
#DEFINES+= YES_IMGUIMINIGAMES
#DEFINES+= IMIMPL_BUILD_SDF IMIMPL_USE_SDF_SHADER IMIMPL_USE_SDF_OUTLINE_SHADER
#DEFINES+= IMIMPL_SHADER_GL3
#DEFINES+= IMIMPL_SHADER_NONE
#DEFINES+=YES_IMGUIBZ2
#DEFINES+=YES_IMGUISTRINGIFIER
#DEFINES+=STBI_NO_GIF
#DEFINES+=NO_IMGUITOOLBAR
#DEFINES+=YES_IMGUIIMAGEEDITOR
#DEFINES+=YES_IMGUIPDFVIEWER
#PKGCONFIG += poppler-glib
# END USER EDITABLE AREA ----------------------------------------------------------------------------



DEFINES+= IMGUI_INCLUDE_IMGUI_USER_H IMGUI_INCLUDE_IMGUI_USER_INL # mandatory for loading addons

INCLUDEPATH+= 	$$IMGUI_BASE_PATH				\
		$$STB_BASE_PATH


HEADERS+=  $$IMGUI_BASE_PATH"/imgui.h"						    \
	   $$IMGUI_BASE_PATH"/addons/imgui_user.h"				    \
	   $$IMGUI_BASE_PATH"/addons/imguibindings/ImImpl_RenderDrawLists.h"	    \
	   $$IMGUI_BASE_PATH"/addons/imguibindings/ImImpl_Binding_Glfw3.h"	    \
	   $$IMGUI_BASE_PATH"/addons/imguibindings/ImImpl_Binding_Glut.h"	    \
	   $$IMGUI_BASE_PATH"/addons/imguibindings/ImImpl_Binding_SDL2.h"	    \
	   $$IMGUI_BASE_PATH"/addons/imguibindings/ImImpl_Binding_WinAPI.h"	    \
	   $$IMGUI_BASE_PATH"/addons/imguibindings/ImImpl_Binding_Direct3D9.h"	    \
	   $$IMGUI_BASE_PATH"/addons/imguibindings/imguibindings.h"		    \
	   $$IMGUI_BASE_PATH"/addons/imguibindings/stb_image.h"			    \
	   $$IMGUI_BASE_PATH"/addons/imguibindings/imguistring/imguistring.h"	    \
	   $$IMGUI_BASE_PATH"/addons/imguifilesystem/imguifilesystem.h"		    \
	   $$IMGUI_BASE_PATH"/addons/imguifilesystem/dirent_portable.h"		    \
	   $$IMGUI_BASE_PATH"/addons/imguilistview/imguilistview.h"		    \
	   $$IMGUI_BASE_PATH"/addons/imguitoolbar/imguitoolbar.h"		    \
	   $$IMGUI_BASE_PATH"/addons/imguivariouscontrols/imguivariouscontrols.h"   \
	   $$IMGUI_BASE_PATH"/addons/imguihelper/imguihelper.h"			    \
	   $$IMGUI_BASE_PATH"/addons/imguistring/imguistring.h"			    \
	   $$IMGUI_BASE_PATH"/addons/imguipanelmanager/imguipanelmanager.h"	    \
	   $$IMGUI_BASE_PATH"/addons/imguistyleserializer/imguistyleserializer.h"   \
	   $$IMGUI_BASE_PATH"/addons/imguidatechooser/imguidatechooser.h"	    \
	   $$IMGUI_BASE_PATH"/addons/imguinodegrapheditor/imguinodegrapheditor.h"   \
	   $$IMGUI_BASE_PATH"/addons/imguicodeeditor/imguicodeeditor.h"		    \
	   $$IMGUI_BASE_PATH"/addons/imguicodeeditor/utf8helper.h"		    \
	   $$IMGUI_BASE_PATH"/addons/imguitabwindow/imguitabwindow.h"		    \
	   $$IMGUI_BASE_PATH"/addons/imguidock/imguidock.h"			    \
	   $$IMGUI_BASE_PATH"/addons/imguiyesaddons/imgui*.h"

SOURCES+=  $$IMGUI_BASE_PATH"/imgui.cpp" \
	   $$IMGUI_BASE_PATH"/imgui_draw.cpp" \
	   $$IMGUI_BASE_PATH"/imgui_demo.cpp" \ #\ # optional: for ImGui::ShowTestWindow()

use_main6 {
SOURCES+=main6.cpp
TARGET = imgui_addons_example6
#DEFINES+=NO_IMGUISTYLESERIALIZER NO_IMGUIDATECHOOSER NO_IMGUILISTVIEW NO_IMGUIGRAPHEDITOR NO_IMGUITOOLBAR NO_IMGUIPANELMANAGER NO_IMGUIFILESYSTEM NO_IMGUICODEEDITOR NO_IMGUITABWINDOW
} #use_main6
!use_main6 {
use_main5 {
SOURCES+=main5.cpp
TARGET = imgui_addons_example5
#DEFINES+=NO_IMGUISTYLESERIALIZER NO_IMGUIDATECHOOSER NO_IMGUILISTVIEW NO_IMGUIGRAPHEDITOR NO_IMGUITOOLBAR NO_IMGUIPANELMANAGER NO_IMGUIFILESYSTEM NO_IMGUICODEEDITOR NO_IMGUITABWINDOW
}  #use_main5
!use_main5 {
use_main4 {
#SOURCES+=main4.cpp main4_2.cpp
SOURCES+=main4_3.cpp
TARGET = imgui_addons_example4
DEFINES+=NO_IMGUISTYLESERIALIZER NO_IMGUIDATECHOOSER NO_IMGUILISTVIEW NO_IMGUIGRAPHEDITOR NO_IMGUITOOLBAR NO_IMGUIPANELMANAGER NO_IMGUIFILESYSTEM NO_IMGUICODEEDITOR
}  #use_main4
!use_main4 {
use_main3 {
SOURCES+=main3.cpp
TARGET = imgui_addons_example3
DEFINES+=NO_IMGUISTYLESERIALIZER NO_IMGUIDATECHOOSER NO_IMGUILISTVIEW NO_IMGUIGRAPHEDITOR NO_IMGUITOOLBAR NO_IMGUIPANELMANAGER
}  #use_main3
!use_main3 {
use_main2 {
SOURCES+=main2.cpp
TARGET = imgui_addons_example2
} #use_main2
!use_main2 {
SOURCES+=main.cpp
TARGET = imgui_addons_example1
} #!use_main2
} #!use_main3
} #!use_main4
} #!use_main5
} #!#use_main6

imguifilesystem_supports_zip_files {
DEFINES+=IMGUI_USE_MINIZIP
LIBS+=-lz
}




OTHER_FILES+= $$IMGUI_BASE_PATH"/addons/imgui_user.inl"				    \
	   $$IMGUI_BASE_PATH"/addons/imguifilesystem/imguifilesystem.cpp"	    \
	   $$IMGUI_BASE_PATH"/addons/imguilistview/imguilistview.cpp"		    \
	   $$IMGUI_BASE_PATH"/addons/imguitoolbar/imguitoolbar.cpp"		    \
	   $$IMGUI_BASE_PATH"/addons/imguivariousbindings/imguivariousbindings.cpp" \
	   $$IMGUI_BASE_PATH"/addons/imguipanelmanager/imguipanelmanager.cpp"	    \
	   $$IMGUI_BASE_PATH"/addons/imguistyeserializer/imguistyeserializer.cpp"   \
	   $$IMGUI_BASE_PATH"/addons/imguibindings/imguibindings.cpp"		    \
	   $$IMGUI_BASE_PATH"/addons/imguibindings/imguistring/imguistring.cpp"	    \
	   $$IMGUI_BASE_PATH"/addons/imguidatechooser/imguidatechooser.cpp"	    \
	   $$IMGUI_BASE_PATH"/addons/imguivariouscontrols/imguivariouscontrols.cpp" \
	   $$IMGUI_BASE_PATH"/addons/imguihelper/imguihelper.cpp"		    \
	   $$IMGUI_BASE_PATH"/addons/imguistring/imguistring.cpp"		    \
	   $$IMGUI_BASE_PATH"/addons/imguinodegrapheditor/imguinodegrapheditor.cpp" \
	   $$IMGUI_BASE_PATH"/addons/imguicodeeditor/imguicodeeditor.cpp"	    \
	   $$IMGUI_BASE_PATH"/addons/imguitabwindow/imguitabwindow.cpp"		    \
	   $$IMGUI_BASE_PATH"/addons/imguidock/imguidock.h"			    \
	   $$IMGUI_BASE_PATH"/addons/imguiyesaddons/imgui*.cpp"			    \
	   README_FIRST.txt



use_glfw3    {
DEFINES+= IMGUI_USE_GLFW_BINDING
PKGCONFIG += glfw3
INCLUDEPATH+=/usr/include/
INCLUDEPATH+=/usr/include/GLFW
HEADERS+= /usr/include/GLFW/glfw3.h
}
use_glut    {
DEFINES+= IMGUI_USE_GLUT_BINDING
LIBS+= -lglut
INCLUDEPATH+=/usr/include/
INCLUDEPATH+=/usr/include/GL
HEADERS+= /usr/include/GL/glut.h
}
use_sdl2    {
DEFINES+= IMGUI_USE_SDL2_BINDING
PKGCONFIG += sdl2
INCLUDEPATH+=/usr/include/
INCLUDEPATH+=/usr/include/SDL2
HEADERS+= /usr/include/SDL2/SDL.h
}
use_glew {
DEFINES+= IMGUI_USE_GLEW
LIBS+=-lGLEW
INCLUDEPATH+=/usr/include/
INCLUDEPATH+=/usr/include/GL
HEADERS+= /usr/include/GL/glew.h
}
use_glad {
GLAD_BASE_PATH ="/home/flix/Desktop/Repositories/glad-openGL4.5"    # This should be changed on a per user basis
DEFINES+= IMGUI_USE_GLAD
INCLUDEPATH+=$$GLAD_BASE_PATH"/include/"
INCLUDEPATH+=$$GLAD_BASE_PATH"/src"
HEADERS+= $$GLAD_BASE_PATH"/include/glad/glad.h"
SOURCES+=$$GLAD_BASE_PATH"/src/glad.c"
}
use_gl3w {
GL3W_BASE_PATH ="../libs/gl3w/"    # This should be changed on a per user basis
DEFINES+= IMGUI_USE_GL3W
INCLUDEPATH+=$$GL3W_BASE_PATH
HEADERS+= $$GL3W_BASE_PATH/GL/gl3w.h
SOURCES+= $$GL3W_BASE_PATH/GL/gl3w.c
}

!use_winapi {
!use_direct3d9	{
LIBS+= -lpthread  -lGL -lX11 -ldl  #-static-libstdc++ -static-libgcc

# Please remove these 3 lines-------------------
#DEFINES+=IMGUIFILESYSTEM_USES_STD_FILESYSTEM
#QMAKE_CXXFLAGS+=--std=c++11
#LIBS+=-lstdc++fs
# ---------------------------------------------

}
}
use_winapi    {
DEFINES+= IMIMPL_USE_WINAPI _WIN32 WIN32 #_MSC_VER
# These paths should be adapted as well:
INCLUDEPATH+=/usr/i686-w64-mingw32/include		\
	     /usr/include/c++/4.6/i686-w64-mingw32	\
	     /usr/include/c++/4.6/i686-w64-mingw32/bits	\
	     /usr/lib/gcc/i686-w64-mingw32/4.6/include
LIBS+= -L"/usr/i686-w64-mingw32/lib" \
       -L"/usr/lib/gcc/i686-w64-mingw32/4.6"

LIBS+= -lopengl32 -luser32 -lkernel32 -static-libgcc -static-libstdc++
#QMAKE_CXXFLAGS+=--std=c++0x
}
use_direct3d9 {
DEFINES+= IMIMPL_USE_DIRECT3D9 _WIN32 WIN32 #_MSC_VER
# These paths should be adapted as well:
INCLUDEPATH+=/usr/i686-w64-mingw32/include		\
	     /usr/include/c++/4.6/i686-w64-mingw32	\
	     /usr/include/c++/4.6/i686-w64-mingw32/bits	\
	     /usr/lib/gcc/i686-w64-mingw32/4.6/include
LIBS+= -L"/usr/i686-w64-mingw32/lib" \
       -L"/usr/lib/gcc/i686-w64-mingw32/4.6"

LIBS+= -ld3d9 -luser32 -lkernel32 -static-libgcc -static-libstdc++
#QMAKE_CXXFLAGS+=--std=c++0x
}

# Dev stuff here (should be commented out)
#DEFINES+=IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#DEFINES+=NO_IMGUIVARIOUSCONTROLS
#DEFINES+=NO_IMGUIPANELMANAGER
#DEFINES+=NO_IMGUIDATECHOOSER
#DEFINES+=IMGUICODEEDITOR_DEBUG
#DEFINES+=IMGUISTRING_STL_FALLBACK
#DEFINES+=IMGUIBINDINGS_DONT_CLEAR_INPUT_DATA_SOON # It's now mandatory for Software Cursors
#DEFINES+=NO_IMGUIVARIOUSCONTROLS_ANIMATEDIMAGE
#DEFINES+=STBI_NO_GIF
#DEFINES+=IMGUIVARIOUSCONTROLS_NO_STDIO
