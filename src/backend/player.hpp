//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#pragma once

#include "rldata.hpp"
#include <ghc/uri.hpp>
#include <cstdint>
#include <memory>

namespace relive {

enum PlayerState { ePAUSED, ePLAYING, eENDOFSTREAM, eENDING, eERROR };

class Player
{
public:
    struct Device {
        std::string name;
        unsigned int channels;
        unsigned int sampleRate;
    };
    using SampleType = int16_t;
    enum Mode { eNone, eFile, eReLiveStream, eMediaStream, eSCastStream };
    Player();
    virtual ~Player();
    void run();

    void play();
    void pause();

    PlayerState state() const;
    int playTime() const;
    void seekTo(int seconds, bool startPlay = true);
    void prev();
    void next();
    int volume() const;
    void volume(int vol);
    float receiveBufferQuote() const;
    float decodeBufferQuote() const;
    
    bool hasSource() const;
    void setSource(Mode mode, ghc::net::uri source, int64_t size = 0);
    void setSource(const Stream& stream);
    void setSource(const Track& track);
    std::shared_ptr<Stream> currentStream();
    
    void playMusic(unsigned char* buffer, int frames);
    void streamStopped();

    static std::string getDynamicDefaultOutputName();
    std::string getCurrentDefaultOutputName() const;
    std::vector<Device> getOutputDevices();

    void configureAudio(std::string deviceName);

protected:
    void disableAudio();
    void startAudio();
    void stopAudio();
    void abortAudio();
    void decodeFrame();
    bool fillBuffer();
    void streamSCast();
    struct impl;
    std::unique_ptr<impl> _impl;
};

}
