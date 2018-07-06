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

#ifndef _HW_RESOURCE_H_
#define _HW_RESOURCE_H_

#include "Resource.h"
///Hardware resources are resource elements that need a 
///singlethreaded hardware initialization (textures, shaders, etc)
class HardwareResource : public Resource {
public:
	HardwareResource() : Resource(), _hardwareInitComplete(false) {}
	HardwareResource(const std::string& name) : Resource(name), _hardwareInitComplete(false) {}
	virtual ~HardwareResource(){}
	inline bool isHWInitComplete() {return _hardwareInitComplete;}

protected:
	template<typename T>
	friend class ImplResourceLoader;
	///Use this as a callback for multi-threaded loading. 
	///Generate hardware elements (buffers, textures, shaders etc. after joining main loading thread.
	virtual bool generateHWResource(const std::string& name) {_hardwareInitComplete = true; return true;}

private:
	bool _hardwareInitComplete;
};
#endif
