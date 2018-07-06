#include "Headers/GUIText.h"

namespace Divide {

void GUIText::onResize(const vec2<I32>& newSize){
    _position.x -= newSize.x;
    _position.y -= newSize.y;
}

void GUIText::mouseMoved(const GUIEvent &event){
}

void GUIText::onMouseUp(const GUIEvent &event){
}

void GUIText::onMouseDown(const GUIEvent &event){
}

};