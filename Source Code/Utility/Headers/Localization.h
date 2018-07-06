/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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
	inline const std::string& currentLanguage() {return _localeFile;}
	///usage: Locale::get("A_B_C") or Locale::get("A_B_C","X") where "A_B_C" is the language key we want
	///and "X" is a default string in case the key does not exist in the INI file
	char* get(const std::string& key,const std::string& defaultValue = "");
};

#endif
