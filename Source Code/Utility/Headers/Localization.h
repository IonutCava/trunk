/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _LOCALE_H_
#define _LOCALE_H_

#define DEFAULT_LANG "enGB"

#include "UnorderedMap.h"

namespace Locale {
	///Each string key in the map matches a key in the language ini file
	///each string value in the map matches the value in the ini file for the given key
	///Basicly, the Unordered_map is a direct copy of the [language] section of the give ini file
	static Unordered_map<std::string, std::string> _languageTable;
	///Default language can be set at compile time
	static std::string _localeFile = DEFAULT_LANG;
	///clear the language table
	inline void clear(){ _languageTable.clear(); }
	///Altough the language can be set at compile time, in-game options may support language changes
	inline void changeLanguage(const std::string& newLanguage){
		///Set the new language code
		_localeFile = newLanguage;
		///And clear the table for the old language
		clear();
	}
	///Query the current language code to detect changes
	inline const std::string& currentLanguage() { return _localeFile; }
	///usage: Locale::get("A_B_C") or Locale::get("A_B_C","X") where "A_B_C" is the language key we want
	///and "X" is a default string in case the key does not exist in the INI file
	char* get(const std::string& key,const std::string& defaultValue = std::string("String not found!"));
};

#endif
