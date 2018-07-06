/*�Copyright 2009-2013 DIVIDE-Studio�*/
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
#ifndef _GUI_EDITOR_H_
#define _GUI_EDITOR_H_

namespace CEGUI{
	class Window;
	class Font;
    class EventArgs;
};

#include "GUIEditorInterface.h"
#include "Core/Headers/Singleton.h"

///Our world editor interface
DEFINE_SINGLETON( GUIEditor )
	public:
		bool init();
		void setVisible(bool visible); //< Hide or show the editor
        bool isVisible();              //< Return true if editor is visible, false if is hidden
		bool tick(U32 deltaMsTime);    //< Used to update time dependent elements
	private:
		GUIEditor();
		~GUIEditor();
		void RegisterHandlers();       //< Register our handler functions
	private:
		bool _init;
		CEGUI::Window *_editorWindow;  //< This will be a pointer to the EditorRoot window.

END_SINGLETON

#endif