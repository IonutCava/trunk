#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Environment/Sky/Headers/Sky.h"

namespace Divide {

    Sky* ImplResourceLoader<Sky>::operator()() {
        Sky* ptr = MemoryManager_NEW Sky(_descriptor.getName());
        if ( !load( ptr, _descriptor.getName() ) ) {
            MemoryManager::DELETE( ptr );
        }

        return ptr;
    }

    DEFAULT_LOADER_IMPL( Sky )

};