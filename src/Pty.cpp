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

#include <stdlib.h>
#include <fcntl.h>
#include <cassert>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <csignal>

using namespace xci::core::log;

namespace xci {


Pty::~Pty()
{
    if (m_master != -1)
        ::close(m_master);
}


bool Pty::open()
{
    m_master = posix_openpt(O_RDWR);
    if (m_master == -1) {
        log_error("posix_openpt: {m}");
        return false;
    }

    if (grantpt(m_master) == -1) {
        log_error("grantpt: {m}");
        return false;
    }

    if (unlockpt(m_master) == -1) {
        log_error("unlockpt: {m}");
        return false;
    }

    // Blocking mode is fine by now
#if 0
    if (fcntl(m_master, F_SETFL, fcntl(m_master, F_GETFL) | O_NONBLOCK) == -1) {
        log_error("fcntl: {m}");
        return false;
    }
#endif

    log_info("Pty open: master {}", m_master);
    return true;
}


pid_t Pty::fork()
{
    if (m_master == -1) {
        log_error("Pty not initialized, cannot fork.");
        return (pid_t) -1;
    }
    pid_t child_pid = ::fork();
    if (child_pid == (pid_t) -1) {
        log_error("fork: {m}");
        return (pid_t) -1;
    }
    if (child_pid != 0) {
        log_info("Pty fork: child pid {}", child_pid);
        return child_pid;
    }

    // === child fall-through ===

    constexpr size_t sn_max = 50;
    char slave_name[sn_max];
    if (ptsname_r(m_master, slave_name, sn_max) != 0) {
        log_error("ptsname_r: {m}");
        _exit(-1);
    }
    log_info("Pty fork: slave {}", slave_name);

    // no longer needed in child
    ::close(m_master);

    int slave_fd = ::open(slave_name, O_RDWR);
    if (slave_fd == -1) {
        log_error("open({}): {m}", slave_name);
        _exit(-1);
    }
    assert(slave_fd > STDERR_FILENO);

    int session_id = setsid();
    if (session_id == (pid_t) -1) {
        log_error("setsid: {m}");
        _exit(-1);
    }

    // acquire controlling tty on BSD
#ifdef TIOCSCTTY
    if (ioctl(slave_fd, TIOCSCTTY, 0) == -1) {
        log_error("ioctl({}, TIOCSCTTY): {m}", slave_fd);
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
        log_error("dup2({}, STDIN_FILENO): {m}", slave_fd);
        _exit(-1);
    }
    if (dup2(slave_fd, STDOUT_FILENO) != STDOUT_FILENO)  {
        log_error("dup2({}, STDOUT_FILENO): {m}", slave_fd);
        _exit(-1);
    }
    if (dup2(slave_fd, STDERR_FILENO) != STDERR_FILENO)  {
        log_error("dup2({}, STDERR_FILENO): {m}", slave_fd);
        _exit(-1);
    }
    ::close(slave_fd);

    return 0;
}


int Pty::poll()
{
    pollfd pfd = {
            m_master, // fd
            POLLIN, // events
            0,  // revents
    };

    int rc = ::poll(&pfd, 1, -1);
    if (rc == -1) {
        log_debug("poll: {m}");
        return -1;
    }
    if (rc == 0) {
        // timeout
        return 0;
    }
    return pfd.revents;
}


size_t Pty::read(char* buffer, size_t size)
{
    ssize_t nread = ::read(m_master, buffer, size);
    if (nread == -1) {
        if (errno != EAGAIN)
            log_error("read: {m}");
        return 0;
    }
    if (nread == 0) {
        // EOF
        log_error("Pty slave closed");
        ::close(m_master);
        m_master = -1;
        return 0;
    }
    return size_t(nread);
}


void Pty::write(const std::string &data)
{
    ssize_t rc = ::write(m_master, data.data(), data.size());
    if (rc == -1) {
        log_error("write: {m}");
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
        log_error("ioctl: {m}");
        return;
    }
}


} // namespace xci
