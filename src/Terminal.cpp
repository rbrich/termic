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
            case Key::KeypadEnter: seq = "\n"; break;
            case Key::Backspace: seq = "\b"; break;
            case Key::Tab: seq = "\t"; break;
            case Key::Up: seq = "\033[A"; break;  // CSI A
            case Key::Down: seq = "\033[B"; break;  // CSI B
            case Key::Right: seq = "\033[C"; break;  // CSI C
            case Key::Left: seq = "\033[D"; break;  // CSI D
            case Key::Home: seq = "\033[H"; break;  // CSI H
            case Key::End: seq = "\033[F"; break;  // CSI F
            case Key::F1: seq = "\033OP"; break;  // SS3 P
            case Key::F2: seq = "\033OQ"; break;  // SS3 Q
            case Key::F3: seq = "\033OR"; break;  // SS3 R
            case Key::F4: seq = "\033OS"; break;  // SS3 S
            case Key::F5: seq = "\033[15~"; break;  // CSI 1 5 ~
            case Key::F6: seq = "\033[17~"; break;  // CSI 1 7 ~
            case Key::F7: seq = "\033[18~"; break;  // CSI 1 8 ~
            case Key::F8: seq = "\033[19~"; break;  // CSI 1 9 ~
            case Key::F9: seq = "\033[20~"; break;  // CSI 2 0 ~
            case Key::F10: seq = "\033[21~"; break;  // CSI 2 1 ~
            case Key::F11: seq = "\033[23~"; break;  // CSI 2 3 ~
            case Key::F12: seq = "\033[24~"; break;  // CSI 2 4 ~
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

    if (ev.mod == ModKey::Shift()) {
        switch (ev.key) {
            case Key::F10:
                // Write contents of current line to stdout (debugging)
                std::cout << escape(current_line().content()) << std::endl;
                return true;
            default:
                log_debug("Terminal::key_event: Unhandled key: Shift + {}", int(ev.key));
                return false;
        }
    }

    m_shell.write(seq);
    cancel_scrollback();
    return true;
}


void Terminal::char_event(View &view, const CharEvent &ev)
{
    log_debug("Input char: {}", ev.code_point);
    m_shell.write(to_utf8(ev.code_point));
}


void Terminal::scroll_event(View& view, const ScrollEvent& ev)
{
    log_debug("Scroll: {}", ev.offset);
    scrollback(ev.offset.y);
}


