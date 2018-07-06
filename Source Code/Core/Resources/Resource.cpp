#include "Headers/Resource.h"

#include "core.h"
#include "Core/Headers/Application.h"

namespace Divide {

	void Resource::refModifyCallback( bool increase ) {
		TrackedObject::refModifyCallback( increase );
	}

}; //namespace Divide