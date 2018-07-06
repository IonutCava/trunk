#include "stdafx.h"

#include "Headers/Defines.h"

#include "Core/Math/Headers/MathHelper.h"

namespace Divide {

TEST(TimeDownCast)
{
    constexpr U32 inputSeconds = 4;
    constexpr U32 inputMilliseconds = 5;
    constexpr U32 inputMicroseconds = 6;

    constexpr U32 microToNanoResult = 6'000u;

    constexpr U32 milliToMicroResult = 5'000u;
    constexpr D64 milliToNanoResult = 5e6;

    constexpr U32 secondsToMilliResult = 4'000u;
    constexpr D64 secondsToMicroResult = 4e6;
    constexpr D64 secondsToNanoResult = 4e9;

    U32 microToNano = Time::MicrosecondsToNanoseconds<U32>(inputMicroseconds);
    U32 milliToMicro = Time::MillisecondsToMicroseconds<U32>(inputMilliseconds);
    D64 milliToNano = Time::MillisecondsToNanoseconds<D64>(inputMilliseconds);
    U32 secondsToMilli = Time::SecondsToMilliseconds<U32>(inputSeconds);
    D64 secondsToMicro = Time::SecondsToMicroseconds<D64>(inputSeconds);
    D64 secondsToNano = Time::SecondsToNanoseconds<D64>(inputSeconds);

    CHECK_EQUAL(microToNanoResult, microToNano);
    CHECK_EQUAL(milliToMicroResult, milliToMicro);
    CHECK_EQUAL(milliToNanoResult, milliToNano);
    CHECK_EQUAL(secondsToMilliResult, secondsToMilli);
    CHECK_EQUAL(secondsToMicroResult, secondsToMicro);
    CHECK_EQUAL(secondsToNanoResult, secondsToNano);
}

TEST(MapTest)
{
    constexpr U32 in_min = 0;
    constexpr U32 in_max = 100;
    constexpr U32 out_min = 0;
    constexpr U32 out_max = 10;
    constexpr U32 in = 20;
    constexpr U32 result = 2;
    CHECK_EQUAL(result, MAP(in, in_min, in_max, out_min, out_max));
}

TEST(TimeUpCast)
{
    constexpr U32 secondsResult = 4;
    constexpr U32 millisecondsResult = 5;
    constexpr U32 microsecondsResult = 6;

    constexpr D64 inputNanoToSeconds = 4e9;
    constexpr D64 inputNanoToMilli = 5e6;
    constexpr U32 inputNanoToMicro = 6'000u;

    constexpr D64 inputMicroToSeconds = 4e6;
    constexpr U32 inputMicroToMilli = 5'000u;
    constexpr U32 inputMilliToSeconds = 4'000u;

    U32 nanoToSeconds = Time::NanosecondsToSeconds<U32>(inputNanoToSeconds);
    U32 nanoToMilli = Time::NanosecondsToMilliseconds<U32>(inputNanoToMilli);
    U32 nanoToMicro = Time::NanosecondsToMicroseconds<U32>(inputNanoToMicro);

    U32 microToSeconds = Time::MicrosecondsToSeconds<U32>(inputMicroToSeconds);
    U32 microToMilli = Time::MicrosecondsToMilliseconds<U32>(inputMicroToMilli);

    U32 milliToSeconds = Time::MillisecondsToSeconds<U32>(inputMilliToSeconds);

    CHECK_EQUAL(secondsResult, nanoToSeconds);
    CHECK_EQUAL(millisecondsResult, nanoToMilli);
    CHECK_EQUAL(microsecondsResult, nanoToMicro);

    CHECK_EQUAL(secondsResult, microToSeconds);
    CHECK_EQUAL(millisecondsResult, microToMilli);

    CHECK_EQUAL(secondsResult, milliToSeconds);

}

}; //namespace Divide