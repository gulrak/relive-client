//---------------------------------------------------------------------------------------
//
// relivedb - A C++ implementation of the reLive protocoll and an sqlite backend
//
//---------------------------------------------------------------------------------------
//
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//---------------------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <mutex>

template <class T>
class RingBuffer
{
public:
    RingBuffer(unsigned int size = 16384)
    {
        _buffer = (T*)(_writePtr = _readPtr = new T[size]);
        _size = size;
    }

    virtual ~RingBuffer()
    {
        delete[] _buffer;
    }

    unsigned int bufferSize() const { return _size; }

    void resize(unsigned int size)
    {
        std::lock_guard<std::mutex> lock{_mutex};
        delete[] _buffer;
        _buffer = (T*)(_writePtr = _readPtr = new T[size]);
        _size = size;
    }

    int push(const T* data, unsigned int size)
    {
        size = (std::min)(size, free());
        std::lock_guard<std::mutex> lock{_mutex};
        int len = size;
        while(size--) {
            if( _writePtr >= _buffer + _size )
                _writePtr = _buffer;
            *_writePtr++ = *data++;
        }
        return len;
    }

    int peek(T* data, unsigned int size)
    {
        size = (std::min)(size, filled());
        std::lock_guard<std::mutex> lock{_mutex};
        int len = size;
        auto readPtr = _readPtr;
        while(size--) {
            if( readPtr >= _buffer + _size )
                readPtr = _buffer;
            *data++ = *readPtr++;
        }
        return len;
    }

    int pull(T* data, unsigned int size)
    {
        size = (std::min)(size, filled());
        std::lock_guard<std::mutex> lock{_mutex};
        int len = size;
        while(size--) {
            if( _readPtr >= _buffer + _size )
                _readPtr = _buffer;
            *data++ = *_readPtr++;
        }
        return len;
    }

    int drop(unsigned int size)
    {
        size = (std::min)(size, filled());
        std::lock_guard<std::mutex> lock{_mutex};
        int len = size;
        while(size--) {
            if( _readPtr >= _buffer + _size )
                _readPtr = _buffer;
            _readPtr++;
        }
        return len;
    }

    bool canPush(unsigned int size) const
    {
        return free() >= size;
    }

    bool canPull(unsigned int size) const
    {
        return filled() >= size;
    }

    unsigned int filled() const
    {
        std::lock_guard<std::mutex> lock{_mutex};
        if( _readPtr <= _writePtr )
            return _writePtr - _readPtr;
        else
            return _writePtr + _size - _readPtr;
    }

    unsigned int free() const
    {
        return _size - filled() - 1;
    }

    virtual void clear()
    {
        std::lock_guard<std::mutex> lock{_mutex};
        _writePtr = _readPtr = _buffer;
    }

private:
    mutable std::mutex _mutex;
    unsigned int _size;
    T* _buffer;
    volatile T* _writePtr;
    volatile T* _readPtr;
};
