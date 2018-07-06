/*
   Copyright (c) 2015 DIVIDE-Studio
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

namespace Divide {

template <typename T>
class Singleton {
   public:
    inline static T& getOrCreateInstance() {
        if (!_instance) {
            _instance = new T;
        }

        return *_instance;
    }

    inline static bool hasInstance() { return _instance != nullptr; }

    inline static T& getInstance() { return *_instance; }

    inline static void createInstance() {
        if (!_instance) {
            _instance = new T;
        }
    }

    inline static void destroyInstance() {
        if (_instance) {
            delete _instance;
            _instance = nullptr;
        }
    }

   protected:
    Singleton() {}

    virtual ~Singleton() {}

   private:
    Singleton(Singleton&) = delete;
    void operator=(Singleton&) = delete;

   private:
    /// C++11 standard assures a static instance should be thread safe:
    /// "if control enters the declaration concurrently while the variable is
    /// being initialized,
    /// the concurrent execution shall wait for completion of the
    /// initialization."
    static T* _instance;
};

template <typename T>
T* Singleton<T>::_instance = 0;

#define DEFINE_SINGLETON_W_SPECIFIER(class_name, specifier)     \
    class class_name specifier : public Singleton<class_name> { \
        friend class Singleton<class_name>;

#define DEFINE_SINGLETON_EXT1_W_SPECIFIER(class_name, base_class, specifier) \
    class class_name specifier : public Singleton<class_name>,               \
                                 public base_class {                         \
        friend class Singleton<class_name>;

#define DEFINE_SINGLETON_EXT2_W_SPECIFIER(class_name, base_class1, \
                                          base_class2, specifier)  \
    class class_name specifier : public Singleton<class_name>,     \
                                 public base_class1,               \
                                 public base_class2 {              \
        friend class Singleton<class_name>;

#define DEFINE_SINGLETON(class_name) \
    DEFINE_SINGLETON_W_SPECIFIER(class_name, )

#define DEFINE_SINGLETON_EXT1(class_name, base_class) \
    DEFINE_SINGLETON_EXT1_W_SPECIFIER(class_name, base_class, )

#define DEFINE_SINGLETON_EXT2(class_name, base_class1, base_class2) \
    DEFINE_SINGLETON_EXT2_W_SPECIFIER(class_name, base_class1, base_class2, )

#define END_SINGLETON  \
    };

};  // namespace Divide

#endif  // _CORE_SINGLETON_H_
