/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _OCTREE_H_
#define _OCTREE_H_

#include "IntersectionRecord.h"
#include "SceneGraphNode.h"

#include <queue>

namespace Divide {
// ref: http://www.gamedev.net/page/resources/_/technical/game-programming/introduction-to-octrees-r3529
class Octree : public std::enable_shared_from_this<Octree> {
    public:
        Octree(U16 nodeMask);
        Octree(U16 nodeMask, const BoundingBox& rootAABB);
        Octree(U16 nodeMask, const BoundingBox& rootAABB, const vectorEASTL<SceneGraphNode*>& nodes);

        ~Octree();

        void update(const U64 deltaTimeUS);
        bool addNode(SceneGraphNode* node);
        bool addNodes(const vectorEASTL<SceneGraphNode*>& nodes);
        void getAllRegions(vectorEASTL<BoundingBox>& regionsOut) const;

        inline const BoundingBox& getRegion() const {
            return _region;
        }

        void updateTree();

        vectorEASTL<IntersectionRecord> allIntersections(const Frustum& region, U16 typeFilterMask);
        vectorEASTL<IntersectionRecord> allIntersections(const Ray& intersectionRay, F32 start, F32 end);
        IntersectionRecord nearestIntersection(const Ray& intersectionRay, F32 start, F32 end, U16 typeFilterMask);
        vectorEASTL<IntersectionRecord> allIntersections(const Ray& intersectionRay, F32 start, F32 end, U16 typeFilterMask);
        
    private:
        U8 activeNodes() const;
        void buildTree();
        void insert(SceneGraphNode* object);
        void findEnclosingBox();
        void findEnclosingCube();
        std::shared_ptr<Octree>
        createNode(const BoundingBox& region, const vectorEASTL<SceneGraphNode*>& objects);

        std::shared_ptr<Octree>
        createNode(const BoundingBox& region, SceneGraphNode* object);

        bool isStatic(const SceneGraphNode& node) const;
        vectorEASTL<IntersectionRecord> getIntersection(const Frustum& frustum, U16 typeFilterMask) const;
        vectorEASTL<IntersectionRecord> getIntersection(const Ray& intersectRay, F32 start, F32 end, U16 typeFilterMask) const;

        size_t getTotalObjectCount() const;
        void updateIntersectionCache(vectorEASTL<SceneGraphNode*>& parentObjects, U16 typeFilterMask);
        
        void handleIntersection(const IntersectionRecord& intersection) const;
        bool getIntersection(SceneGraphNode& node, const Frustum& frustum, IntersectionRecord& irOut) const;
        bool getIntersection(SceneGraphNode& node1, SceneGraphNode& node2, IntersectionRecord& irOut) const;
        bool getIntersection(SceneGraphNode& node, const Ray& intersectRay, F32 start, F32 end, IntersectionRecord& irOut) const;
        

    private:
        //ToDo: make this work in a multi-threaded environment
        //mutable I8 _frustPlaneCache;
        U16 _nodeMask;
        I32 _curLife;
        I32 _maxLifespan;
        BoundingBox _region;
        std::shared_ptr<Octree> _parent;
        std::array<bool, 8> _activeNodes;
        vectorEASTL<SceneGraphNode*> _objects;
        std::array<std::shared_ptr<Octree>, 8> _childNodes;
        vectorEASTL<SceneGraphNode*> _movedObjects;

        vectorEASTL<IntersectionRecord> _intersectionsCache;

        static vectorEASTL<SceneGraphNode*> s_intersectionsObjectCache;
        static std::queue<SceneGraphNode*> s_pendingInsertion;
        static std::mutex s_pendingInsertLock;
        static bool s_treeReady;
        static bool s_treeBuilt;
};

};  // namespace Divide

#endif