/*
   Copyright (c) 2017 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _LOCALE_H_
#define _LOCALE_H_

static const char* DEFAULT_LANG = "enGB";

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

enum class ErrorCode : I32;

namespace Locale {
/// Each string key in the map matches a key in the language ini file
/// each string value in the map matches the value in the ini file for the given key
/// Basicly, the hashMapImpl is a direct copy of the [language] section of the
/// give ini file
static hashMapImpl<U64, stringImpl> g_languageTable;
/// Default language can be set at compile time
static stringImpl g_localeFile = DEFAULT_LANG;
/// Reset everything and load the specified language file.
ErrorCode init(const stringImpl& newLanguage = DEFAULT_LANG);
/// clear the language table
void clear();
/// perform maintnance tasks
void idle();
/// Altough the language can be set at compile time, in-game options may support
/// language changes
void changeLanguage(const stringImpl& newLanguage);
/// Query the current language code to detect changes
inline const stringImpl& currentLanguage() { return g_localeFile; }
/// usage: Locale::get(_ID("A_B_C")) or Locale::get(_ID("A_B_C"),"X") where "A_B_C" is the
/// language key we want
/// and "X" is a default string in case the key does not exist in the INI file
const char* get(U64 key, const char* defaultValue);
const char* get(U64 key);

/// Callback for external file changes. 
void onLanguageFileModify(const char* languageFile);

};  // namespace Locale
};  // namespace Divide

#endif
