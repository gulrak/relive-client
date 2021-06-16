//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#pragma once

#include <memory>

#include <ghc/uri.hpp>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

inline std::shared_ptr<httplib::Client> createClient(const ghc::net::uri& uri)
{
    if (uri.scheme() == "https") {
        auto client = std::make_shared<httplib::SSLClient>(uri.host().c_str(), uri.port());
        client->enable_server_certificate_verification(false);
        return client;
    }
    return std::make_shared<httplib::Client>(uri.host().c_str(), uri.port());
}
