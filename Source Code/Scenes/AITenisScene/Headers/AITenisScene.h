/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _AITENIS_SCENE_H
#define _AITENIS_SCENE_H

#include "Scenes/Headers/Scene.h"

class AIEntity;
class Sphere3D;
class NPC;

class AITenisScene : public Scene {

public:
	AITenisScene() : Scene(),
		_aiPlayer1(NULL),
		_aiPlayer2(NULL),
		_aiPlayer3(NULL),
		_aiPlayer4(NULL),
		_player1(NULL),
		_player2(NULL),
		_player3(NULL),
		_player4(NULL),
		_podea(NULL),
		_fileu(NULL),
		_mingeSGN(NULL),
		_minge(NULL){
		_impulsLateral = 0;
		_directieEchipa1SpreEchipa2 = true;
		_directieAscendenta = true;
		_atinsTerenEchipa1 = false;
		_atinsTerenEchipa2 = false;
		_pierdutEchipa1 = false;
		_aplicaImpulsLateral = false;
		_scorEchipa1 = 0;
		_scorEchipa2 = 0;
		_mousePressed = false;
	}
	~AITenisScene() {}
	void render();
	void preRender();

	bool load(const std::string& name);
	bool loadResources(bool continueOnErrors);
	bool unload();
	void processInput();
	void processEvents(F32 time);
	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void onMouseMove(const OIS::MouseEvent& key);
	void onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button);
	void onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);

private:
	void procesareJoc(boost::any a, CallbackParam b);
	void startJoc();
	void reseteazaJoc();
private:
	std::vector<F32> _eventTimers;
	F32 angleLR,angleUD,moveFB,moveLR;
	I8 _scor;
	vec4<F32> _sunVector;
	Sphere3D* _minge;
	SceneGraphNode* _mingeSGN;
	SceneGraphNode* _fileu;
	SceneGraphNode* _podea;
	vec2<F32> _prevMouse;
	bool _mousePressed;

private: //Joc
	F32 _impulsLateral;
	bool _directieEchipa1SpreEchipa2;
	bool _directieAscendenta;
	bool _atinsTerenEchipa1;
	bool _atinsTerenEchipa2;
	bool _pierdutEchipa1;
	bool _aplicaImpulsLateral;
	I8 _scorEchipa1;
	I8 _scorEchipa2;
	AIEntity *_aiPlayer1, *_aiPlayer2, *_aiPlayer3, *_aiPlayer4;
	NPC *_player1, *_player2, *_player3, *_player4;
};

#endif