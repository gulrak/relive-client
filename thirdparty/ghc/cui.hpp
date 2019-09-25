//---------------------------------------------------------------------------------------
//
// ghc::cui - A C++ wrapper to ncurses for C++11/C++14/C++17
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

#ifndef _WIN32
#if defined(__APPLE__) && defined(__MACH__)
#include <ncurses.h>
#else
#include <ncursesw/ncurses.h>
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include <wchar.h>
#else
#include <curses.h>
#endif
#include <algorithm>
#include <backend/logging.hpp>
#include <cctype>
#include <clocale>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

#define GHC_INLINE inline

namespace ghc {
namespace cui {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class cui_error : public std::runtime_error
{
public:
    cui_error(const std::string& msg)
        : std::runtime_error(msg)
    {
    }
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct cell
{
    enum alignment { eLeft, eCenter, eRight };
    cell(alignment a, int w, int at, const std::string& txt)
        : align(a)
        , width(w)
        , attr(at)
        , text(txt)
    {
    }
    alignment align;
    int width;
    int attr;
    std::string text;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class window_base
{
public:
    window_base(int x, int y, int w, int h);
    virtual ~window_base();

    int width() const { return _width; }
    int height() const { return _height; }

    void clear();
    void print(int x, int y, const std::string& text, int attr = 0);
    void drawBox(int x, int y, int w, int h);
    void drawHLine(int x, int y, int w, chtype lch = 0, chtype rch = 0);
    void drawVLine(int x, int y, int h);
    void fill(char c);

protected:
    int _xpos = 0;
    int _ypos = 0;
    int _width = 0;
    int _height = 0;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class application;
class window : public window_base
{
public:
    window(int x, int y, int w, int h);
    ~window() override;

    void redraw() { on_redraw(); }

protected:
    virtual void on_idle() {}
    virtual void on_mouse(const MEVENT& event) {}
    virtual void on_resize(int w, int h) {}
    virtual void on_redraw() {}
    virtual void on_event(int event) {}

private:
    friend class application;
    void handle_resize(int w, int h);
};

using window_ptr = std::shared_ptr<window>;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class list_view : public window
{
public:
    class model
    {
    public:
        virtual ~model() {}
        virtual int size() const = 0;
        virtual std::vector<cell> line(int index, int width) const = 0;
        void select(int index) { _selected = index; }

    private:
        friend class list_view;
        mutable int _offset = 0;
        mutable int _selected = 0;
    };
    list_view(int x, int y, int w, int h, const model& viewModel);
    ~list_view() override;

    int selected();
    void select(int index);

protected:
    void on_redraw() override;
    void on_event(int event) override;
    void printCells(int y, const std::vector<cell>& cells, int attr = 0);

private:
    const model& _model;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class text_view : public window
{
public:
    text_view(int x, int y, int w, int h, const std::string& text, bool preformatted = false);
    ~text_view() override;

protected:
    void on_redraw() override;
    void on_event(int event) override;

private:
    const std::string& _text;
    int _offset = 0;
    int _textLines = 0;
    bool _preformatted;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class log_view : public window
{
public:
    class model
    {
    public:
        virtual ~model() {}
        virtual int size() const = 0;
        virtual std::vector<cell> line(int index, int width) const = 0;
        void position(int index) { _position = index; }

    private:
        friend class log_view;
        mutable int _offset = 0;
        mutable int _position = 0;
    };
    log_view(int x, int y, int w, int h, const model& viewModel);
    ~log_view() override;

protected:
    void on_redraw() override;
    void on_event(int event) override;

private:
    const model& _model;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class application : public window_base
{
public:
    application(int argc, char* argv[]);
    ~application() override;
    int run();
    void quit();

    void destroyWindow(window_ptr window);

    template <typename Window, typename... Args>
    typename std::enable_if<std::is_base_of<ghc::cui::window, Window>::value, typename std::shared_ptr<Window>>::type create_window(Args&&... args);

    static application* instance();

protected:
    void yield();
    virtual void on_idle() {}
    virtual void on_mouse(const MEVENT& event) {}
    virtual void on_redraw() {}
    virtual void on_resize(int w, int h) {}
    virtual void on_event(int event) {}
    virtual void on_init() {}
    virtual void on_exit() {}

private:
    void handle_resize();
    static application*& instance_pointer();
    mmask_t _oldMask = 0;
    volatile bool _quit = false;
    std::vector<std::weak_ptr<window>> _windows;
};

//-------------------------------------------------------------------------------------------------
//  Implementation
//-------------------------------------------------------------------------------------------------

namespace detail {
enum utf8_states_t { S_STRT = 0, S_RJCT = 8 };
GHC_INLINE unsigned consumeUtf8Fragment(const unsigned state, const uint8_t fragment, uint32_t& codepoint)
{
    static const uint32_t utf8_state_info[] = {
        // encoded states
        0x11111111u, 0x11111111u, 0x77777777u, 0x77777777u, 0x88888888u, 0x88888888u, 0x88888888u, 0x88888888u, 0x22222299u, 0x22222222u, 0x22222222u, 0x22222222u, 0x3333333au, 0x33433333u, 0x9995666bu, 0x99999999u,
        0x88888880u, 0x22818108u, 0x88888881u, 0x88888882u, 0x88888884u, 0x88888887u, 0x88888886u, 0x82218108u, 0x82281108u, 0x88888888u, 0x88888883u, 0x88888885u, 0u,          0u,          0u,          0u,
    };
    uint8_t category = fragment < 128 ? 0 : (utf8_state_info[(fragment >> 3) & 0xf] >> ((fragment & 7) << 2)) & 0xf;
    codepoint = (state ? (codepoint << 6) | (fragment & 0x3f) : (0xff >> category) & fragment);
    return state == S_RJCT ? static_cast<unsigned>(S_RJCT) : static_cast<unsigned>((utf8_state_info[category + 16] >> (state << 2)) & 0xf);
}

GHC_INLINE int characterWidth(std::uint32_t codepoint)
{
#ifndef _WIN32
    return ::wcwidth(static_cast<wchar_t>(codepoint));
#else
    return 1 + (codepoint >= 0x1100 && (codepoint <= 0x115f ||                                                                                                // Hangul Jamo init. consonants
                                        codepoint == 0x2329 || codepoint == 0x232a || (codepoint >= 0x2e80 && codepoint <= 0xa4cf && codepoint != 0x303f) ||  // CJK ... Yi
                                        (codepoint >= 0xac00 && codepoint <= 0xd7a3) ||                                                                       // Hangul Syllables
                                        (codepoint >= 0xf900 && codepoint <= 0xfaff) ||                                                                       // CJK Compatibility Ideographs
                                        (codepoint >= 0xfe10 && codepoint <= 0xfe19) ||                                                                       // Vertical forms
                                        (codepoint >= 0xfe30 && codepoint <= 0xfe6f) ||                                                                       // CJK Compatibility Forms
                                        (codepoint >= 0xff00 && codepoint <= 0xff60) ||                                                                       // Fullwidth Forms
                                        (codepoint >= 0xffe0 && codepoint <= 0xffe6) || (codepoint >= 0x20000 && codepoint <= 0x2fffd) || (codepoint >= 0x30000 && codepoint <= 0x3fffd)));
#endif
}

GHC_INLINE int utf8Increment(std::string::const_iterator& iter, const std::string::const_iterator& end)
{
    unsigned utf8_state = S_STRT;
    std::uint32_t codepoint = 0;
    while (iter != end) {
        if ((utf8_state = consumeUtf8Fragment(utf8_state, (uint8_t)*iter++, codepoint)) == S_STRT) {
            return characterWidth(codepoint);
        }
        else if (utf8_state == S_RJCT) {
            return 1;
        }
    }
    return 1;
}

GHC_INLINE int utf8Length(const std::string& utf8String)
{
    std::string::const_iterator iter = utf8String.begin();
    int length = 0;
    while (iter != utf8String.end()) {
        length += utf8Increment(iter, utf8String.end());
    }
    return length;
}

GHC_INLINE std::pair<std::string, int> utf8Substr(const std::string& utf8String, std::string::size_type from, std::string::size_type count = std::string::npos)
{
    std::string::const_iterator iter = utf8String.begin();
    while (from && iter != utf8String.end()) {
        from -= utf8Increment(iter, utf8String.end());
    }
    auto iter2 = iter;
    int length = 0;
    while (count && iter2 != utf8String.end()) {
        auto itemp = iter2;
        auto l = utf8Increment(iter2, utf8String.end());
        if (l > count) {
            iter2 = itemp;
            break;
        }
        count -= l;
        length += l;
    }
    return std::make_pair(std::string(iter, iter2), length);
}

GHC_INLINE std::vector<std::string> utf8Lines(const std::string& utf8String, int width)
{
    std::string::const_iterator iter = utf8String.begin();
    std::string::const_iterator end = utf8String.end();
    std::vector<std::string> lines;
    std::string line;
    int lineLength = 0, wordLength = 0;
    line.reserve(width);
    while (iter != end) {
        std::string word;
        while (iter != end && std::isspace(static_cast<unsigned char>(*iter)) && *iter != '\n') {
            ++iter;
        }
        auto wordStart = iter;
        while (iter != end && !std::isspace(static_cast<unsigned char>(*iter))) {
            wordLength += utf8Increment(iter, end);
        }
        if (wordLength) {
            word.assign(wordStart, iter);
            if (lineLength + wordLength + 1 < width) {
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
        if (*iter == '\n') {
            lines.push_back(line);
            ++iter;
            line.clear();
            lineLength = 0;
        }
    }
    if (lineLength) {
        lines.push_back(line);
    }
    return lines;
}

}  // namespace detail

GHC_INLINE window_base::window_base(int x, int y, int w, int h)
    : _xpos(x)
    , _ypos(y)
    , _width(w)
    , _height(h)
{
}

GHC_INLINE window_base::~window_base() {}

GHC_INLINE void window_base::clear()
{
    fill(' ');
}

GHC_INLINE void window_base::print(int x, int y, const std::string& text, int attr)
{
    if (attr) {
        attron(attr);
    }
    mvaddstr(_ypos + y, _xpos + x, text.c_str());
    if (attr) {
        attroff(attr);
    }
}

GHC_INLINE void window_base::drawBox(int x, int y, int w, int h)
{
    if (w >= 2 && h >= 2) {
        mvaddch(_ypos + y, _xpos + x, ACS_ULCORNER);
        mvaddch(_ypos + y, _xpos + x + w - 1, ACS_URCORNER);
        mvaddch(_ypos + y + h - 1, _xpos + x, ACS_LLCORNER);
        mvaddch(_ypos + y + h - 1, _xpos + x + w - 1, ACS_LRCORNER);
        drawHLine(x + 1, y, w - 2);
        drawHLine(x + 1, y + h - 1, w - 2);
        drawVLine(x, y + 1, h - 2);
        drawVLine(x + w - 1, y + 1, h - 2);
    }
}

GHC_INLINE void window_base::drawHLine(int x, int y, int w, chtype lch, chtype rch)
{
    mvhline(_ypos + y, _xpos + x, ACS_HLINE, w);
    if (lch) {
        mvaddch(_ypos + y, _xpos + x, lch);
    }
    if (rch) {
        mvaddch(_ypos + y, _xpos + x + w - 1, rch);
    }
}

GHC_INLINE void window_base::drawVLine(int x, int y, int h)
{
    mvvline(_ypos + y, _xpos + x, ACS_VLINE, h);
}

GHC_INLINE void window_base::fill(char c)
{
    auto fs = std::string(_width, c);
    for (int y = 0; y < _height; ++y) {
        print(0, y, fs);
    }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GHC_INLINE window::window(int x, int y, int w, int h)
    : window_base(x, y, w, h)
{
}

GHC_INLINE window::~window() {}

GHC_INLINE void window::handle_resize(int w, int h)
{
    on_resize(w, h);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GHC_INLINE list_view::list_view(int x, int y, int w, int h, const model& viewModel)
    : ghc::cui::window(x, y, w, h)
    , _model(viewModel)
{
}

GHC_INLINE list_view::~list_view() {}

GHC_INLINE int list_view::selected()
{
    return _model._selected;
}

GHC_INLINE void list_view::select(int index)
{
    if (!_model.size()) {
        _model._selected = -1;
    }
    _model._selected = index < _model.size() ? _model._selected : _model.size() - 1;
    redraw();
}

GHC_INLINE void list_view::printCells(int y, const std::vector<cell>& cells, int attr)
{
    print(0, y, std::string(width(), ' '), attr);
    if (!cells.empty()) {
        int x = 0;
        for (const auto& c : cells) {
            auto len = detail::utf8Length(c.text);
            std::string text = c.text;
            if (len > c.width) {
                std::tie(text, len) = detail::utf8Substr(text, 0, c.width);
            }
            switch (c.align) {
                case cell::eLeft:
                    if (c.width) {
                        print(x, y, text, c.attr | attr);
                    }
                    break;
                case cell::eCenter:
                    if (c.width) {
                        auto xoff = (c.width - len) / 2;
                        print(x + xoff, y, text, c.attr | attr);
                    }
                    break;
                case cell::eRight:
                    if (c.width) {
                        print(x + c.width - len, y, text, c.attr | attr);
                    }
                    break;
            }
            x += c.width + 1;
        }
    }
}

GHC_INLINE void list_view::on_redraw()
{
    clear();
    int size = _model.size();
    if (_model._selected >= size) {
        _model._selected = size - 1;
    }
    while (_model._selected > _model._offset + height() - 2) {
        ++_model._offset;
    }
    while (_model._selected < _model._offset && _model._offset > 0) {
        --_model._offset;
    }
    // fetch header
    auto cells = _model.line(-1, _width);
    printCells(0, cells, A_BOLD);
    for (int i = 0; i < _height - 1; ++i) {
        auto cells = _model.line(_model._offset + i, _width);
        if (_model._offset + i == _model._selected) {
            printCells(i + 1, cells, A_REVERSE);
        }
        else {
            printCells(i + 1, cells);
        }
    }
}

GHC_INLINE void list_view::on_event(int event)
{
    if (_model.size()) {
        switch (event) {
            case KEY_UP:
                if (_model._selected > 0) {
                    --_model._selected;
                }
                break;
            case KEY_DOWN:
                if (_model._selected < _model.size() - 1) {
                    ++_model._selected;
                }
                break;
            case KEY_PPAGE: {
                auto lines = (std::min)(height() - 2, _model._selected);
                if (lines > 0) {
                    _model._selected -= lines;
                }
                break;
            }
            case KEY_NPAGE: {
                auto lines = (std::min)(height() - 2, _model.size() - 1 - _model._selected);
                if (lines > 0) {
                    _model._selected += lines;
                }
                break;
            }
            default:
                break;
        }
        redraw();
    }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

text_view::text_view(int x, int y, int w, int h, const std::string& text, bool preformatted)
    : ghc::cui::window(x, y, w, h)
    , _text(text)
    , _offset(0)
    , _preformatted(preformatted)
{
    _textLines = 1000;
}

text_view::~text_view() {}

void text_view::on_redraw()
{
    clear();
    std::string::const_iterator iter = _text.begin();
    std::string::const_iterator end = _text.end();
    int y = -_offset;
    std::string line;
    if (_preformatted) {
        std::istringstream is(_text);
        while (y < height() && std::getline(is, line, '\n')) {
            if (y >= 0) {
                print(0, y, line);
            }
            ++y;
        }
        return;
    }
    int lineLength = 0, wordLength = 0;
    line.reserve(width());
    while (iter != end && y < height()) {
        std::string word;
        while (iter != end && std::isspace(*iter) && *iter != '\n') {
            ++iter;
        }
        auto wordStart = iter;
        while (iter != end && !std::isspace(*iter)) {
            detail::utf8Increment(iter, end);
            ++wordLength;
        }
        if (wordLength) {
            word.assign(wordStart, iter);
            if (lineLength + wordLength + 1 < width()) {
                line += word + " ";
                lineLength += wordLength + 1;
            }
            else {
                if (y >= 0) {
                    print(0, y, line);
                }
                ++y;
                line = word + " ";
                lineLength = wordLength + 1;
            }
            wordLength = 0;
        }
        if (*iter == '\n') {
            if (y >= 0) {
                print(0, y, line);
            }
            ++iter;
            ++y;
            line.clear();
            lineLength = 0;
        }
    }
}

void text_view::on_event(int event)
{
    switch (event) {
        case KEY_UP:
            if (_offset > 0) {
                --_offset;
            }
            break;
        case KEY_DOWN:
            if (_offset < _textLines) {
                ++_offset;
            }
            break;
        case KEY_PPAGE: {
            auto lines = (std::min)(height() - 2, _offset);
            if (lines > 0) {
                _offset -= lines;
            }
            break;
        }
        case KEY_NPAGE: {
            auto lines = (std::min)(height() - 2, _textLines - 1 - _offset);
            if (lines > 0) {
                _offset += lines;
            }
            break;
        }
        default:
            break;
    }
    redraw();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

log_view::log_view(int x, int y, int w, int h, const model& viewModel)
    : ghc::cui::window(x, y, w, h)
    , _model(viewModel)
{
}

log_view::~log_view() {}

void log_view::on_redraw()
{
    clear();
    int yy = height();
    int pos = _model._position;
    while (yy >= 0 && pos >= 0) {
        auto cells = _model.line(pos, width());
        if (!cells.empty()) {
            int headerWidth = 0;
            for (const auto& c : cells) {
                headerWidth += c.width ? c.width + 1 : 0;
            }
            auto lines = detail::utf8Lines(cells.back().text, width() - headerWidth);
            int x = 0;
            for (const auto& c : cells) {
                int y = yy - lines.size();
                switch (c.align) {
                    case cell::eLeft:
                        if (c.width) {
                            print(x, y, c.text, c.attr);
                        }
                        else {
                            for (auto iter = lines.begin(); iter != lines.end(); ++iter) {
                                if (y >= 0) {
                                    print(x, y, *iter, c.attr);
                                }
                                ++y;
                            }
                        }
                        break;
                    case cell::eCenter:
                        break;
                    case cell::eRight:
                        if (c.width) {
                            print(x + c.width - detail::utf8Length(c.text), y, c.text, c.attr);
                        }
                        else {
                            for (auto iter = lines.begin(); iter != lines.end(); ++iter) {
                                if (y >= 0) {
                                    print(x + c.width - detail::utf8Length(*iter), y, *iter, c.attr);
                                }
                                ++y;
                            }
                        }
                        break;
                }
                x += c.width + 1;
            }
            yy -= lines.size();
        }
        else {
            --yy;
        }
        --pos;
    }
}

void log_view::on_event(int event) {}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GHC_INLINE application::application(int argc, char* argv[])
    : window_base(0, 0, 0, 0)
{
    ::setlocale(LC_ALL, "");
    WINDOW* window = ::initscr();
    getmaxyx(window, _height, _width);
    ::mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, &_oldMask);
    ::cbreak();
    ::noecho();
    ::curs_set(0);
    ::intrflush(window, FALSE);
    ::keypad(window, TRUE);
    ::printf("\033[?1003h\n");
    ::halfdelay(1);
    ::refresh();
    if (_width < 105 || _height < 10) {
        ::printf("\e[8;%d;%dt", 30, 105);
        //::system("resize -s 30 105");
        //::resizeterm(30, 105);
    }
    instance_pointer() = this;
}

GHC_INLINE application::~application()
{
    ::flushinp();
    ::mmask_t dummy;
    ::mousemask(_oldMask, &dummy);
    ::endwin();
}

GHC_INLINE application* application::instance()
{
    return instance_pointer();
}

GHC_INLINE application*& application::instance_pointer()
{
    static application* inst = nullptr;
    return inst;
}

template <typename Window, typename... Args>
inline typename std::enable_if<std::is_base_of<ghc::cui::window, Window>::value, typename std::shared_ptr<Window>>::type application::create_window(Args&&... args)
{
    auto win = std::make_shared<Window>(std::forward<Args>(args)...);
    _windows.push_back(win);
    return win;
}

GHC_INLINE void application::yield()
{
    auto event = ::getch();
#ifndef NDEBUG
    if (event != -1 && width() > 40 && height() > 10) {
        auto es = std::to_string(event);
        es = std::string(5 - es.size(), ' ') + es;
        print(1, 0, "Event:" + es);
    }
#endif
    switch (event) {
        case ERR:
            on_idle();
            break;
        case 27:
            //_quit = true;
            break;
        case KEY_MOUSE:
            ::MEVENT mouseEvent;
#ifdef WIN32
#else
            ::getmouse(&mouseEvent);
            on_mouse(mouseEvent);
#endif
            break;
        case KEY_RESIZE:
            handle_resize();
            break;
        default:
            on_event(event);
            for (auto& win : _windows) {
                auto w = win.lock();
                if (w)
                    w->on_event(event);
            }
            break;
    }
    std::remove_if(_windows.begin(), _windows.end(), [](std::weak_ptr<window>& win) { return win.expired(); });
    on_redraw();
    ::refresh();
}

GHC_INLINE int application::run()
{
    try {
        on_init();
        while (!_quit) {
            yield();
        }
        on_exit();
    }
    catch (const std::exception& e) {
        std::cerr << "An error occured: " << e.what() << std::endl;
        throw;
        return -1;
    }
    return 0;
}

GHC_INLINE void application::quit()
{
    _quit = true;
}

GHC_INLINE void application::handle_resize()
{
    clear();
    getmaxyx(stdscr, _height, _width);
    on_resize(_width, _height);
    for (const auto& window : _windows) {
        auto w = window.lock();
        if (w) {
            w->on_resize(_width, _height);
        }
    }
    ::refresh();
}

}  // namespace cui
}  // namespace ghc
