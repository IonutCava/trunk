#include "Headers/Localization.h"

#include "Core/Headers/ErrorCodes.h"
#include <SimpleINI/include/SimpleIni.h>

namespace Divide {
namespace Locale {

static CSimpleIni g_languageFile;
/// Is everything loaded and ready for use?
static bool g_initialized = false;

ErrorCode init(const stringImpl& newLanguage) {
    clear();

    g_localeFile = newLanguage;
    // Use SimpleIni library for cross-platform INI parsing
    g_languageFile.SetUnicode();
    g_languageFile.SetMultiLine(true);

    stringImpl file = "localisation/" + g_localeFile + ".ini";

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
        
        hashAlg::emplace(g_languageTable,
                         _ID_RT(keyValuePairIt->first.pItem),
                         value);
    }

    g_initialized = true;
    return ErrorCode::NO_ERR;
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
    init(newLanguage);
}

const char* get(U64 key, const char* defaultValue) {
    if (g_initialized) {
        typedef hashMapImpl<U64, stringImpl>::const_iterator citer;
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

const char* get(U64 key) {
    return get(key, "key not found");
}


};  // namespace Locale
};  // namespace Divide
