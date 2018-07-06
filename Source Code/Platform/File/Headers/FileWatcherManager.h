/*
Copyright (c) 2018 DIVIDE-Studio
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

#ifndef _PLATFORM_FILE_FILE_WATCHER_MANAGER_H_
#define _PLATFORM_FILE_FILE_WATCHER_MANAGER_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {
    class FileWatcher : public GUIDWrapper {
      public:
        FW::FileWatcher& operator()() {
            return _impl;
        }

      protected:
        bool _updated = false;
        friend class FileWatcherManager;
        FW::FileWatcher _impl;
    };

    class FileWatcherManager {
      public:
        static void idle();
        static FileWatcher& allocateWatcher();
        static void deallocateWatcher(I64 fileWatcherGUID);
        static void deallocateWatcher(const FileWatcher& fw);

      private:
        static vectorImpl<std::unique_ptr<FileWatcher>> s_fileWatchers;
    }; //FileWatcherManager
}; //namespace Divide

#endif //_PLATFORM_FILE_FILE_WATCHER_MANAGER_H_