#include "Headers/GUIButton.h"

#include <CEGUI/CEGUI.h>

namespace Divide {

GUIButton::GUIButton(const stringImpl& id, const stringImpl& text,const stringImpl& guiScheme,
                     const vec2<I32>& position, const vec2<U32>& dimensions,
                     const vec3<F32>& color, CEGUI::Window* parent,
                     ButtonCallback callback) : GUIElement(parent,GUI_BUTTON,position),
                                                _text(text),
                                                _dimensions(dimensions),
                                                _color(color),
                                                _callbackFunction(callback),
                                                _highlight(false),
                                                _pressed(false),
                                                _btnWindow(nullptr)
{
  _btnWindow = CEGUI::WindowManager::getSingleton().createWindow(stringAlg::fromBase(guiScheme+"/Button"), stringAlg::fromBase(id));
  _btnWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(0,position.x),CEGUI::UDim(1,-1.0f * position.y)));
  _btnWindow->setSize(CEGUI::USize(CEGUI::UDim(0,dimensions.x),CEGUI::UDim(0,dimensions.y)));
  _btnWindow->setText(text.c_str());
  _btnWindow->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&GUIButton::joystickButtonPressed,this));
  _parent->addChild(_btnWindow);
  _btnWindow->setEnabled(true);
  setActive(true);
}

GUIButton::~GUIButton(){
    _parent->removeChild(_btnWindow);
}

void GUIButton::setTooltip(const stringImpl& tooltipText){
    _btnWindow->setTooltipText(tooltipText.c_str());
}

void GUIButton::setFont(const stringImpl& fontName,const stringImpl& fontFileName, U32 size){
    if(!fontName.empty()){
        if(!CEGUI::FontManager::getSingleton().isDefined(fontName.c_str())){
            CEGUI::FontManager::getSingleton().createFreeTypeFont(fontName.c_str(), size, true, fontFileName.c_str());
        }

        if(CEGUI::FontManager::getSingleton().isDefined(fontName.c_str())){
            _btnWindow->setFont(fontName.c_str());
        }
    }
}

bool GUIButton::joystickButtonPressed(const CEGUI::EventArgs& /*e*/){
    if(!_callbackFunction.empty()){
        _callbackFunction();
        return true;
    }
    return false;
}

};