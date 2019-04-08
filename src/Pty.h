// Pty.h created on 2018-07-27, part of XCI term
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

#ifndef XCITERM_PTY_H
#define XCITERM_PTY_H

#include <string>
#include <xci/core/geometry.h>

namespace xci::term {


// Open pseudo-terminal and fork slave process.
class Pty {
public:
    ~Pty() { close(); }

    /// Open master PTY device, returns true on success.
    bool open();

    /// Close master PTY. Safe to call when already closed.
    void close();

    /// True when PTY is closed.
    bool is_closed() const { return m_master == -1; }

    /// Fork and open slave PTY in child.
    /// \return     -1 on error, PID >0 from parent, 0 from child
    pid_t fork();

    /// Master PTY file descriptor for event polling.
    int fileno() const { return m_master; }

    /// Blocking read
    /// \return     -1 on error, 0 on EOF, N (bytes read) on success
    ssize_t read(char* buffer, size_t size);

    /// Blocking write
    void write(const std::string &data);

    /// Set window size in characters
    void set_winsize(core::Vec2u size_chars);

private:
    int m_master = -1;
};


} // namespace xci::term

#endif // XCITERM_PTY_H
