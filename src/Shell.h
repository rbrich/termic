// Shell.h created on 2018-07-28, part of XCI term
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

#ifndef XCITERM_SHELL_H
#define XCITERM_SHELL_H

#include "Pty.h"
#include <xci/graphics/Window.h>
#include <thread>
#include <atomic>

namespace xci {


class Shell {
public:
    explicit Shell(const xci::graphics::Window &window) : m_window(window) {}
    ~Shell();

    bool start();

    bool data_ready() const;
    std::string read();

    void write(const std::string& data);

    Pty& pty() { return m_pty; }

private:
    void thread_main();

private:
    const xci::graphics::Window& m_window;  // only for wakeup()
    Pty m_pty;
    pid_t m_pid = -1;
    std::thread m_thread;

    // Synchronized read buffer
    static constexpr size_t c_read_max = 4 * 1024;
    std::array<char, c_read_max> m_read_buffer;
    size_t m_read_size = 0;
    std::atomic_bool m_data_ready {false};
};


} // namespace xci

#endif // XCITERM_SHELL_H
