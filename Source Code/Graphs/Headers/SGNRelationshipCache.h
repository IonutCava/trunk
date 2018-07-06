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

#ifndef _SCENE_GRAPH_NODE_RELATIONSHIP_CACHE_H_
#define _SCENE_GRAPH_NODE_RELATIONSHIP_CACHE_H_

#include "Platform/Headers/PlatformDefines.h"
#include "Platform/Threading/Headers/SharedMutex.h"

namespace Divide {

class SceneGraphNode;
class SGNRelationshipCache {
public:
    enum class RelationShipType : U32 {
        GRANDPARENT = 0, //<applies for all levels above 0
        PARENT,
        CHILD,
        GRANDCHILD,
        COUNT
    };
public:
    SGNRelationshipCache(SceneGraphNode& parent);

    bool isValid() const;
    void invalidate();
    bool rebuild();

    // this will issue a rebuild if the cache is invalid
    RelationShipType clasifyNode(I64 GUID) const;
protected:
    void updateChildren(U8 level, vectorImpl<std::pair<I64, U8>>& cache) const;
    void updateParents(U8 level, vectorImpl<std::pair<I64, U8>>& cache) const;

protected:
    // We need a way to accelerate relationship testing
    // We can cache a full recursive list of children
    // pair: GUID ... child level (0 = child, 1 = grandchild, ...)
    vectorImpl<std::pair<I64, U8>> _childrenRecursiveCache;
    // pair: GUID ... parent level (0 = parent, 1 = grandparent, ...)
    vectorImpl<std::pair<I64, U8>> _parentRecursiveCache;
    std::atomic_bool _isValid;
    SceneGraphNode& _parentNode;

    mutable SharedLock _updateMutex;
};

}; //namespace Divide
#endif //_SCENE_GRAPH_NODE_RELATIONSHIP_CACHE_H_