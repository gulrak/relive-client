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

#include <backend/netutility.hpp>

#ifdef __APPLE__
#define MA_NO_RUNTIME_LINKING
#endif
#define MA_NO_WAV
#define MA_NO_MP3
#define MA_NO_FLAC
#define MINIAUDIO_IMPLEMENTATION
#include <mackron/miniaudio.h>

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

extern "C" {
void playStreamCallback(ma_device* pDevice, void* output, const void* /*input*/, ma_uint32 frameCount)
{
    auto* player = static_cast<relive::Player*>(pDevice->pUserData);
    player->playMusic((unsigned char*)output, static_cast<int>(frameCount));
}

void streamStoppedCallback(ma_device* pDevice)
{
    auto* player = static_cast<relive::Player*>(pDevice->pUserData);
    player->streamStopped();
}
}

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
    std::string _currentDeviceName;
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
    ma_context _maContext;
    ma_device _maDevice;
    std::atomic<PlayerState> _state;
    enum BackendState { eUninitialized, eInitialized, eReadyForPlayback };
    std::atomic<BackendState> _backendState;
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
        , _sampleBuffer(16 * 1024)
        , _chunk(_chunkSize)
        , _state(ePAUSED)
        , _progress(0)
        , _backendState(eUninitialized)
    {
        mp3dec_init(&_mp3d);
    }
};

Player::Player()
    : _impl(std::make_unique<impl>(this))
{
    ma_result rc = 0;
    if ((rc = ma_context_init(NULL, 0, NULL, &_impl->_maContext)) != MA_SUCCESS) {
        ERROR_LOG(0, "Error while initializing miniaudio context: " << rc);
    }
    else {
        _impl->_backendState = impl::eInitialized;
    }
    configureAudio(getDynamicDefaultOutputName());
}

Player::~Player()
{
    _impl->_isRunning = false;
    _impl->_worker.join();
    disableAudio();
    ma_context_uninit(&_impl->_maContext);
}

void Player::configureAudio(std::string deviceName)
{
    static bool firstTime = true;
    if(!_impl->_isRunning || _impl->_backendState == impl::eUninitialized) return;
    DEBUG_LOG(1, "Configuring audio output...");
    std::scoped_lock lock{_impl->_mutex};
    if(firstTime) {
        firstTime = false;
    }
    else {
        disableAudio();
    }
    _impl->_currentDeviceName = deviceName;
    ma_result rc = 0;
    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;
    if (ma_context_get_devices(&_impl->_maContext, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) == MA_SUCCESS) {
        for(int retry = 0; retry < 2; ++retry) {
            for(size_t i = 0; i < playbackCount; ++i) {
                if ((pPlaybackInfos[i].isDefault && deviceName == getDynamicDefaultOutputName()) || deviceName == pPlaybackInfos[i].name) {
                    ma_device_config config = ma_device_config_init(ma_device_type_playback);
                    config.playback.pDeviceID = &pPlaybackInfos[i].id;  // &pPlaybackInfos[chosenPlaybackDeviceIndex].id;
                    config.playback.format = ma_format_s16;
                    config.playback.channels = 2;
                    config.sampleRate = _impl->_frameRate;
                    config.dataCallback = &playStreamCallback;
                    config.stopCallback = &streamStoppedCallback;
                    config.pUserData = this;
                    if ((rc = ma_device_init(&_impl->_maContext, &config, &_impl->_maDevice)) != MA_SUCCESS) {
                        ERROR_LOG(0, "Error while initializing device: " << rc);
                        _impl->_backendState = impl::eInitialized;
                    }
                    else {
                        DEBUG_LOG(1, "Configured audio device: " << _impl->_maDevice.playback.name);
                        _impl->_backendState = impl::eReadyForPlayback;
                        return;
                    }
                }
            }
            deviceName = getDynamicDefaultOutputName();
        }
    }
    else {
        ERROR_LOG(0, "Couldn't enumaerate devices with miniaudio.");
        _impl->_backendState = impl::eInitialized;
    }
}

void Player::disableAudio()
{
    DEBUG_LOG(1, "Disabling audio output");
    _impl->_isPlaying = false;
    _impl->_state = ePAUSED;
    stopAudio();
    std::scoped_lock lock{_impl->_mutex};
    ma_device_uninit(&_impl->_maDevice);
}

