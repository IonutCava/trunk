#include "Headers/MathMatrices.h"
#include "Headers/Quaternion.h"

#include <boost/thread/tss.hpp>
#include <cstdarg>
#include <fstream>

namespace Divide {
namespace Util {

static boost::thread_specific_ptr<vectorImpl<GlobalFloatEvent>> _globalFloatEvents;

void GetPermutations(const stringImpl& inputString,
                     vectorImpl<stringImpl>& permutationContainer) {
    permutationContainer.clear();
    stringImpl tempCpy(inputString);
    std::sort(std::begin(tempCpy), std::end(tempCpy));
    do {
        permutationContainer.push_back(inputString);
    } while (std::next_permutation(std::begin(tempCpy), std::end(tempCpy)));
}

bool IsNumber(const stringImpl& s) {
    F32 number;
    if (std::istringstream(s) >> number) {
        return !(number == 0 && s[0] != 0);
    }
    return false;
}

stringImpl GetTrailingCharacters(const stringImpl& input, size_t count) {
    size_t inputLength = input.length();
    assert(count > 0 && count <= inputLength);
    return input.substr(inputLength - count, inputLength);
}

void ReadTextFile(const stringImpl& filePath, stringImpl& contentOut) {
    std::ifstream inFile(filePath, std::ios::in);

    if (!inFile.eof() && !inFile.fail())
    {
        assert(inFile.good());
        inFile.seekg(0, std::ios::end);
        contentOut.reserve(inFile.tellg());
        inFile.seekg(0, std::ios::beg);

        contentOut.assign((std::istreambuf_iterator<char>(inFile)),
                           std::istreambuf_iterator<char>());
    }

    inFile.close();
}

stringImpl ReadTextFile(const stringImpl& filePath) {
    stringImpl content;
    ReadTextFile(filePath, content);
    return content;
}

void WriteTextFile(const stringImpl& filePath, const stringImpl& content) {
    if (filePath.empty()) {
        return;
    }
    std::ofstream outputFile(filePath, std::ios::out);
    outputFile << content;
    outputFile.close();
    assert(outputFile.good());
}

vectorImpl<stringImpl>& Split(const stringImpl& input, char delimiter,
                              vectorImpl<stringImpl>& elems) {
    stringImpl item;
    elems.resize(0);
    std::istringstream ss(input);
    while (std::getline(ss, item, delimiter)) {
        vectorAlg::emplace_back(elems, item);
    }
    return elems;

}

vectorImpl<stringImpl> Split(const stringImpl& input, char delimiter) {
    vectorImpl<stringImpl> elems;
    Split(input, delimiter, elems);
    return elems;
}

stringImpl StringFormat(const stringImpl fmt_str, ...) {
    // Reserve two times as much as the length of the fmt_str
    I32 final_n, n = to_int(fmt_str.size()) * 2; 
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(true) {
        /// Wrap the plain char array into the unique_ptr
        formatted.reset(new char[n]); 
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n) {
            n += std::abs(final_n - n + 1);
        } else {
            break;
        }
    }

    return formatted.get();
}

bool CompareIgnoreCase(const stringImpl& a, const stringImpl&b) {
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


bool HasExtension(const stringImpl& filePath, const stringImpl& extension) {
    stringImpl ext("." + extension);
    return CompareIgnoreCase(GetTrailingCharacters(filePath, ext.length()), ext);
}

void CStringRemoveChar(char* str, char charToRemove) {
    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != charToRemove) {
            dst++;
        }
    }
    *dst = '\0';
}

void ToByteColor(const vec4<F32>& floatColor, vec4<U8>& colorOut) {
    colorOut.set(FLOAT_TO_CHAR(floatColor.r),
                 FLOAT_TO_CHAR(floatColor.g),
                 FLOAT_TO_CHAR(floatColor.b),
                 FLOAT_TO_CHAR(floatColor.a));
}

void ToByteColor(const vec3<F32>& floatColor, vec3<U8>& colorOut) {
    colorOut.set(FLOAT_TO_CHAR_SNORM(floatColor.r),
                 FLOAT_TO_CHAR_SNORM(floatColor.g),
                 FLOAT_TO_CHAR_SNORM(floatColor.b));
}

void ToUIntColor(const vec4<F32>& floatColor, vec4<U32>& colorOut) {
    colorOut.set(FLOAT_TO_CHAR_SNORM(floatColor.r),
                 FLOAT_TO_CHAR_SNORM(floatColor.g),
                 FLOAT_TO_CHAR_SNORM(floatColor.b),
                 FLOAT_TO_CHAR_SNORM(floatColor.a));
}

void ToUIntColor(const vec3<F32>& floatColor, vec3<U32>& colorOut) {
    colorOut.set(to_uint(FLOAT_TO_CHAR_SNORM(floatColor.r)),
                 to_uint(FLOAT_TO_CHAR_SNORM(floatColor.g)),
                 to_uint(FLOAT_TO_CHAR_SNORM(floatColor.b)));
}

void ToFloatColor(const vec4<U8>& byteColor, vec4<F32>& colorOut) {
    colorOut.set(CHAR_TO_FLOAT_SNORM(byteColor.r),
                 CHAR_TO_FLOAT_SNORM(byteColor.g),
                 CHAR_TO_FLOAT_SNORM(byteColor.b),
                 CHAR_TO_FLOAT_SNORM(byteColor.a));
}

void ToFloatColor(const vec3<U8>& byteColor, vec3<F32>& colorOut) {
    colorOut.set(CHAR_TO_FLOAT_SNORM(byteColor.r),
                 CHAR_TO_FLOAT_SNORM(byteColor.g),
                 CHAR_TO_FLOAT_SNORM(byteColor.b));
}

