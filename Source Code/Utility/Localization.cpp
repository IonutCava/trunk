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
    stringImpl g_localeFile;

    std::unique_ptr<LanguageData> g_data = nullptr;

    /// External modification monitoring system
    std::unique_ptr<FW::FileWatcher> g_LanguageFileWatcher = nullptr;

    /// Callback for external file changes. 
    UpdateListener g_fileWatcherListener([](const char* languageFile, FileUpdateEvent evt) {
        if (evt == FileUpdateEvent::DELETE) {
            return;
        }

        // If we modify our currently active language, reinit the Locale system
        if (strcmp((g_localeFile + g_languageFileExtension).c_str(), languageFile) == 0) {
            changeLanguage(g_localeFile.c_str());
        }
    });

}; //detail

LanguageData::LanguageData()
{
}

LanguageData::~LanguageData()
{
}

void LanguageData::changeLanguage(const char* newLanguage) {
    _languageTable.clear();

    for (const DELEGATE_CBK<void, const char* /*new language*/>& languageChangeCbk : _languageChangeCallbacks) {
        languageChangeCbk(newLanguage);
    }
}

const char* LanguageData::get(U64 key, const char* defaultValue) {
    typedef hashMap<U64, stringImpl>::const_iterator citer;
    // When we ask for a string for the given key, we check our language cache first
    citer entry = _languageTable.find(key);
    if (entry != std::cend(_languageTable)) {
        // Usually, the entire language table is loaded.
        return entry->second.c_str();
    }
    DIVIDE_UNEXPECTED_CALL("Locale error: INVALID STRING KEY!");

    return defaultValue;
}

void LanguageData::add(U64 key, const char* value) {
    hashAlg::emplace(_languageTable, key, value);
}

void LanguageData::addLanguageChangeCallback(const DELEGATE_CBK<void, const char* /*new language*/>& cbk) {
    _languageChangeCallbacks.push_back(cbk);
}

ErrorCode init(const char* newLanguage) {
    if (!Config::Build::IS_SHIPPING_BUILD) {
        if (!detail::g_LanguageFileWatcher) {
            detail::g_LanguageFileWatcher.reset(new FW::FileWatcher());
            detail::g_fileWatcherListener.addIgnoredEndCharacter('~');
            detail::g_fileWatcherListener.addIgnoredExtension("tmp");
            detail::g_LanguageFileWatcher->addWatch(Paths::g_exePath + Paths::g_localisationPath.c_str(), &detail::g_fileWatcherListener);
        }
    }

    if (!detail::g_data) {
        detail::g_data = std::make_unique<LanguageData>();
    }

    // Override old data
    detail::g_data->changeLanguage(newLanguage);

    // Use SimpleIni library for cross-platform INI parsing
    CSimpleIni languageFile(true, false, true);

    detail::g_localeFile = stringImpl(newLanguage);
    assert(!detail::g_localeFile.empty());

    stringImpl file = Paths::g_localisationPath + detail::g_localeFile + g_languageFileExtension;

    if (languageFile.LoadFile(file.c_str()) != SI_OK) {
        return ErrorCode::NO_LANGUAGE_INI;
    }

    // Load all key-value pairs for the "language" section
    const CSimpleIni::TKeyVal* keyValue = languageFile.GetSection("language");

    assert(keyValue != nullptr && "Locale::init error: No 'language' section found");
    // And add all pairs to the language table
    CSimpleIni::TKeyVal::const_iterator keyValuePairIt = keyValue->begin();
    for (; keyValuePairIt != keyValue->end(); ++keyValuePairIt) {
        detail::g_data->add(_ID(keyValuePairIt->first.pItem), keyValuePairIt->second);
    }

    return ErrorCode::NO_ERR;
}

void clear() {
    detail::g_data.reset();
}

void idle() {
    if (detail::g_LanguageFileWatcher) {
        detail::g_LanguageFileWatcher->update();
    }
}

/// Although the language can be set at compile time, in-game options may support
/// language changes
void changeLanguage(const char* newLanguage) {
    /// Set the new language code
    init(newLanguage);
}

const char* get(U64 key, const char* defaultValue) {
    if (detail::g_data) {
        return detail::g_data->get(key, defaultValue);
    }

    return defaultValue;
}

const char* get(U64 key) {
    return get(key, "key not found");
}

void addChangeLanguageCallback(const DELEGATE_CBK<void, const char* /*new language*/>& cbk) {
    assert(detail::g_data);

    detail::g_data->addLanguageChangeCallback(cbk);
}

const stringImpl& currentLanguage() {
    return detail::g_localeFile;
}

};  // namespace Locale
};  // namespace Divide
