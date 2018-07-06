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

void ReadTextFile(const stringImpl& filePath, stringImpl& contentOut) {
    std::ifstream inFile(filePath.c_str(), std::ios::in);

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
    std::ofstream outputFile(filePath.c_str(), std::ios::out);
    outputFile << content;
    outputFile.close();
    assert(outputFile.good());
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

    return stringImpl(formatted.get());
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
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != charToRemove);
    }
    *pw = '\0';
}

void ToByteColour(const vec4<F32>& floatColour, vec4<U8>& colourOut) {
    colourOut.set(FLOAT_TO_CHAR(floatColour.r),
                 FLOAT_TO_CHAR(floatColour.g),
                 FLOAT_TO_CHAR(floatColour.b),
                 FLOAT_TO_CHAR(floatColour.a));
}

void ToByteColour(const vec3<F32>& floatColour, vec3<U8>& colourOut) {
    colourOut.set(FLOAT_TO_CHAR_SNORM(floatColour.r),
                  FLOAT_TO_CHAR_SNORM(floatColour.g),
                  FLOAT_TO_CHAR_SNORM(floatColour.b));
}

void ToIntColour(const vec4<F32>& floatColour, vec4<I32>& colourOut) {
    colourOut.set(FLOAT_TO_SCHAR_SNORM(floatColour.r),
                  FLOAT_TO_SCHAR_SNORM(floatColour.g),
                  FLOAT_TO_SCHAR_SNORM(floatColour.b),
                  FLOAT_TO_SCHAR_SNORM(floatColour.a));
}

void ToIntColour(const vec3<F32>& floatColour, vec3<I32>& colourOut) {
    colourOut.set(to_uint(FLOAT_TO_SCHAR_SNORM(floatColour.r)),
                  to_uint(FLOAT_TO_SCHAR_SNORM(floatColour.g)),
                  to_uint(FLOAT_TO_SCHAR_SNORM(floatColour.b)));
}

void ToUIntColour(const vec4<F32>& floatColour, vec4<U32>& colourOut) {
    colourOut.set(FLOAT_TO_CHAR_SNORM(floatColour.r),
                 FLOAT_TO_CHAR_SNORM(floatColour.g),
                 FLOAT_TO_CHAR_SNORM(floatColour.b),
                 FLOAT_TO_CHAR_SNORM(floatColour.a));
}

void ToUIntColour(const vec3<F32>& floatColour, vec3<U32>& colourOut) {
    colourOut.set(to_uint(FLOAT_TO_CHAR_SNORM(floatColour.r)),
                 to_uint(FLOAT_TO_CHAR_SNORM(floatColour.g)),
                 to_uint(FLOAT_TO_CHAR_SNORM(floatColour.b)));
}

void ToFloatColour(const vec4<U8>& byteColour, vec4<F32>& colourOut) {
    colourOut.set(CHAR_TO_FLOAT_SNORM(byteColour.r),
                 CHAR_TO_FLOAT_SNORM(byteColour.g),
                 CHAR_TO_FLOAT_SNORM(byteColour.b),
                 CHAR_TO_FLOAT_SNORM(byteColour.a));
}

void ToFloatColour(const vec3<U8>& byteColour, vec3<F32>& colourOut) {
    colourOut.set(CHAR_TO_FLOAT_SNORM(byteColour.r),
                 CHAR_TO_FLOAT_SNORM(byteColour.g),
                 CHAR_TO_FLOAT_SNORM(byteColour.b));
}

void ToFloatColour(const vec4<U32>& uintColour, vec4<F32>& colourOut) {
    colourOut.set(uintColour.r / 255.0f,
                 uintColour.g / 255.0f,
                 uintColour.b / 255.0f,
                 uintColour.a / 255.0f);
}

void ToFloatColour(const vec3<U32>& uintColour, vec3<F32>& colourOut) {
    colourOut.set(uintColour.r / 255.0f,
                 uintColour.g / 255.0f,
                 uintColour.b / 255.0f);
}

vec4<U8> ToByteColour(const vec4<F32>& floatColour) {
    vec4<U8> tempColour;
    ToByteColour(floatColour, tempColour);
    return tempColour;
}

vec3<U8>  ToByteColour(const vec3<F32>& floatColour) {
    vec3<U8> tempColour;
    ToByteColour(floatColour, tempColour);
    return tempColour;
}

vec4<I32> ToIntColour(const vec4<F32>& floatColour) {
    vec4<I32> tempColour;
    ToIntColour(floatColour, tempColour);
    return tempColour;
}

vec3<I32> ToIntColour(const vec3<F32>& floatColour) {
    vec3<I32> tempColour;
    ToIntColour(floatColour, tempColour);
    return tempColour;
}

vec4<U32> ToUIntColour(const vec4<F32>& floatColour) {
    vec4<U32> tempColour;
    ToUIntColour(floatColour, tempColour);
    return tempColour;
}

vec3<U32> ToUIntColour(const vec3<F32>& floatColour) {
    vec3<U32> tempColour;
    ToUIntColour(floatColour, tempColour);
    return tempColour;
}

vec4<F32> ToFloatColour(const vec4<U8>& byteColour) {
    vec4<F32> tempColour;
    ToFloatColour(byteColour, tempColour);
    return tempColour;
}

vec3<F32> ToFloatColour(const vec3<U8>& byteColour) {
    vec3<F32> tempColour;
    ToFloatColour(byteColour, tempColour);
    return tempColour;
}

vec4<F32> ToFloatColour(const vec4<U32>& uintColour) {
    vec4<F32> tempColour;
    ToFloatColour(uintColour, tempColour);
    return tempColour;
}

vec3<F32> ToFloatColour(const vec3<U32>& uintColour) {
    vec3<F32> tempColour;
    ToFloatColour(uintColour, tempColour);
    return tempColour;
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

bool FileExists(const char* filePath) {
    return std::ifstream(filePath).good();
}

};  // namespace Util
};  // namespace Divide
