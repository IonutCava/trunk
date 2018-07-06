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
        for (const NodeContainer::value_type& entry : container.second) {
            if (entry.second) {
                hashMapImpl<I64, size_t>::const_iterator itCbk = _deletionCallbackID.find(entry.second->getGUID());
                assert(itCbk != std::end(_deletionCallbackID));
                entry.second->unregisterDeletionCallback(itCbk->second);
            }
        }
        container.second.clear();
    }
    _nodeContainerMap.clear();
    _nodePositionsMap.clear();
}

void VisualSensor::followSceneGraphNode(U32 containerID,
                                        SceneGraphNode& node) {

    NodeContainerMap::iterator container = _nodeContainerMap.find(containerID);

    if (container != std::end(_nodeContainerMap)) {
        NodeContainer::const_iterator nodeEntry =
            container->second.find(node.getGUID());
        if (nodeEntry == std::end(container->second)) {
            hashAlg::emplace(container->second, node.getGUID(), &node);
            size_t deletionCallbackID = node.registerDeletionCallback(
                DELEGATE_BIND(&VisualSensor::unfollowSceneGraphNode, this,
                              containerID, node.getGUID()));
            std::pair<hashMapImpl<I64, size_t>::iterator, bool> result =
            hashAlg::emplace(_deletionCallbackID, node.getGUID(),
                             deletionCallbackID);
            assert(result.second);
        } else {
            Console::errorfn(
                "VisualSensor: Added the same node to follow twice!");
        }
    } else {
        NodeContainer& newContainer = _nodeContainerMap[containerID];

        hashAlg::emplace(newContainer, node.getGUID(), &node);
        size_t deletionCallbackID = node.registerDeletionCallback(
            DELEGATE_BIND(&VisualSensor::unfollowSceneGraphNode, this,
                          containerID, node.getGUID()));
        std::pair<hashMapImpl<I64, size_t>::iterator, bool> result =
            hashAlg::emplace(_deletionCallbackID, node.getGUID(),
                             deletionCallbackID);
        assert(result.second);
    }

    NodePositions& positions = _nodePositionsMap[containerID];
    hashAlg::emplace(positions, node.getGUID(),
                     node.getComponent<PhysicsComponent>()->getPosition());
}

void VisualSensor::unfollowSceneGraphNode(U32 containerID, U64 nodeGUID) {
    DIVIDE_ASSERT(nodeGUID != 0,
                  "VisualSensor error: Invalid node GUID specified for "
                  "unfollow function");
    NodeContainerMap::iterator container = _nodeContainerMap.find(containerID);
    if (container != std::end(_nodeContainerMap)) {
        NodeContainer::iterator nodeEntry = container->second.find(nodeGUID);
        if (nodeEntry != std::end(container->second)) {
            hashMapImpl<I64, size_t>::const_iterator itCbk = _deletionCallbackID.find(nodeGUID);
            assert(itCbk != std::end(_deletionCallbackID));
            nodeEntry->second->unregisterDeletionCallback(itCbk->second);
            container->second.erase(nodeEntry);
            NodePositions& positions = _nodePositionsMap[containerID];
            NodePositions::const_iterator it = positions.find(nodeGUID);
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
            positions[entry.first] =
                entry.second->getComponent<PhysicsComponent>()->getPosition();
        }
    }
}

SceneGraphNode* VisualSensor::getClosestNode(U32 containerID) {
    NodeContainerMap::iterator container = _nodeContainerMap.find(containerID);
    if (container != std::end(_nodeContainerMap)) {
        NodePositions& positions = _nodePositionsMap[container->first];
        NPC* const unit = _parentEntity->getUnitRef();
        if (unit) {
            const vec3<F32>& currentPosition = unit->getCurrentPosition();
            U64 currentNearest = positions.begin()->first;
            U32 currentDistanceSq = std::numeric_limits<U32>::max();
            for (const NodePositions::value_type& entry : positions) {
                U32 temp = currentPosition.distanceSquared(entry.second);
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
    return nullptr;
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