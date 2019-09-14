//---------------------------------------------------------------------------------------
//
// ghc::options - A simple C++ commandline parser class
//
//---------------------------------------------------------------------------------------
//
// Copyright (c) 2012019, Steffen Schümann <s.schuemann@pobox.com>
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

#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#ifndef _WIN32
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#endif

namespace ghc {
    
class options
{
private:
    using handler_type = std::function<void(const std::string&)>;
    struct option_info
    {
        enum class type { no_arg, arg_optional, arg_needed };
        std::string _name;
        handler_type _handler;
        std::weak_ptr<option_info> _master;
        type _type;
        std::string _description;
        std::vector<std::string> _variants;
        option_info(const std::string& option, const std::string description, type t, handler_type handler)
        : _name(option), _handler(handler), _type(t), _description(description) {}
    };
public:
    options(const int argc, char** argv, size_t offset = 1)
    : _arg0(argv[0])
    , _cmdargs(argv + offset, argv + argc)
    {
    }
    
    // short option: -x
    // long option: --xaver
    // for optional argument append ?
    // for needed argument append !
    void onOpt(std::string option, const std::string& description, handler_type handler)
    {
        auto optinfo = create_option(option, description, handler);
        _options.insert(std::make_pair(optinfo->_name, optinfo));
    }
    
    void onOpt(std::initializer_list<std::string> options, const std::string& description, handler_type handler)
    {
        std::shared_ptr<option_info> master;
        for (auto option : options) {
            auto optinfo = create_option(option, description, handler);
            if (master) {
                optinfo->_master = master;
                master->_variants.push_back(optinfo->_name);
            }
            else {
                master = optinfo;
            }
            _options.insert(std::make_pair(optinfo->_name, optinfo));
        }
    }
    
    void onPositional(const std::string& description, handler_type handler)
    {
        _positional_description = description;
        _positional_handler = handler;
    }
    
    void onMissingArg(handler_type handler)
    {
        _missing_argument_hamdler = handler;
    }
    
    void onUnknownOpt(handler_type handler)
    {
        _unknown_option_handler = handler;
    }
    
    void usage(std::ostream& stream, int parameter_width = 16) const
    {
        stream << "\nUSAGE: " << _arg0 << " [options]" << std::endl;
        stream << std::endl;
        for (auto pair : _options) {
            if (pair.second->_master.lock() == pair.second) {
                std::string description, parameter;
                auto pos = pair.second->_description.find('\t');
                if (pos != std::string::npos) {
                    parameter = pair.second->_description.substr(0, pos);
                    description = pair.second->_description.substr(pos+1);
                }
                else {
                    description = pair.second->_description;
                }
                bool first = true;
#ifndef _WIN32
                //stream << "\033[1m";
#endif
                for (auto name : pair.second->_variants) {
                    if (!first) {
                        stream << name << " " << parameter << std::endl;
                    }
                    else {
                        first = false;
                    }
                }
                stream << std::left << std::setw(parameter_width) << (pair.first + " " + parameter);
                auto descLines = lines(description, terminalWidth() - parameter_width);
                auto firstDesc = descLines.front();
                descLines.erase(descLines.begin());
                if (pair.first.length() + parameter.length() + 2 >= parameter_width) {
                        stream << std::endl << std::string(parameter_width, ' ');
                }
#ifndef _WIN32
                //stream << "\033[22m";
#endif
                stream << firstDesc << std::endl;
                for(const auto& line : descLines) {
                    stream << std::string(parameter_width, ' ') << line << std::endl;
                }
                stream << std::endl;
            }
        }
    }
    
    void parse()
    {
        _argiter = _cmdargs.begin();
        nextArg(0);
        if (_argiter != _cmdargs.end()) {
            while (!_current.empty() && _argiter != _cmdargs.end()) {
                if (!_current.empty() && _current[0] == '-') {
                    bool found = false;
                    for (auto pair : _options) {
                        if (checkOption(*pair.second)) {
                            pair.second->_handler(consumeOption(*pair.second));
                            found = true;
                        }
                    }
                    if (!found) {
                        if (_unknown_option_handler) {
                            _unknown_option_handler(_current);
                        }
                        else {
                            throw std::runtime_error("Unknown option: " + _current);
                        }
                    }
                }
                else if (_positional_handler) {
                    _positional_handler(_current);
                    nextArg();
                }
                else {
                    throw std::runtime_error("Bad argument: " + _current);
                }
            }
        }
    }
    
private:
    static int terminalWidth()
    {
#ifndef _WIN32
        struct ::winsize w;
        ::ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return w.ws_col >= 20 ? w.ws_col : 80;
#else
        return 80;
#endif
    }

