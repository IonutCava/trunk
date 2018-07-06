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

#ifndef _AI_VISUAL_SENSOR_H_
#define _AI_VISUAL_SENSOR_H_

#include "Sensor.h"
#include "Utility/Headers/UnorderedMap.h"
class SceneGraphNode;
namespace AI {

/// SGN GUID, SGN pointer
typedef Unordered_map<U64, SceneGraphNode* > NodeContainer;
/// Container ID, NodeContainer
typedef Unordered_map<U32, NodeContainer> NodeContainerMap;
/// SGN GUID, Last position
typedef Unordered_map<U64, vec3<F32> > NodePositions;
/// Container ID, NodePositions
typedef Unordered_map<U32, NodePositions> NodePositionsMap;

class VisualSensor : public Sensor {
public: 
    void update(const U64 deltaTime);
    void followSceneGraphNode(U32 containerID, SceneGraphNode* const node);
    void unfollowSceneGraphNode(U32 containerID, U64 nodeGUID);

    SceneGraphNode* const getClosestNode(U32 containerID);

protected:
    friend class AIEntity;
	VisualSensor(AIEntity* const parentEntity);

    NodeContainerMap::iterator findContainer(U32 container);
    NodeContainer::const_iterator findNodeEntry(NodeContainerMap::const_iterator containerIt, U64 GUID) const;

protected:
    NodeContainerMap _nodeContainerMap;
    NodePositionsMap _nodePositionsMap;
};
}; //namespace AI
#endif