void ToFloatColor(const vec4<U32>& uintColor, vec4<F32>& colorOut) {
    colorOut.set(uintColor.r / 255.0f,
                 uintColor.g / 255.0f,
                 uintColor.b / 255.0f,
                 uintColor.a / 255.0f);
}

void ToFloatColor(const vec3<U32>& uintColor, vec3<F32>& colorOut) {
    colorOut.set(uintColor.r / 255.0f,
                 uintColor.g / 255.0f,
                 uintColor.b / 255.0f);
}

vec4<U8> ToByteColor(const vec4<F32>& floatColor) {
    vec4<U8> tempColor;
    ToByteColor(floatColor, tempColor);
    return tempColor;
}

vec3<U8>  ToByteColor(const vec3<F32>& floatColor) {
    vec3<U8> tempColor;
    ToByteColor(floatColor, tempColor);
    return tempColor;
}

vec4<U32> ToUIntColor(const vec4<F32>& floatColor) {
    vec4<U32> tempColor;
    ToUIntColor(floatColor, tempColor);
    return tempColor;
}

vec3<U32> ToUIntColor(const vec3<F32>& floatColor) {
    vec3<U32> tempColor;
    ToUIntColor(floatColor, tempColor);
    return tempColor;
}

vec4<F32> ToFloatColor(const vec4<U8>& byteColor) {
    vec4<F32> tempColor;
    ToFloatColor(byteColor, tempColor);
    return tempColor;
}

vec3<F32> ToFloatColor(const vec3<U8>& byteColor) {
    vec3<F32> tempColor;
    ToFloatColor(byteColor, tempColor);
    return tempColor;
}

vec4<F32> ToFloatColor(const vec4<U32>& uintColor) {
    vec4<F32> tempColor;
    ToFloatColor(uintColor, tempColor);
    return tempColor;
}

vec3<F32> ToFloatColor(const vec3<U32>& uintColor) {
    vec3<F32> tempColor;
    ToFloatColor(uintColor, tempColor);
    return tempColor;
}

F32 PACK_VEC3(const vec3<F32>& value) {
    return PACK_VEC3(value.x, value.y, value.z);
}

void Normalize(vec3<F32>& inputRotation, bool degrees, bool normYaw,
               bool normPitch, bool normRoll) {
    if (normYaw) {
        F32 yaw = degrees ? Angle::DegreesToRadians(inputRotation.yaw)
                          : inputRotation.yaw;
        if (yaw < -M_PI) {
            yaw = fmod(yaw, to_float(M_PI) * 2.0f);
            if (yaw < -M_PI) {
                yaw += to_float(M_PI) * 2.0f;
            }
            inputRotation.yaw = Angle::RadiansToDegrees(yaw);
        } else if (yaw > M_PI) {
            yaw = fmod(yaw, to_float(M_PI) * 2.0f);
            if (yaw > M_PI) {
                yaw -= to_float(M_PI) * 2.0f;
            }
            inputRotation.yaw = degrees ? Angle::RadiansToDegrees(yaw) : yaw;
        }
    }
    if (normPitch) {
        F32 pitch = degrees ? Angle::DegreesToRadians(inputRotation.pitch)
                            : inputRotation.pitch;
        if (pitch < -M_PI) {
            pitch = fmod(pitch, to_float(M_PI) * 2.0f);
            if (pitch < -M_PI) {
                pitch += to_float(M_PI) * 2.0f;
            }
            inputRotation.pitch = Angle::RadiansToDegrees(pitch);
        } else if (pitch > M_PI) {
            pitch = fmod(pitch, to_float(M_PI) * 2.0f);
            if (pitch > M_PI) {
                pitch -= to_float(M_PI) * 2.0f;
            }
            inputRotation.pitch =
                degrees ? Angle::RadiansToDegrees(pitch) : pitch;
        }
    }
    if (normRoll) {
        F32 roll = degrees ? Angle::DegreesToRadians(inputRotation.roll)
                           : inputRotation.roll;
        if (roll < -M_PI) {
            roll = fmod(roll, to_float(M_PI) * 2.0f);
            if (roll < -M_PI) {
                roll += to_float(M_PI) * 2.0f;
            }
            inputRotation.roll = Angle::RadiansToDegrees(roll);
        } else if (roll > M_PI) {
            roll = fmod(roll, to_float(M_PI) * 2.0f);
            if (roll > M_PI) {
                roll -= to_float(M_PI) * 2.0f;
            }
            inputRotation.roll = degrees ? Angle::RadiansToDegrees(roll) : roll;
        }
    }
}

void FlushFloatEvents() {
    vectorImpl<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if( !vec ) {
        vec = new vectorImpl<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }
    vec->clear();
}

void RecordFloatEvent(const char* eventName, F32 eventValue, U64 timestamp) {
    vectorImpl<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if( !vec ) {
        vec = new vectorImpl<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }
    vectorAlg::emplace_back(*vec, eventName, eventValue, timestamp);
}

const vectorImpl<GlobalFloatEvent>& GetFloatEvents() {
    vectorImpl<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if (!vec) {
        vec = new vectorImpl<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }

    return *vec;
}

void PlotFloatEvents(const char* eventName,
                     vectorImpl<GlobalFloatEvent> eventsCopy,
                     GraphPlot2D& targetGraph) {
    targetGraph._plotName = eventName;
    targetGraph._coords.clear();
    for (GlobalFloatEvent& crtEvent : eventsCopy) {
        if (std::strcmp(eventName, crtEvent._eventName.c_str()) == 0) {
            vectorAlg::emplace_back(
                targetGraph._coords,
                Time::MicrosecondsToMilliseconds<F32>(crtEvent._timeStamp),
                crtEvent._eventValue);
        }
    }
}
};  // namespace Util
};  // namespace Divide
