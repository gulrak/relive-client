//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#include "player.hpp"
#include "logging.hpp"
#include "ringbuffer.hpp"
#include "system.hpp"

namespace fs = ghc::filesystem;

#define MINIMP3_ONLY_MP3
#define MINIMP3_NONSTANDARD_BUT_LOGICAL
#define MINIMP3_IMPLEMENTATION
#include <minimp3.h>

#ifdef RELIVE_RTAUDIO_BACKEND
#include <RtAudio.h>
#elif defined(RELIVE_MINIAUDIO_BACKEND)
#ifdef __APPLE__
#define MA_NO_RUNTIME_LINKING
#endif
#define MA_NO_WAV
#define MA_NO_MP3
#define MA_NO_FLAC
#define MINIAUDIO_IMPLEMENTATION
#include <mackron/miniaudio.h>
#elif defined(RELIVE_PORTAUDIO_BACKEND)
#include <portaudio.h>
#else
#error "No audio backend selected, set either RELIVE_RTAUDIO_BACKEND or RELIVE_PORTAUDIO_BACKEND!"
#endif

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#ifdef RELIVE_RTAUDIO_BACKEND
int playStreamCallback(void* output, void* /*input*/, unsigned int frameCount, double /*streamTime*/, RtAudioStreamStatus /*status*/, void* userData)
{
    auto* player = static_cast<relive::Player*>(userData);
    player->playMusic((unsigned char*)output, frameCount);
    return 0;
}
#elif defined(RELIVE_MINIAUDIO_BACKEND)
extern "C" {
void playStreamCallback(ma_device* pDevice, void* output, const void* /*input*/, ma_uint32 frameCount)
{
    auto* player = static_cast<relive::Player*>(pDevice->pUserData);
    player->playMusic((unsigned char*)output, static_cast<int>(frameCount));
}

void streamStoppedCallback(ma_device* pDevice)
{
    auto* player = static_cast<relive::Player*>(pDevice->pUserData);

}
}
#elif defined(RELIVE_PORTAUDIO_BACKEND)
extern "C" {
int playStreamCallback(const void* /*input*/, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* /*timeInfo*/, PaStreamCallbackFlags /*statusFlags*/, void* userData)
{
    auto* player = static_cast<relive::Player*>(userData);
    player->playMusic((unsigned char*)output, frameCount);
    //std::clog << "callback" << std::endl;
    return 0;
}
}
#endif

namespace relive {

using SampleType = short;

struct Player::impl
{
    std::recursive_mutex _mutex;
    std::atomic_bool _isRunning;
    std::atomic_bool _isPlaying;
    std::atomic_bool _needsRefresh;
    std::thread _worker;
    Mode _mode = eNone;
    ghc::net::uri _source;
    std::shared_ptr<httplib::Client> _session;
    std::shared_ptr<Stream> _streamInfo;
    int64_t _offset = 0;          // fetch offset in stream (bytes)
    int64_t _decodePosition = 0;  // decode offset in stream (bytes)
    int64_t _playPosition = 0;    // play offset in stream (sample frames)
    int64_t _size = 0;
    int _chunkSize = 128 * 1024;
    int _frameRate = 44100;
    int _numChannels = 2;
    int _volume = 75;
    RingBuffer<char> _receiveBuffer;
    RingBuffer<SampleType> _sampleBuffer;
    std::vector<char> _chunk;
    mp3dec_t _mp3d;
    mp3dec_frame_info_t _mp3info;
#ifdef RELIVE_RTAUDIO_BACKEND
    RtAudio _dac;
#elif defined(RELIVE_MINIAUDIO_BACKEND)
    ma_context _maContext;
    ma_device _maDevice;
#elif defined(RELIVE_PORTAUDIO_BACKEND)
    PaStream* _paStream;
#endif
    PlayerState _state;
    int _progress;

