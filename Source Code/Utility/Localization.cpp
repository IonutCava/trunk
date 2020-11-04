#include "stdafx.h"

#include "Headers/Localization.h"

#include "Core/Headers/ErrorCodes.h"
#include <SimpleINI/include/SimpleIni.h>

#include "Platform/File/Headers/FileManagement.h"
#include "Platform/File/Headers/FileUpdateMonitor.h"

namespace Divide {
namespace Locale {

namespace detail {
    /// Default language can be set at compile time
    Str64 g_localeFile = {};

    eastl::unique_ptr<LanguageData> g_data = nullptr;

    /// External modification monitoring system
    eastl::unique_ptr<FW::FileWatcher> g_LanguageFileWatcher = nullptr;

    /// Callback for external file changes. 
    UpdateListener g_fileWatcherListener([](std::string_view languageFile, FileUpdateEvent evt) {
        if (evt == FileUpdateEvent::DELETE) {
            return;
        }

        // If we modify our currently active language, reinit the Locale system
        if ((g_localeFile + g_languageFileExtension).c_str() == languageFile) {
            changeLanguage(g_localeFile.c_str());
        }
    });

}; //detail

void LanguageData::setChangeLanguageCallback(const DELEGATE<void, std::string_view /*new language*/>& cbk) {
    _languageChangeCallback = cbk;
}

ErrorCode LanguageData::changeLanguage(std::string_view newLanguage) {
    // Use SimpleIni library for cross-platform INI parsing
    CSimpleIni languageFile(true, false, true);

    detail::g_localeFile = { newLanguage.data(), newLanguage.length() };
    assert(!detail::g_localeFile.empty());

    const ResourcePath file = Paths::g_localisationPath + (detail::g_localeFile + g_languageFileExtension);

    if (languageFile.LoadFile(file.c_str()) != SI_OK) {
        return ErrorCode::NO_LANGUAGE_INI;
    }

    _languageTable.clear();

    // Load all key-value pairs for the "language" section
    const CSimpleIni::TKeyVal* keyValue = languageFile.GetSection("language");

    assert(keyValue != nullptr && "Locale::init error: No 'language' section found");
    // And add all pairs to the language table
    CSimpleIni::TKeyVal::const_iterator keyValuePairIt = keyValue->begin();
    for (; keyValuePairIt != keyValue->end(); ++keyValuePairIt) {
        hashAlg::emplace(_languageTable, _ID(keyValuePairIt->first.pItem), keyValuePairIt->second);
    }

    if (_languageChangeCallback) {
        _languageChangeCallback(newLanguage);
    }

    return ErrorCode::NO_ERR;
}

const char* LanguageData::get(const U64 key, const char* defaultValue) {
    // When we ask for a string for the given key, we check our language cache first
    const auto& entry = _languageTable.find(key);
    if (entry != std::cend(_languageTable)) {
        // Usually, the entire language table is loaded.
        return entry->second.c_str();
    }

    DIVIDE_UNEXPECTED_CALL("Locale error: INVALID STRING KEY!");

    return defaultValue;
}

ErrorCode init(const char* newLanguage) {
    if_constexpr (!Config::Build::IS_SHIPPING_BUILD && Config::ENABLE_LOCALE_FILE_WATCHER) {
        if (!detail::g_LanguageFileWatcher) {
            detail::g_LanguageFileWatcher.reset(new FW::FileWatcher());
            detail::g_fileWatcherListener.addIgnoredEndCharacter('~');
            detail::g_fileWatcherListener.addIgnoredExtension("tmp");
            detail::g_LanguageFileWatcher->addWatch(FW::String((Paths::g_exePath + Paths::g_localisationPath).str()), &detail::g_fileWatcherListener);
        }
    }

    if (!detail::g_data) {
        detail::g_data = eastl::make_unique<LanguageData>();
    }

    return changeLanguage(newLanguage);
}

void clear() noexcept {
    detail::g_data.reset();
}

void idle() {
    if (detail::g_LanguageFileWatcher) {
        detail::g_LanguageFileWatcher->update();
    }
}

/// Although the language can be set at compile time, in-game options may support language changes
ErrorCode changeLanguage(const char* newLanguage) {
    assert(detail::g_data != nullptr);

    /// Set the new language code (override old data)
    return detail::g_data->changeLanguage(newLanguage);
}

const char* get(const U64 key, const char* defaultValue) {
    if (detail::g_data) {
        return detail::g_data->get(key, defaultValue);
    }

    return defaultValue;
}

const char* get(const U64 key) {
    return get(key, "key not found");
}

void setChangeLanguageCallback(const DELEGATE<void, std::string_view /*new language*/>& cbk) {
    assert(detail::g_data);

    detail::g_data->setChangeLanguageCallback(cbk);
}

const Str64& currentLanguage() noexcept {
    return detail::g_localeFile;
}

};  // namespace Locale
};  // namespace Divide
