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

#ifndef ANIMATION_EVALUATOR_H_
#define ANIMATION_EVALUATOR_H_

#include "Bone.h"
#include <assimp/anim.h>

class AnimationChannel{
public:
	std::string _name;
	vectorImpl<aiVectorKey > _positionKeys;
	vectorImpl<aiQuatKey   > _rotationKeys;
	vectorImpl<aiVectorKey > _scalingKeys;
	/** The number of position keys */
	U32 _numPositionKeys;
	U32 _numRotationKeys;
	U32 _numScalingKeys;
};

struct aiAnimation;
class AnimEvaluator{
public:

	AnimEvaluator(): _lastFrameIndex(0),
		             _lastTime(0.0),
					 _ticksPerSecond(0.0),
					 _duration(0.0),
					 _playAnimationForward(true)
	{}

	AnimEvaluator( const aiAnimation* pAnim);

	void Evaluate( D32 pTime, Unordered_map<std::string, Bone*>& bones);
	void Save(std::ofstream& file);
	void Load(std::ifstream& file);
	U32 GetFrameIndexAt(D32 time);

	inline U32 GetFrameIndex() const {return _lastFrameIndex;}
	inline U32 GetFrameCount() const {return _transforms.size();}
	inline vectorImpl<mat4<F32> >& GetTransforms(D32 dt){ return _transforms[GetFrameIndexAt(dt)]; }

protected:
	friend class SceneAnimator;
	std::string _name;

	/// Array to return transformations results inside.
	vectorImpl<vectorImpl<mat4<F32> >> _transforms;
	vectorImpl<vectorImpl<mat4<F32> >> _quatTransforms;

	/// play forward == true, play backward == false
	bool _playAnimationForward;
	D32 _lastTime;
	D32 _duration;
	D32 _ticksPerSecond;

private:

	U32 _lastFrameIndex;
	vectorImpl<vec3<U32> > _lastPositions;
	///vector that holds all bone channels
	vectorImpl<AnimationChannel> _channels;
};

#endif