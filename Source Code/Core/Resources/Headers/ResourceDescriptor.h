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

#ifndef RESOURCE_DESCRIPTOR_H_
#define RESOURCE_DESCRIPTOR_H_

#include "core.h"
#include <boost/shared_ptr.hpp>

namespace Divide {

///Dummy class so that resource loaders can have fast access to extra information in a general format
class PropertyDescriptor {
public:

    enum DescriptorType{
        DESCRIPTOR_TEXTURE = 0,
        DESCRIPTOR_SAMPLER = 1,
		DESCRIPTOR_PARTICLE = 2
    };

    explicit PropertyDescriptor(const DescriptorType& type) : _type(type)
	{
	}

protected:
    friend class ResourceDescriptor;
    ///Initialize descriptor values to their safe defaults
    virtual void setDefaultValues() = 0;
    ///Used to clone the property descriptor pointer
    virtual PropertyDescriptor* clone() const = 0;

protected:
    ///usefull for switch statements
    DescriptorType _type;
};

class ResourceDescriptor{
public:
    explicit ResourceDescriptor(const stringImpl& name = "default",
                                const stringImpl& resourceLocation = "default",
                                bool flag = false, U32 id = 0, U8 enumValue = 0);

    virtual ~ResourceDescriptor();

    ResourceDescriptor(const ResourceDescriptor& old);
    ResourceDescriptor& operator= (ResourceDescriptor const& old);

    inline const  stringImpl&         getPropertyListString()  const {return _properties;}
    inline const  stringImpl&         getResourceLocation()    const {return _resourceLocation;}
    inline const  stringImpl&         getName()			       const {return _name;}

    template<typename T>
    inline const T* getPropertyDescriptor() const {
        return dynamic_cast<T*>(_propertyDescriptor); 
    }

    inline bool hasPropertyDescriptor()         const {return _propertyDescriptor != nullptr;}
    inline bool getFlag()					    const {return _flag;}
    inline bool getThreaded()                   const {return _threaded;}
    inline U8   getEnumValue()                  const {return _enumValue;}
    inline U32  getId()						    const {return _id;}
    inline P32  getMask()                       const {return _mask;}

    inline void setPropertyList(const stringImpl& propertyListString)    {_properties = propertyListString;}
    inline void setResourceLocation(const stringImpl& resourceLocation)  {_resourceLocation = resourceLocation;}
    inline void setEnumValue(U8 enumValue)                               {_enumValue = enumValue;}
    inline void setName(const stringImpl& name)					         {_name = name;}
    inline void setFlag(bool flag)				                         {_flag = flag;}
    inline void setId(U32 id)					                         {_id = id;}
    inline void setBoolMask(P32 mask)                                    {_mask = mask;}
    inline void setThreadedLoading(const bool threaded)                  {_threaded = threaded;}

    template<typename T>
    inline void setPropertyDescriptor(const T& descriptor) {
		MemoryManager::SAFE_UPDATE(_propertyDescriptor, New T(descriptor));
    }

private:
    stringImpl _name;			   ///< Item name
    stringImpl _resourceLocation; ///< Physical file location
    stringImpl _properties;       ///< Comma separated list of properties
    bool        _flag;
    bool        _threaded;
    U32         _id;
    P32         _mask;             ///<4 bool values representing  ... anything ...
    U8          _enumValue;

    PropertyDescriptor* _propertyDescriptor; ///< Use for extra resource properties: textures, samplers, terrain etc.
};

}; //namespace Divide

#endif