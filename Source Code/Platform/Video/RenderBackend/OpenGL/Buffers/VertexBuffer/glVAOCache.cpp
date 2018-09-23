#include "stdafx.h"

#include "Headers/glVAOCache.h"
#include "Utility/Headers/Localization.h"

#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

namespace Divide {
namespace GLUtil {

glVAOCache::glVAOCache() noexcept
{
}

glVAOCache::~glVAOCache()
{
    clear();
}

void glVAOCache::clear() {
    for (VAOMap::value_type& value : _cache) {
        if (value.second != 0) {
            Divide::GL_API::s_vaoPool.deallocate(value.second);
        }
    }
    _cache.clear();
}

// Returns an existing or a new VAO that matches the specified attribute specification
bool glVAOCache::getVAO(const AttribFlags& flags, GLuint& vaoOut) {
    size_t hash = 0;
    return getVAO(flags, vaoOut, hash);
}

bool glVAOCache::getVAO(const AttribFlags& flags, GLuint& vaoOut, size_t& hashOut) {
    vaoOut = 0;

    // Hash the received attributes
    hashOut = std::hash<AttribFlags>()(flags);

    // See if we already have a matching VAO
    VAOMap::iterator it = _cache.find(hashOut);
    if (it != std::cend(_cache)) {
        // Remember it if we do
        vaoOut = it->second;
        // Return true on a cache hit;
        return true;
    }

    // Otherwise allocate a new VAO and save it in the cache
    vaoOut = Divide::GL_API::s_vaoPool.allocate();
    assert(vaoOut != 0 && Locale::get(_ID("ERROR_VAO_INIT")));
    hashAlg::insert(_cache, hashOut, vaoOut);
    return false;
}

}; //namespace GLUtil
}; //namespace Divide