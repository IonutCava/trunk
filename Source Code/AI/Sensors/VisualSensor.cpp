#include "Headers/VisualSensor.h"

#include "AI/Headers/AIEntity.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {
using namespace AI;

VisualSensor::VisualSensor(AIEntity* const parentEntity) : Sensor(parentEntity, VISUAL_SENSOR) 
{
}

void VisualSensor::followSceneGraphNode(U32 containerID, SceneGraphNode* const node) {
    DIVIDE_ASSERT(node != nullptr, "VisualSensor error: Invalid node specified for follow function");
    NodeContainerMap::iterator container = findContainer(containerID);

    if (container != _nodeContainerMap.end()) {
        NodeContainer::const_iterator nodeEntry = findNodeEntry(container, node->getGUID());
        if (nodeEntry == container->second.end()) {
            container->second.emplace(node->getGUID(), node);
            node->registerdeletionCallback(DELEGATE_BIND(&VisualSensor::unfollowSceneGraphNode, this, containerID, node->getGUID()));
        } else {
            ERROR_FN("VisualSensor: Added the same node to follow twice!");
        }
    } else {
        NodeContainer& newContainer = _nodeContainerMap[containerID];
        newContainer.emplace(node->getGUID(), node);
        node->registerdeletionCallback(DELEGATE_BIND(&VisualSensor::unfollowSceneGraphNode, this, containerID, node->getGUID()));
    }

    NodePositions& positions = _nodePositionsMap[containerID];
    positions.emplace(node->getGUID(), node->getComponent<PhysicsComponent>()->getConstTransform()->getPosition());
}

void VisualSensor::unfollowSceneGraphNode(U32 containerID, U64 nodeGUID) {
    DIVIDE_ASSERT(nodeGUID != 0, "VisualSensor error: Invalid node GUID specified for unfollow function");
    NodeContainerMap::iterator container = findContainer(containerID);
    if (container != _nodeContainerMap.end()) {
        NodeContainer::const_iterator nodeEntry = findNodeEntry(container, nodeGUID);
        if (nodeEntry != container->second.end()) {
            container->second.erase(nodeEntry);
        } else {
            ERROR_FN("VisualSensor: Specified node does not exist in specified container!");
        }
    } else {
        ERROR_FN("VisualSensor: Invalid container specified for unfollow!");
    }
}


void VisualSensor::update(const U64 deltaTime) {
    for(NodeContainerMap::value_type& container : _nodeContainerMap) {
        NodePositions& positions = _nodePositionsMap[container.first];
        for(NodeContainer::value_type& entry : container.second){
            positions[entry.first] = entry.second->getComponent<PhysicsComponent>()->getConstTransform()->getPosition();
        }
    }
}

SceneGraphNode* const VisualSensor::getClosestNode(U32 containerID) {
    NodeContainerMap::iterator container = findContainer(containerID);
    if (container != _nodeContainerMap.end()) {
        NodePositions& positions = _nodePositionsMap[container->first];
        NPC* const unit = _parentEntity->getUnitRef();
        if (unit) {
            const vec3<F32>& currentPosition = unit->getCurrentPosition();
            U64 currentNearest = 0;
            U32 currentDistanceSq = 0;
            for(NodePositions::value_type& entry : positions) {
                U32 temp = currentPosition.distanceSquared(entry.second);           
                if (temp < currentDistanceSq) {
                    currentDistanceSq = temp;
                    currentNearest = entry.first;
                }
            }
            if (currentNearest != 0) {
                NodeContainer::const_iterator nodeEntry = findNodeEntry(container, currentNearest);
                if (nodeEntry != container->second.end()) {
                    return nodeEntry->second;
                }
            }
        }
    }
    return nullptr;
}

F32 VisualSensor::getDistanceToNodeSq(U32 containerID, U64 nodeGUID) {
    DIVIDE_ASSERT(nodeGUID != 0, "VisualSensor error: Invalid node GUID specified for distance request");
    NPC* const unit = _parentEntity->getUnitRef();
    if (unit) {
        return unit->getCurrentPosition().distanceSquared(getNodePosition(containerID, nodeGUID));          
    }

    return std::numeric_limits<F32>::max();
}

vec3<F32> VisualSensor::getNodePosition(U32 containerID, U64 nodeGUID) {
    DIVIDE_ASSERT(nodeGUID != 0, "VisualSensor error: Invalid node GUID specified for position request");
    NodeContainerMap::iterator container = findContainer(containerID);
    if (container != _nodeContainerMap.end()) {
        NodePositions& positions = _nodePositionsMap[container->first];
        NodePositions::iterator it = positions.find(nodeGUID);
        if (it != positions.end()) {
            return it->second;          
        }
    }

    return vec3<F32>(std::numeric_limits<F32>::max());
}

NodeContainerMap::iterator VisualSensor::findContainer(U32 container)  {
    return _nodeContainerMap.find(container);
}

NodeContainer::const_iterator VisualSensor::findNodeEntry(NodeContainerMap::const_iterator containerIt, U64 GUID) const {
    return containerIt->second.find(GUID);
}

}; //namespace Divide