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
#include <atomic>
#include <array>
#include <functional>

namespace xci::term {

class Terminal;


// Run actual shell (e.g. Bash) in child process,
// with established PTY.
class Shell {
public:
    ~Shell();

    bool start();
    void stop();

    // following usable after start()
    int fileno() const { return m_pty.fileno(); }
    ssize_t read(char* buffer, size_t size);
    void write(const std::string& data);
    int join();
    bool is_closed() const { return m_pty.is_closed(); }

    Pty& pty() { return m_pty; }

private:
    Pty m_pty;
    pid_t m_pid = -1;
};


} // namespace xci::term

#endif // XCITERM_SHELL_H
