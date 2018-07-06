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

#ifndef _RENDER_PASS_MANAGER_H_
#define _RENDER_PASS_MANAGER_H_
#include "core.h"
class RenderPass;

struct RenderPassItem{
	RenderPass* _rp;
	U8 _sortKey;
	RenderPassItem(U8 sortKey, RenderPass *rp ) : _rp(rp), _sortKey(sortKey)
	{
	}
};

class SceneRenderState;

DEFINE_SINGLETON (RenderPassManager)

public:
	///Call every renderqueue's render function in order
	void render(SceneRenderState* const sceneRenderState = NULL);
	///Add a new pass with the specified key
	void addRenderPass(RenderPass* const renderPass, U8 orderKey);
	///Remove a renderpass from the manager, optionally not deleting it
	void removeRenderPass(RenderPass* const renderPass,bool deleteRP = true);
	///Find a renderpass by name and remove it from the manager, optionally not deleting it
	void removeRenderPass(const std::string& name,bool deleteRP = true);
	U16 getLastTotalBinSize(U8 renderPassId);

private:
	~RenderPassManager();
	vectorImpl<RenderPassItem > _renderPasses;

END_SINGLETON

#endif
