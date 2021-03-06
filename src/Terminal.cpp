// Terminal.cpp created on 2018-07-28
// This file is part of Termic project <https://github.com/rbrich/termic>
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Terminal.h"
#include "utility.h"
#include <xci/core/log.h>
#include <xci/core/string.h>  // NOLINT(modernize-deprecated-headers) - FP
#include <fmt/ostream.h>
#include <iostream>
#include <cstdlib>

namespace xci::term {

using namespace xci::core;
using namespace xci::graphics;
using namespace xci::widgets;
using namespace std::chrono_literals;


void Terminal::resize(graphics::View &view)
{
    auto orig_size = size_in_cells();
    TextTerminal::resize(view);
    auto new_size = size_in_cells();
    if (orig_size != new_size) {
        log::debug("Terminal: resize {} cells", size_in_cells());
        m_shell.pty().set_winsize(size_in_cells());
    }
}


bool Terminal::key_event(View &view, const KeyEvent &ev)
{
    if (ev.action == Action::Release)
        return false;

    std::string seq;

    if (ev.mod == ModKey::None()) {
        switch (ev.key) {
            case Key::Escape: seq = "\033"; break;
            case Key::Enter:
            case Key::KeypadEnter: seq = "\n"; break;
            case Key::Backspace: seq = "\b"; break;
            case Key::Tab: seq = "\t"; break;
            case Key::Up: seq = m_mode.app_cursor_keys ? "\033OA" : "\033[A"; break;  // SS3 A / CSI A
            case Key::Down: seq = m_mode.app_cursor_keys ? "\033OB" : "\033[B"; break;  // SS3 B / CSI B
            case Key::Right: seq = m_mode.app_cursor_keys ? "\033OC" : "\033[C"; break;  // SS3 C / CSI C
            case Key::Left: seq = m_mode.app_cursor_keys ? "\033OD" : "\033[D"; break;  // SS3 D / CSI D
            case Key::Home: seq = m_mode.app_cursor_keys ? "\033OH" : "\033[H"; break;  // SS3 H / CSI H
            case Key::End: seq = m_mode.app_cursor_keys ? "\033OF" : "\033[F"; break;  // SS3 F / CSI F
            case Key::PageUp: seq = "\033[5~"; break;  // CSI 5 ~
            case Key::PageDown: seq = "\033[6~"; break;  // CSI 6 ~
            case Key::Insert: seq = "\033[2~"; break;  // CSI 2 ~
            case Key::Delete: seq = "\033[3~"; break;  // CSI 3 ~
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
                log::debug("Terminal::key_event: Unhandled key: {}", int(ev.key));
                return false;
        }
        m_shell.write(seq);
        cancel_scrollback();
        view.refresh();
        return true;
    }

    if (ev.mod == ModKey::Ctrl()) {
        // ^A .. ^Z, ^[, ^\, ^]
        if (ev.key >= Key::A && ev.key <= Key::RightBracket) {
            // '\1' .. '\x1d'
            seq = char(int(ev.key) - int(Key::A) + 1);
        } else {
            log::debug("Terminal::key_event: Unhandled key: Ctrl + {}", int(ev.key));
            return false;
        }
        m_shell.write(seq);
        cancel_scrollback();
        view.refresh();
        return true;
    }

    if (ev.mod == ModKey::Shift()) {
        switch (ev.key) {
            case Key::F1:
                // Write contents of first line to stdout (debugging)
                std::cout << escape(line(0).content()) << std::endl;
                return true;
            case Key::F2:
                // Write contents of current line to stdout (debugging)
                std::cout << escape(current_line().content()) << std::endl;
                return true;
            default:
                log::debug("Terminal::key_event: Unhandled key: Shift + {}", int(ev.key));
                return false;
        }
    }

    if (ev.mod == ModKey::ShiftCtrl()) {
        switch (ev.key) {
            case Key::C:
                // TODO: select & copy
                view.window()->set_clipboard_string("Hello!");
                break;
            case Key::V:
                // Clipboard paste
                m_shell.write(view.window()->get_clipboard_string());
                break;
            default:
                return false;
        }
        cancel_scrollback();
        view.refresh();
        return true;
    }

    return false;
}


void Terminal::char_event(View &view, const CharEvent &ev)
{
    log::debug("Input char: {}", ev.code_point);
    m_shell.write(to_utf8(ev.code_point));
    view.refresh();
}


void Terminal::scroll_event(View& view, const ScrollEvent& ev)
{
    log::debug("Scroll: {}", ev.offset);
    scrollback(ev.offset.y * 3.0);
    view.refresh();
}


void Terminal::decode_input(std::string_view data)
{
    using S = InputState;
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
                        m_input_text += "   ";
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
                            log::debug("Unknown cc: {}", int(c));
                        else
                            m_input_text += c;
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
                    case ' ':  // 32
                    case '#':  // 35
                    case '%':  // 37
                    case '(':  // 40
                    case ')':  // 41
                    case '*':  // 42
                    case '+':  // 43
                    case '-':  // 45
                    case '.':  // 46
                    case '/':  // 47
                        m_input_state = S::Escape_1;
                        break;
                    case '7':  // DECSC - Save Cursor
                        flush_text();
                        m_saved_cursor = cursor_pos();
                        break;
                    case '8':  // DECRC - Restore Cursor
                        flush_text();
                        set_cursor_pos(m_saved_cursor);
                        break;
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
                    default:
                        log::debug("Unknown seq: ESC {}", c);
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
                    log::debug("Unknown seq: ESC {} {}", m_input_seq.back(), c);
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
                flush_text();
                // Remove CSI at start and command char at the end
                std::string_view params = m_input_seq;
                params.remove_prefix(2);
                params.remove_suffix(1);
                TRACE("CSI {} {}", params, c);
                if (!params.empty() && (
                        (params.front() >= '\x3c' && params.front() <= '\x3f') ||
                        (params.back() >= 'p' && params.back() <= '~'))) {
                    // private use
                    decode_private(c, params);
                } else {
                    decode_ctlseq(c, params);
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
                log::debug("Unknown seq: OSC {} {}", m_input_seq.substr(2), int(c));
                m_input_seq.clear();
                m_input_state = S::Normal;
                break;
        }
    }
    flush_text();
}


