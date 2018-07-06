/*
   Copyright (c) 2016 DIVIDE-Studio
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

#include "Core/Headers/Application.h"

#ifndef _PHYSICS_DEVICE_H_
#define _PHYSICS_DEVICE_H_
#include "Physics/PhysX/Headers/PhysX.h"

#ifndef _PHYSICS_API_FOUND_
#error "No physics library implemented!"
#endif

namespace Divide {

class PhysicsAsset;
DEFINE_SINGLETON_EXT1_W_SPECIFIER(PXDevice, PhysicsAPIWrapper, final)

  public:
    enum class PhysicsAPI : U32 {
        PhysX = 0,
        ODE,
        Bullet,
        COUNT
    };

    inline void setAPI(PhysicsAPI API) { _API_ID = API; }
    inline PhysicsAPI getAPI() const { return _API_ID; }

    ErrorCode initPhysicsAPI(U8 targetFrameRate);
    bool closePhysicsAPI();

    void updateTimeStep(U8 timeStepFactor);
    void update(const U64 deltaTime);
    void process(const U64 deltaTime);
    void idle();
    void setPhysicsScene(PhysicsSceneInterface* const targetScene);
    void initScene();

    PhysicsSceneInterface* NewSceneInterface(Scene& scene);

    PhysicsAsset* createRigidActor(const SceneGraphNode& node) override;

  private:
    PXDevice();
    ~PXDevice();

  private:
    PhysicsAPI _API_ID;
    PhysicsAPIWrapper* _api;

END_SINGLETON

#define PHYSICS_DEVICE PXDevice::instance()

};  // namespace Divide

#endif