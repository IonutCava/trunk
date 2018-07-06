/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#ifndef _AI_ACTION_LIST_H_
#define _AI_ACTION_LIST_H_

#include "AI/Headers/AIEntity.h"

enum AI_MSG;

class SceneGraphNode;
class ActionList{
public:
	ActionList() : _entity(NULL){};
	virtual void processData() = 0;
	virtual void processInput() = 0;
	virtual void update(SceneGraphNode* node) = 0;
	virtual void addEntityRef(AIEntity* entity) = 0;
	virtual void processMessage(AIEntity* sender, AI_MSG msg, const boost::any& msg_content) = 0;
protected:
	AIEntity*  _entity;

};

#endif