#include "stdafx.h"

#include "Headers/DockedWindow.h"
#include "Headers/PanelManager.h"

namespace Divide {

    DockedWindow::DockedWindow(PanelManager& parent, const stringImpl& name)
        : _parent(parent),
          _name(name)
    {
    }

    DockedWindow::~DockedWindow()
    {
    }

}; //namespace Divide