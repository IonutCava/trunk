#include "Headers/GUIText.h"

void GUIText::onResize(const vec2<I32>& newSize){
	_position.x -= newSize.x;
	_position.y -= newSize.y;
}

void GUIText::onMouseMove(const GUIEvent &event){

}

void GUIText::onMouseUp(const GUIEvent &event){

}

void GUIText::onMouseDown(const GUIEvent &event){

}