    impl(Player* player)
        : _isRunning(true)
        , _isPlaying(false)
        , _worker(&Player::run, player)
        , _offset(0)
        , _decodePosition(0)
        , _playPosition(0)
        , _size(0)
        , _chunkSize(128 * 1024)
        , _frameRate(44100)
        , _numChannels(2)
        , _volume(75)
        , _receiveBuffer(1024 * 1024)
        , _sampleBuffer(256 * 1024)
        , _chunk(_chunkSize)
#ifdef RELIVE_PORTAUDIO_BACKEND
        , _paStream(nullptr)
#endif
        , _state(ePAUSED)
        , _progress(0)
    {
        mp3dec_init(&_mp3d);
    }
};

Player::Player()
    : _impl(std::make_unique<impl>(this))
{
#ifdef RELIVE_RTAUDIO_BACKEND
    _impl->_dac.showWarnings(false);
#endif
#ifdef RELIVE_PORTAUDIO_BACKEND
#ifdef __linux__
    freopen("/dev/null", "w", stderr);
#endif
    PaError err = Pa_Initialize();
#ifdef __linux__
    freopen("/dev/tty", "w", stderr);
#endif
    if (err != paNoError) {
        ERROR_LOG(0, "Couldn't initalize PortAudio: " << Pa_GetErrorText(err));
        throw std::runtime_error(std::string("Couldn't initalize PortAudio: ") + Pa_GetErrorText(err));
    }
#endif
    configureAudio();
}

Player::~Player()
{
    {
        std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
        disableAudio();
        _impl->_isRunning = false;
    }
    _impl->_worker.join();
}

void Player::configureAudio()
{
    static bool firstTime = true;
    DEBUG_LOG(1, "Configuring audio output...");
    if(firstTime) {
        firstTime = false;
    }
    else {
        disableAudio();
    }
#ifdef RELIVE_RTAUDIO_BACKEND
    RtAudio::StreamParameters parameters;
    parameters.deviceId = _impl->_dac.getDefaultOutputDevice();
    parameters.nChannels = _impl->_numChannels;
    parameters.firstChannel = 0;
    unsigned int bufferFrames = _impl->_frameRate;
    try {
        _impl->_dac.openStream(&parameters, NULL, RTAUDIO_SINT16, _impl->_frameRate, &bufferFrames, &playStreamCallback, this);
        //--std::cout << "Buffer choosen: " << bufferFrames << ", latency: " << _impl->_dac.getStreamLatency() <<  std::endl;
    }
    catch (RtAudioError& e) {
        ERROR_LOG(0, "Error while initializing audio: " << e.getMessage());
    }
#elif defined(RELIVE_MINIAUDIO_BACKEND)
    ma_result rc = 0;
    if ((rc = ma_context_init(NULL, 0, NULL, &_impl->_maContext)) != MA_SUCCESS) {
        ERROR_LOG(0, "Error while initializing miniaudio context: " << rc);
    }
    else {
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.pDeviceID = nullptr;  // &pPlaybackInfos[chosenPlaybackDeviceIndex].id;
        config.playback.format = ma_format_s16;
        config.playback.channels = 2;
        config.sampleRate = _impl->_frameRate;
        config.dataCallback = &playStreamCallback;
        config.pUserData = this;
        if ((rc = ma_device_init(&_impl->_maContext, &config, &_impl->_maDevice)) != MA_SUCCESS) {
            ERROR_LOG(0, "Error while initializing device: " << rc);
        }
        else {
            DEBUG_LOG(1, "Configured audio device: " << _impl->_maDevice.playback.name);
        }
    }
#elif defined(RELIVE_PORTAUDIO_BACKEND)
    PaStreamParameters param;
    param.channelCount = _impl->_numChannels;
    param.device = Pa_GetDefaultOutputDevice();
    param.hostApiSpecificStreamInfo = NULL;
    param.suggestedLatency = 1.0;  // TODO: optimize
    param.sampleFormat = paInt16;
    int rc = 0;
    if ((rc = Pa_OpenStream(&_impl->_paStream, NULL, &param, _impl->_frameRate, paFramesPerBufferUnspecified, paNoFlag, playStreamCallback, this)) < 0) {
        ERROR_LOG(0, "Error while initializing audio: " << Pa_GetErrorText(rc));
    }
#endif
}

void Player::disableAudio()
{
    DEBUG_LOG(1, "Disabling audio output");
    _impl->_isPlaying = false;
    _impl->_state = ePAUSED;
    stopAudio();
#ifdef RELIVE_RTAUDIO_BACKEND
    if (_impl->_dac.isStreamOpen()) {
        _impl->_dac.closeStream();
    }
#elif defined(RELIVE_MINIAUDIO_BACKEND)
    ma_device_uninit(&_impl->_maDevice);
    ma_context_uninit(&_impl->_maContext);
#elif defined(RELIVE_PORTAUDIO_BACKEND)
    if (_impl->_paStream) {
        Pa_CloseStream(_impl->_paStream);
        _impl->_paStream = nullptr;
    }
#endif
}

void Player::startAudio()
{
#ifdef RELIVE_RTAUDIO_BACKEND
    try {
        _impl->_dac.startStream();
    }
    catch (RtAudioError& e) {
        ERROR_LOG(0, "Error while initializing audio: " << e.getMessage());
    }
#elif defined(RELIVE_MINIAUDIO_BACKEND)
    if(ma_device_start(&_impl->_maDevice) != MA_SUCCESS) {
        ERROR_LOG(0, "Error starting miniaudio device.");
    }
#elif defined(RELIVE_PORTAUDIO_BACKEND)
    auto rc = Pa_StartStream(_impl->_paStream);
    if (rc != paNoError) {
        ERROR_LOG(0, "Error while initializing audio: " << Pa_GetErrorText(rc));
    }
#endif
}

void Player::stopAudio()
{
    // std::cout << "stop audio start" << std::endl;
#ifdef RELIVE_RTAUDIO_BACKEND
    try {
        _impl->_dac.stopStream();
    }
    catch (RtAudioError& e) {
        ERROR_LOG(0, "Error while stopping audio: " << e.getMessage());
    }
#elif defined(RELIVE_MINIAUDIO_BACKEND)
    if(_impl->_maDevice.state != MA_STATE_STOPPED) {
        if (ma_device_stop(&_impl->_maDevice) != MA_SUCCESS) {
            ERROR_LOG(0, "Error stopping miniaudio device.");
        }
    }
#elif defined(RELIVE_PORTAUDIO_BACKEND)
    if (_impl->_paStream) {
        Pa_StopStream(_impl->_paStream);
        // Pa_AbortStream(_impl->_paStream);
    }
#endif
    // std::cout << "stop audio end" << std::endl;
}

void Player::abortAudio()
{
    // std::cout << "stop audio start" << std::endl;
#ifdef RELIVE_RTAUDIO_BACKEND
    try {
        _impl->_dac.abortStream();
    }
    catch (RtAudioError& e) {
        ERROR_LOG(0, "Error while stopping audio: " << e.getMessage());
    }
#elif defined(RELIVE_MINIAUDIO_BACKEND)
    stopAudio();
#elif defined(RELIVE_PORTAUDIO_BACKEND)
    if (_impl->_paStream) {
        // Pa_StopStream(_impl->_paStream);
        Pa_AbortStream(_impl->_paStream);
    }
#endif
    // std::cout << "stop audio end" << std::endl;
}

void Player::play()
{
    if (hasSource()) {
        PlayerState state;
        {
            std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
            state = _impl->_state;
        }
        if (state == eENDOFSTREAM) {
            seekTo(0);
        }
        std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
        _impl->_isPlaying = true;
        _impl->_state = ePLAYING;
        startAudio();
    }
}

void Player::pause()
{
    // std::cout << "begin pause" << std::endl;
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
    _impl->_isPlaying = false;
    _impl->_state = ePAUSED;
    stopAudio();
    // std::cout << "end pause" << std::endl;
}

PlayerState Player::state() const
{
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
    return _impl->_state;
}

bool Player::hasSource() const
{
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
    return !_impl->_source.empty();
}

int Player::playTime() const
{
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
    return int(_impl->_playPosition / _impl->_frameRate);
}

void Player::seekTo(int seconds, bool startPlay)
{
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
    abortAudio();
    if (_impl->_streamInfo) {
        auto tt = seconds > 0 ? (double)seconds - 0.05 : (double)seconds;
        _impl->_offset = (int64_t)(_impl->_streamInfo->_size * (tt / _impl->_streamInfo->_duration));
        _impl->_decodePosition = _impl->_offset;
        _impl->_playPosition = ((double)_impl->_streamInfo->_duration * _impl->_offset / _impl->_streamInfo->_size + 0.1) * _impl->_frameRate;
        _impl->_receiveBuffer.clear();
        _impl->_sampleBuffer.clear();
        if(startPlay) {
            play();
        }
    }
}

void Player::prev()
{
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
    if (_impl->_streamInfo) {
        const Track* prevTrack = nullptr;
        auto currentTime = playTime();
        for (const auto& track : _impl->_streamInfo->_tracks) {
            if (currentTime > 1 && track._time <= currentTime - 1) {
                prevTrack = &track;
            }
            else {
                break;
            }
        }
        if (prevTrack) {
            seekTo(prevTrack->_time);
        }
    }
}

void Player::next()
{
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
    if (_impl->_streamInfo) {
        int64_t nextPos = 0;
        auto currentTime = playTime();
        for (const auto& track : _impl->_streamInfo->_tracks) {
            if (track._time > currentTime) {
                seekTo(track._time);
                break;
            }
        }
    }
}

int Player::volume() const
{
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
    return _impl->_volume;
}

void Player::volume(int vol)
{
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
    if (vol < 0) {
        vol = 0;
    }
    else if (vol > 100) {
        vol = 100;
    }
    _impl->_volume = vol;
}

void Player::run()
{
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
    while (_impl->_isRunning) {
        if (_impl->_isPlaying) {
            if (_impl->_state != eENDOFSTREAM) {
                if (_impl->_receiveBuffer.free() > _impl->_chunkSize) {
                    if (!fillBuffer()) {
                        std::this_thread::sleep_for(1000ms);
                    }
                }
                else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(_impl->_receiveBuffer.filled() * 100 / _impl->_receiveBuffer.bufferSize() > 66 ? 250 : 100));
                }
            }
            else {
                std::this_thread::sleep_for(100ms);
            }
        }
        else {
            std::this_thread::sleep_for(100ms);
        }
    }
}

