/*
   Copyright (c) 2016 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _TRIGGER_H_
#define _TRIGGER_H_

#include "Graphs/Headers/SceneNode.h"

namespace Divide {

class Unit;
class Task;
class ImpostorSphere;
/// When a unit touches the circle described by
class Trigger : public SceneNode {
    typedef std::shared_ptr<Task> Task_ptr;

   public:
    Trigger(const stringImpl& name);
    ~Trigger();

    /// Dummy function from SceneNode;
    bool onRender(SceneGraphNode& sgn, RenderStage currentStage);
    /// Checks if the unit has activated this trigger and launches the Task
    /// If we receive a nullptr unit as a param, we use the camera position
    bool check(Unit* const unit, const vec3<F32>& camEyePos = VECTOR3_ZERO);
    /// Sets a new Task for this trigger
    void updateTriggeredTask(Task_ptr triggeredTask);
    /// Trigger's the Task regardless of position
    bool trigger();
    /// Draw a sphere at the trigger's position
    /// The impostor has the radius of the trigger's radius
    inline void setDrawImpostor(bool state) { _drawImpostor = state; }
    /// Enable or disable the trigger
    inline void setEnabled(bool state) { _enabled = state; }
    /// Set the callback, the position and the radius of the trigger
    void setParams(Task_ptr triggeredTask, const vec3<F32>& triggerPosition,
                   F32 radius);
    /// Just update the callback
    inline void setParams(Task_ptr triggeredTask) {
        setParams(triggeredTask, _triggerPosition, _radius);
    }

    /// SceneNode concrete implementations
    bool unload();

   private:
    /// The Task to be launched when triggered
    Task_ptr _triggeredTask;
    /// The trigger circle's center position
    vec3<F32> _triggerPosition;
    /// The trigger's radius
    F32 _radius;
    /// Draw the impostor?
    bool _drawImpostor;
    /// used for debug rendering / editing - Ionut
    std::shared_ptr<ImpostorSphere> _triggerImpostor;
    bool _enabled;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(Trigger);

};  // namespace Divide

#endif