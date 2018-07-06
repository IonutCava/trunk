/*
   Copyright (c) 2014 DIVIDE-Studio
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

#ifndef _AI_SCENE_IMPLEMENTATION_H_
#define _AI_SCENE_IMPLEMENTATION_H_

#include "AI/Headers/AIEntity.h"
#include "AI/Headers/GOAPContext.h"

enum AIMsg;
#include "Core/Headers/cdigginsAny.h"

/// Provides a scene-level AI implementation
class AISceneImpl{
public:
    AISceneImpl(const GOAPContext& context) : _entity(nullptr),
                                              _context(context)
    {
    }
	virtual void processData(const U64 deltaTime) = 0;
	virtual void processInput(const U64 deltaTime) = 0;
	virtual void update(NPC* unitRef = nullptr) = 0;
	virtual void addEntityRef(AIEntity* entity) = 0;
	virtual void processMessage(AIEntity* sender, AIMsg msg, const cdiggins::any& msg_content) = 0;

    const GOAPContext& getGOAPContext() const {return _context;}
protected:
	AIEntity*  _entity;
    GOAPContext _context;
};

#endif