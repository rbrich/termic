// main.cpp created on 2018-07-27
// This file is part of Termic project <https://github.com/rbrich/termic>
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Terminal.h"
#include "Shell.h"
#include "CircularBuffer.h"
#include <xci/widgets/Theme.h>
#include <xci/widgets/FpsDisplay.h>
#include <xci/graphics/Window.h>
#include <xci/core/file.h>
#include <xci/core/log.h>
#include <xci/core/Vfs.h>
#include <xci/core/dispatch.h>
#include <xci/config.h>

#include <chrono>
#include <cstdlib>

using namespace xci::term;
using namespace xci::widgets;
using namespace xci::graphics;
using namespace xci::core;
using namespace std::chrono_literals;

int main()
{
    Logger::init();
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE_DIR))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};
    window.create({800, 600}, "Termic");

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    Dispatch dispatch;
    CircularBuffer<640 * 1024> buffer;
    Shell shell;
    Terminal terminal (theme, shell);

    if (!shell.start())
        return EXIT_FAILURE;

    IOWatch io_watch(dispatch.loop(), shell.fileno(), IOWatch::Read,
            [&shell, &buffer, &window](int fd, IOWatch::Event event){
        switch (event) {
            case IOWatch::Event::Read: {
                auto wb = buffer.acquire_write_buffer();
                auto nread = shell.read(wb.data(), wb.size());
                if (nread > 0) {
                    buffer.bytes_written(size_t(nread));
                    window.wakeup();
#ifdef __APPLE__
                    // MacOS needs this to give the rendering thread some time slots
                    std::this_thread::sleep_for(50us);
#else
                    std::this_thread::yield();
#endif
                } else {
                    shell.join();
                }
                break;
            }
            case IOWatch::Event::Error:
                shell.stop();
                shell.join();
                break;
            default: break;
        }
    });

    FpsDisplay fps_display {theme};

    window.set_update_callback(
        [&terminal, &buffer, &shell]
        (View& v, std::chrono::nanoseconds elapsed) {
            auto rb = buffer.read_buffer();
            if (!rb.empty()) {
                terminal.decode_input(rb);
                buffer.bytes_read(rb.size());
                v.refresh();
            }
            if (shell.is_closed()) {
                v.window()->close();
            }
        });

    // Make the terminal fullscreen
    window.set_size_callback([&terminal, &fps_display](View& view) {
        auto s = view.viewport_size();
        terminal.set_size(s);
        fps_display.set_position({s.x - 120, 20});
        fps_display.set_size({100, 20});
    });

    Composite root(theme);
    root.add(terminal);
    root.add(fps_display);
    root.set_focus(terminal);

    window.set_key_callback([&](View& view, KeyEvent ev) {
        if (ev.action == Action::Press && ev.mod == ModKey::Shift() && ev.key == Key::F11)
            window.toggle_fullscreen();
    });

    Bind bind(window, root);
    window.set_refresh_mode(RefreshMode::OnDemand);
    window.set_view_mode(ViewOrigin::TopLeft, ViewScale::FixedScreenPixels);
    window.display();

    dispatch.terminate();
    return EXIT_SUCCESS;
}
