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

/*Code references:
	http://nolimitsdesigns.com/game-design/open-asset-import-library-animation-loader/
*/

#ifndef BONE_H_
#define BONE_H_

#include "AnimationUtils.h"

class Bone {
public:

	std::string _name;

	aiMatrix4x4 _offsetMatrix;
	aiMatrix4x4 _localTransform;
	aiMatrix4x4 _globalTransform;
	aiMatrix4x4 _originalLocalTransform;

	Bone* _parent;
	vectorImpl<Bone*> _children;

    //index in the current animation's channel array.
	Bone(const std::string& name) : _name(name), _parent(0){}
	Bone() : _parent(0){ }
	~Bone(){SAFE_DELETE_vector(_children); }
};

#endif