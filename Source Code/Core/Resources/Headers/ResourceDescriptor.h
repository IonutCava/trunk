/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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
															  _enumValue(enumValue){}

	const  std::string& getPropertyListString() const {return _properties;}
	const  std::string& getResourceLocation()   const {return _resourceLocation;}
	const  std::string& getName()			    const {return _name;}
	inline bool getFlag()					    const {return _flag;}
	inline U8   getEnumValue()                  const {return _enumValue;}
	inline U32  getId()						    const {return _id;}

	inline void setPropertyList(const std::string& propertyListString)    {_properties = propertyListString;}
	inline void setResourceLocation(const std::string& resourceLocation)  {_resourceLocation = resourceLocation;}
	inline void setEnumValue(U8 enumValue)                                {_enumValue = enumValue;}
	inline void setName(const std::string& name)					      {_name = name;}
	inline void setFlag(bool flag)				                          {_flag = flag;}
	inline void setId(U32 id)					                          {_id = id;}

private:
	std::string _name;			   ///< Item name
	std::string _resourceLocation; ///< Physical file location
	std::string _properties;       ///< Comma separated list of properties
	bool        _flag;
	U32         _id;
	U8          _enumValue;
};

#endif