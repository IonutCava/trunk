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


#ifndef _RENDER_PASS_H_
#define _RENDER_PASS_H_

#include "core.h"
class SceneRenderState;
class RenderPass {
public:
	RenderPass(const std::string& name);
	~RenderPass();
	virtual void render(SceneRenderState* const sceneRenderState = NULL);
	inline U16 getLasTotalBinSize() {return _lastTotalBinSize;}
	inline void updateMaterials(bool state) {_updateMaterials = state;}
	inline const std::string& getName() {return _name;}
private:
	std::string _name;
	U8 _lastTotalBinSize;
	bool _updateMaterials;
};

#endif