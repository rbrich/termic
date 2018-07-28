// main.cpp created on 2018-07-27, part of XCI term
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

#include "Shell.h"
#include <xci/widgets/TextTerminal.h>
#include <xci/graphics/Window.h>
#include <xci/util/file.h>
#include <xci/util/log.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <cassert>

using namespace xci::widgets;
using namespace xci::graphics;
using namespace xci::util;
using namespace xci::util::log;

int main()
{
    xci::util::chdir_to_share();

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI TextTerminal demo");

    if (!Theme::load_default_theme())
        return EXIT_FAILURE;

    Shell shell(window);
    if (!shell.start())
        return EXIT_FAILURE;

    TextTerminal terminal;
    terminal.set_color(TextTerminal::Color4bit::BrightWhite, TextTerminal::Color4bit::Blue);
    terminal.set_font_style(TextTerminal::FontStyle::Bold);

    window.set_update_callback([&shell, &terminal]() {
        if (shell.data_ready()) {
            auto buffer = shell.read();
            terminal.add_text(buffer);
        }
    });

    window.set_char_callback([&shell](View&, const CharEvent&) {
        shell.write("ls -la ..\n");
    });

    // Make the terminal fullscreen
    window.set_size_callback([&](View& v) {
        auto s = v.scalable_size();
        terminal.set_position({-s * 0.5});
        terminal.set_size(s);
    });

    Bind bind(window, terminal);
    window.set_refresh_mode(RefreshMode::OnEvent);
    window.display();
    return EXIT_SUCCESS;
}
