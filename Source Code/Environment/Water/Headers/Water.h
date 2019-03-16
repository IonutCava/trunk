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

#ifndef _WATER_PLANE_H_
#define _WATER_PLANE_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

#include "ECS/Components/Headers/RenderingComponent.h"

namespace Divide {

class Texture;
class ShaderProgram;

class WaterPlane : public SceneNode {
   public:
    explicit WaterPlane(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name);
    ~WaterPlane();

    bool pointUnderwater(const SceneGraphNode& sgn, const vec3<F32>& point);

    inline const std::shared_ptr<Quad3D>& getQuad() const { return _plane; }

    void updatePlaneEquation(const SceneGraphNode& sgn,
                             Plane<F32>& plane,
                             bool reflection);

    // width, length, depth
    const vec3<U16>& getDimensions() const;

    void saveToXML(boost::property_tree::ptree& pt) const override;
    void loadFromXML(const boost::property_tree::ptree& pt)  override;

   protected:
    void buildDrawCommands(SceneGraphNode& sgn,
                           RenderStagePass renderStagePass,
                           RenderPackage& pkgInOut) override;

    void postLoad(SceneGraphNode& sgn) override;

   protected:
    template <typename T>
    friend class ImplResourceLoader;

    bool load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) override;

   private:
    void updateReflection(RenderCbkParams& renderParams, GFX::CommandBuffer& bufferInOut);
    void updateRefraction(RenderCbkParams& renderParams, GFX::CommandBuffer& bufferInOut);

   private:
    vec3<U16> _dimensions;
    /// the water's "geometry"
    std::shared_ptr<Quad3D> _plane;

    Camera* _reflectionCam;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(WaterPlane);

};  // namespace Divide

#endif