void Player::setSource(Mode mode, ghc::net::uri source, int64_t size)
{
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
    abortAudio();
    _impl->_mode = mode;
    _impl->_source = source;
    _impl->_offset = 0;
    _impl->_decodePosition = 0;
    _impl->_playPosition = 0;
    _impl->_size = size;
    _impl->_state = ePAUSED;
    _impl->_streamInfo.reset();
    _impl->_receiveBuffer.clear();
    _impl->_sampleBuffer.clear();
    switch (mode) {
        case eFile:
            _impl->_size = fs::file_size(_impl->_source.request_path());
            break;
        case eReLiveStream:
            _impl->_session = std::make_shared<httplib::Client>(source.host().c_str(), source.port());
            break;
        case eMediaStream:
            _impl->_session = std::make_shared<httplib::Client>(source.host().c_str(), source.port());
            break;
        case eSCastStream:
            _impl->_session = std::make_shared<httplib::Client>(source.host().c_str(), source.port());
            break;
        default:
            break;
    }
    configureAudio();
}

void Player::setSource(const Stream& stream)
{
    if (!stream._tracks.empty() && stream._station && !stream._station->_api.empty()) {
        auto station = stream._station;
        auto api = ghc::net::uri(station->_api.front());
        api.scheme("http");
        // api.path(api.path() + "getstreamdata/?v=11&streamid=" + std::to_string(stream._reliveId));
        api.path(api.path() + "getmediadata/?v=11&streamid=" + std::to_string(stream._reliveId));
        setSource(eReLiveStream, api, stream._size);
        {
            std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
            _impl->_streamInfo = std::make_shared<Stream>(stream);
        }
    }
}

