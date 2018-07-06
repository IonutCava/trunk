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

#ifndef _RENDER_QUEUE_H_
#define _RENDER_QUEUE_H_

#include "RenderBin.h"
#include "Core/Headers/Singleton.h"
#include "Utility/Headers/UnorderedMap.h"

class SceneNode;

///This class manages all of the RenderBins and renders them in the correct order
DEFINE_SINGLETON( RenderQueue )
    typedef Unordered_map<RenderBin::RenderBinType, RenderBin* > RenderBinMap;
    typedef Unordered_map<U16, RenderBin::RenderBinType > RenderBinIDType;

public:
    ///
    void sort(const RenderStage& currentRenderStage);
    void refresh(bool force = false);
    void addNodeToQueue(SceneGraphNode* const sgn, const vec3<F32>& eyePos);
    U16 getRenderQueueStackSize();
    SceneGraphNode* getItem(U16 renderBin, U16 index);
    RenderBin*      getBin(RenderBin::RenderBinType rbType);

    inline U16        getRenderQueueBinSize()     { return (U16)_sortedRenderBins.size(); }
    inline RenderBin* getBinSorted(U16 renderBin) { return _sortedRenderBins[renderBin]; }
    inline RenderBin* getBin(U16 renderBin)       { return getBin(_renderBinId[renderBin]); }
    inline bool       isSorted()                  { return _isSorted;}
protected:
    friend class RenderPassManager;
    ///See lock/unlock functions in RenderPassManager for more details
    void lock();
    void unlock(bool resetNodes = false);

private:
    RenderQueue();
    ~RenderQueue();

    RenderBin* getBinForNode(SceneNode* const nodeType);
    RenderBin* getOrCreateBin(const RenderBin::RenderBinType& rbType);

private:
    RenderBinMap _renderBins;
    RenderBinIDType _renderBinId;
    vectorImpl<RenderBin* > _sortedRenderBins;
    bool _renderQueueLocked;
    bool _isSorted;

END_SINGLETON

#endif