#include "stdafx.h"

#include "Headers/Localization.h"

#include "Core/Headers/ErrorCodes.h"
#include <SimpleINI/include/SimpleIni.h>

#include "Platform/File/Headers/FileManagement.h"
#include "Platform/File/Headers/FileUpdateMonitor.h"

namespace Divide {
namespace Locale {

namespace {
    CSimpleIni g_languageFile;
    constexpr char* g_languageFileExtension = ".ini";

    /// External modification monitoring system
    std::unique_ptr<FW::FileWatcher> s_LanguageFileWatcher = nullptr;

    UpdateListener s_fileWatcherListener([](const char* languageFile, FileUpdateEvent evt) {
        detail::onLanguageFileModify(languageFile, evt);
    });
};

namespace detail {
    void onLanguageFileModify(const char* languageFile, FileUpdateEvent evt) {
        if (evt == FileUpdateEvent::DELETE) {
            return;
        }

        // If we modify our currently active language, reinit the Locale system
        if (strcmp((s_localeFile + g_languageFileExtension).c_str(), languageFile) == 0) {
            changeLanguage(s_localeFile);
        }
    }
};

ErrorCode init(const stringImpl& newLanguage) {
    clear();
    if (!Config::Build::IS_SHIPPING_BUILD) {
        if (!s_LanguageFileWatcher) {
            s_LanguageFileWatcher.reset(new FW::FileWatcher());
            s_fileWatcherListener.addIgnoredEndCharacter('~');
            s_fileWatcherListener.addIgnoredExtension("tmp");
            s_LanguageFileWatcher->addWatch(Paths::g_localisationPath.c_str(), &s_fileWatcherListener);
        }
    }

    detail::s_localeFile = newLanguage;
    // Use SimpleIni library for cross-platform INI parsing
    g_languageFile.SetUnicode();
    g_languageFile.SetMultiLine(true);

    stringImpl file = Paths::g_localisationPath + detail::s_localeFile + g_languageFileExtension;

    if (g_languageFile.LoadFile(file.c_str()) != SI_OK) {
        return ErrorCode::NO_LANGUAGE_INI;
    }

    // Load all key-value pairs for the "language" section
    const CSimpleIni::TKeyVal* keyValue = g_languageFile.GetSection("language");

    assert(keyValue != nullptr &&
           "Locale::init error: No 'language' section found");
    // And add all pairs to the language table
    CSimpleIni::TKeyVal::const_iterator keyValuePairIt = keyValue->begin();
    for (; keyValuePairIt != keyValue->end(); ++keyValuePairIt) {
        stringImpl value(keyValuePairIt->second);
        
        hashAlg::emplace(detail::s_languageTable,
                         _ID_RT(keyValuePairIt->first.pItem),
                         value);
    }

    detail::s_initialized = true;
    return ErrorCode::NO_ERR;
}

void clear() {
    detail::s_languageTable.clear();
    g_languageFile.Reset();
    detail::s_initialized = false;
}

void idle() {
    if (s_LanguageFileWatcher) {
        s_LanguageFileWatcher->update();
    }
}

/// Altough the language can be set at compile time, in-game options may support
/// language changes
void changeLanguage(const stringImpl& newLanguage) {
    /// Set the new language code
    init(newLanguage);

    for (const DELEGATE_CBK<void, const char* /*new language*/>& languageChangeCbk : detail::s_languageChangeCallbacks) {
        languageChangeCbk(newLanguage.c_str());
    }
}

const char* get(U64 key, const char* defaultValue) {
    if (detail::s_initialized) {
        typedef hashMapImpl<U64, stringImpl>::const_iterator citer;
        // When we ask for a string for the given key, we check our language cache first
        citer entry = detail::s_languageTable.find(key);
        if (entry != std::cend(detail::s_languageTable)) {
            // Usually, the entire language table is loaded.
            return entry->second.c_str();
        }
        DIVIDE_UNEXPECTED_CALL("Locale error: INVALID STRING KEY!");
    }

    return defaultValue;
}

const char* get(U64 key) {
    return get(key, "key not found");
}

void addChangeLanguageCallback(const DELEGATE_CBK<void, const char* /*new language*/>& cbk) {
    detail::s_languageChangeCallbacks.emplace_back(cbk);
}

};  // namespace Locale
};  // namespace Divide
