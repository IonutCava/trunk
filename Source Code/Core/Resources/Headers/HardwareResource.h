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


#ifndef _HW_RESOURCE_H_
#define _HW_RESOURCE_H_

#include "Resource.h"
///Hardware resources are resource elements that need a
///singlethreaded hardware initialization (textures, shaders, etc)
class HardwareResource : public Resource {
public:
	HardwareResource() : Resource(),
						 _hardwareInitComplete(false)
	{
	}

	HardwareResource(const std::string& name) : Resource(name),
												_hardwareInitComplete(false)
	{
	}

	virtual ~HardwareResource(){}

	inline bool isHWInitComplete() const {return _hardwareInitComplete;}
	///Use this as a callback for multi-threaded loading.
	///Generate hardware elements (buffers, textures, shaders etc. after joining main loading thread.
	virtual bool generateHWResource(const std::string& name)  {_hardwareInitComplete = true; return true;}

private:
	boost::atomic_bool _hardwareInitComplete;
};
#endif
