/*
   Copyright (c) 2013 DIVIDE-Studio
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

#define SCENE_GRAPH_UPDATE(pointer) DELEGATE_BIND(&SceneGraph::update, pointer)

#include "SceneGraphNode.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

class Ray;
class Scene;
class SceneState;
class SceneGraph  {
public:

    SceneGraph();

    ~SceneGraph(){
        D_PRINT_FN(Locale::get("DELETE_SCENEGRAPH"));
        _root->unload(); //< Should recursivelly call unload on the entire scene graph
        //Should recursivelly call delete on the entire scene graph
        SAFE_DELETE(_root);
    }

    inline  SceneGraphNode* getRoot() const {return _root;}

    inline  vectorImpl<BoundingBox >& getBBoxes(){
        _boundingBoxes.clear();
        return _root->getBBoxes(_boundingBoxes);
    }

    inline  SceneGraphNode* findNode(const std::string& name, bool sceneNodeName = false){
        return _root->findNode(name,sceneNodeName);
    }

    /// Update transforms and bounding boxes
    void update();
    /// Update all nodes. Called from "updateSceneState" from class Scene
    void sceneUpdate(const U32 sceneTime, SceneState& sceneState);

    void print();

    void startUpdateThread();

    void idle();

    void Intersect(const Ray& ray, F32 start, F32 end, vectorImpl<SceneGraphNode* >& selectionHits);
    void addToDeletionQueue(SceneGraphNode* node) {_pendingDeletionNodes.push_back(node);}

private:
    boost::mutex    _rootAccessMutex;
    SceneGraphNode* _root;
    bool            _updateRunning;
    vectorImpl<BoundingBox>        _boundingBoxes;
    vectorImpl<SceneGraphNode* >   _pendingDeletionNodes;
};
#endif