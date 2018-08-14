// Terminal.h created on 2018-07-28, part of XCI term
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

#ifndef XCITERM_TERMINAL_H
#define XCITERM_TERMINAL_H

#include "Shell.h"
#include <xci/widgets/TextTerminal.h>
#include <xci/graphics/Window.h>

namespace xci {

class Terminal: public xci::widgets::TextTerminal {
    using Buffer = widgets::terminal::Buffer;

public:
    explicit Terminal(const xci::graphics::Window &window) : m_shell(window) {}

    bool start_shell();

    void resize(graphics::View& view) override;
    void update(std::chrono::nanoseconds elapsed) override;
    bool key_event(graphics::View& view, const graphics::KeyEvent& ev) override;
    void char_event(graphics::View& view, const graphics::CharEvent& ev) override;
    void scroll_event(graphics::View& view, const graphics::ScrollEvent& ev) override;

    // Decode input from shell. Data are mix of UTF-8 text,
    // control codes and escape sequences. This will call
    // other methods like add_text, set_color for each fragment of data.
    void decode_input(const std::string& data);

private:

    void decode_ctlseq(char c, std::string_view params);
    void decode_sgr(std::string_view params);
    void decode_private(char f, std::string_view params);

private:
    Shell m_shell;

    // Normal / Alternate Screen Buffer
    // These variables contain state of the *other* buffer.
    // Current buffer and cursor is inside TextTerminal instance.
    std::unique_ptr<Buffer> m_alternate_buffer = std::make_unique<Buffer>();
    util::Vec2u m_saved_cursor;

    enum class InputState {
        Normal,
        Escape,
        Escape_1,  // single argument escape sequences
        CSI,
        OSC,
    };
    InputState m_input_state = InputState::Normal;
    std::string m_input_seq;

    static constexpr Color4bit c_fg_default = Color4bit::White;
    static constexpr Color4bit c_bg_default = Color4bit::Black;

    // modes
    struct {
        bool insert : 1;
        bool bracketed_paste : 1;  // TODO
        bool alternate_screen_buffer : 1;  // Normal / Alternate Screen Buffer
    } m_mode = {};
};

} // namespace xci

#endif // XCITERM_TERMINAL_H
