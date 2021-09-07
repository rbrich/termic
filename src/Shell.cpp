// Shell.cpp created on 2018-07-28, part of XCI term
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
#include "Terminal.h"
#include <xci/core/log.h>
#include <unistd.h>
#include <poll.h>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cassert>
#include <pwd.h>
#include <sys/wait.h>

using namespace std::chrono_literals;
using namespace xci::core;

namespace xci::term {


Shell::~Shell()
{
    stop();
    join();
}


bool Shell::start()
{
    if (!m_pty.open())
        return false;

    m_pid = m_pty.fork();
    if (m_pid == -1)
        return false;

    if (m_pid == 0) {
        // child
        ::setenv("TERM", "xterm", 1);
        auto* shell = getpwuid(getuid())->pw_shell;
        if (execlp(shell, shell, nullptr) == -1) {
            log::error("execlp: {m}");
            _exit(-1);
        }
        __builtin_unreachable();
    } else {
        // parent
        assert(m_pid > 0);
        return true;
    }
}


void Shell::stop()
{
    if (m_pid != -1)
        ::kill(m_pid, SIGHUP);
}


ssize_t Shell::read(char* buffer, size_t size)
{
    return m_pty.read(buffer, size);
}


void Shell::write(const std::string &data)
{
    m_pty.write(data);
}


int Shell::join()
{
    m_pty.close();
    if (m_pid == -1)
        return 0;

    int wstatus;
    waitpid(m_pid, &wstatus, 0);
    m_pid = -1;

    if (WIFEXITED(wstatus))
        log::info("Shell exited: {}", WEXITSTATUS(wstatus));
    if (WIFSIGNALED(wstatus))
        log::warning("Shell killed: {}", WTERMSIG(wstatus));
    return wstatus;
}


} // namespace xci::term
