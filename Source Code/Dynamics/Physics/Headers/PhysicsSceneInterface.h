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

#ifndef _PHYSICS_SCENE_INTERFACE_H_
#define _PHYSICS_SCENE_INTERFACE_H_

class Scene;
class Transform;
class PhysicsSceneInterface {

public:
	PhysicsSceneInterface(Scene* parentScene) :  _parentScene(parentScene){}

	virtual ~PhysicsSceneInterface(){}
	virtual bool init() = 0;
	virtual bool exit() = 0;
	virtual void idle() = 0;
	virtual void release() = 0;
	virtual void update() = 0;
	virtual void process(F32 timeStep) = 0;

protected:
	Scene* _parentScene;
};

#endif