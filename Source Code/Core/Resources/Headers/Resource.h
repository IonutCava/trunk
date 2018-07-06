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

#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#include "Core/Math/Headers/MathMatrices.h"
#include "Core/MemoryManagement/Headers/TrackedObject.h"
#include <mutex>

namespace Divide {

/// When "CreateResource" is called, the resource is in "RES_UNKNOWN" state.
/// Once it has been instantiated it will move to the "RES_CREATED" state.
/// Calling "load" on a non-created resource will fail.
/// After "load" is called, the resource is move to the "RES_LOADING" state
/// Nothing can be done to the resource when it's in "RES_LOADING" state!
/// Once loading is complete, preferably in another thread,
/// the resource state will become "RES_LOADED" and ready to be used (e.g. added
/// to the SceneGraph)
/// Calling "unload" is only available for "RES_LOADED" state resources.
/// Calling this method will set the state to "RES_LOADING"
/// Once unloading is complete, the resource will become "RES_CREATED".
/// It will still exist, but won't contain any data.
/// RES_UNKNOWN and RES_CREATED are safe to delete

enum class ResourceState : U32 {
    RES_UNKNOWN = 0,  //<The resource exists, but it's state is undefined
    RES_CREATED = 1,  //<The pointer has been created and instantiated, but no
                      //data has been loaded
    RES_LOADING = 2,  //<The resource is loading or unloading, creating or
                      //deleting data, parsing scripts, etc
    RES_LOADED  = 3,  //<The resource is loaded and available
    RES_SPECIAL = 4,  //<The resource was not loaded via the resource manager (e.g. SceneRoot or SceneTransform)
    COUNT
};

class NOINITVTABLE Resource : public TrackedObject {
    friend class ResourceCache;
    friend class ResourceLoader;
    template <typename X>
    friend class ImplResourceLoader;

   public:
    explicit Resource(const stringImpl& name);
    explicit Resource(const stringImpl& name,
                      const stringImpl& resourceLocation);

    virtual ~Resource();

    /// Loading and unloading interface
    virtual bool load();

    virtual bool unload();

    /// Name management
    const stringImpl& getName() const;

    /// Physical file location
    const stringImpl& getResourceLocation() const;

    ResourceState getState() const;

    void setStateCallback(ResourceState targetState, DELEGATE_CBK<void> cbk);

   protected:
    void setState(ResourceState currentState);

   protected:
    stringImpl _name;
    stringImpl _resourceLocation;  ///< Physical file location
    std::atomic<ResourceState> _resourceState;
    std::mutex _callbackLock;
    std::array<DELEGATE_CBK<void>, to_const_uint(ResourceState::COUNT)> _loadingCallbacks;
};

enum class GeometryType : U32 {
    VEGETATION,  ///< For special rendering subroutines
    PRIMITIVE,   ///< Simple objects: Boxes, Spheres etc
    GEOMETRY     ///< All other forms of geometry
};

struct FileData {
    stringImpl ItemName;
    stringImpl ModelName;
    vec3<F32> scale;
    vec3<F32> position;
    vec3<F32> orientation;
    vec3<F32> color;
    GeometryType type;
    F32 data;  ///< general purpose
    stringImpl data2;
    stringImpl data3;
    F32 version;
    bool castsShadows;
    bool receivesShadows;
    /// Used to determine if it's a static object or dynamic. Affects lighting,
    /// navigation, etc.
    bool staticUsage;
    /// Used to determine if the object should be added to the nav mesh
    /// generation process or not
    bool navigationUsage;
    /// Used to determine if the object should be added to physics simulations
    bool physicsUsage;
    /// If physicsUsage is true, this determines if the node can be pushed
    /// around by other actors
    /// or if it is a static(fixed in space) actor
    bool physicsPushable;
    /// Used to force a geometry level parsing for nav mesh creation instead of
    /// the default
    /// bounding-box level
    bool useHighDetailNavMesh;
};

struct TerrainInfo {
    TerrainInfo() { position.set(0, 0, 0); }
    /// "variables" contains the various strings needed for each terrain such as
    /// texture names,
    /// terrain name etc.
    hashMapImpl<ULL, stringImpl> variables;
    F32 grassDensity;
    F32 treeDensity;
    F32 grassScale;
    F32 treeScale;
    vec3<F32> position;
    vec2<F32> scale;
    bool active;
};

};  // namespace Divide

#endif