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

#include "Terminal.h"
#include "Shell.h"
#include "SyncedBuffer.h"
#include <xci/widgets/Theme.h>
#include <xci/widgets/FpsDisplay.h>
#include <xci/graphics/Window.h>
#include <xci/core/file.h>
#include <xci/core/log.h>
#include <xci/core/Vfs.h>
#include <xci/core/dispatch.h>
#include <xci/core/chrono.h>
#include <xci/config.h>
#include <cstdlib>

using namespace xci::term;
using namespace xci::widgets;
using namespace xci::graphics;
using namespace xci::core;

int main()
{
    Logger::init();
    Vfs::default_instance().mount(XCI_SHARE_DIR);

    Window& window = Window::default_instance();
    window.create({800, 600}, "XCI Term");

    if (!Theme::load_default_theme())
        return EXIT_FAILURE;

    Dispatch dispatch;
    SyncedBuffer buffer;
    Shell shell;
    auto terminal = std::make_shared<Terminal>(shell);

    if (!shell.start())
        return EXIT_FAILURE;

    IOWatch io_watch(dispatch.loop(), shell.fileno(), IOWatch::Read,
            [&shell, &buffer, &window](int fd, IOWatch::Event event){
        switch (event) {
            case IOWatch::Event::Read: {
                auto wb = buffer.write_buffer();
                auto nread = shell.read(wb.data, wb.size);
                if (nread > 0) {
                    wb.size = nread;
                    buffer.bytes_written(wb);
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

    auto fps_display = std::make_shared<FpsDisplay>();

    window.set_update_callback(
        [&terminal, &buffer, &shell]
        (View& v, std::chrono::nanoseconds elapsed) {
            auto rb = buffer.read_buffer();
            if (rb.size > 0) {
                terminal->decode_input({rb.data, rb.size});
                buffer.bytes_read(rb);
                v.refresh();
            }
            if (shell.is_closed()) {
                v.window()->close();
            }
        });

    // Make the terminal fullscreen
    window.set_size_callback([terminal, fps_display](View& view) {
        auto s = view.scalable_size();
        terminal->set_position({-s * 0.5f});
        terminal->set_size(s);
        fps_display->set_position({s.x * 0.5f - 0.51f, -s.y * 0.5f + 0.01f});
    });

    Composite root;
    root.add(terminal);
    root.add(fps_display);
    root.set_focus(terminal);

    Bind bind(window, root);
    window.set_refresh_mode(RefreshMode::OnDemand);
    window.set_refresh_interval(1);  // 0 = unlimited, 1 = vsync (60fps max, works fine with OnDemand)
    window.display();

    dispatch.terminate();
    return EXIT_SUCCESS;
}
