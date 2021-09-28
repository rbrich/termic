// Shell.h created on 2018-07-28
// This file is part of Termic project <https://github.com/rbrich/termic>
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCITERM_SHELL_H
#define XCITERM_SHELL_H

#include "Pty.h"
#include <xci/graphics/Window.h>
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
