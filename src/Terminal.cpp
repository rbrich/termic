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
#include "utility.h"
#include <xci/util/log.h>
#include <xci/util/string.h>
#include <sstream>
#include <cstdlib>

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

    std::string seq;

    if (ev.mod == ModKey::None()) {
        switch (ev.key) {
            case Key::Enter: seq = "\n"; break;
            case Key::Backspace: seq = "\b"; break;
            case Key::Up: seq = "\033[A"; break;
            case Key::Down: seq = "\033[B"; break;
            case Key::Right: seq = "\033[C"; break;
            case Key::Left: seq = "\033[D"; break;
            case Key::Home: seq = "\033[H"; break;
            case Key::End: seq = "\033[F"; break;
            default:
                log_debug("Terminal::key_event: Unhandled key: {}", int(ev.key));
                return false;
        }
    }

    if (ev.mod == ModKey::Ctrl()) {
        // ^A .. ^Z, ^[, ^\, ^]
        if (ev.key >= Key::A && ev.key <= Key::RightBracket) {
            // '\1' .. '\x1d'
            seq = char(int(ev.key) - int(Key::A) + 1);
        } else {
            log_debug("Terminal::key_event: Unhandled key: Ctrl + {}", int(ev.key));
            return false;
        }
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
                        if (c >= 0 && c < 32)
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
                } else if (c == ']') {
                    m_input_state = S::OSC;
                    break;
                }
                log_debug("Unknown seq: ESC {}", c);
                text += m_input_seq;
                m_input_seq.clear();
                m_input_state = S::Normal;
                break;

            case S::CSI: {
                m_input_seq += c;
                if (c >= '0' && c <= '?') {
                    // continue reading parameters
                    break;
                }
                // Remove CSI at start and command char at the end
                std::string_view params = m_input_seq;
                params.remove_prefix(2);
                params.remove_suffix(1);
                TRACE("CSI {} {}", params, c);
                switch (c) {
                    case 'm':  // SGR - select graphic rendition
                        flush_text();
                        decode_sgr(params);
                        break;
                    case 'C':  // CUF - cursor forward
                        if (params.empty()) {
                            set_cursor_pos(cursor_pos() + Vec2i{1, 0});
                        } else {
                            log_debug("Unknown CUF param: {}", params);
                        };
                        break;
                    case 'K':  // EL - erase in line
                        flush_text();
                        {
                            int p = 0;
                            bool more = cseq_next_param(params, p);
                            if (more) {
                                log_debug("Ignored EL params: {}", params);
                            }
                            switch (p) {
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
                    case 'P': {
                        // DCH - delete character (CSI p P)
                        int p = 1;  // default
                        bool more = cseq_next_param(params, p);
                        if (more) {
                            log_debug("Ignored DCH params: {}", params);
                        }
                        flush_text();
                        current_line().erase(cursor_pos().x, p);
                        break;
                    }
                    default:
                        log_debug("Unknown seq: CSI {}", m_input_seq.substr(2));
                        break;
                }
                m_input_seq.clear();
                m_input_state = S::Normal;
                break;
            }

            case S::OSC:  // Operating System Command
                m_input_seq += c;
                if ((c >= '\x08' && c <= '\x0d') || (c >= ' ' && c <= '~')) {
                    // continue reading OSC control string
                    break;
                }
                log_debug("Unknown seq: OSC {} {}", m_input_seq.substr(2), int(c));
                m_input_seq.clear();
                m_input_state = S::Normal;
                break;
        }
    }
    flush_text();
}


void Terminal::decode_sgr(std::string_view params)
{
    TRACE("params: {}", params);
    bool more_params = true;
    while (more_params) {
        int p = 0;
        more_params = cseq_next_param(params, p);

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
    }
}


} // namespace xci
