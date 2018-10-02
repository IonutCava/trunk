#include "stdafx.h"

#include "Headers/DockedWindow.h"

namespace Divide {

    DockedWindow::DockedWindow(Editor& parent, const stringImpl& name)
        : _parent(parent),
          _name(name)
    {
    }

    DockedWindow::~DockedWindow()
    {
    }

}; //namespace Divide