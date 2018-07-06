/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _GUI_ELEMENT_H_
#define _GUI_ELEMENT_H_

#include "Core/Math/Headers/MathMatrices.h"

namespace CEGUI {
    class Window;
};

namespace Divide {

enum GUIType
{
    GUI_TEXT            = 0,
    GUI_BUTTON            = 1,
    GUI_FLASH            = 2,
    GUI_CONSOLE         = 3,
    GUI_MESSAGE_BOX     = 4,
    GUI_CONFIRM_DIALOG  = 5,
    GUIType_PLACEHOLDER = 6
};

struct GUIEvent {
   /// ascii character code 'a', 'A', 'b', '*', etc (if device==keyboard) - possibly a uchar or something
   U16                  ascii;            
   /// SI_LSHIFT, etc
   U8                   modifier;         
   /// for unprintables, 'tab', 'return', ...
   U16                  keyCode;          
   /// for mouse events
   vec2<F32>            mousePoint;       
   U8                   mouseClickCount;
};

class RenderStateBlock;
class GUIElement{
    friend class GUI;

public:
    GUIElement(CEGUI::Window* const parent, const GUIType& type,const vec2<I32>& position);
    virtual ~GUIElement();

    inline const stringImpl& getName() const {return _name;}
    inline const vec2<I32>&   getPosition()  const {return _position;}
    inline void  setPosition(const vec2<I32>& pos)        {_position = pos;}
    inline const GUIType getType()   const { return _guiType; }
    inline size_t getStateBlockHash() const { return _guiSBHash; }
    inline const bool isActive()  const {return _active;}
    inline const bool isVisible() const {return _visible;}

    inline  void  setName(const stringImpl& name) {_name = name;}
    virtual void  setVisible(const bool visible)   {_visible = visible;}
    virtual void  setActive(const bool active)     {_active = active;}

    inline void  addChildElement(GUIElement* child)    {}

    virtual void setTooltip(const stringImpl& tooltipText) {}
    virtual void onResize(const vec2<I32>& newSize){_position.x -= newSize.x;_position.y -= newSize.y;}

    inline  void lastDrawTimer(const U64 time) { _lastDrawTimer = time; }
    virtual void mouseMoved(const GUIEvent &event){};
    virtual void onMouseUp(const GUIEvent &event){};
    virtual void onMouseDown(const GUIEvent &event){};
/*  virtual void onRightMouseUp(const GUIEvent &event);
    virtual void onRightMouseDown(const GUIEvent &event);
    virtual bool onKeyUp(const GUIEvent &event);
    virtual bool onKeyDown(const GUIEvent &event);
*/
protected:
    vec2<I32> _position;
    GUIType   _guiType;
    size_t    _guiSBHash;
    U64       _lastDrawTimer;
    CEGUI::Window*    _parent;

private:
    stringImpl _name;
    bool        _visible;
    bool        _active;
};

}; //namespace Divide

#endif