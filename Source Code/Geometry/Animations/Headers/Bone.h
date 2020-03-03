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

/*Code references:
    http://nolimitsdesigns.com/game-design/open-asset-import-library-animation-loader/
*/

#pragma once
#ifndef BONE_H_
#define BONE_H_

#include "AnimationUtils.h"

namespace Divide {

class Bone {
   protected:
       stringImpl _name;
       U64        _nameKey;
   public:
    I32 _boneID;
    mat4<F32> _offsetMatrix;
    mat4<F32> _localTransform;
    mat4<F32> _globalTransform;
    mat4<F32> _originalLocalTransform;

    Bone* _parent;
    vector<Bone*> _children;

    // index in the current animation's channel array.
    Bone(const stringImpl& name) noexcept
        : _name(name),
          _nameKey(_ID(name.c_str())),
          _parent(nullptr),
          _boneID(-1)
    {
    }

    Bone() : Bone("")
    {
    }

    ~Bone()
    {
        MemoryManager::DELETE_CONTAINER(_children);
    }

    inline size_t hierarchyDepth() const {
        size_t size = _children.size();
        for (const Bone* child : _children) {
            size += child->hierarchyDepth();
        }

        return size;
    }

    inline Bone* find(const stringImpl& name) {
        return find(_ID(name.c_str()));
    }

    inline Bone* find(U64 nameKey) {
        if (_nameKey == nameKey) {
            return this;
        }

        for (Bone* child : _children) {
            Bone* childNode = child->find(nameKey);
            if (childNode != nullptr) {
                return childNode;
            }
        }

        return nullptr;
    }

    inline void createBoneList(vector<Bone*>& boneList) {
        boneList.push_back(this);
        for (Bone* child : _children) {
            child->createBoneList(boneList);
        }
    }

    inline const stringImpl& name() const noexcept {
        return _name;
    }

    inline void name(const stringImpl& name) {
        _name = name;
        _nameKey = _ID(name.c_str());
    }
};

};  // namespace Divide

#endif