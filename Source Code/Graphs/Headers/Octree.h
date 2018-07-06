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

#ifndef _OCTREE_H_
#define _OCTREE_H_

#include "IntersectionRecord.h"
#include "SceneGraphNode.h"

namespace Divide {
// ref: http://www.gamedev.net/page/resources/_/technical/game-programming/introduction-to-octrees-r3529
class Octree : public std::enable_shared_from_this<Octree> {
    public:
        Octree(U32 nodeMask);
        Octree(U32 nodeMask, const BoundingBox& rootAABB);
        Octree(U32 nodeMask, const BoundingBox& rootAABB, const vectorImpl<SceneGraphNode_wptr>& nodes);

        ~Octree();

        void update(const U64 deltaTime);
        bool addNode(SceneGraphNode_wptr node);
        bool addNodes(const vectorImpl<SceneGraphNode_wptr>& nodes);
        void getAllRegions(vectorImpl<BoundingBox>& regionsOut) const;

        inline const BoundingBox& getRegion() const {
            return _region;
        }

        void updateTree();

        vectorImpl<IntersectionRecord> allIntersections(const Frustum& region, U32 typeFilterMask);
        vectorImpl<IntersectionRecord> allIntersections(const Ray& intersectionRay, F32 start, F32 end);
        IntersectionRecord nearestIntersection(const Ray& intersectionRay, F32 start, F32 end, U32 typeFilterMask);
        vectorImpl<IntersectionRecord> allIntersections(const Ray& intersectionRay, F32 start, F32 end, U32 typeFilterMask);
        
    private:
        U8 activeNodes() const;
        void buildTree();
        void insert(SceneGraphNode_wptr object);
        void findEnclosingBox();
        void findEnclosingCube();
        std::shared_ptr<Octree>
        createNode(const BoundingBox& region, const vectorImpl<SceneGraphNode_wptr>& objects);

        std::shared_ptr<Octree>
        createNode(const BoundingBox& region, SceneGraphNode_wptr object);

        bool isStatic(const SceneGraphNode& node) const;
        vectorImpl<IntersectionRecord> getIntersection(const Frustum& frustum, U32 typeFilterMask) const;
        vectorImpl<IntersectionRecord> getIntersection(const Ray& intersectRay, F32 start, F32 end, U32 typeFilterMask) const;

        size_t getTotalObjectCount() const;
        void updateIntersectionCache(vectorImpl<SceneGraphNode_wptr>& parentObjects, U32 typeFilterMask);
        
        void handleIntersection(IntersectionRecord intersection) const;
        bool getIntersection(SceneGraphNode& node, const Frustum& frustum, IntersectionRecord& irOut) const;
        bool getIntersection(SceneGraphNode& node1, SceneGraphNode& node2, IntersectionRecord& irOut) const;
        bool getIntersection(SceneGraphNode& node, const Ray& intersectRay, F32 start, F32 end, IntersectionRecord& irOut) const;
        

    private:
        U32 _nodeMask;
        I32 _curLife;
        I32 _maxLifespan;
        BoundingBox _region;
        std::shared_ptr<Octree> _parent;
        std::array<bool, 8> _activeNodes;
        vectorImpl<SceneGraphNode_wptr> _objects;
        std::array<std::shared_ptr<Octree>, 8> _childNodes;
        vectorImpl<SceneGraphNode_wptr> _movedObjects;

        vectorImpl<IntersectionRecord> _intersectionsCache;

        static vectorImpl<SceneGraphNode_wptr> s_intersectionsObjectCache;
        static std::queue<SceneGraphNode_wptr> s_pendingInsertion;
        static bool s_treeReady;
        static bool s_treeBuilt;
};

};  // namespace Divide

#endif