void Player::setSource(const Track& track)
{
    if (track._stream && track._stream->_station && !track._stream->_station->_api.empty()) {
        setSource(*track._stream);
        {
            std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
            auto tt = track._time > 0 ? (double)track._time - 0.05 : (double)track._time;
            _impl->_offset = (int64_t)(track._stream->_size * (tt / track._stream->_duration));
            _impl->_decodePosition = _impl->_offset;
            _impl->_playPosition = static_cast<int64_t>(((double)track._stream->_duration * _impl->_offset / track._stream->_size + 0.1) * _impl->_frameRate);
            DEBUG_LOG(1, "New play position: " << _impl->_offset << "/" << _impl->_playPosition);
        }
    }
}

std::shared_ptr<Stream> Player::currentStream()
{
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
    return _impl->_streamInfo;
}

bool Player::fillBuffer()
{
    switch (_impl->_mode) {
        case eNone:
            break;
        case eFile: {
            fs::path file;
            int64_t offset;
            {
                std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
                file = _impl->_source.request_path();
                offset = _impl->_offset;
            }
            if (fs::exists(file)) {
                fs::ifstream is(file);
                if (is.seekg(offset)) {
                    is.read(_impl->_chunk.data(), _impl->_chunkSize);
                    if (is.gcount()) {
                        std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
                        if (_impl->_offset == offset) {
                            // add only if we have not changed offset due to seek/pause/change of stream
                            _impl->_receiveBuffer.push(_impl->_chunk.data(), is.gcount());
                            _impl->_offset += is.gcount();
                        }
                        DEBUG_LOG(2, "File: Pushed " << is.gcount() << " bytes into stream buffer");
                    }
                }
            }
            break;
        }
        case eReLiveStream: {
            httplib::Headers headers = {{"User-Agent", relive::userAgent()}};
            int64_t fetchSize;
            int64_t offset;
            std::string range;
            {
                std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
                offset = _impl->_offset;
                fetchSize = _impl->_size > offset ? (std::min)(_impl->_size - offset, static_cast<int64_t>(_impl->_chunkSize)) : 0;
                range = "&start=" + std::to_string(offset) + "&length=" + std::to_string(fetchSize);
            }
            if (fetchSize > 0) {
                std::string path = _impl->_source.request_path() + range;
                DEBUG_LOG(2, "Fetching eReLiveStream: " << path);
                auto res = _impl->_session->Get(path.c_str(), headers);
                if (res && res->status == 200) {
                    DEBUG_LOG(3, "reLiveStream: About to push " << res->body.size() << " bytes, (buffer has " << _impl->_receiveBuffer.free() << " bytes free)");
                    {
                        std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
                        if (_impl->_offset == offset) {
                            // add only if we have not changed offset due to seek/pause/change of stream
                            _impl->_receiveBuffer.push(res->body.data(), res->body.size());
                            _impl->_offset += res->body.size();
                        }
                    }
                    DEBUG_LOG(2, "reLiveStream: Pushed " << res->body.size() << " bytes into stream buffer");
                }
                else {
                    if (res) {
                        ERROR_LOG(1, "reLiveStream: fetch failed (" << res->status << ") for range " << range);
                    }
                    else {
                        ERROR_LOG(1, "reLiveStream: fetch failed for range " << range);
                    }
                }
            }
            break;
        }
        case eMediaStream: {
            auto range = "bytes=" + std::to_string(_impl->_offset) + "-" + std::to_string(_impl->_offset + _impl->_chunkSize - 1);
            httplib::Headers headers = {{"User-Agent", relive::userAgent()}, {"Range", range}};
            std::string path = _impl->_source.request_path();
            DEBUG_LOG(2, "Fetching eMediaStream: " << _impl->_source.str() << " - Range: " << range);
            auto res = _impl->_session->Get(path.c_str(), headers);
            if (res && res->status == 206) {
                _impl->_receiveBuffer.push(res->body.data(), res->body.size());
                _impl->_offset += res->body.size();
                DEBUG_LOG(2, "MediaStream: Pushed " << res->body.size() << " bytes into stream buffer");
            }
            else {
                if (res) {
                    ERROR_LOG(1, "MediaStream: fetch failed (" << res->status << ") for range " << range);
                }
                else {
                    ERROR_LOG(1, "MediaStream: fetch failed for range " << range);
                }
            }
            break;
        }
        case eSCastStream: {
            streamSCast();
        }
        default:
            break;
    }
    return false;
}

