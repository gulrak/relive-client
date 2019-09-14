//---------------------------------------------------------------------------------------
//
// ghc::net::uri - A C++ class wrapping URLs/URIs
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
#ifndef GHC_URI_H
#define GHC_URI_H

#include <iomanip>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

#define GHC_INLINE inline

namespace ghc {
namespace net {

class uri_error : public std::runtime_error
{
public:
    uri_error(const std::string& msg)
        : std::runtime_error(msg)
    {
    }
};

class uri
{
public:
    explicit uri(const std::string& uriStr = std::string());
    uri(const std::string& scheme, const std::string& authority, const std::string& path, const std::string& query = std::string(), const std::string& fragment = std::string());
    virtual ~uri();

    uri& operator=(const uri&) = default;

    const std::string& scheme() const;
    void scheme(const std::string& value);

    std::string authority() const;
    void authority(const std::string& value);

    const std::string& user_info() const;
    void user_info(const std::string& username, const std::string& passwd);

    const std::string& host() const;
    void host(const std::string& value);

    uint16_t port() const;
    void port(uint16_t value);

    const std::string& path() const;
    void path(const std::string& value);

    const std::string& query() const;
    void query(const std::string& value);

    const std::string& fragment() const;
    void fragment(const std::string& value);

    std::string request_path() const;

    uint16_t well_known_port() const;

    std::string str() const;

    bool empty() const;

private:
    std::string _scheme;
    std::string _userInfo;
    std::string _host;
    uint16_t _port;
    std::string _path;
    std::string _query;
    std::string _fragment;
};

inline std::string encode_uri(const std::string& uri)
{
    std::ostringstream encoded;
    encoded.fill('0');
    encoded << std::hex;

    for (char c : uri) {
        if (::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        }
        else if (c == ' ') {
            encoded << '+';
        }
        else {
            encoded << '%' << std::setw(2) << ((int)c) << std::setw(0);
        }
    }

    return encoded.str();
}

inline std::string decode_uri(const std::string& uri)
{
    std::ostringstream decoded;
    const char* src = uri.c_str();
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (::isxdigit(a) && ::isxdigit(b))) {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            decoded << 16 * a + b;
            src += 3;
        }
        else if (*src == '+') {
            decoded << ' ';
            ++src;
        }
        else {
            decoded << *src++;
        }
    }
    return decoded.str();
}

GHC_INLINE uri::uri(const std::string& uriStr)
    : _port(0)
{
    static std::regex uriRe("^(?:([a-zA-Z][a-zA-Z0-9\\+\\-\\.]*):)?(?://([^/?#]*))?([^?#]*)(?:\\?([^#]*))?(?:#(.*))?");

    std::match_results<std::string::const_iterator> match;
    std::string decodedURI = decode_uri(uriStr);
    if (!std::regex_match(decodedURI, match, uriRe, std::regex_constants::match_default)) {
        throw uri_error("Invalid URI: '" + uriStr);
    }
    if (match[1].matched) {
        _scheme = match[1];
    }
    if (match[2].matched) {
        authority(match[2]);
    }
    if (match[3].matched) {
        _path = match[3];
    }
    if (match[4].matched) {
        _query = match[4];
    }
    if (match[5].matched) {
        _fragment = match[5];
    }
}

GHC_INLINE uri::uri(const std::string& scheme, const std::string& authority, const std::string& path, const std::string& query, const std::string& fragment)
    : _scheme(scheme)
    , _port(0)
    , _path(path)
    , _query(query)
    , _fragment(fragment)
{
    this->authority(authority);
}

GHC_INLINE uri::~uri() {}

GHC_INLINE const std::string& uri::scheme() const
{
    return _scheme;
}

GHC_INLINE void uri::scheme(const std::string& value)
{
    if(_scheme != value) {
        _scheme = value;
        std::transform(_scheme.begin(), _scheme.end(), _scheme.begin(), ::tolower);
        _port = well_known_port();
    }
}

GHC_INLINE std::string uri::authority() const
{
    std::stringstream result;
    if (!_userInfo.empty()) {
        result << _userInfo << "@";
    }
    if (!_host.empty()) {
        if (_host.find(':') != _host.npos) {
            result << "[" << _host << "]";
        }
        else {
            result << _host;
        }
    }
    if (_port && _port != well_known_port()) {
        result << ":" << _port;
    }
    return result.str();
}

GHC_INLINE void uri::authority(const std::string& value)
{
    auto userInfoEnd = value.find('@');
    if (userInfoEnd == value.npos) {
        userInfoEnd = 0;
    }
    auto portPos = value.find(':', userInfoEnd);
    if (userInfoEnd) {
        _userInfo = value.substr(0, userInfoEnd);
        ++userInfoEnd;
    }
    _host = value.substr(userInfoEnd, portPos);
    if (!_host.empty() && _host[0] == '[') {
        _host.erase(0, 1);
    }
    if (!_host.empty() && _host[_host.size() - 1] == ']') {
        _host.erase(_host.size() - 1);
    }
    if (portPos != value.npos) {
        _port = std::stoi(value.substr(portPos + 1));
    }
    else {
        _port = well_known_port();
    }
}

GHC_INLINE const std::string& uri::user_info() const
{
    return _userInfo;
}

GHC_INLINE void uri::user_info(const std::string& username, const std::string& passwd)
{
    _userInfo = username + ":" + passwd;
}

GHC_INLINE const std::string& uri::host() const
{
    return _host;
}

GHC_INLINE void uri::host(const std::string& value)
{
    _host = value;
}

GHC_INLINE uint16_t uri::port() const
{
    return _port;
}

GHC_INLINE void uri::port(uint16_t value)
{
    _port = value;
}

GHC_INLINE const std::string& uri::path() const
{
    return _path;
}

GHC_INLINE void uri::path(const std::string& value)
{
    _path = value;
}

GHC_INLINE const std::string& uri::query() const
{
    return _query;
}

GHC_INLINE void uri::query(const std::string& value)
{
    _query = value;
}

GHC_INLINE const std::string& uri::fragment() const
{
    return _fragment;
}

GHC_INLINE void uri::fragment(const std::string& value)
{
    _fragment = value;
}

GHC_INLINE std::string uri::str() const
{
    std::stringstream result;
    if (!_scheme.empty()) {
        result << _scheme << ":";
    }
    std::string authTemp = authority();
    if (!authTemp.empty()) {
        result << "//" << authTemp;
    }
    result << _path;
    if (!_query.empty()) {
        result << "?" << _query;
    }
    if (!_fragment.empty()) {
        result << "#" << fragment();
    }
    return result.str();
}

GHC_INLINE uint16_t uri::well_known_port() const
{
    const std::map<std::string, uint16_t> wellKnownPorts{{"http", 80}, {"https", 443}, {"ftp", 21}};
    if (!scheme().empty()) {
        auto kpi = wellKnownPorts.find(scheme());
        if (kpi != wellKnownPorts.end()) {
            return kpi->second;
        }
    }
    return 0;
}

GHC_INLINE std::string uri::request_path() const
{
    std::stringstream result;
    result << _path;
    if (!_query.empty()) {
        result << "?" << _query;
    }
    if (!_fragment.empty()) {
        result << "#" << fragment();
    }
    return result.str();
}

GHC_INLINE bool uri::empty() const
{
    return _scheme.empty() && authority().empty() && _path.empty() && _query.empty() && _fragment.empty();
}

}  // namespace net
}  // namespace ghc

#endif  // GHC_URI_H
