/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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