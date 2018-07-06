#include "Headers/glResources.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"

namespace Divide {
    namespace GLUtil {
        /*-----------Object Management----*/
        GLuint _invalidObjectID = std::numeric_limits<U32>::max();

        /*-----------Context Management----*/
        bool _applicationClosing = false;
        bool _contextAvailable = false;
        GLFWwindow* _mainWindow     = nullptr;
        GLFWwindow* _loaderWindow   = nullptr;

        /*----------- GLU overrides ------*/
    }//namespace GLUtil
}// namespace Divide