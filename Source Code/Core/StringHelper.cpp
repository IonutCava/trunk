#include "stdafx.h"

#include "Headers/StringHelper.h"

namespace Divide {
namespace Util {

void GetPermutations(const stringImpl& inputString, vectorImpl<stringImpl>& permutationContainer) {
    permutationContainer.clear();
    stringImpl tempCpy(inputString);
    std::sort(std::begin(tempCpy), std::end(tempCpy));
    do {
        permutationContainer.push_back(inputString);
    } while (std::next_permutation(std::begin(tempCpy), std::end(tempCpy)));
}

bool IsNumber(const stringImpl& s) {
    F32 number = 0.0f;
    if (istringstreamImpl(s) >> number) {
        return !(IS_ZERO(number) && s[0] != 0);
    }
    return false;
}

stringImpl GetTrailingCharacters(const stringImpl& input, size_t count) {
    size_t inputLength = input.length();
    count = std::min(inputLength, count);
    assert(count > 0);
    return input.substr(inputLength - count, inputLength);
}

stringImpl GetStartingCharacters(const stringImpl& input, size_t count) {
    size_t inputLength = input.length();
    count = std::min(inputLength, count);
    assert(count > 0);
    return input.substr(0, inputLength - count);
}

vectorImpl<stringImpl>& Split(const stringImpl& input, char delimiter,
    vectorImpl<stringImpl>& elems) {
    elems.resize(0);
    if (!input.empty()) {
        istringstreamImpl ss(input);
        stringImpl item;
        while (std::getline(ss, item, delimiter)) {
            vectorAlg::emplace_back(elems, item);
        }
    }

    return elems;
}

vectorImpl<stringImpl> Split(const stringImpl& input, char delimiter) {
    vectorImpl<stringImpl> elems;
    Split(input, delimiter, elems);
    return elems;
}

stringImpl StringFormat(const char *const format, ...) {
    vectorImpl<char> temp;
    std::size_t length = 63;
    std::va_list args;
    while (temp.size() <= length) {
        temp.resize(length + 1);
        va_start(args, format);
        const auto status = std::vsnprintf(temp.data(), temp.size(), format, args);
        va_end(args);
        assert(status >= 0 && "string formatting error");
        length = static_cast<std::size_t>(status);
    }

    return stringImpl(temp.data(), length);
}


U32 LineCount(const stringImpl& str) {
    return to_U32(std::count(std::cbegin(str), std::cend(str), '\n')) + 1;
}

bool CompareIgnoreCase(const stringImpl& a, const stringImpl& b) {
    if (a.length() == b.length()) {
        return std::equal(std::cbegin(b),
            std::cend(b),
            std::cbegin(a),
            [](unsigned char a, unsigned char b) {
            return std::tolower(a) == std::tolower(b);
        });
    }

    return false;
}

void CStringRemoveChar(char* str, char charToRemove) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != charToRemove);
    }
    *pw = '\0';
}

}; //namespace Util

}; //namespace Divide