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

#ifndef _TRIGGER_H_
#define _TRIGGER_H_

#include "core.h"
#include "Graphs/Headers/SceneNode.h"

class Unit;
class Task;
class Impostor;
/// When a unit touches the circle described by
class Trigger  : public SceneNode {

	typedef std::tr1::shared_ptr<Task> Task_ptr;

public:
	Trigger();
	~Trigger();

	/// Checks if the unit has activated this trigger and launches the Task
	/// If we receive a NULL unit as a param, we use the camera position
	bool check(Unit* const unit,const vec3<F32>& camEyePos = vec3<F32>(0,0,0));
	/// Sets a new Task for this trigger
	void updateTriggeredTask(Task_ptr triggeredTask);
	/// Trigger's the Task regardless of position
	bool trigger();
	///Draw a sphere at the trigger's position
	///The impostor has the radius of the trigger's radius
	inline void  setDrawImpostor(bool state) {_drawImpostor = state;}
	///Enable or disable the trigger
	inline void  setEnabled(bool state) {_enabled = state;}
    /// Set the callback, the position and the radius of the trigger
	void setParams(Task_ptr triggeredTask, const vec3<F32>& triggerPosition, F32 radius);
	/// Just update the callback
	inline void setParams(Task_ptr triggeredTask) {setParams(triggeredTask,_triggerPosition,_radius);}

	///Dummy function from SceneNode;
	void onDraw() {};

	///SceneNode concrete implementations
	bool unload();
	void postLoad(SceneGraphNode* const sgn);	

	///When the SceneGraph calls the trigger's render function, we draw the impostor if needed
	virtual void render(SceneGraphNode* const sgn);

	///SceneNode test
	bool isInView(bool distanceCheck,BoundingBox& boundingBox,const BoundingSphere& sphere){return _drawImpostor;}

private:
	/// The Task to be launched when triggered
	Task_ptr _triggeredTask;
	/// The trigger circle's center position
	vec3<F32> _triggerPosition;
	/// The trigger's radius
	F32 _radius;
	/// Draw the impostor?
	bool _drawImpostor;
	Impostor* _triggerImpostor; ///< used for debug rendering / editing - Ionut

	SceneGraphNode *_triggerSGN, *_impostorSGN;
	bool _enabled;
};

#endif