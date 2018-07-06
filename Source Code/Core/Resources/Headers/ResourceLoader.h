/*
   Copyright (c) 2017 DIVIDE-Studio
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

#ifndef RESOURCE_LOADER_H_
#define RESOURCE_LOADER_H_

#include "ResourceDescriptor.h"
#include "Resource.h"

namespace Divide {

class Resource;
// Used to delete resources
struct DeleteResource {
    DeleteResource(ResourceCache& context)
        : _context(context)
    {
    }

    void operator()(Resource* res);

    ResourceCache& _context;
};

class PlatformContext;
class NOINITVTABLE ResourceLoader : private NonCopyable {
   public:
    ResourceLoader(ResourceCache& cache, PlatformContext& context, const ResourceDescriptor& descriptor)
        : _cache(cache),
          _context(context),
          _descriptor(descriptor) 
    {
    }

    virtual Resource_ptr operator()() = 0;

   protected:
    ResourceCache& _cache;
    PlatformContext& _context;
    ResourceDescriptor _descriptor;
};

template <typename ResourceType>
class ImplResourceLoader : public ResourceLoader {
   public:
    ImplResourceLoader(ResourceCache& cache, PlatformContext& context, const ResourceDescriptor& descriptor)
        : ResourceLoader(cache, context, descriptor)
    {
    }

    Resource_ptr operator()();

   protected:

    bool load(std::shared_ptr<ResourceType> res, const DELEGATE_CBK<void, Resource_ptr>& onLoadCallback) {
        if (!res) {
            return false;
        }

        res->setState(ResourceState::RES_LOADING);
        return res->load(onLoadCallback);
    }
};

};  // namespace Divide

#endif
