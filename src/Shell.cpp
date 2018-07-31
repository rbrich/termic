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


Shell::~Shell()
{
    ::kill(m_pid, SIGTERM);
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
        ::setenv("TERM", "ansi", 1);
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
    std::lock_guard<std::mutex> guard(m_mutex);
    return !m_data.empty();
}


std::string Shell::read()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::string result = std::move(m_data);
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
        if (rc & POLLIN) {
            auto data = m_pty.read();
            std::lock_guard<std::mutex> guard(m_mutex);
            m_data += data;
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
