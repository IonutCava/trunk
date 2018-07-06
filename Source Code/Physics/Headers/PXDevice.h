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

#pragma once
#ifndef _PHYSICS_DEVICE_H_
#define _PHYSICS_DEVICE_H_

#include "Core/Headers/Application.h"
#include "Core/Headers/KernelComponent.h"
#include "Physics/Headers/PhysicsAPIWrapper.h"

namespace Divide {

class PhysicsAsset;
class PXDevice final : public KernelComponent,
                       public PhysicsAPIWrapper {
public:
    enum class PhysicsAPI : U8 {
        PhysX = 0,
        ODE,
        Bullet,
        COUNT
    };

    explicit PXDevice(Kernel& parent);
    ~PXDevice();

    inline void setAPI(PhysicsAPI API) { _API_ID = API; }
    inline PhysicsAPI getAPI() const { return _API_ID; }

    ErrorCode initPhysicsAPI(U8 targetFrameRate, F32 simSpeed) override;
    bool closePhysicsAPI()  override;

    void updateTimeStep(U8 timeStepFactor, F32 simSpeed)  override;
    void update(const U64 deltaTimeUS)  override;
    void process(const U64 deltaTimeUS);
    void idle()  override;
    void setPhysicsScene(PhysicsSceneInterface* const targetScene);

    PhysicsSceneInterface* NewSceneInterface(Scene& scene)  override;

    PhysicsAsset* createRigidActor(const SceneGraphNode& node, RigidBodyComponent& parentComp) override;

    inline PhysicsAPIWrapper& getImpl() { assert(_api != nullptr); return *_api; }
    inline const PhysicsAPIWrapper& getImpl() const { assert(_api != nullptr); return *_api; }

private:
    F32 _simulationSpeed;
    PhysicsAPI _API_ID;
    std::unique_ptr<PhysicsAPIWrapper> _api;

};

};  // namespace Divide

#endif