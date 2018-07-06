#include "stdafx.h"

#include "Headers/VisualSensor.h"

#include "AI/Headers/AIEntity.h"
#include "Core/Headers/Console.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {
using namespace AI;

VisualSensor::VisualSensor(AIEntity* const parentEntity)
    : Sensor(parentEntity, SensorType::VISUAL_SENSOR)
{
}

VisualSensor::~VisualSensor()
{
    for (NodeContainerMap::value_type& container : _nodeContainerMap) {
        container.second.clear();
    }
    _nodeContainerMap.clear();
    _nodePositionsMap.clear();
}

void VisualSensor::followSceneGraphNode(U32 containerID, SceneGraphNode_wptr node) {

    NodeContainerMap::iterator container = _nodeContainerMap.find(containerID);

    SceneGraphNode_ptr sgnNode(node.lock());
    assert(sgnNode);

    if (container != std::end(_nodeContainerMap)) {
        std::pair<NodeContainer::const_iterator, bool> result;
        result = hashAlg::emplace(container->second,
                                  sgnNode->getGUID(),
                                  node);
        if (!result.second) {
            Console::errorfn("VisualSensor: Added the same node to follow twice!");
        }
    } else {
        hashAlg::emplace(_nodeContainerMap[containerID],
                         sgnNode->getGUID(),
                         node);
       
    }

    NodePositions& positions = _nodePositionsMap[containerID];
    hashAlg::emplace(positions,
                     sgnNode->getGUID(),
                     sgnNode->get<PhysicsComponent>()->getPosition());
}

void VisualSensor::unfollowSceneGraphNode(U32 containerID, U64 nodeGUID) {
    DIVIDE_ASSERT(nodeGUID != 0,
                  "VisualSensor error: Invalid node GUID specified for "
                  "unfollow function");
    NodeContainerMap::iterator container = _nodeContainerMap.find(containerID);
    if (container != std::end(_nodeContainerMap)) {
        NodeContainer::iterator nodeEntry = container->second.find(nodeGUID);
        if (nodeEntry != std::end(container->second)) {
         
            container->second.erase(nodeEntry);
            NodePositions& positions = _nodePositionsMap[containerID];
            NodePositions::iterator it = positions.find(nodeGUID);
            if (it != std::end(positions)) {
                positions.erase(it);
            }
        }
    }
}

void VisualSensor::update(const U64 deltaTime) {
    for (const NodeContainerMap::value_type& container : _nodeContainerMap) {
        NodePositions& positions = _nodePositionsMap[container.first];
        for (const NodeContainer::value_type& entry : container.second) {
            SceneGraphNode_ptr sgn(entry.second.lock());
            if (sgn) {
                positions[entry.first] =
                    sgn->get<PhysicsComponent>()->getPosition();
            }
        }
    }
}

SceneGraphNode_wptr VisualSensor::getClosestNode(U32 containerID) {
    NodeContainerMap::iterator container = _nodeContainerMap.find(containerID);
    if (container != std::end(_nodeContainerMap)) {
        NodePositions& positions = _nodePositionsMap[container->first];
        NPC* const unit = _parentEntity->getUnitRef();
        if (unit) {
            const vec3<F32>& currentPosition = unit->getCurrentPosition();
            U64 currentNearest = positions.begin()->first;
            F32 currentDistanceSq = std::numeric_limits<F32>::max();
            for (const NodePositions::value_type& entry : positions) {
                F32 temp = currentPosition.distanceSquared(entry.second);
                if (temp < currentDistanceSq) {
                    currentDistanceSq = temp;
                    currentNearest = entry.first;
                }
            }
            if (currentNearest != 0) {
                NodeContainer::const_iterator nodeEntry =
                    container->second.find(currentNearest);
                if (nodeEntry != std::end(container->second)) {
                    return nodeEntry->second;
                }
            }
        }
    }

    return SceneGraphNode_wptr();
}

F32 VisualSensor::getDistanceToNodeSq(U32 containerID, U64 nodeGUID) {
    DIVIDE_ASSERT(
        nodeGUID != 0,
        "VisualSensor error: Invalid node GUID specified for distance request");
    NPC* const unit = _parentEntity->getUnitRef();
    if (unit) {
        return unit->getCurrentPosition().distanceSquared(
            getNodePosition(containerID, nodeGUID));
    }

    return std::numeric_limits<F32>::max();
}

vec3<F32> VisualSensor::getNodePosition(U32 containerID, U64 nodeGUID) {
    DIVIDE_ASSERT(
        nodeGUID != 0,
        "VisualSensor error: Invalid node GUID specified for position request");
    NodeContainerMap::iterator container = _nodeContainerMap.find(containerID);
    if (container != std::end(_nodeContainerMap)) {
        NodePositions& positions = _nodePositionsMap[container->first];
        NodePositions::iterator it = positions.find(nodeGUID);
        if (it != std::end(positions)) {
            return it->second;
        }
    }

    return vec3<F32>(std::numeric_limits<F32>::max());
}

};  // namespace Divide