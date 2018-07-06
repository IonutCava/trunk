/*
   Copyright (c) 2015 DIVIDE-Studio
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

#include "SceneGraphNode.h"

namespace Divide {

class Octree : public std::enable_shared_from_this<Octree> {
    /// Minimum cube size is 1x1x1
    static const I32 MIN_SIZE = 1;
    static const I32 MAX_LIFE_SPAN_LIMIT = 64;

    public:
        Octree();
        Octree(const BoundingBox& rootAABB);
        Octree(const BoundingBox& rootAABB,
                vectorImpl<SceneGraphNode_wptr> objects);
        ~Octree();

        void update(const U64 deltaTime);
        void registerMovedNode(SceneGraphNode_ptr node);
    private:
        void updateTree();
        void buildTree();
        void insert(SceneGraphNode_wptr object);
        void findEnclosingCube();
        std::shared_ptr<Octree>
        createNode(const BoundingBox& region,
                   vectorImpl<SceneGraphNode_wptr> objects) const;

        std::shared_ptr<Octree>
        createNode(const BoundingBox& region, SceneGraphNode_wptr object) const;
    private:
        I32 _curLife;
        I32 _maxLifespan;
        bool _hasChildren;
        std::weak_ptr<Octree> _parent;
        BoundingBox _region;
        std::array<std::shared_ptr<Octree>, 8> _childNodes;
        std::array<bool,   8> _activeNodes;
        vectorImpl<SceneGraphNode_wptr> _objects;
        hashMapImpl<I64, SceneGraphNode_wptr> _movedObjects;
        static std::queue<SceneGraphNode_ptr> _pendingInsertion;
        static bool _treeReady;
        static bool _treeBuilt;
            
};

};  // namespace Divide

#endif