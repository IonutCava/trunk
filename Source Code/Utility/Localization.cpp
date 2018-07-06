#include "Headers/Localization.h"

#include "Core/Headers/Application.h"
#include <SimpleIni.h>

namespace Divide {
namespace Locale {
    static CSimpleIni g_languageFile;

    bool init(const stringImpl& newLanguage) {
        changeLanguage(newLanguage);
        //Use SimpleIni library for cross-platform INI parsing
        g_languageFile.SetUnicode();
        stringImpl file = "localisation/" + g_localeFile + ".ini";
      
        if (g_languageFile.LoadFile(file.c_str()) != SI_OK) {
            Application::getInstance().throwError(NO_LANGUAGE_INI);
            return false;
        }

          //Load all key-value pairs for the "language" section
        const CSimpleIni::TKeyVal * keyValue = g_languageFile.GetSection("language");

        assert(keyValue != nullptr && "Locale::init error: No 'language' section found");
        //And add all pairs to the language table
        CSimpleIni::TKeyVal::const_iterator keyValuePairIt = keyValue->begin();
        for ( ;keyValuePairIt != keyValue->end(); ++keyValuePairIt) {
            hashAlg::emplace(g_languageTable, stringImpl(keyValuePairIt->first.pItem), stringImpl(keyValuePairIt->second));
        }

        g_initialized = true;
        return true;
    }

    void clear() { 
        g_languageTable.clear(); 
        g_languageFile.Reset();
        g_initialized = false;
    }

    ///Altough the language can be set at compile time, in-game options may support language changes
    void changeLanguage(const stringImpl& newLanguage){
        ///Set the new language code
        g_localeFile = newLanguage;
        ///And clear the table for the old language
        clear();
    }

    char* get(const stringImpl& key, const stringImpl& defaultValue) {
        assert(g_initialized == true && "Locale::get error: Get() called without initializing the language subsytem");
        //When we ask for a string for the given key, we check our language cache first
        if(g_languageTable.find(key) != std::end(g_languageTable)){
            //Usually, the entire language table is loaded.
            return const_cast<char*>(g_languageTable[key].c_str());
        }
    
        g_languageTable[key] = defaultValue;
        assert(g_languageTable[key].compare("String not found!") != 0 && "Locale error: INVALID STRING KEY!");
      
        return const_cast<char*>(defaultValue.c_str());
    }

}; //namespace Locale
}; //namespace Divide