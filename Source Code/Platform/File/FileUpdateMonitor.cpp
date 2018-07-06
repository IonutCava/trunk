#include "stdafx.h"

#include "Headers/FileUpdateMonitor.h"
#include "Platform/File/Headers/FileManagement.h"

namespace Divide {

UpdateListener::UpdateListener(const FileUpdateCbk& cbk)
    : _cbk(cbk)
{
}

void UpdateListener::addIgnoredExtension(const char* extension) {
    _ignoredExtensions.emplace_back(extension);
}

void UpdateListener::addIgnoredEndCharacter(char character) {
    _ignoredEndingCharacters.emplace_back(character);
}

void UpdateListener::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
{
    // We can ignore files that end in a specific character. Many text editors, for examnple, append a '~' at the end of temp files
    if (!_ignoredEndingCharacters.empty()) {
        if (std::find_if(std::cbegin(_ignoredEndingCharacters),
            std::cend(_ignoredEndingCharacters),
            [filename](char character) {
            return std::tolower(filename.back()) == std::tolower(character);
        }) != std::cend(_ignoredEndingCharacters)) {
            return;
        }
    }

    // We can specify a list of extensions to ignore for a specific listener to avoid, for example, parsing temporary OS files
    if (!_ignoredExtensions.empty()) {
        if (std::find_if(std::cbegin(_ignoredExtensions),
            std::cend(_ignoredExtensions),
            [filename](const stringImpl& extension) {
            return hasExtension(filename, extension);
        }) != std::cend(_ignoredExtensions)) {
            return;
        }
    }
    if (_cbk) {
        FileUpdateEvent evt = FileUpdateEvent::COUNT;
        switch (action)
        {
        case FW::Actions::Add: evt = FileUpdateEvent::ADD; break;
        case FW::Actions::Delete: evt = FileUpdateEvent::DELETE; break;
        case FW::Actions::Modified: evt = FileUpdateEvent::MODIFY; break;
        default: DIVIDE_UNEXPECTED_CALL("Unknown file event!");
        }

        _cbk(filename.c_str(), evt);
    }
}

}; //namespace Divide;