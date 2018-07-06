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

#ifndef _AITENIS_SCENE_H
#define _AITENIS_SCENE_H

#include "Scenes/Headers/Scene.h"

class AIEntity;
class AICoordination;
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
		_floor(NULL),
		_net(NULL),
		_ballSGN(NULL),
		_ball(NULL){
		_sideImpulseFactor = 0;
		_directionTeam1ToTeam2 = true;
		_upwardsDirection = true;
		_touchedTerrainTeam1 = false;
		_touchedTerrainTeam2 = false;
		_lostTeam1 = false;
		_applySideImpulse = false;
		_scoreTeam1 = 0;
		_scoreTeam2 = 0;
		_mousePressed = false;
		_gamePlaying = false;
        _gameGUID = 0;
	}

	void preRender();

	bool load(const std::string& name);
	bool loadResources(bool continueOnErrors);
	bool initializeAI(bool continueOnErrors);
	bool deinitializeAI(bool continueOnErrors);
	void processInput();
	void processTasks(const U32 time);
	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void onMouseMove(const OIS::MouseEvent& key);
	void onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button);
	void onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);
	void onJoystickMovePOV(const OIS::JoyStickEvent& key,I8 pov);

private:
    //ToDo: replace with Physics system collision detection
    void checkCollisions();
	void playGame(boost::any a, CallbackParam b);
	void startGame();
	void resetGame();

private:
	vec3<F32> _sunvector;
	Sphere3D* _ball;
	SceneGraphNode* _ballSGN;
	SceneGraphNode* _net;
	SceneGraphNode* _floor;
	vec2<F32> _prevMouse;
	bool _mousePressed;

private: //Game stuff
	mutable SharedLock _gameLock;
    mutable boost::atomic_bool _collisionPlayer1;
    mutable boost::atomic_bool _collisionPlayer2;
    mutable boost::atomic_bool _collisionPlayer3;
    mutable boost::atomic_bool _collisionPlayer4;
    mutable boost::atomic_bool _collisionNet;
    mutable boost::atomic_bool _collisionFloor;
	mutable boost::atomic_bool _directionTeam1ToTeam2;
	mutable boost::atomic_bool _upwardsDirection;
	mutable boost::atomic_bool _touchedTerrainTeam1;
	mutable boost::atomic_bool _touchedTerrainTeam2;
	mutable boost::atomic_bool _lostTeam1;
    mutable boost::atomic_bool _applySideImpulse;
	mutable boost::atomic_bool _gamePlaying;
	mutable boost::atomic_int  _scoreTeam1;
	mutable boost::atomic_int  _scoreTeam2;
    F32 _sideImpulseFactor;
    U32 _gameGUID;
	///AIEntities are the "processors" behing the NPC's
	AIEntity *_aiPlayer1, *_aiPlayer2, *_aiPlayer3, *_aiPlayer4;
	///NPC's are the actual game entities
	NPC *_player1, *_player2, *_player3, *_player4;
	///Team's are factions for AIEntites so they can manage friend/foe situations
	AICoordination *_team1, *_team2;
};
#endif