void Player::streamSCast()
{
    using namespace std::chrono_literals;
    httplib::Headers headers = {{"User-Agent", relive::userAgent()}, {"Icy-MetaData", "1"}};
    size_t size = 0;
    size_t metaint = 0;
    size_t block = 0;
    bool inMetaData = false;
    std::string metaData;
    DEBUG_LOG(2, "Starting eSCStream: " << _impl->_source.str());
    auto res = _impl->_session->Get(
        _impl->_source.request_path().c_str(), headers,
        [&](const httplib::Response& response) {
            metaint = httplib::detail::get_header_value_uint64(response.headers, "icy-metaint");
            block = metaint;
            if (response.status != 200 || metaint <= 0) {
                return false;
            }
            return true;
        },
        [&](const char* data, uint64_t data_length) {
            // std::clog << "Received " << data_length << " bytes, offset " << offset << " [" << block << (inMetaData ? "M" : "-") << "]" << std::endl;
            while (data_length) {
                if (inMetaData) {
                    if (!block) {
                        block = static_cast<unsigned char>(*data) * 16;
                        // std::clog << "Metadata-length: " << block << std::endl;
                        ++data;
                        --data_length;
                        if (!block) {
                            block = metaint;
                            inMetaData = false;
                        }
                        else {
                            metaData.clear();
                        }
                    }
                    else {
                        if (data_length < block) {
                            // std::clog << "Metadata: " << " [" << block << (inMetaData ? "M" : "-") << "]" << std::endl;
                            metaData.append(data, data_length);
                            data += data_length;
                            block -= static_cast<size_t>(data_length);
                            data_length = 0;
                        }
                        else {
                            metaData.append(data, block);
                            // std::clog << "MetaData: " << metaData.c_str()  << " [" << metaData.size() << " Bytes]" << std::endl;
                            data += block;
                            data_length -= block;
                            block = metaint;
                            inMetaData = false;
                        }
                    }
                }
                else {
                    if (data_length < block) {
                        while (_impl->_receiveBuffer.free() < data_length) {
                            // std::clog << "sleep" << std::endl;
                            std::this_thread::sleep_for(100ms);
                        }
                        _impl->_receiveBuffer.push(data, data_length);
                        DEBUG_LOG(2, "Received " << data_length << " stream bytes"
                                                 << " [" << block << (inMetaData ? "M" : "-") << "]");
                        size += data_length;
                        data += data_length;
                        block -= static_cast<size_t>(data_length);
                        data_length = 0;
                    }
                    else {
                        while (_impl->_receiveBuffer.free() < block) {
                            // std::clog << "sleep" << std::endl;
                            std::this_thread::sleep_for(100ms);
                        }
                        _impl->_receiveBuffer.push(data, block);
                        DEBUG_LOG(2, "Received " << block << " stream bytes");
                        size += block;
                        data += block;
                        data_length -= block;
                        block = 0;
                        inMetaData = true;
                    }
                }
            }
            return _impl->_isPlaying && _impl->_isRunning ? true : false;
        });
    pause();
}

