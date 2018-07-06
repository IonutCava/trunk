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

#ifndef RESOURCE_LOADER_H_
#define RESOURCE_LOADER_H_

#include "ResourceDescriptor.h"

class Resource;
class ResourceLoader : private boost::noncopyable {

public:
	ResourceLoader(const ResourceDescriptor& descriptor) : _descriptor(descriptor) {}

	virtual Resource* operator()() = 0;

protected:
	ResourceDescriptor _descriptor;
};

template<typename ResourceType>
class ImplResourceLoader : public ResourceLoader {

public:
	ImplResourceLoader(const ResourceDescriptor& descriptor)  : ResourceLoader(descriptor) {}
	ResourceType* operator()();

protected:
	virtual bool load(ResourceType* const res, const std::string& name);
};

#define DEFAULT_LOADER_IMPL(X)	template<> \
								bool ImplResourceLoader<X>::load(X* const res, const std::string& name){ \
									return res->setInitialData(name);\
								}

#define DEFAULT_HW_LOADER_IMPL(X) template<> \
								  bool ImplResourceLoader<X>::load(X* const res, const std::string& name){ \
									if(res->setInitialData(name)){\
										return res->generateHWResource(name); \
									} \
									return false; \
								  }
#endif