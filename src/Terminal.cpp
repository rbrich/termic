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
    for (char c : data) {
        switch (m_input_state) {
            case S::Normal:
                switch (c) {
                    case 7:   // BEL
                        bell();
                        break;
                    case 8:   // BS
                        // TODO: cursor back
                        break;
                    case 9:   // HT
                        text += "   ";
                        break;
                    case 10:  // LF
                        // TODO: cursor down / new line
                        text += c;
                        break;
                    case 13:  // CR
                        // TODO: cursor to line beginning
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
                switch (c) {
                    case 'm':  // SGR – Select Graphic Rendition
                        if (!text.empty()) {
                            add_text(text);
                            text.clear();
                        }
                        decode_sgr(m_input_seq);
                        break;
                    default:
                        log_debug("Unknown seq: {}", m_input_seq);
                        break;
                }
                m_input_seq.clear();
                m_input_state = S::Normal;
                break;
        }
    }
    if (!text.empty()) {
        add_text(text);
    }
}


void Terminal::decode_sgr(const std::string &sgr)
{
    // Remove CSI at start and 'm' at end
    auto params = sgr.substr(2, sgr.size() - 3);
    std::istringstream params_stream(params);
    log_debug("SGR {}", params);
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
