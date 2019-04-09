#include "stdafx.h"

#include "Headers/ResourceDescriptor.h"

#include "Core/Math/Headers/MathHelper.h"

namespace Divide {


size_t PropertyDescriptor::getHash() const {
    Util::Hash_combine(_hash, to_U32(_type));
    return _hash;
}

ResourceDescriptor::ResourceDescriptor(const stringImpl& resourceName)
    : _propertyDescriptor(nullptr),
      _resourceName(resourceName),
      _assetName(resourceName),
       _waitForReady(true),
      _flag(false),
      _ID(0),
      _enumValue(0),
      _data(0u)
{
    _mask.i = 0;
    _threaded = true;
}

ResourceDescriptor::~ResourceDescriptor()
{
}

ResourceDescriptor::ResourceDescriptor(const ResourceDescriptor& old)
    : _propertyDescriptor(nullptr),
      _assetName(old._assetName),
      _assetLocation(old._assetLocation),
      _resourceName(old._resourceName),
      _flag(old._flag),
      _threaded(old._threaded),
      _ID(old._ID),
      _mask(old._mask),
      _enumValue(old._enumValue),
      _data(old._data),
      _onLoadCallback(old._onLoadCallback)
{
    if (old._propertyDescriptor != nullptr) {
        old._propertyDescriptor->clone(_propertyDescriptor);
    }
}

ResourceDescriptor& ResourceDescriptor::operator=(ResourceDescriptor const& old) {
    if (this != &old) {
        _assetName = old._assetName;
        _resourceName = old._resourceName;
        _assetLocation = old._assetLocation;
        _flag = old._flag;
        _threaded = old._threaded;
        _ID = old._ID;
        _mask = old._mask;
        _enumValue = old._enumValue;
        _data = old._data;
        _onLoadCallback = old._onLoadCallback;
        if (old._propertyDescriptor != nullptr) {
            old._propertyDescriptor->clone(_propertyDescriptor);
        }
    }

    return *this;
}

ResourceDescriptor::ResourceDescriptor(ResourceDescriptor&& old) noexcept 
    :  _assetName(std::move(old._assetName)),
       _resourceName(std::move(old._resourceName)),
       _assetLocation(std::move(old._assetLocation)),
       _flag(std::move(old._flag)),
       _threaded(std::move(old._threaded)),
       _ID(std::move(old._ID)),
       _mask(std::move(old._mask)),
       _enumValue(std::move(old._enumValue)),
       _data(std::move(old._data)),
       _onLoadCallback(std::move(old._onLoadCallback)),
       _propertyDescriptor(std::move(old._propertyDescriptor))
{
    old._propertyDescriptor = nullptr;
}

size_t ResourceDescriptor::getHash() const {
    _hash = 31;
    stringImpl file = _assetLocation + "/" + _assetName;
    Util::ReplaceStringInPlace(file, "//", "/");
    Util::Hash_combine(_hash, file);
    Util::Hash_combine(_hash, _flag);
    Util::Hash_combine(_hash, _ID);
    Util::Hash_combine(_hash, _mask.i);
    Util::Hash_combine(_hash, _enumValue);
    Util::Hash_combine(_hash, _data.x);
    Util::Hash_combine(_hash, _data.y);
    Util::Hash_combine(_hash, _data.z);
    if (_propertyDescriptor) {
        Util::Hash_combine(_hash, _propertyDescriptor->getHash());
    }

    return _hash;
}

};