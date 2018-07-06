/*
   Copyright (c) 2015 DIVIDE-Studio
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

namespace Divide {

class Resource;
class HardwareResource;
class ResourceLoader : private NonCopyable {
   public:
    ResourceLoader(const ResourceDescriptor& descriptor)
        : _descriptor(descriptor) {}

    virtual Resource* operator()() = 0;

   protected:
    ResourceDescriptor _descriptor;
};

template <typename ResourceType>
class ImplResourceLoader : public ResourceLoader {
   public:
    ImplResourceLoader(const ResourceDescriptor& descriptor)
        : ResourceLoader(descriptor) {}

    ResourceType* operator()();

   protected:
    virtual bool load(ResourceType* const res, const stringImpl& name);
};

#define DEFAULT_LOADER_IMPL(X)                                               \
    template <>                                                              \
    bool ImplResourceLoader<X>::load(X* const res, const stringImpl& name) { \
        res->setState(RES_LOADING);                                          \
        return ResourceCache::getInstance().load(res, name);                 \
    }

#define DEFAULT_HW_LOADER_IMPL(X)                                            \
    template <>                                                              \
    bool ImplResourceLoader<X>::load(X* const res, const stringImpl& name) { \
        res->setState(RES_LOADING);                                          \
        return ResourceCache::getInstance().loadHW(res, name);               \
    }
};  // namespace Divide

#endif
