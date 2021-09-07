// Pty.cpp created on 2018-07-27, part of XCI term
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

#include "Pty.h"

#include <xci/core/log.h>

#include <cstdlib>
#include <fcntl.h>
#include <cassert>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <csignal>

using namespace xci::core;

namespace xci::term {


void Pty::close()
{
    if (m_master != -1) {
        ::close(m_master);
        m_master = -1;
    }
}


bool Pty::open()
{
    m_master = posix_openpt(O_RDWR);
    if (m_master == -1) {
        log::error("Pty open: posix_openpt: {m}");
        return false;
    }

    if (grantpt(m_master) == -1) {
        log::error("Pty open: grantpt: {m}");
        return false;
    }

    if (unlockpt(m_master) == -1) {
        log::error("Pty open: unlockpt: {m}");
        return false;
    }

    log::info("Pty open: master {}", m_master);
    return true;
}


pid_t Pty::fork()
{
    if (m_master == -1) {
        log::error("Pty not initialized, cannot fork.");
        return (pid_t) -1;
    }
    pid_t child_pid = ::fork();
    if (child_pid == (pid_t) -1) {
        log::error("fork: {m}");
        return (pid_t) -1;
    }
    if (child_pid != 0) {
        log::info("Pty fork: child pid {}", child_pid);
        return child_pid;
    }

    // === child fall-through ===

    constexpr size_t sn_max = 50;
    char slave_name[sn_max];
    if (ptsname_r(m_master, slave_name, sn_max) != 0) {
        log::error("ptsname_r: {m}");
        _exit(-1);
    }
    log::info("Pty fork: slave {}", slave_name);

    // no longer needed in child
    ::close(m_master);

    int slave_fd = ::open(slave_name, O_RDWR);
    if (slave_fd == -1) {
        log::error("open({}): {m}", slave_name);
        _exit(-1);
    }
    assert(slave_fd > STDERR_FILENO);

    int session_id = setsid();
    if (session_id == (pid_t) -1) {
        log::error("setsid: {m}");
        _exit(-1);
    }

    // acquire controlling tty on BSD
#ifdef TIOCSCTTY
    if (ioctl(slave_fd, TIOCSCTTY, 0) == -1) {
        log::error("ioctl({}, TIOCSCTTY): {m}", slave_fd);
        _exit(-1);
    }
#endif

    // TODO
    // set slave tty attributes
    //if (tcsetattr(slave_fd, TCSANOW, termios) == -1)
    // set slave tty window size
    //if (ioctl(slave_fd, TIOCSWINSZ, window_size) == -1)

    // Duplicate pty slave to be child's stdin, stdout, and stderr
    if (dup2(slave_fd, STDIN_FILENO) != STDIN_FILENO) {
        log::error("dup2({}, STDIN_FILENO): {m}", slave_fd);
        _exit(-1);
    }
    if (dup2(slave_fd, STDOUT_FILENO) != STDOUT_FILENO)  {
        log::error("dup2({}, STDOUT_FILENO): {m}", slave_fd);
        _exit(-1);
    }
    if (dup2(slave_fd, STDERR_FILENO) != STDERR_FILENO)  {
        log::error("dup2({}, STDERR_FILENO): {m}", slave_fd);
        _exit(-1);
    }
    ::close(slave_fd);

    return 0;
}


ssize_t Pty::read(char* buffer, size_t size)
{
    for (;;) {
        ssize_t nread = ::read(m_master, buffer, size);
        if (nread == -1) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            log::error("read: {m}");
            return -1;
        }
        return nread;
    }
}


void Pty::write(const std::string &data)
{
    ssize_t rc = ::write(m_master, data.data(), data.size());
    if (rc == -1) {
        log::error("write: {m}");
        return;
    }
    assert(size_t(rc) == data.size());
}


void Pty::set_winsize(core::Vec2u size_chars)
{
    winsize ws = {};
    ws.ws_row = (unsigned short)(size_chars.y);
    ws.ws_col = (unsigned short)(size_chars.x);
    if (ioctl(m_master, TIOCSWINSZ, &ws) == -1) {
        log::error("ioctl: {m}");
        return;
    }
}


} // namespace xci::term