void Terminal::decode_ctlseq(char c, std::string_view params)
{
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
        case 'G': {  // CHA - Cursor Horizontal Absolute
            unsigned column = 1;
            cseq_parse_params("CHA", params, column);
            set_cursor_x(column - 1);
            break;
        }
        case 'H': {  // CUP - Cursor Position
            unsigned row = 1;
            unsigned column = 1;
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
                    erase_to_end_of_page();
                    break;
                case 1:
                    // erase from the beginning of the page
                    // up to and including the cursor position
                    erase_to_cursor();
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
                    log::warning("Unknown ED param: {}", p);
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
                    erase_in_line(cursor_pos().x, 0);
                    break;
                case 1:
                    // clear from cursor to beginning of the line
                    erase_in_line(0, cursor_pos().x + 1);
                    break;
                case 2:
                    // clear entire line
                    erase_in_line(0, 0);
                    break;
                default:
                    log::warning("Unknown EL param: {}", p);
                    break;
            }
            break;
        }
        case 'P': {  // DCH - Delete Character
            unsigned p = 1;
            cseq_parse_params("DCH", params, p);
            current_line().delete_text(cursor_pos().x, p);
            break;
        }
        case 'X': {  // ECH - Erase Character
            unsigned p = 1;
            cseq_parse_params("ECH", params, p);
            std::string spaces(p, ' ');
            current_line().add_text(cursor_pos().x, spaces, /*attr=*/{}, /*insert=*/false);
            break;
        }
        case 'c':  {  // DA - Device Attributes
            unsigned p = 0;
            cseq_parse_params("DA", params, p);
            if (p != 0) {
                log::debug("Unknown DA params: {}{}", p, params);
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
                    log::debug("Unknown SM param: {}", p);
                    break;
            }
            break;
        }
        case 'l': {  // RM - Reset Mode
            unsigned p = 0;
            cseq_parse_params("RM", params, p);
            switch (p) {
                case 4:  // IRM - Insert/Replace Mode
                    m_mode.insert = false;
                    break;
                default:
                    log::debug("Unknown RM param: {}", p);
                    break;
            }
            break;
        }
        case 'm':  // SGR - Select Graphic Rendition
            decode_sgr(params);
            break;

        case 'r': { // DECSTBM - Set Scrolling Region (Set Top and Bottom Margins)
            unsigned top = 0;
            unsigned bottom = 0;
            cseq_parse_params("DECSTBM", params, top, bottom);
            set_cursor_pos({0, 0});
            log::debug("DECSTBM (Set Scrolling Region): {} {} (not implemented)", top, bottom);
            break;
        }
        default:
            log::debug("Unknown seq: CSI {} {}", params, c);
            break;
    }
}