    static std::vector<std::string> lines(const std::string& text, int width)
    {
        std::string::const_iterator iter = text.begin();
        std::string::const_iterator end = text.end();
        std::vector<std::string> lines;
        std::string line;
        int lineLength = 0, wordLength = 0;
        line.reserve(width);
        while(iter != end) {
            std::string word;
            while(iter != end && std::isspace(*iter) && *iter != '\n') {
                ++iter;
            }
            auto wordStart = iter;
            while(iter != end && !std::isspace(*iter)) {
                ++iter;
                ++wordLength;
            }
            if(wordLength) {
                word.assign(wordStart, iter);
                if(lineLength + wordLength + 1 < width) {
                    line += word + " ";
                    lineLength += wordLength + 1;
                }
                else {
                    lines.push_back(line);
                    line = word + " ";
                    lineLength = wordLength + 1;
                }
                wordLength = 0;
            }
            if(*iter == '\n') {
                lines.push_back(line);
                ++iter;
                line.clear();
                lineLength = 0;
            }
        }
        if(lineLength) {
            lines.push_back(line);
        }
        return lines;
    }
    
    static bool starts_with(const std::string& str, const std::string& prefix)
    {
        if(prefix.length() > str.length()) { return false; }
        return std::equal(prefix.begin(), prefix.end(), str.begin());
    }

    static bool ends_with(const std::string& str, const std::string& suffix)
    {
        if(suffix.length() > str.length()) { return false; }
        return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
    }

    static bool equals_i(const char* str1, const char* str2)
    {
        while (std::tolower((unsigned char)*str1) == std::tolower((unsigned char)*str2++)) {
            if (*str1++ == 0) {
                return true;
            }
        }    
        return false;
    }

    bool is_short_option(const std::string& option)
    {
        return option.length() >= 2 && option[0] == '-' && option[1] != '-';
    }
    
    bool checkOption(const option_info& optinfo)
    {
        return optinfo._name[1] == '-' ? _current == optinfo._name : starts_with(_current, optinfo._name);
    }
    
    void nextArg(size_t skip = 1)
    {
        while (skip && _argiter != _cmdargs.end()) {
            ++_argiter;
            --skip;
        }
        if (_argiter != _cmdargs.end()) {
            _current = *_argiter;
            _next = (_argiter+1) != _cmdargs.end() ? *(_argiter+1) : "";
        }
        else {
            _current.clear();
            _next.clear();
        }
    }
    
    std::string consumeOption(const option_info& optinfo)
    {
        std::string arg;
        switch (optinfo._type) {
            case option_info::type::no_arg:
                if (is_short_option(_current) && _current.length()>2) {
                    _current.erase(1,1);
                }
                else {
                    nextArg();
                }
                break;
            case option_info::type::arg_optional:
                if (is_short_option(_current) && _current.length() > 2) {
                        arg = _current.substr(2);
                        nextArg();
                }
                else if (!_next.empty() && _next[0] != '-') {
                    arg = _next;
                    nextArg(2);
                }
                else {
                    nextArg();
                }
                break;
            case option_info::type::arg_needed:
                if (is_short_option(_current) && _current.length() > 2) {
                    arg = _current.substr(2);
                    nextArg();
                }
                else if (!_next.empty() && _next[0] != '-') {
                    arg = _next;
                    nextArg(2);
                }
                else {
                    if (_missing_argument_hamdler) {
                        _missing_argument_hamdler(optinfo._name);
                    }
                    else {
                        throw std::runtime_error("Missing argument for option: " + optinfo._name);
                    }
                }
                break;
        }
        return arg;
    }
    
    std::shared_ptr<option_info> create_option(std::string option, const std::string& description, std::function<void(const std::string&)> handler) const
    {
        option_info::type type = option_info::type::no_arg;
        if (option.length() > 2) {
            if (ends_with(option, " ")) {
                option.erase(option.length()-1);
            }
            else if (ends_with(option, "?")) {
                type = option_info::type::arg_optional;
                option.erase(option.length()-1);
            }
            else if (ends_with(option, "!")) {
                type = option_info::type::arg_needed;
                option.erase(option.length()-1);
            }
        }
        auto optinfo = std::make_shared<option_info>(option, description, type, handler);
        optinfo->_master = optinfo;
        optinfo->_variants.push_back(optinfo->_name);
        return optinfo;
    }
    
    std::string _arg0;
    std::string _current;
    std::string _next;
    std::vector<std::string>::const_iterator _argiter;
    std::vector<std::string> _cmdargs;
    std::map<std::string, std::shared_ptr<option_info>> _options;
    handler_type _missing_argument_hamdler;
    handler_type _unknown_option_handler;
    handler_type _positional_handler;
    std::string _positional_description;
};

}
