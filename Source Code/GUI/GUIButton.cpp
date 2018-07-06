#include "Headers/GUIButton.h"

#include <CEGUI/CEGUI.h>

GUIButton::GUIButton(const std::string& id, const std::string& text,const std::string& guiScheme,
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
  _btnWindow = CEGUI::WindowManager::getSingleton().createWindow(guiScheme+"/Button",id);
  _btnWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(0,position.x),CEGUI::UDim(1,-1.0f * position.y)));
  _btnWindow->setSize(CEGUI::USize(CEGUI::UDim(0,dimensions.x),CEGUI::UDim(0,dimensions.y)));
  _btnWindow->setText(text);
  _btnWindow->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&GUIButton::buttonPressed,this));
  _parent->addChild(_btnWindow);
  setActive(true);
}

GUIButton::~GUIButton(){
    _parent->removeChild(_btnWindow);
}

void GUIButton::setTooltip(const std::string& tooltipText){
    _btnWindow->setTooltipText(tooltipText);
}

void GUIButton::setFont(const std::string& fontName,const std::string& fontFileName, U32 size){
    if(!fontName.empty()){
        if(!CEGUI::FontManager::getSingleton().isDefined(fontName)){
            CEGUI::FontManager::getSingleton().createFreeTypeFont(fontName,size,true,fontFileName);
        }

        if(CEGUI::FontManager::getSingleton().isDefined(fontName)){
            _btnWindow->setFont(fontName);
        }
    }
}

bool GUIButton::buttonPressed(const CEGUI::EventArgs& /*e*/){
    if(!_callbackFunction.empty()){
        _callbackFunction();
        return true;
    }
    return false;
}