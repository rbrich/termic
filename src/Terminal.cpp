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

namespace xci {

using namespace xci::util;
using namespace xci::util::log;
using namespace xci::graphics;


bool Terminal::start_shell()
{
    return m_shell.start();
}


void Terminal::update()
{
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
    add_text(data);
//   set_color(TextTerminal::Color4bit::BrightWhite, TextTerminal::Color4bit::Blue);
//   set_font_style(TextTerminal::FontStyle::Bold);

}


} // namespace xci
