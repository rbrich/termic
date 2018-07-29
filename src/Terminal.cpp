// Terminal.cpp created on 2018-07-28, part of XCI term
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Terminal.h"
#include <xci/util/log.h>
#include <xci/util/string.h>
#include <sstream>

namespace xci {

using namespace xci::util;
using namespace xci::util::log;
using namespace xci::graphics;
using namespace xci::widgets;


bool Terminal::start_shell()
{
    return m_shell.start();
}


void Terminal::update(std::chrono::nanoseconds elapsed)
{
    TextTerminal::update(elapsed);
    if (m_shell.data_ready()) {
        auto buffer = m_shell.read();
        decode_input(buffer);
    }
}


bool Terminal::key_event(View &view, const KeyEvent &ev)
{
    if (ev.action == Action::Release)
        return false;
    log_debug("Input key: {}", int(ev.key));
    std::string seq;
    switch (ev.key) {
        case Key::Enter: seq = "\n"; break;
        case Key::Backspace: seq = "\b"; break;
        case Key::Up: seq = "\x1b[A"; break;
        case Key::Down: seq = "\x1b[B"; break;
        case Key::Right: seq = "\x1b[C"; break;
        case Key::Left: seq = "\x1b[D"; break;
        default: return false;
    }
    m_shell.write(seq);
    return true;
}


void Terminal::char_event(View &view, const CharEvent &ev)
{
    log_debug("Input char: {}", ev.code_point);
    m_shell.write(to_utf8(ev.code_point));
}


void Terminal::decode_input(const std::string &data)
{
    using S = InputState;
    std::string text;
    auto flush_text = [&text, this]() {
        if (!text.empty()) {
            add_text(text);
            text.clear();
        }
    };
    for (char c : data) {
        switch (m_input_state) {
            case S::Normal:
                switch (c) {
                    case 7:   // BEL
                        bell();
                        break;
                    case 8:   // BS
                        flush_text();
                        set_cursor_pos(cursor_pos() - Vec2i{1, 0});
                        break;
                    case 9:   // HT
                        text += "   ";
                        break;
                    case 10:  // LF
                        // cursor down / new line
                        flush_text();
                        set_cursor_pos(cursor_pos() + Vec2i{0, 1});
                        break;
                    case 13:  // CR
                        // cursor to line beginning
                        flush_text();
                        set_cursor_pos({0, cursor_pos().y});
                        break;
                    case 27:  // ESC
                        m_input_seq += c;
                        m_input_state = S::Escape;
                        break;
                    default:
                        if (c < 32)
                            log_debug("Unknown cc: {}", int(c));
                        else
                            text += c;
                        break;
                }
                break;

            case S::Escape:
                m_input_seq += c;
                if (c == '[') {
                    m_input_state = S::CSI;
                    break;
                }
                log_debug("Unknown seq: ESC {}", c);
                text += m_input_seq;
                m_input_seq.clear();
                m_input_state = S::Normal;
                break;

            case S::CSI:
                m_input_seq += c;
                if (isdigit(c) || c == ';')
                    break;
                // Remove CSI at start and command char at the end
                auto params = m_input_seq.substr(2, m_input_seq.size() - 3);
                switch (c) {
                    case 'm':  // SGR - Select Graphic Rendition
                        flush_text();
                        decode_sgr(params);
                        break;
                    case 'C':  // CUF - Cursor Forward
                        if (params.empty()) {
                            set_cursor_pos(cursor_pos() + Vec2i{1, 0});
                        } else {
                            log_debug("Unknown CUF param: {}", params);
                        };
                        break;
                    case 'K':  // EL - Erase in Line
                        flush_text();
                        {
                            int n = atoi(params.c_str());
                            switch (n) {
                                case 0:
                                    // clear from cursor to the end of the line
                                    current_line().erase(cursor_pos().x, size_in_cells().x);
                                    break;
                                case 1:
                                    // clear from cursor to beginning of the line
                                    current_line().erase(0, cursor_pos().x);
                                    break;
                                case 2:
                                    // clear entire line
                                    current_line().erase(0, size_in_cells().x);
                                    break;
                                default:
                                    log_debug("Unknown EL param: {}", params);
                                    break;
                            }
                        }
                        break;
                    default:
                        log_debug("Unknown seq: CSI {}", m_input_seq.substr(1));
                        break;
                }
                m_input_seq.clear();
                m_input_state = S::Normal;
                break;
        }
    }
    flush_text();
}


void Terminal::decode_sgr(const std::string &params)
{
    std::istringstream params_stream(params);
    //log_debug("SGR {}", params);
    do {
        int p = 0;
        params_stream >> p;

        if (p == 0) {
            // reset all attributes
            m_fg = c_fg_default;
            m_bg = c_bg_default;
            set_color(m_fg, m_bg);
            set_font_style(FontStyle::Regular);
            set_decoration(Decoration::None);
            set_mode(Mode::Normal);
        } else if (p == 1) {
            set_font_style(FontStyle::Bold);
        } else if (p >= 30 && p <= 37) {
            m_fg = Color4bit(p - 30);
            set_color(m_fg, m_bg);
        } else if (p == 39) {
            m_fg = c_fg_default;
            set_color(m_fg, m_bg);
        } else if (p >= 40 && p <= 47) {
            m_bg = Color4bit(p - 40);
            set_color(m_fg, m_bg);
        } else if (p == 49) {
            m_bg = c_bg_default;
            set_color(m_fg, m_bg);
        } else if (p >= 90 && p <= 97) {
            m_fg = Color4bit(p - 90 + 8);
            set_color(m_fg, m_bg);
        } else if (p >= 100 && p <= 107) {
            m_bg = Color4bit(p - 100 + 8);
            set_color(m_fg, m_bg);
        } else {
            log_debug("Unknown SGR {}", p);
        }

        // consume separator (;)
        char sep = 0;
        params_stream >> sep;
        assert(sep == ';' || params_stream.eof());
    } while (params_stream);
}


} // namespace xci
