// Terminal.h created on 2018-07-28
// This file is part of Termic project <https://github.com/rbrich/termic>
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCITERM_TERMINAL_H
#define XCITERM_TERMINAL_H

#include "Shell.h"
#include <xci/widgets/TextTerminal.h>
#include <xci/widgets/Widget.h>
#include <xci/graphics/Window.h>
#include <xci/core/dispatch.h>

#include <string_view>

namespace xci::term {

// Terminal widget. Single Terminal instance can manage single shell session.
// For multi-terminal program (e.g. tabbed view), multiple instances have to be created.
class Terminal: public widgets::TextTerminal {
    using Buffer = widgets::terminal::Buffer;

public:
    explicit Terminal(widgets::Theme& theme, Shell& shell)
        : widgets::TextTerminal(theme), m_shell(shell) { m_mode.autowrap = true; }

    void resize(graphics::View& view) override;

    bool key_event(graphics::View& view, const graphics::KeyEvent& ev) override;
    void char_event(graphics::View& view, const graphics::CharEvent& ev) override;
    void scroll_event(graphics::View& view, const graphics::ScrollEvent& ev) override;

    // Decode input from shell. Data are mix of UTF-8 text,
    // control codes and escape sequences. This will call
    // other methods like add_text, set_color for each fragment of data.
    void decode_input(std::string_view data);

private:

    void decode_ctlseq(char c, std::string_view params);
    void decode_sgr(std::string_view params);
    void decode_private(char f, std::string_view params);
    void flush_text();

private:
    Shell& m_shell;
    std::string m_input_text;

    // Normal / Alternate Screen Buffer
    // These variables contain state of the *other* buffer.
    // Current buffer and cursor is inside TextTerminal instance.
    std::unique_ptr<Buffer> m_alternate_buffer = std::make_unique<Buffer>();
    core::Vec2u m_saved_cursor;

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
        bool insert : 1;  // SM 4
        bool app_cursor_keys : 1;  // DECSET 1
        bool autowrap : 1;  // DECSET 7
        bool bracketed_paste : 1;  // TODO
        bool alternate_screen_buffer : 1;  // Normal / Alternate Screen Buffer
    } m_mode = {};
};

} // namespace xci::term

#endif // XCITERM_TERMINAL_H
