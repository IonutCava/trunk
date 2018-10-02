#include "stdafx.h"

#include "Headers/OutputWindow.h"
#include "Editor/Headers/Editor.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {
    OutputWindow::OutputWindow(Editor& parent)
        : DockedWindow(parent, "Output")
    {

    }

    OutputWindow::~OutputWindow()
    {

    }

    void OutputWindow::draw() {
        // Draw Output Window
        Attorney::EditorOutputWindow::drawOutputWindow(_parent.context().editor());
    }
};