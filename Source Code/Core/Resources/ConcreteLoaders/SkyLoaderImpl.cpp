#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Environment/Sky/Headers/Sky.h"

namespace Divide {

	Sky* ImplResourceLoader<Sky>::operator()() {
		Sky* ptr = New Sky(_descriptor.getName());
        if ( !load( ptr, _descriptor.getName() ) ) {
            MemoryManager::SAFE_DELETE( ptr );
        }

		return ptr;
	}

	DEFAULT_LOADER_IMPL( Sky )

};