#include "stdafx.h"

#include "Headers/SGNRelationshipCache.h"
#include "Headers/SceneGraphNode.h"

namespace Divide {

SGNRelationshipCache::SGNRelationshipCache(SceneGraphNode& parent)
    : _parentNode(parent)
{
    _isValid = false;
}

bool SGNRelationshipCache::isValid() const {
    return _isValid;
}

void SGNRelationshipCache::invalidate() {
    _isValid = false;
}

bool SGNRelationshipCache::rebuild() {
    UniqueLockShared w_lock(_updateMutex);
    updateChildren(0, _childrenRecursiveCache);
    updateParents(0, _parentRecursiveCache);
    _isValid = true;
    return true;
}

SGNRelationshipCache::RelationShipType
SGNRelationshipCache::clasifyNode(I64 GUID) const {
    assert(isValid());

    if (GUID != _parentNode.getGUID()) {
        SharedLock r_lock(_updateMutex);
        for (const std::pair<I64, U8>& entry : _childrenRecursiveCache) {
            if (entry.first == GUID) {
                return entry.second > 0 
                                    ? RelationShipType::GRANDCHILD
                                    : RelationShipType::CHILD;
            }
        }
        for (const std::pair<I64, U8>& entry : _parentRecursiveCache) {
            if (entry.first == GUID) {
                return entry.second > 0
                    ? RelationShipType::GRANDPARENT
                    : RelationShipType::PARENT;
            }
        }
    }

    return RelationShipType::COUNT;
}


void SGNRelationshipCache::updateChildren(U8 level, vector<std::pair<I64, U8>>& cache) const {
    _parentNode.forEachChild([level, &cache](const SceneGraphNode* child) {
        cache.push_back(std::make_pair(child->getGUID(), level));
        child->relationshipCache().updateChildren(level + 1, cache);
    });
}

void SGNRelationshipCache::updateParents(U8 level, vector<std::pair<I64, U8>>& cache) const {
    SceneGraphNode* parent = _parentNode.getParent();
    // We ignore the root note when considering grandparent status
    if (parent && parent->getParent()) {
        cache.push_back(std::make_pair(parent->getGUID(), level));
        parent->relationshipCache().updateParents(level + 1, cache);
    }
}

}; //namespace Divide