void Player::streamStopped()
{
    PlayerState state = ePLAYING;
    if(_impl->_state.compare_exchange_strong(state, eERROR)) {
        DEBUG_LOG(3, "Stream stopped...");
    }
}

void Player::startAudio()
{
    std::scoped_lock lock{_impl->_mutex};
    if(ma_device_start(&_impl->_maDevice) != MA_SUCCESS) {
        ERROR_LOG(0, "Error starting miniaudio device.");
    }
}

void Player::stopAudio()
{
    // std::cout << "stop audio start" << std::endl;
    std::scoped_lock lock{_impl->_mutex};
    if(_impl->_maDevice.state != MA_STATE_STOPPED) {
        if (ma_device_stop(&_impl->_maDevice) != MA_SUCCESS) {
            ERROR_LOG(0, "Error stopping miniaudio device.");
        }
    }
    // std::cout << "stop audio end" << std::endl;
}

void Player::abortAudio()
{
    // std::cout << "stop audio start" << std::endl;
    stopAudio();
    // std::cout << "stop audio end" << std::endl;
}

void Player::play()
{
    if (hasSource()) {
        PlayerState state;
        {
            std::scoped_lock lock{_impl->_mutex};
            state = _impl->_state;
        }
        if (state == eENDOFSTREAM) {
            seekTo(0);
        }
        std::scoped_lock lock{_impl->_mutex};
        _impl->_isPlaying = true;
        _impl->_state = ePLAYING;
        startAudio();
    }
}

void Player::pause()
{
    // std::cout << "begin pause" << std::endl;
    std::scoped_lock lock{_impl->_mutex};
    _impl->_isPlaying = false;
    _impl->_state = ePAUSED;
    stopAudio();
    // std::cout << "end pause" << std::endl;
}

PlayerState Player::state() const
{
    std::scoped_lock lock{_impl->_mutex};
    return _impl->_state;
}

bool Player::hasSource() const
{
    std::scoped_lock lock{_impl->_mutex};
    return !_impl->_source.empty();
}

int Player::playTime() const
{
    std::scoped_lock lock{_impl->_mutex};
    return int(_impl->_playPosition / _impl->_frameRate);
}