#define BUFFER_PEEK_SIZE 4096u

void Player::decodeFrame()
{
    // std::clog << "decode..." << std::endl;
    if (_impl->_receiveBuffer.canPull(200) && _impl->_sampleBuffer.free() >= MINIMP3_MAX_SAMPLES_PER_FRAME) {
        // std::clog << "decode... (" << _impl->_receiveBuffer.filled() << " available)" << std::endl;
        short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        uint8_t mp3[BUFFER_PEEK_SIZE];
        auto avail = _impl->_receiveBuffer.filled();
        auto peekSize = (std::min)(avail, BUFFER_PEEK_SIZE);
        DEBUG_LOG(4, "decoding from " << avail << " buffered bytes");
        _impl->_receiveBuffer.peek((char*)mp3, BUFFER_PEEK_SIZE);
        int samples = mp3dec_decode_frame(&_impl->_mp3d, mp3, peekSize, pcm, &_impl->_mp3info);
        DEBUG_LOG(4, "dropping " << _impl->_mp3info.frame_bytes << " decoded or skipped bytes from receive buffer");
        _impl->_receiveBuffer.drop(_impl->_mp3info.frame_bytes);
        _impl->_decodePosition += _impl->_mp3info.frame_bytes;
        if (samples > 0 && _impl->_mp3info.frame_bytes > 0) {
            _impl->_sampleBuffer.push(pcm, samples * _impl->_mp3info.channels);
            DEBUG_LOG(4, "decoded " << _impl->_mp3info.frame_bytes << " bytes into " << samples << " samples (" << _impl->_mp3info.hz << "Hz)");
            //--std::clog << "Decoded " << _impl->_mp3info.frame_bytes << " bytes into " << samples << " samples (" << _impl->_mp3info.hz << "Hz)" << std::endl;
        }
        else {
            // std::clog << "No samples but " << _impl->_mp3info.frame_bytes << " frame bytes" << std::endl;
        }
    }
    // DEBUG_LOG(4, "decoded " << _impl->_decodePosition << "/" << _impl->_size << " bytes");
    if (_impl->_sampleBuffer.free() > 0 && _impl->_size && _impl->_decodePosition + 200 >= _impl->_size) {
        /*SampleType t[64];
        memset(t, 0, sizeof(t));
        while(_impl->_sampleBuffer.free() >= 64)
            _impl->_sampleBuffer.push(t, 64);*/
        _impl->_state = eENDING;
    }
}

