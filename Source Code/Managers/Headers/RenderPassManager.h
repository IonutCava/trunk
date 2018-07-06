/*
   Copyright (c) 2014 DIVIDE-Studio
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

#ifndef _MANAGERS_RENDER_PASS_MANAGER_H_
#define _MANAGERS_RENDER_PASS_MANAGER_H_

#include "core.h"

namespace Divide {

class SceneGraph;

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
    void render(const SceneRenderState& sceneRenderState, SceneGraph* activeSceneGraph);
    ///Add a new pass with the specified key
    void addRenderPass(RenderPass* const renderPass, U8 orderKey);
    ///Remove a renderpass from the manager, optionally not deleting it
    void removeRenderPass(RenderPass* const renderPass,bool deleteRP = true);
    ///Find a renderpass by name and remove it from the manager, optionally not deleting it
    void removeRenderPass(const stringImpl& name,bool deleteRP = true);
    U16 getLastTotalBinSize(U8 renderPassId) const;

    ///Lock or unlock the render bin (if nothing changes: camera, nodes' positions, etc)
    ///Prevent culling and reparsing of the scenegraph when rendering the scene multiple times from the same PoV
    void lock();
    ///Use lock/unlock, for example, when rendering the scene with a Z prepass. Only the prepass needs to parse the scene
    void unlock(bool resetNodes = false);
    ///simple lock check
    inline bool isLocked() const {return _renderPassesLocked;}
protected:
    friend class RenderPassCuller;
    ///simple node reset flag
    inline bool isResetQueued()           const {return _renderPassesResetQueued;}
    inline void isResetQueued(bool state)       {_renderPassesResetQueued  = state;}
private:
    RenderPassManager();
    ~RenderPassManager();

private:
    vectorImpl<RenderPassItem > _renderPasses;
    bool _renderPassesLocked;
    bool _renderPassesResetQueued;

END_SINGLETON

}; //namespace Divide

#endif
