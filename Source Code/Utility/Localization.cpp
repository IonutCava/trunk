#include "Headers/Localization.h"

#include "Core/Headers/Application.h"
#include <SimpleIni.h>

namespace Divide {
namespace Locale {
    char* get(const stringImpl& key, const stringImpl& defaultValue) {
        //When we ask for a string for the given key, we check our language cache first
        if(_languageTable.find(key) != _languageTable.end()){
            //Usually, the entire language table is loaded.
            return (char*)_languageTable[key].c_str();
        }
        //If we did not find our key in the cache, we need to populate the cache table
        //This may occur, for example, on language change
        stringImpl file = "localisation/" + _localeFile + ".ini";
        //Use SimpleIni library for cross-platform INI parsing
        CSimpleIniA ini;
        ini.SetUnicode();
        if (ini.LoadFile(file.c_str()) != SI_OK) {
            Application::getInstance().throwError(NO_LANGUAGE_INI);
        }
        //Load all key-value pairs for the "language" section
        const CSimpleIni::TKeyVal * keyValue = ini.GetSection("language");
        if (keyValue) {
            //And add all pairs to the language table
            CSimpleIni::TKeyVal::const_iterator keyValuePairIt = keyValue->begin();
            for ( ;keyValuePairIt != keyValue->end(); ++keyValuePairIt) {
				hashAlg::emplace(_languageTable, stringImpl(keyValuePairIt->first.pItem), stringImpl(keyValuePairIt->second));
            }
        }else{
            _languageTable[key] = defaultValue;
            assert(_languageTable[key].compare("String not found!") != 0 && "Locale error: INVALID STRING KEY!");
        }
        //Return our desired value now
        return (char*)_languageTable[key].c_str();
    }
}; //namespace Locale
}; //namespace Divide