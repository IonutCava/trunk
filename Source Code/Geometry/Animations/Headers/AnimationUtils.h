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

#ifndef ANIMATION_UTILITIES_H_
#define ANIMATION_UTILITIES_H_

#define ANIMATION_TICKS_PER_SECOND 20.0f
#define MAXBONESPERMESH 60

#include "core.h"
#include <assimp/scene.h>
namespace AnimUtils {
	void TransformMatrix(mat4<F32>& out,const aiMatrix4x4& in);
	void TransformMatrix(aiMatrix4x4& out,const mat4<F32>& in);
};

#endif