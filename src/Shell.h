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
#include <xci/compat/string_view.h>
#include <thread>
#include <atomic>
#include <array>
#include <functional>

namespace xci {

class Terminal;


// Run actual shell (e.g. Bash) in child process,
// with established PTY.
class Shell {
public:
    ~Shell();

    using ReadCallback = std::function<void(std::string_view)>;
    using ExitCallback = std::function<void(int status)>;

    bool start(ReadCallback read_cb, ExitCallback exit_cb);

    void write(const std::string& data);

    Pty& pty() { return m_pty; }

private:
    void thread_main();
    void read();

private:
    Pty m_pty;
    pid_t m_pid = -1;
    std::thread m_thread;
    ReadCallback m_read_cb;
    ExitCallback m_exit_cb;

    // Synchronized read buffer
    static constexpr size_t c_read_max = 64 * 1024;
    std::array<char, c_read_max> m_read_buffer {};
};


} // namespace xci

#endif // XCITERM_SHELL_H
