/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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
	std::vector<Bone*> _children;

    //index in the current animation's channel array.
	Bone(const std::string& name) : _name(name), _parent(0){}
	Bone() : _parent(0){ }
	~Bone(){SAFE_DELETE_VECTOR(_children); }
};

#endif