void Player::playMusic(unsigned char* buffer, int frames)
{
    auto requestedTime = frames * 1000 / _impl->_frameRate;
    //--std::clog << "play " << frames << " (~" << requestedTime << "ms), sample buffer contains " << (_impl->_sampleBuffer.filled() / _impl->_numChannels) << std::endl;
    auto start = std::chrono::steady_clock::now();
    auto* dst = (SampleType*)buffer;
    bool didDecode = false;
    if (_impl->_state != eENDOFSTREAM) {
        if (!_impl->_receiveBuffer.filled()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(requestedTime / 2));
            //--std::clog << "Buffer after sleep: " << _impl->_receiveBuffer.filled() << " bytes" << std::endl;
        }
        else {
            //--std::clog << "Buffer filled with: " << _impl->_receiveBuffer.filled() << " bytes" << std::endl;
        }
        if (_impl->_sampleBuffer.filled() < (unsigned int)frames * _impl->_numChannels && _impl->_receiveBuffer.filled()) {
            // unsigned int lastFill;
            do {
                // lastFill = _impl->_sampleBuffer.filled();
                decodeFrame();
            } while (_impl->_sampleBuffer.filled() < (unsigned int)frames * _impl->_numChannels && _impl->_receiveBuffer.filled() &&
                     std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < requestedTime * 2 / 3);
        }
    }

    int zeros = 0;
    if (_impl->_state == ePAUSED || _impl->_state == eENDOFSTREAM) {
        for (; frames; --frames) {
            ++zeros;
            *dst++ = 0;
            if (_impl->_numChannels == 2)
                *dst++ = 0;
        }
    }
    else {
        int len = _impl->_sampleBuffer.pull(dst, frames * _impl->_numChannels);
        _impl->_playPosition += len / _impl->_numChannels;
        auto* ptr = (SampleType*)buffer;
        for (int i = 0; i < len; ++i, ++ptr) {
            *ptr = (SampleType)((((int)*ptr) * _impl->_volume) / 100);
        }
        dst += len;
        if (len < frames * _impl->_numChannels) {
            if (_impl->_state == eENDING) {
                DEBUG_LOG(3, "Stream play ended.");
                _impl->_state = eENDOFSTREAM;
            }
            while (len++ < frames * _impl->_numChannels) {
                ++zeros;
                *dst++ = 0;
            }
        }
    }
    //--auto dt = std::chrono::steady_clock::now() - start;
    //--std::cout << "playMusic: " << std::chrono::duration_cast<std::chrono::microseconds>(dt).count() << "us, " << zeros << " frames silence" << std::endl;
}

std::vector<Player::Device> Player::getOutputDevices()
{
    std::vector<Device> result;
    std::lock_guard<std::recursive_mutex> lock{_impl->_mutex};
#ifdef RELIVE_RTAUDIO_BACKEND
    unsigned int devices = _impl->_dac.getDeviceCount();
    RtAudio::DeviceInfo info;
    for (unsigned int i = 0; i < devices; ++i) {
        info = _impl->_dac.getDeviceInfo(i);
        if (info.probed && info.outputChannels >= 2) {
            result.push_back(Device{info.name, info.outputChannels, info.preferredSampleRate});
        }
    }
#elif defined(RELIVE_MINIAUDIO_BACKEND)
    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;
    if (ma_context_get_devices(&_impl->_maContext, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) == MA_SUCCESS) {
        for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
            result.push_back(Device{pPlaybackInfos[iDevice].name, pPlaybackInfos[iDevice].maxChannels, pPlaybackInfos[iDevice].maxSampleRate});
            //printf("%d - [%s] %s\n", iDevice, pPlaybackInfos[iDevice].id.coreaudio, pPlaybackInfos[iDevice].name);
        }
    }
    else {
        ERROR_LOG(0, "Couldn't enumaerate devices with miniaudio.");
    }
#endif
    return result;
}

}