void Terminal::decode_input(const std::string &data)
{
    using S = InputState;
    std::string text;
    auto flush_text = [&text, this]() {
        if (!text.empty()) {
            add_text(text, m_mode.insert);
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
                        set_cursor_pos(cursor_pos() - Vec2u{1, 0});
                        break;
                    case 9:   // HT
                        text += "   ";
                        break;
                    case 10:  // LF
                        // cursor down / new line
                        flush_text();
                        set_cursor_pos(cursor_pos() + Vec2u{0, 1});
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
                switch (c) {
                    case 27: {  // ESC
                        m_input_seq.clear();
                        break;
                    }
                    case 'D':  // IND - Index
                        flush_text();
                        set_cursor_pos(cursor_pos() + Vec2u{0, 1});
                        break;
                    case 'E':  // NEL - Next Line
                        flush_text();
                        set_cursor_pos({0, cursor_pos().y + 1});
                        break;
                    case 'M':  // RI - Reverse Index
                        flush_text();
                        set_cursor_pos(cursor_pos() - Vec2u{0, 1});
                        break;
                    case '[':
                        m_input_state = S::CSI;
                        break;
                    case ']':
                        m_input_state = S::OSC;
                        break;
                    case '#':
                    case ' ':
                    case '%':
                    case '(':
                    case ')':
                    case '*':
                    case '+':
                    case '-':
                    case '.':
                    case '/':
                        m_input_state = S::Escape_1; break;
                    default:
                        log_debug("Unknown seq: ESC {}", c);
                        m_input_seq.clear();
                        m_input_state = S::Normal;
                        break;
                }
                break;

            case S::Escape_1: {
                if (c == 27) {  // ESC
                    m_input_seq.clear();
                    m_input_state = S::Escape;
                    break;
                }
                int prev = m_input_seq.back();
                // ISO 2022 character set switching
                if (prev == '(' && c == 'B') {
                    // Select US ASCII charset -> NOOP
                } else if (prev == '#' && c == '8') {
                    // DECALN - Screen Alignment Pattern
                    flush_text();
                    set_cursor_pos({0, 0});
                } else {
                    log_debug("Unknown seq: ESC {} {}", m_input_seq.back(), c);
                }
                m_input_seq.clear();
                m_input_state = S::Normal;
                break;
            }
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
                flush_text();
                if (!params.empty() && (
                        (params.front() >= '\x3c' && params.front() <= '\x3f') ||
                        (params.back() >= 'p' && params.back() <= '~'))) {
                    // private use
                    decode_private(c, params);
                }
                switch (c) {
                    case 'A': {  // CUU - Cursor Up
                        unsigned p = 1;
                        cseq_parse_params("CUU", params, p);
                        set_cursor_pos(cursor_pos() - Vec2u{0, p});
                        break;
                    }
                    case 'B': {  // CUD - Cursor Down
                        unsigned p = 1;
                        cseq_parse_params("CUD", params, p);
                        set_cursor_pos(cursor_pos() + Vec2u{0, p});
                        break;
                    }
                    case 'C': {  // CUF - Cursor Right (Forward)
                        unsigned p = 1;
                        cseq_parse_params("CUF", params, p);
                        set_cursor_pos(cursor_pos() + Vec2u{p, 0});
                        break;
                    }
                    case 'D': {  // CUB - Cursor Left (Back)
                        unsigned p = 1;
                        cseq_parse_params("CUB", params, p);
                        set_cursor_pos(cursor_pos() - Vec2u{p, 0});
                        break;
                    }
                    case 'H': {  // CUP - Cursor Position
                        unsigned row = 1, column = 1;
                        cseq_parse_params("CUP", params, row, column);
                        set_cursor_pos({column - 1, row - 1});
                        break;
                    }
                    case 'J': {  // ED - Erase in Page (Display)
                        unsigned p = 0;
                        cseq_parse_params("ED", params, p);
                        switch (p) {
                            case 0:
                                // erase from cursor to the end of page
                                break;
                            case 1:
                                // erase from the beginning of the page
                                // up to and including the cursor position
                                break;
                            case 2:
                                // erase all characters in the page
                                erase_page();
                                break;
                            case 3:
                                // erase scrollback buffer (xterm extension)
                                erase_buffer();
                                break;
                            default:
                                log_warning("Unknown ED param: {}", p);
                                break;
                        }
                        break;
                    }
                    case 'K': {  // EL - Erase in Line
                        unsigned p = 0;
                        cseq_parse_params("EL", params, p);
                        switch (p) {
                            case 0:
                                // clear from cursor to the end of the line
                                current_line().erase_text(cursor_pos().x, size_in_cells().x);
                                break;
                            case 1:
                                // clear from cursor to beginning of the line
                                current_line().erase_text(0, cursor_pos().x);
                                break;
                            case 2:
                                // clear entire line
                                current_line().erase_text(0, size_in_cells().x);
                                break;
                            default:
                                log_warning("Unknown EL param: {}", p);
                                break;
                        }
                        break;
                    }
                    case 'P': {  // DCH - Delete Character (CSI p P)
                        unsigned p = 1;
                        cseq_parse_params("DCH", params, p);
                        current_line().erase_text(cursor_pos().x, p);
                        break;
                    }
                    case 'c':  {  // DA - Device Attributes
                        unsigned p = 0;
                        cseq_parse_params("DA", params, p);
                        if (p != 0) {
                            log_debug("Unknown DA params: {}{}", p, params);
                            break;
                        }
                        // Say we are "VT100 with Advanced Video Option"
                        m_shell.write("\033[?1;2c");
                        break;
                    }
                    case 'd': {  // VPA - Line Position Absolute
                        unsigned p = 1;
                        cseq_parse_params("VPA", params, p);
                        set_cursor_pos({0, p - 1});
                        break;
                    }
                    case 'e': {  // VPR - Line Position Forward
                        unsigned p = 1;
                        cseq_parse_params("VPR", params, p);
                        set_cursor_pos({0, cursor_pos().y + p});
                        break;
                    }
                    case 'f': {  // HVP - Horizontal and Vertical Position
                        unsigned row = 1, column = 1;
                        cseq_parse_params("HVP", params, row, column);
                        set_cursor_pos({column - 1, row - 1});
                        break;
                    }
                    case 'h': {  // SM - Set Mode
                        unsigned p = 0;
                        cseq_parse_params("SM", params, p);
                        switch (p) {
                            case 4:  // IRM - Insert/Replace Mode
                                m_mode.insert = true;
                                break;
                            default:
                                log_debug("Unknown SM param: {}", p);
                                break;
                        }
                    }
                    case 'l': {  // RM - Reset Mode
                        unsigned p = 0;
                        cseq_parse_params("RM", params, p);
                        switch (p) {
                            case 4:  // IRM - Insert/Replace Mode
                                m_mode.insert = false;
                                break;
                            default:
                                log_debug("Unknown RM param: {}", p);
                                break;
                        }
                    }
                    case 'm':  // SGR - Select Graphic Rendition
                        decode_sgr(params);
                        break;

                    case 'r': { // DECSTBM - Set Scrolling Region (Set Top and Bottom Margins)
                        unsigned top = 0;
                        unsigned bottom = 0;
                        cseq_parse_params("DECSTBM", params, top, bottom);
                        set_cursor_pos({0, 0});
                        log_debug("DECSTBM: {} {} (not implemented)", top, bottom);
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
        unsigned p = 0;
        more_params = cseq_next_param(params, p);

        if (p == 0) {
            // reset all attributes
            set_fg(c_fg_default);
            set_bg(c_bg_default);
            set_font_style(FontStyle::Regular);
            set_decoration(Decoration::None);
            set_mode(Mode::Normal);
        } else if (p == 1) {
            set_font_style(FontStyle::Bold);
        } else if (p >= 30 && p <= 37) {
            set_fg(Color4bit(p - 30));
        } else if (p == 38 && more_params) {
            unsigned p1 = 0;
            more_params = cseq_next_param(params, p1);
            if (p1 == 5 && more_params) {
                // 8-bit color
                unsigned idx = 0;
                more_params = cseq_next_param(params, idx);
                set_fg(Color8bit(idx));
            } else if (p1 == 2 && more_params) {
                // 24-bit color
                unsigned r = 0, g = 0, b = 0;
                more_params =
                        cseq_next_param(params, r) &&
                        cseq_next_param(params, g) &&
                        cseq_next_param(params, b);
                set_fg(Color24bit(uint8_t(r), uint8_t(g), uint8_t(b)));
            } else {
                log_debug("Unknown SGR {};{}", p, p1);
            }
        } else if (p == 39) {
            set_fg(c_fg_default);
        } else if (p >= 40 && p <= 47) {
            set_bg(Color4bit(p - 40));
        } else if (p == 48 && more_params) {
            unsigned p1 = 0;
            more_params = cseq_next_param(params, p1);
            if (p1 == 5 && more_params) {
                // 8-bit color
                unsigned idx = 0;
                more_params = cseq_next_param(params, idx);
                set_bg(Color8bit(idx));
            } else if (p1 == 2 && more_params) {
                // 24-bit color
                unsigned r = 0, g = 0, b = 0;
                more_params =
                        cseq_next_param(params, r) &&
                        cseq_next_param(params, g) &&
                        cseq_next_param(params, b);
                set_bg(Color24bit(uint8_t(r), uint8_t(g), uint8_t(b)));
            } else {
                log_debug("Unknown SGR {};{}", p, p1);
            }
        } else if (p == 49) {
            set_bg(c_bg_default);
        } else if (p >= 90 && p <= 97) {
            set_fg(Color4bit(p - 90 + 8));
        } else if (p >= 100 && p <= 107) {
            set_bg(Color4bit(p - 100 + 8));
        } else {
            log_debug("Unknown SGR {}", p);
        }
    }
}


void Terminal::decode_private(char f, std::string_view params)
{
    if (params == "?2004" && (f == 'h' || f == 'l')) {
        // bracketed paste mode
        m_mode.bracketed_paste = bool(f == 'h');
        log_debug("Terminal: bracketed_paste_mode = {}",
                  bool(m_mode.bracketed_paste));
        return;
    }
    log_debug("Unknown private seq {}", params);
}


} // namespace xci
