/*
   Copyright (c) 2013 DIVIDE-Studio
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

class ResourceDescriptor{
public:
	ResourceDescriptor(const std::string& name = "default",
					   const std::string& resourceLocation = "default",
					   bool flag = false, U32 id = RAND_MAX, U8 enumValue = -1) : _name(name),
															  _resourceLocation(resourceLocation),
															  _flag(flag),
															  _id(id),
															  _enumValue(enumValue)
    {
        _mask.i = 0;
		_threaded = true;
    }

	const  std::string& getPropertyListString() const {return _properties;}
	const  std::string& getResourceLocation()   const {return _resourceLocation;}
	const  std::string& getName()			    const {return _name;}
	inline bool getFlag()					    const {return _flag;}
	inline bool getThreaded()                   const {return _threaded;}
	inline U8   getEnumValue()                  const {return _enumValue;}
	inline U32  getId()						    const {return _id;}
    inline P32  getMask()                       const {return _mask;}

	inline void setPropertyList(const std::string& propertyListString)    {_properties = propertyListString;}
	inline void setResourceLocation(const std::string& resourceLocation)  {_resourceLocation = resourceLocation;}
	inline void setEnumValue(U8 enumValue)                                {_enumValue = enumValue;}
	inline void setName(const std::string& name)					      {_name = name;}
	inline void setFlag(bool flag)				                          {_flag = flag;}
	inline void setId(U32 id)					                          {_id = id;}
    inline void setBoolMask(P32 mask)                                     {_mask = mask;}
	inline void setThreadedLoading(const bool threaded)                   {_threaded = threaded;}

private:
	std::string _name;			   ///< Item name
	std::string _resourceLocation; ///< Physical file location
	std::string _properties;       ///< Comma separated list of properties
	bool        _flag;
	bool        _threaded;
	U32         _id;
    P32         _mask;             ///<4 bool values representing  ... anything ...
	U8          _enumValue;
};

#endif