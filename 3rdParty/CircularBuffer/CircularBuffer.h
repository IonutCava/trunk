/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _CIRCULAR_BUFFER_H_
#define _CIRCULAR_BUFFER_H_

//ref: https://github.com/embeddedartistry/embedded-resources/blob/master/examples/cpp/circular_buffer.cpp
namespace Divide {

template <class T>
class circular_buffer {
public:
    explicit circular_buffer(size_t size) :
        buf_(std::unique_ptr<T[]>(new T[size])),
        max_size_(size)
    {

    }

    void put(T item)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        buf_[head_] = item;

        if (full_)
        {
            tail_ = (tail_ + 1) % max_size_;
        }

        head_ = (head_ + 1) % max_size_;

        full_ = head_ == tail_;
    }

    T get(size_t i)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return buf_[i];
    }

    T get()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (empty())
        {
            return T();
        }

        //Read data and advance the tail (we now have a free space)
        auto val = buf_[tail_];
        full_ = false;
        tail_ = (tail_ + 1) % max_size_;

        return val;
    }

    void reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        head_ = tail_;
        full_ = false;
    }

    bool empty() const
    {
        //if head and tail are equal, we are empty
        return (!full_ && (head_ == tail_));
    }

    bool full() const
    {
        //If tail is ahead the head by 1, we are full
        return full_;
    }

    size_t capacity() const
    {
        return max_size_;
    }

    size_t size() const
    {
        size_t size = max_size_;

        if (!full_)
        {
            if (head_ >= tail_)
            {
                size = head_ - tail_;
            }
            else
            {
                size = max_size_ + head_ - tail_;
            }
        }

        return size;
    }

private:
    std::mutex mutex_;
    std::unique_ptr<T[]> buf_;
    size_t head_ = 0;
    size_t tail_ = 0;
    const size_t max_size_;
    bool full_ = 0;
};

}; //namespace Divide
#endif //_CIRCULAR_BUFFER_H_