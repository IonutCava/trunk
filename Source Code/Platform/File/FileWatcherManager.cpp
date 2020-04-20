#include "stdafx.h"

#include "Headers/FileWatcherManager.h"

namespace Divide {
    vectorEASTL<std::unique_ptr<FileWatcher>> FileWatcherManager::s_fileWatchers;

    FileWatcher& FileWatcherManager::allocateWatcher() {
        s_fileWatchers.emplace_back(std::make_unique<FileWatcher>());
        return *s_fileWatchers.back();
    }

    void FileWatcherManager::deallocateWatcher(const FileWatcher& fw) {
        deallocateWatcher(fw.getGUID());
    }

    void FileWatcherManager::deallocateWatcher(I64 fileWatcherGUID) {
        s_fileWatchers.erase(
            std::remove_if(std::begin(s_fileWatchers), std::end(s_fileWatchers),
                           [&fileWatcherGUID](const std::unique_ptr<FileWatcher>& it) noexcept
                           -> bool { return it->getGUID() == fileWatcherGUID; }),
            std::end(s_fileWatchers));
    }

    void FileWatcherManager::idle() {
        // Expensive: update just one per frame
        for (const std::unique_ptr<FileWatcher>& fw : s_fileWatchers) {
            if (!fw->_updated) {
                fw->_impl.update();
                fw->_updated = true;
                return;
            }
        }

        // If we got here, we updated all of our watchers at least once
        for (const std::unique_ptr<FileWatcher>& fw : s_fileWatchers) {
            fw->_updated = false;
        }
    }
}; //namespace Divide