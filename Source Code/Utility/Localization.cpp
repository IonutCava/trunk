#include "Headers/Localization.h"

#include "Core/Headers/Application.h"
#include <SimpleINI/include/SimpleIni.h>

namespace Divide {
namespace Locale {

static CSimpleIni g_languageFile;
/// Is everything loaded and ready for use?
static bool g_initialized = false;

bool init(const stringImpl& newLanguage) {
    changeLanguage(newLanguage);
    // Use SimpleIni library for cross-platform INI parsing
    g_languageFile.SetUnicode();
    g_languageFile.SetMultiLine(true);

    stringImpl file = "localisation/" + g_localeFile + ".ini";

    if (g_languageFile.LoadFile(file.c_str()) != SI_OK) {
        Application::instance().throwError(ErrorCode::NO_LANGUAGE_INI);
        return false;
    }

    // Load all key-value pairs for the "language" section
    const CSimpleIni::TKeyVal* keyValue = g_languageFile.GetSection("language");

    assert(keyValue != nullptr &&
           "Locale::init error: No 'language' section found");
    // And add all pairs to the language table
    CSimpleIni::TKeyVal::const_iterator keyValuePairIt = keyValue->begin();
    for (; keyValuePairIt != keyValue->end(); ++keyValuePairIt) {
        stringImpl value(keyValuePairIt->second);
        
        hashAlg::emplace(g_languageTable,
                         _ID_RT(keyValuePairIt->first.pItem),
                         value);
    }

    g_initialized = true;
    return true;
}

void clear() {
    g_languageTable.clear();
    g_languageFile.Reset();
    g_initialized = false;
}

/// Altough the language can be set at compile time, in-game options may support
/// language changes
void changeLanguage(const stringImpl& newLanguage) {
    /// Set the new language code
    g_localeFile = newLanguage;
    /// And clear the table for the old language
    clear();
}

const char* get(ULL key, const char* defaultValue) {
    if (g_initialized) {
        typedef hashMapImpl<ULL, stringImpl>::const_iterator citer;
        // When we ask for a string for the given key, we check our language cache first
        citer entry = g_languageTable.find(key);
        if (entry != std::cend(g_languageTable)) {
            // Usually, the entire language table is loaded.
            return entry->second.c_str();
        }
        DIVIDE_UNEXPECTED_CALL("Locale error: INVALID STRING KEY!");
    }

    return defaultValue;
}

const char* get(ULL key) {
    return get(key, "key not found");
}


};  // namespace Locale
};  // namespace Divide
