/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GUI_EDITOR_INTERFACE_H_
#define _GUI_EDITOR_INTERFACE_H_
#define NULL 0
#include "Hardware/Platform/Headers/PlatformDefines.h"
namespace CEGUI{
	class Window;
};
///Abstract interface for various editor plugins, suchs as light managers, ai managers, scene graph interface, etc
class GUIEditorInterface {
protected:
	GUIEditorInterface() : _parent(NULL) {}
	virtual ~GUIEditorInterface() {}
	virtual bool init(CEGUI::Window *parent) {_parent = parent; return (_parent != NULL);}
	///Handle tick with time difference from last call
	virtual bool tick(U32 deltaMsTime) = 0;
	CEGUI::Window *_parent;
};

#endif