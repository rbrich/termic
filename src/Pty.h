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


class Pty {
public:
    bool open();

    // Returns -1 on error, PID >0 from parent, 0 from child
    pid_t fork();

    std::string read();

private:
    int m_master = -1;
    int m_child_pid = -1;
};


#endif // XCITERM_PTY_H
