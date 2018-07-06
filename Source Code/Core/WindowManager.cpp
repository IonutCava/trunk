#include "Headers/WindowManager.h"

namespace Divide {

WindowManager::WindowManager()
    : _hasFocus(true),
      _minimized(false),
      _displayIndex(0),
      _activeWindowType(WindowType::WINDOW)
{
}

}; //namespace Divide