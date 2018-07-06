#include "Headers/ResourceDescriptor.h"

namespace Divide {

ResourceDescriptor::ResourceDescriptor(const stringImpl& resourceName)
    : _propertyDescriptor(nullptr),
      _name(resourceName),
      _flag(false),
      _ID(0),
      _enumValue(0),
      _userPtr(nullptr)
{
    _mask.i = 0;
    _threaded = true;
}

ResourceDescriptor::~ResourceDescriptor()
{
    MemoryManager::DELETE(_propertyDescriptor);
}

ResourceDescriptor::ResourceDescriptor(const ResourceDescriptor& old)
    : _propertyDescriptor(nullptr)
{
      _name = old._name;
      _resourceName = old._resourceName;
      _resourceLocation = old._resourceLocation;
      _properties = old._properties;
      _flag = old._flag;
      _threaded = old._threaded;
      _ID = old._ID;
      _mask = old._mask;
      _enumValue = old._enumValue;
      _userPtr = old._userPtr;
      if (old._propertyDescriptor != nullptr) {
          _propertyDescriptor = old._propertyDescriptor->clone();
      }
}

ResourceDescriptor& ResourceDescriptor::operator=(
    ResourceDescriptor const& old) {
    if (this != &old) {
        _name = old._name;
        _resourceName = old._resourceName;
        _resourceLocation = old._resourceLocation;
        _properties = old._properties;
        _flag = old._flag;
        _threaded = old._threaded;
        _ID = old._ID;
        _mask = old._mask;
        _enumValue = old._enumValue;
        _userPtr = old._userPtr;
        if (old._propertyDescriptor != nullptr) {
            MemoryManager::SAFE_UPDATE(_propertyDescriptor,
                                       old._propertyDescriptor->clone());
        }
    }

    return *this;
}
};