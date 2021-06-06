//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
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
