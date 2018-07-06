/*
Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _CONDITIONAL_WAIT_H_
#define _CONDITIONAL_WAIT_H_

namespace Divide {
#define GET_MACRO_1(_1,_2,NAME,...) NAME
#define GET_MACRO_2(_1,_2,_3,NAME,...) NAME
#define GET_MACRO_3(_1,_2,_3,_4,NAME,...) NAME

#define WAIT_FOR_CONDITION_1(condition, yeld) \
{                                             \
    assert_type<bool>(yeld);                  \
                                              \
    while (!(condition)) {                    \
        if (yeld) {                           \
            std::this_thread::yield();        \
        }                                     \
    }                                         \
}

#define WAIT_FOR_CONDITION_2(condition) \
    WAIT_FOR_CONDITION_1(condition, true)

#define WAIT_FOR_CONDITION(...) \
    GET_MACRO_1(__VA_ARGS__,\
                WAIT_FOR_CONDITION_1,\
                WAIT_FOR_CONDITION_2)(__VA_ARGS__)

#define WAIT_FOR_CONDITION_TIMEOUT_1(condition, timeoutMS, yeld) \
{                                                                \
    assert_type<bool>(yeld);                                     \
    assert_type<D64>(timeoutMS);                                 \
                                                                 \
    if (timeoutMS >= 0.0) {                                      \
        D64 start = Time::ElapsedMilliseconds(true);             \
                                                                 \
        while (!(condition)) {                                   \
            D64 end = Time::ElapsedMilliseconds(true);           \
            if (end - start >= timeoutMS) {                      \
                break;                                           \
            }                                                    \
                                                                 \
            if (yeld) {                                          \
                std::this_thread::yield();                       \
            }                                                    \
        }                                                        \
    } else {                                                     \
        WAIT_FOR_CONDITION(condition, yeld);                     \
    }                                                            \
}

#define WAIT_FOR_CONDITION_TIMEOUT_2(condition, timeoutMS) \
    WAIT_FOR_CONDITION_TIMEOUT_1(condition, timeoutMS, true)

#define WAIT_FOR_CONDITION_TIMEOUT_3(condition) \
    WAIT_FOR_CONDITION_TIMEOUT_2(condition, 1.0)

#define WAIT_FOR_CONDITION_TIMEOUT(...) \
    GET_MACRO_2(__VA_ARGS__, \
                WAIT_FOR_CONDITION_TIMEOUT_1, \
                WAIT_FOR_CONDITION_TIMEOUT_2, \
                WAIT_FOR_CONDITION_TIMEOUT_3)(__VA_ARGS__)

#define WAIT_FOR_CONDITION_CALLBACK_1(condition, cbk, yeld) \
{                                                           \
    assert_type<bool>(yeld);                                \
    assert_type<std::function>(cbk);                        \
                                                            \
    while (!(condition)) {                                  \
        cbk();                                              \
                                                            \
        if (yeld) {                                         \
            std::this_thread::yield();                      \
        }                                                   \
    }                                                       \
}                                                         

#define WAIT_FOR_CONDITION_CALLBACK_2(condition, cbk) \
    WAIT_FOR_CONDITION_CALLBACK_1(condition, cbk, true) 

#define WAIT_FOR_CONDITION_CALLBACK_3(condition) \
    WAIT_FOR_CONDITION(condition, true) 

#define WAIT_FOR_CONDITION_CALLBACK(...) \
    GET_MACRO_2(__VA_ARGS__, \
                WAIT_FOR_CONDITION_CALLBACK_1, \
                WAIT_FOR_CONDITION_CALLBACK_2, \
                WAIT_FOR_CONDITION_CALLBACK_3)(__VA_ARGS__)

#define WAIT_FOR_CONDITION_CALLBACK_TIMEOUT_1(condition, cbk, timeoutMS, yeld) \
{                                                                              \
    assert_type<bool>(yeld);                                                   \
    assert_type<std::function>(cbk);                                           \
    assert_type<D64>(timeoutMS);                                               \
                                                                               \
    if (timeoutMS >= 0.0) {                                                    \
        D64 start = Time::ElapsedMilliseconds(true);                           \
                                                                               \
        while (!(condition)) {                                                 \
            cbk();                                                             \
                                                                               \
            D64 end = Time::ElapsedMilliseconds(true);                         \
            if (end - start >= timeoutMS) {                                    \
                break;                                                         \
            }                                                                  \
                                                                               \
            if (yeld) {                                                        \
                std::this_thread::yield();                                     \
            }                                                                  \
        }                                                                      \
    } else {                                                                   \
        WAIT_FOR_CONDITION(condition, yeld);                                   \
    }                                                                          \
}

#define WAIT_FOR_CONDITION_CALLBACK_TIMEOUT_2(condition, cbk, timeoutMS) \
    WAIT_FOR_CONDITION_CALLBACK_TIMEOUT_1(condition, cbk, timeoutMS, true)

#define WAIT_FOR_CONDITION_CALLBACK_TIMEOUT_3(condition, cbk) \
    WAIT_FOR_CONDITION_CALLBACK_TIMEOUT_2(condition, cbk, 0.0)

#define WAIT_FOR_CONDITION_CALLBACK_TIMEOUT_4(condition) \
    WAIT_FOR_CONDITION(condition)

#define WAIT_FOR_CONDITION_CALLBACK_TIMEOUT(...) \
    GET_MACRO_3(__VA_ARGS__, \
                WAIT_FOR_CONDITION_CALLBACK_TIMEOUT_1, \
                WAIT_FOR_CONDITION_CALLBACK_TIMEOUT_2, \
                WAIT_FOR_CONDITION_CALLBACK_TIMEOUT_3, \
                WAIT_FOR_CONDITION_CALLBACK_TIMEOUT_4)(__VA_ARGS__)

};

#endif

