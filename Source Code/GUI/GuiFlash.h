#ifndef _GUI_FLASH_H
#define _GUI_FLASH_H

#include "GUI/GUI.h"



class GuiFlash : public GuiElement
{
private:
   typedef GuiElement Parent;

public:
   GuiFlash();
   ~GuiFlash();

	void playMovie();
	
};

#endif