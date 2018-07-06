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

#ifndef _SCENE_GRAPH_H_
#define _SCENE_GRAPH_H_

#include "SceneGraphNode.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
class Ray;
class Scene;
class SceneState;

class SceneGraph  {
public:

    SceneGraph();
    ~SceneGraph();

    inline  SceneGraphNode* getRoot() const { return _root;}

    inline vectorImpl<BoundingBox >& getBBoxes(){
        _boundingBoxes.clear();
        _root->getBBoxes(_boundingBoxes);
        return _boundingBoxes;
    }

    inline  SceneGraphNode* findNode(const stringImpl& name, bool sceneNodeName = false){
        return _root->findNode(name,sceneNodeName);
    }

    /// Update all nodes. Called from "updateSceneState" from class Scene
    void sceneUpdate(const U64 deltaTime, SceneState& sceneState);

    void print();
    void idle();

    void Intersect(const Ray& ray, F32 start, F32 end, vectorImpl<SceneGraphNode* >& selectionHits);
    inline void addToDeletionQueue(SceneGraphNode* node) {
        _pendingDeletionNodes.push_back(node);
    }

    inline void getShadowCastersAndReceivers(vectorImpl<const SceneGraphNode* >& casters, vectorImpl<const SceneGraphNode* >& receivers, bool visibleOnly = false) const {
        casters.clear();
        receivers.clear();
        casters.reserve(_nodeCount);
        receivers.reserve(_nodeCount);
        _root->getShadowCastersAndReceivers(casters, receivers, visibleOnly);
    }

protected:
    friend class SceneGraphNode;
    inline void incNodeCount() { _nodeCount++; }
    inline void decNodeCount() { _nodeCount--; }
    void printInternal(SceneGraphNode* const sgn);

private:
    I32             _nodeCount;
    boost::mutex    _rootAccessMutex;
    SceneGraphNode* _root;
    bool            _updateRunning;
    vectorImpl<BoundingBox>        _boundingBoxes;
    vectorImpl<SceneGraphNode* >   _pendingDeletionNodes;
};

}; //namespace Divide
#endif