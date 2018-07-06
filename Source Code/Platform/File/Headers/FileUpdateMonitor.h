/*
Copyright (c) 2017 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _PLATFORM_FILE_FILE_UPDATE_MONITOR_H_
#define _PLATFORM_FILE_FILE_UPDATE_MONITOR_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

enum class FileUpdateEvent : U8 {
    ADD = 0,
    DELETE,
    MODIFY,
    COUNT
};

typedef DELEGATE_CBK<void, const char* /*file*/, FileUpdateEvent> FileUpdateCbk;

class UpdateListener : public FW::FileWatchListener
{
public:
    UpdateListener(const FileUpdateCbk& cbk)
        : _cbk(cbk)
    {
    }

    void addIgnoredExtension(const char* extension) {
        _ignoredExtensions.emplace_back(extension);
    }

    void addIgnoredEndCharacter(char character) {
        _ignoredEndingCharacters.emplace_back(character);
    }

    void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
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
                             [filename](const stringImpl& extension){
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
private:
    FileUpdateCbk _cbk;
    vectorImpl<char> _ignoredEndingCharacters;
    vectorImpl<stringImpl> _ignoredExtensions;

};

}; //namespace Divide

#endif //_PLATFORM_FILE_FILE_UPDATE_MONITOR_H_
