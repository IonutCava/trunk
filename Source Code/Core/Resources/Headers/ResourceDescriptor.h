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
#ifndef RESOURCE_DESCRIPTOR_H_
#define RESOURCE_DESCRIPTOR_H_

#include "Core/Headers/Hashable.h"
#include "Core/Math/Headers/MathVectors.h"

namespace Divide {

/// Dummy class so that resource loaders can have fast access to extra
/// information in a general format
class PropertyDescriptor : public Hashable {
   public:
    enum class DescriptorType : U8 {
        DESCRIPTOR_TEXTURE = 0,
        DESCRIPTOR_PARTICLE,
        DESCRIPTOR_TERRAIN_INFO,
        DESCRIPTOR_SHADER,
        DESCRIPTOR_COUNT
    };

    explicit PropertyDescriptor(const DescriptorType& type) noexcept : _type(type)
    {
    }

    virtual ~PropertyDescriptor()
    {
    }

    virtual size_t getHash() const;

   protected:
    friend class ResourceDescriptor;
    /// Used to clone the property descriptor pointer
    virtual void clone(std::shared_ptr<PropertyDescriptor>& target) const = 0;

   protected:
    /// useful for switch statements
    DescriptorType _type;
};

class CachedResource;
FWD_DECLARE_MANAGED_CLASS(CachedResource);

class ResourceDescriptor : public Hashable {
   public:
    ///resourceName is the name of the resource instance, not an actual asset name! Use "assetName" for that
    explicit ResourceDescriptor(const stringImpl& resourceName);

    ~ResourceDescriptor();

    ResourceDescriptor(const ResourceDescriptor& old);
    ResourceDescriptor& operator=(ResourceDescriptor const& old);
    ResourceDescriptor(ResourceDescriptor&& old) noexcept;

    inline const stringImpl& resourceName() const { return _resourceName; }
    inline void resourceName(const stringImpl& name) { _resourceName = name; }

    template <typename T>
    inline const std::shared_ptr<T> getPropertyDescriptor() const {
        static_assert(std::is_base_of<PropertyDescriptor, T>::value, "Invalid Property Descriptor");
        return std::dynamic_pointer_cast<T>(_propertyDescriptor);
    }

    inline bool hasPropertyDescriptor() const {
        return _propertyDescriptor != nullptr;
    }
    inline const stringImpl& assetName() const { return _assetName; }
    inline const stringImpl& assetLocation() const { return _assetLocation; }

    inline bool getFlag() const { return _flag; }
    inline bool getThreaded() const { return _threaded; }
    inline U32  getEnumValue() const { return _enumValue; }
    inline U32  getID() const { return _ID; }
    inline P32  getMask() const { return _mask; }
    inline const vec3<U16>& getData() const { return _data; }
    inline bool waitForReady() const { return _waitForReady; }
    inline const DELEGATE_CBK<void, CachedResource_wptr>& waitForReadyCbk() const { return _waitForReadyCbk; }

    const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback() const {
        return _onLoadCallback;
    }

    inline void assetLocation(const stringImpl& assetLocation) {
        _assetLocation = assetLocation;
        while (_assetLocation.back() == '/') {
            _assetLocation.pop_back();
        }
    }

    inline void setEnumValue(U32 enumValue) { _enumValue = enumValue; }
    inline void assetName(const stringImpl& name) { _assetName = name; }
    inline void setFlag(bool flag) { _flag = flag; }
    inline void setID(U32 ID) { _ID = ID; }
    inline void setBoolMask(P32 mask) { _mask = mask; }
    inline void setData(const vec3<U16>& data) { _data.set(data); }
    inline void waitForReady(bool state) { _waitForReady = state; }
    inline void waitForReadyCbk(const DELEGATE_CBK<void, CachedResource_wptr>& cbk) { _waitForReadyCbk = cbk; }

    inline void setThreadedLoading(const bool threaded) {
        _threaded = threaded;
    }

    template <typename T>
    inline void setPropertyDescriptor(const T& descriptor) {
        static_assert(std::is_base_of<PropertyDescriptor, T>::value, "Invalid Property Descriptor");
        _propertyDescriptor.reset(new T(descriptor));
    }

    void setOnLoadCallback(const DELEGATE_CBK<void, CachedResource_wptr>& callback) {
        _onLoadCallback = callback;
    }

    size_t getHash() const override;

   private:
    /// Resource instance name (for lookup)
    stringImpl _resourceName;
    /// Asset file name
    stringImpl _assetName;
    /// Asset file location
    stringImpl _assetLocation;
    bool _flag = false;
    bool _threaded = true;
    bool _waitForReady = true;
    DELEGATE_CBK<void, CachedResource_wptr> _waitForReadyCbk;
    U32 _ID = 0;
    /// 4 bool values representing  ... anything ...
    P32 _mask;
    U32 _enumValue = 0;
    /// Use for extra resource properties: textures, samplers, terrain etc.
    std::shared_ptr<PropertyDescriptor> _propertyDescriptor;
    /// Callback to use when the resource finished loading (includes threaded loading)
    DELEGATE_CBK<void, CachedResource_wptr> _onLoadCallback;
    /// general data
    vec3<U16> _data;
};

};  // namespace Divide

#endif
