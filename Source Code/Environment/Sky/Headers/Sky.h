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

#ifndef _SKY_H
#define _SKY_H

#include "Graphs/Headers/SceneNode.h"

namespace Divide {

class RenderStateBlock;

FWD_DECLARE_MANAGED_CLASS(Texture);
FWD_DECLARE_MANAGED_CLASS(Sphere3D);
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);
FWD_DECLARE_MANAGED_CLASS(SceneGraphNode);

enum class RenderStage : U8;

enum class RebuildCommandsState : U8 {
    NONE,
    REQUESTED,
    DONE
};

class Sky : public SceneNode {
   public:
    explicit Sky(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, U32 diameter);
    ~Sky();

    void enableSun(bool state, const FColour3& sunColour, const vec3<F32>& sunVector);
   protected:
    void postLoad(SceneGraphNode& sgn) override;

    void buildDrawCommands(SceneGraphNode& sgn,
                                RenderStagePass renderStagePass,
                                RenderPackage& pkgInOut) override;

    bool preRender(SceneGraphNode& sgn,
                   const Camera& camera,
                   RenderStagePass renderStagePass,
                   bool refreshData,
                   bool& rebuildCommandsOut) override;

    void sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) override;

   protected:
    template <typename T>
    friend class ImplResourceLoader;

    bool load() override;

    const char* getResourceTypeName() const override { return "Sky"; }

  private:
    bool _enableSun;
    FColour3 _sunColour;
    vec3<F32>_sunVector;
    RebuildCommandsState _rebuildDrawCommands;
    U32       _diameter;
    GFXDevice& _context;
    Texture_ptr  _skybox;
    Sphere3D_ptr _sky;
    ShaderProgram_ptr _skyShader;
    ShaderProgram_ptr _skyShaderPrePass;
    size_t _skyboxRenderStateHash;
    size_t _skyboxRenderStateHashPrePass;

    size_t _skyboxRenderStateReflectedHash;
    size_t _skyboxRenderStateReflectedHashPrePass;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Sky);

};  // namespace Divide

#endif
