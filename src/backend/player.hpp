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

#include "rldata.hpp"
#include <ghc/uri.hpp>
#include <cstdint>
#include <memory>

#define RELIVE_RTAUDIO_BACKEND
//#define RELIVE_PORTAUDIO_BACKEND

enum PlayerState { ePAUSED, ePLAYING, eENDOFSTREAM, eENDING };

class Player
{
public:
    using SampleType = int16_t;
    enum Mode { eNone, eFile, eReLiveStream, eMediaStream, eSCastStream };
    Player();
    virtual ~Player();
    void run();

    void play();
    void pause();

    PlayerState state() const;
    int playTime() const;
    void seekTo(int seconds);
    void prev();
    void next();
    int volume() const;
    void volume(int vol);
    
    bool hasSource() const;
    void setSource(Mode mode, ghc::net::uri source, int64_t size = 0);
    void setSource(const Stream& stream);
    void setSource(const Track& track);
    std::shared_ptr<Stream> currentStream();
    
    void playMusic(unsigned char* buffer, int frames);

protected:
    void configureAudio();
    void disableAudio();
    void startAudio();
    void stopAudio();
    bool decodeFrame();
    bool fillBuffer();
    void streamSCast();
    struct impl;
    std::unique_ptr<impl> _impl;
};
