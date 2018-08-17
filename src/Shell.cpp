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
#include <xci/util/log.h>
#include <unistd.h>
#include <poll.h>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cassert>
#include <pwd.h>
#include <sys/wait.h>

using namespace std::chrono_literals;
using namespace xci::util::log;

namespace xci {


Shell::~Shell()
{
    ::kill(m_pid, SIGHUP);
    if (m_thread.joinable())
        m_thread.join();
}


bool Shell::start()
{
    if (!m_pty.open())
        return false;

    pid_t pid = m_pty.fork();
    if (pid == -1)
        return false;
    if (pid == 0) {
        // child
        ::setenv("TERM", "xterm", 1);
        auto* shell = getpwuid(getuid())->pw_shell;
        if (execlp(shell, shell, nullptr) == -1) {
            log_error("execlp: {m}");
            _exit(-1);
        }
        assert(!"not reached");
        _exit(-1);
    }

    // parent
    m_pid = pid;
    m_thread = std::thread([this]() { thread_main(); });
    return true;
}


bool Shell::data_ready() const
{
    return m_data_ready.load(std::memory_order_acquire);
}


std::string Shell::read()
{
    assert(m_data_ready.load(std::memory_order_acquire));
    std::string result(m_read_buffer.data(), m_read_size);
    m_data_ready.store(false, std::memory_order::memory_order_release);
    return result;
}


void Shell::write(const std::string &data)
{
    m_pty.write(data);
}


void Shell::thread_main()
{
    while (!m_pty.eof()) {
        int rc = m_pty.poll();
        if (rc == -1 || rc & (POLLERR | POLLHUP))
            break;
        if (rc & POLLIN && !m_data_ready.load(std::memory_order_acquire)) {
            m_read_size = m_pty.read(m_read_buffer.data(), c_read_max);
            m_data_ready.store(true, std::memory_order::memory_order_release);
            m_window.wakeup();
            std::this_thread::yield();
        }
    }
    int wstatus;
    waitpid(m_pid, &wstatus, 0);
    if (WIFEXITED(wstatus))
        log_info("Shell exited: {}", WEXITSTATUS(wstatus));
    if (WIFSIGNALED(wstatus))
        log_warning("Shell killed: {}", WTERMSIG(wstatus));
    m_window.close();
}


} // namespace xci
