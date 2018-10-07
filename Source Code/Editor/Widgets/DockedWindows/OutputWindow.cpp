#include "stdafx.h"

#include "Headers/OutputWindow.h"
#include "Editor/Headers/Editor.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {
    OutputWindow::OutputWindow(Editor& parent, const Descriptor& descriptor)
        : DockedWindow(parent, descriptor)
    {

    }

    OutputWindow::~OutputWindow()
    {

    }

    void OutputWindow::drawInternal() {
        // Draw Output Window
        Attorney::EditorOutputWindow::drawOutputWindow(_parent.context().editor());
    }
};