void Terminal::decode_sgr(std::string_view params)
{
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
            set_mode(Mode::Bright);
        } else if (p >= 30 && p <= 37) {
            set_fg(Color4bit(p - 30));
        } else if (p == 38 && more_params) {
            // Note that this is semicolon-separated xterm-compatible format.
            // According to ITU T.416, the parameter list should be colon separated
            // and should include color space identifier for RGB:
            //   "\e[38;2:<color-space-id>:<r>:<g>:<b>m"
            //   "\e[38;5:<index>m"
            // This is what we accept instead (xterm compatibility):
            //   "\e[38;2;<r>;<g>;<b>m"
            //   "\e[38;5;<index>m"
            // There is also hybrid colon-separated-but-colorspace-less format.
            // Let's ignore both that and the original standard - nobody use those.
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
                log::debug("Unknown SGR {};{}", p, p1);
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
                log::debug("Unknown SGR {};{}", p, p1);
            }
        } else if (p == 49) {
            set_bg(c_bg_default);
        } else if (p >= 90 && p <= 97) {
            set_fg(Color4bit(p - 90 + 8));
        } else if (p >= 100 && p <= 107) {
            set_bg(Color4bit(p - 100 + 8));
        } else {
            log::debug("Unknown SGR {}", p);
        }
    }
}


void Terminal::decode_private(char f, std::string_view params)
{
    if (!params.empty() && params[0] == '?' && (f == 'h' || f == 'l')) {
        // DECSET - DEC Private Mode Set [CSI ? <mode> h]
        // DECRST - DEC Private Mode Reset [CSI ? <mode> l]
        bool mode_set = bool(f == 'h');
        unsigned mode = 0;
        params.remove_prefix(1);
        cseq_parse_params(mode_set ? "DECSET" : "DECRST", params, mode);
        switch (mode) {
            case 1:
                // DECCKM - Cursor Keys Mode
                m_mode.app_cursor_keys = mode_set;
                break;
            case 3:
                // DECCOLM - 80 / 132 Column Mode
                log::debug("Terminal: request for {} column mode ignored",
                          (mode_set ? 132u : 80u));
                //set_req_cells({mode_set ? 132u : 80u, req_cells().y});
                return;
            case 7:
                // DECAWM - Autowrap Mode
                m_mode.autowrap = mode_set;
                break;
            case 47:
                // Normal / Alternate Screen Buffer (xterm)
                if (mode_set != m_mode.alternate_screen_buffer) {
                    auto orig_cursor = cursor_pos();
                    m_alternate_buffer = set_buffer(std::move(m_alternate_buffer));
                    set_cursor_pos(m_saved_cursor);
                    m_saved_cursor = orig_cursor;
                }
                m_mode.alternate_screen_buffer = mode_set;
                return;
            case 1048:
                if (mode_set) {
                    // Save cursor as in DECSC (xterm)
                    m_saved_cursor = cursor_pos();
                } else {
                    // Restore cursor as in DECRC (xterm)
                    set_cursor_pos(m_saved_cursor);
                }
                return;
            case 1049:
                if (mode_set && !m_mode.alternate_screen_buffer) {
                    // Save cursor as in DECSC (xterm)
                    // After saving the cursor, switch to the Alternate Screen Buffer,
                    // clearing it first.
                    m_mode.alternate_screen_buffer = true;
                    m_saved_cursor = cursor_pos();
                    m_alternate_buffer = set_buffer(std::move(m_alternate_buffer));
                    erase_buffer();
                }
                if (!mode_set && m_mode.alternate_screen_buffer) {
                    // Use Normal Screen Buffer and restore cursor as in DECRC (xterm)
                    m_mode.alternate_screen_buffer = false;
                    m_alternate_buffer = set_buffer(std::move(m_alternate_buffer));
                    set_cursor_pos(m_saved_cursor);
                }
                return;
            case 2004:
                // bracketed paste mode
                m_mode.bracketed_paste = mode_set;
                log::debug("Terminal: bracketed_paste_mode = {}",
                          bool(m_mode.bracketed_paste));
                return;
            default:
                log::debug("Unknown DECSET/DECRST: {} {}", mode, f);
                return;
        }
    }
    log::debug("Unknown private seq {}", params);
}


void Terminal::flush_text()
{
    if (!m_input_text.empty()) {
        std::string_view sv(m_input_text);
        // Check if there is partial UTF-8 character at the end
        size_t partial = utf8_partial_end(sv);
        if (sv.size() == partial)
            return;
        sv.remove_suffix(partial);
        TRACE("flush_text {} (insert={})", text, bool(m_mode.insert));
        add_text(sv, m_mode.insert, m_mode.autowrap);
        m_input_text.erase(0, sv.size());
    }
}


} // namespace xci::term
