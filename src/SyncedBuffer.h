// SyncedBuffer.h created on 2019-04-07, part of XCI toolkit
// Copyright 2019 Radek Brich
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

#ifndef XCITERM_SYNCEDBUFFER_H
#define XCITERM_SYNCEDBUFFER_H

#include <atomic>
#include <array>
#include <cstddef>  // size_t

namespace xci::term {


/// Synchronized cyclical buffer
///
/// read_p (or R) is read pointer (not real pointer, just a cursor in the buffer)
/// write_p (or W) is write pointer
/// size (or S) is total size of the buffer
///
/// Possible states:
/// 1. R == W        -- empty buffer (nothing to read)
/// 2. R < W         -- (W - R) bytes available for reading
/// 3. R > W         -- W cycled, (S - R) bytes available for reading
/// 4. R == W+1      -- full buffer (nowhere to write)
///
/// In states 1. and 2. the writer can send (S - W) bytes
/// In states 3. and 4. the writer can send (R - W - 1) bytes
///
/// Whenever the writer fills the buffer to the end, W is reset to 0.
/// Whenever the reader reads to the end of buffer (state 3.), R is reset to 0.

class SyncedBuffer {
public:

    // writer - moves write_p, checks read_p

    struct WBuf {
        char* data;
        std::size_t size;
    };

    WBuf write_buffer() {
        auto w = m_write_p.load(std::memory_order_acquire);
        auto r = m_read_p.load(std::memory_order_acquire);
        if (r <= w) {
            return {m_buffer.data() + w, m_buffer.size() - w};
        } else {  // r > w
            return {m_buffer.data() + w, size_t(r - w - 1)};
        }
    }

    void bytes_written(const WBuf& v) {
        int w = v.data - m_buffer.data();
        if (w + v.size == m_buffer.size()) {
            m_write_p.store(0, std::memory_order_release);
        } else {
            m_write_p.store(w + v.size, std::memory_order_release);
        }
    }

    // reader - moves read_p, checks write_p

    struct RBuf {
        const char* data;
        std::size_t size;
    };

    RBuf read_buffer() const {
        auto w = m_write_p.load(std::memory_order_acquire);
        auto r = m_read_p.load(std::memory_order_acquire);
        if (r <= w) {
            return {m_buffer.data() + r, size_t(w - r)};
        } else {
            return {m_buffer.data() + r, m_buffer.size() - r};
        }
    }

    void bytes_read(const RBuf& v) {
        int r = v.data - m_buffer.data();
        if (r + v.size == m_buffer.size()) {
            m_read_p.store(0, std::memory_order_release);
        } else {
            m_read_p.store(r + v.size, std::memory_order_release);
        }
    }

private:
    std::array<char, 640 * 1024> m_buffer;
    std::atomic<int> m_write_p {0};
    std::atomic<int> m_read_p {0};
};


} // namespace xci::term

#endif // include guard
