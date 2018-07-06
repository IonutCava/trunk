#include "Headers/VisualSensor.h"

#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"

using namespace AI;

VisualSensor::VisualSensor() : Sensor(VISUAL_SENSOR) 
{
}

void VisualSensor::followSceneGraphNode(U32 containerID, SceneGraphNode* const node) {
    DIVIDE_ASSERT(node != nullptr, "VisualSensor error: Invalid node specified for follow function");
    NodeContainerMap::iterator container = findContainer(containerID);
    if (container != _nodeContainerMap.end()) {
        NodeContainer::const_iterator nodeEntry = findNodeEntry(container, node->getGUID());
        if (nodeEntry == container->second.end()) {
            container->second.insert(std::make_pair(node->getGUID(), node));
            node->registerdeletionCallback(DELEGATE_BIND(&VisualSensor::unfollowSceneGraphNode, this, containerID, node->getGUID()));
        } else {
            ERROR_FN("VisualSensor: Added the same node to follow twice!");
        }
    } else {
        _nodeContainerMap[containerID].insert(std::make_pair(node->getGUID(), node));
    }
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
        for(NodeContainer::value_type& entry : container.second){
        }
    }
}

NodeContainerMap::iterator VisualSensor::findContainer(U32 container)  {
    return _nodeContainerMap.find(container);
}

NodeContainer::const_iterator VisualSensor::findNodeEntry(NodeContainerMap::const_iterator containerIt, U64 GUID) const {
    return containerIt->second.find(GUID);
}