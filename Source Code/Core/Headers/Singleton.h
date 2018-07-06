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

#ifndef _CORE_SINGLETON_H_
#define _CORE_SINGLETON_H_

#ifndef __NARG__
 // http://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
#define __NARG__(...)  __NARG_I_(__VA_ARGS__,__RSEQ_N())
#define EXPAND( x ) x
#define __NARG_I_(...) EXPAND(__ARG_N(__VA_ARGS__))
#define __ARG_N( \
      _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
     _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
     _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
     _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
     _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
     _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
     _61,_62,_63,N,...) N
#define __RSEQ_N() \
     63,62,61,60,                   \
     59,58,57,56,55,54,53,52,51,50, \
     49,48,47,46,45,44,43,42,41,40, \
     39,38,37,36,35,34,33,32,31,30, \
     29,28,27,26,25,24,23,22,21,20, \
     19,18,17,16,15,14,13,12,11,10, \
     9,8,7,6,5,4,3,2,1,0

 // general definition for any function name

#define _VFUNC_(name, n) name##n
#define _VFUNC(name, n) _VFUNC_(name, n)
#define VFUNC(func, ...) EXPAND(_VFUNC(func, __NARG__(__VA_ARGS__)) (__VA_ARGS__))
#endif

//#define _DEBUG_SINGLETONS
#if defined(_DEBUG) && defined(_DEBUG_SINGLETONS)
#include <assert.h>
#endif

namespace Divide {

template <typename T>
class Singleton {
protected:
   public:

    // Alias for instance() to easily explain intent
    inline static T& createInstance() {
        return instance();
    }

    inline static T& instance() { 
#   if defined(_DEBUG) && defined(_DEBUG_SINGLETONS)
        assert(!_zombified);
#    endif
        static T *instance = new T();
        return *instance;
    }

    inline static void destroyInstance() {
        delete &instance();
#       if defined(_DEBUG) && defined(_DEBUG_SINGLETONS)
        _zombified = true;
#       endif
    }

   protected:
    Singleton()
    {
    }

    virtual ~Singleton() 
    {
    }

   private:
    Singleton(Singleton&) = delete;
    void operator=(Singleton&) = delete;

#if defined(_DEBUG) && defined(_DEBUG_SINGLETONS)
   private:
    static bool _zombified;
#endif
};

#if defined(_DEBUG) && defined(_DEBUG_SINGLETONS)
template <typename T>
bool Singleton<T>::_zombified = false;
#endif

#define DEFINE_SINGLETON_W_SPECIFIER_2(class_name, specifier)   \
    class class_name specifier : public Singleton<class_name> { \
        friend class Singleton<class_name>;

#define DEFINE_SINGLETON_W_SPECIFIER_3(class_name, base_class, specifier) \
    class class_name specifier : public Singleton<class_name>,            \
                                 public base_class {                      \
        friend class Singleton<class_name>;

#define DEFINE_SINGLETON_W_SPECIFIER_4(class_name, base_class1, \
                                       base_class2, specifier)  \
    class class_name specifier : public Singleton<class_name>,  \
                                 public base_class1,            \
                                 public base_class2 {           \
        friend class Singleton<class_name>;

#define DEFINE_SINGLETON_1(class_name) \
    DEFINE_SINGLETON_W_SPECIFIER_2(class_name, )

#define DEFINE_SINGLETON_2(class_name, base_class) \
    DEFINE_SINGLETON_W_SPECIFIER_3(class_name, base_class, )

#define DEFINE_SINGLETON_3(class_name, base_class1, base_class2) \
    DEFINE_SINGLETON_W_SPECIFIER_4(class_name, base_class1, base_class2, )


#define DEFINE_SINGLETON(...) VFUNC(DEFINE_SINGLETON_, __VA_ARGS__)

#define DEFINE_SINGLETON_W_SPECIFIER(...) VFUNC(DEFINE_SINGLETON_W_SPECIFIER_, __VA_ARGS__)

#define END_SINGLETON  \
    };

};  // namespace Divide

#endif  // _CORE_SINGLETON_H_
