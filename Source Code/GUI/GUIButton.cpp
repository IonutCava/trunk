#include "Headers\GUIButton.h"

void GUIButton::onMouseMove(const GUIEvent &event){

	if(event.mousePoint.x > _position.x   &&  event.mousePoint.x < _position.x+_dimensions.x &&
	   event.mousePoint.y > _position.y   &&  event.mousePoint.y < _position.y+_dimensions.y )	{
		if(isActive()) _highlight = true;
	}else{
		_highlight = false;
	}
}

void GUIButton::onMouseUp(const GUIEvent &event){

	if (_pressed){
		if (_callbackFunction) 	_callbackFunction();
		_pressed = false;
	}
}

void GUIButton::onMouseDown(const GUIEvent &event){

	if (_highlight) _pressed = true;
	else _pressed = false;
}