void Player::seekTo(int seconds, bool startPlay)
{
    std::scoped_lock lock{_impl->_mutex};
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

float Player::receiveBufferQuote() const
{
    return float(_impl->_receiveBuffer.filled()) / _impl->_receiveBuffer.bufferSize();
}

float Player::decodeBufferQuote() const
{
    return float(_impl->_sampleBuffer.filled()) / _impl->_sampleBuffer.bufferSize();
}

void Player::prev()
{
    std::scoped_lock lock{_impl->_mutex};
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
    std::scoped_lock lock{_impl->_mutex};
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
    std::scoped_lock lock{_impl->_mutex};
    return _impl->_volume;
}

void Player::volume(int vol)
{
    std::scoped_lock lock{_impl->_mutex};
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
#ifdef TRACY_ENABLED
    tracy::SetThreadName("Player");
#endif
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
    auto lastDeviceCheck = std::chrono::steady_clock::now();
    while (_impl->_isRunning) {
        auto now = std::chrono::steady_clock::now();
        if(_impl->_state == eERROR) {
            configureAudio(getDynamicDefaultOutputName());
        }
        if(now - lastDeviceCheck > std::chrono::milliseconds(1000)) {
            lastDeviceCheck = now;
            auto currentDefault = getCurrentDefaultOutputName();
            bool changed = false;
            {
                std::scoped_lock lock{_impl->_mutex};
                changed = _impl->_currentDeviceName == getDynamicDefaultOutputName() && currentDefault != _impl->_maDevice.playback.name;
            }
            if(changed) {
                configureAudio(getDynamicDefaultOutputName());
            }
        }
        if (_impl->_isPlaying) {
            if (_impl->_state != eENDOFSTREAM) {
                if (_impl->_receiveBuffer.free() > _impl->_chunkSize) {
                    if (!fillBuffer()) {
                        std::this_thread::sleep_for(500ms);
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
    std::scoped_lock lock{_impl->_mutex};
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
        case eMediaStream:
        case eSCastStream:
            _impl->_session = createClient(source);
            break;
        default:
            break;
    }
    configureAudio(_impl->_currentDeviceName);
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
            std::scoped_lock lock{_impl->_mutex};
            _impl->_streamInfo = std::make_shared<Stream>(stream);
        }
    }
}

void Player::setSource(const Track& track)
{
    if (track._stream && track._stream->_station && !track._stream->_station->_api.empty()) {
        setSource(*track._stream);
        {
            std::scoped_lock lock{_impl->_mutex};
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
    std::scoped_lock lock{_impl->_mutex};
    return _impl->_streamInfo;
}

bool Player::fillBuffer()
{
    ZoneScopedN("fillBuffer");
    switch (_impl->_mode) {
        case eNone:
            break;
        case eFile: {
            fs::path file;
            int64_t offset;
            {
                std::scoped_lock lock{_impl->_mutex};
                file = _impl->_source.request_path();
                offset = _impl->_offset;
            }
            if (fs::exists(file)) {
                fs::ifstream is(file);
                if (is.seekg(offset)) {
                    is.read(_impl->_chunk.data(), _impl->_chunkSize);
                    if (is.gcount()) {
                        std::scoped_lock lock{_impl->_mutex};
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
                std::scoped_lock lock{_impl->_mutex};
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
                        std::scoped_lock lock{_impl->_mutex};
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
                        DEBUG_LOG(3, "Metadata-length: " << block);
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
                            DEBUG_LOG(3, "Metadata: " << " [" << block << (inMetaData ? "M" : "-") << "]");
                            metaData.append(data, data_length);
                            data += data_length;
                            block -= static_cast<size_t>(data_length);
                            data_length = 0;
                        }
                        else {
                            metaData.append(data, block);
                            DEBUG_LOG(3, "Metadata: " << heuristicUtf8(metaData.c_str())  << " [" << metaData.size() << " Bytes]");
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
    ZoneScopedN("decodeFrame");
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
    FrameMarkStart("playMusic");
    auto requestedTime = frames * 1000 / _impl->_frameRate;
    //--std::clog << "play " << frames << " (~" << requestedTime << "ms), sample buffer contains " << (_impl->_sampleBuffer.filled() / _impl->_numChannels) << std::endl;
    auto start = std::chrono::steady_clock::now();
    auto* dst = (SampleType*)buffer;
    bool didDecode = false;
    if (_impl->_state != eENDOFSTREAM && _impl->_state != eERROR) {
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
                for(int i = 0; i < 3; ++i) decodeFrame();
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
    FrameMarkEnd("playMusic");
    //--auto dt = std::chrono::steady_clock::now() - start;
    //--std::cout << "playMusic: " << std::chrono::duration_cast<std::chrono::microseconds>(dt).count() << "us, " << zeros << " frames silence" << std::endl;
}

std::string Player::getDynamicDefaultOutputName()
{
    return "[System Default]";
}

std::string Player::getCurrentDefaultOutputName() const
{
    ZoneScopedN("getCurrentDefaultOutputName");
    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;
    std::string fallback;
    if (ma_context_get_devices(&_impl->_maContext, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) == MA_SUCCESS) {
        for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
            if(pPlaybackInfos[iDevice].isDefault) {
                return pPlaybackInfos[iDevice].name;
            }
            if(fallback.empty() && pPlaybackInfos[iDevice].minChannels >= 2) {
                fallback = pPlaybackInfos[iDevice].name;
            }
        }
    }
    else {
        ERROR_LOG(0, "Couldn't enumaerate devices with miniaudio.");
    }
    return fallback;
}

std::vector<Player::Device> Player::getOutputDevices()
{
    std::vector<Device> result;
    std::scoped_lock lock{_impl->_mutex};
    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;
    result.push_back(Device{getDynamicDefaultOutputName(), 2, 41000});
    if (ma_context_get_devices(&_impl->_maContext, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) == MA_SUCCESS) {
        for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
            result.push_back(Device{pPlaybackInfos[iDevice].name, pPlaybackInfos[iDevice].maxChannels, pPlaybackInfos[iDevice].maxSampleRate});
            //printf("%d - [%s] %s\n", iDevice, pPlaybackInfos[iDevice].id.coreaudio, pPlaybackInfos[iDevice].name);
        }
    }
    else {
        ERROR_LOG(0, "Couldn't enumaerate devices with miniaudio.");
    }
    return result;
}

}
