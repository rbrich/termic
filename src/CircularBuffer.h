// CircularBuffer.h created on 2019-04-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCITERM_CIRCULARBUFFER_H
#define XCITERM_CIRCULARBUFFER_H

#include <atomic>
#include <semaphore>
#include <array>
#include <string_view>
#include <span>
#include <cstddef>  // size_t

namespace xci::term {


/// SPSC-synchronized circular buffer
/// (single producer, single consumer)
///
/// write_p (W) is write index (head)
/// read_p (R) is read index (tail)
/// size (S) is total size of the buffer
///
/// Possible states:
/// 1. R == W        -- empty buffer (nothing to read)
/// 2. R < W         -- (W - R) bytes available for reading
/// 3. R > W+1       -- W cycled, (S - R + W) bytes available for reading
/// 4. R == W+1      -- full buffer (nowhere to write)
///
/// In states 1. and 2. (R <= W) the writer can send (S - W) bytes
/// In states 3. and 4. (R > W) the writer can send (R - W - 1) bytes
///
/// Whenever the writer fills the buffer to the end, W is cycled to 0.
/// Whenever the reader reads to the end of buffer (state 3.), R is cycled to 0.
///
/// References:
/// * https://en.wikipedia.org/wiki/Circular_buffer
/// * https://www.kernel.org/doc/html/latest/core-api/circular-buffers.html
///
/// \tparam Size    size of the buffer

template <size_t Size>
class CircularBuffer {
public:

    // writer - moves write_p, checks read_p

    /// Get the part of buffer ready for writing.
    /// \returns Empty span when the buffer is full.
    std::span<char> write_buffer() {
        // We're the only writer - W can't change, R may grow or cycle
        auto r = m_read_p.load(std::memory_order_acquire);
        auto w = m_write_p.load(std::memory_order_relaxed);
        if (r <= w) {
            return {m_buffer.data() + w, m_buffer.size() - w};
        } else {  // r > w
            return {m_buffer.data() + w, size_t(r - w - 1)};
        }
    }

    /// Get the part of buffer ready for writing.
    /// Block if the buffer is full.
    std::span<char> acquire_write_buffer() {
        auto wbuf = write_buffer();
        if (wbuf.empty()) {
            // Buffer is full, nowhere to write
            // The race condition is not a problem:
            // * writer stores full=true
            // * reader reads it and resets it
            // * reader releases full_sem - counter -> 1
            // * writer acquires full_sem - doesn't block, counter -> 0
            // Normally, writer blocks on acquire until reader releases full_sem.
            m_full.store(true);
            m_full_sem.acquire();
            return write_buffer();
        }
        return wbuf;
    }

    void bytes_written(size_t written) {
        auto w = m_write_p.load(std::memory_order_relaxed);
        if (w + written == m_buffer.size()) {
            m_write_p.store(0, std::memory_order_release);
        } else {
            m_write_p.store(w + written, std::memory_order_release);
        }
    }

    // reader - moves read_p, checks write_p

    std::string_view read_buffer() const {
        // We're the only reader - R can't change, W may grow or cycle
        auto w = m_write_p.load(std::memory_order_acquire);
        auto r = m_read_p.load(std::memory_order_relaxed);
        if (r <= w) {
            return {m_buffer.data() + r, size_t(w - r)};
        } else {  // r > w
            return {m_buffer.data() + r, m_buffer.size() - r};
        }
    }

    void bytes_read(size_t read) {
        auto r = m_read_p.load(std::memory_order_relaxed);
        if (r + read == m_buffer.size()) {
            m_read_p.store(0, std::memory_order_release);
        } else {
            m_read_p.store(r + read, std::memory_order_release);
        }
        auto was_full = m_full.exchange(false);
        if (was_full)
            m_full_sem.release();
    }

private:
    std::array<char, Size> m_buffer;
    std::atomic<unsigned> m_write_p {0};
    std::atomic<unsigned> m_read_p {0};
    std::binary_semaphore m_full_sem {0};
    std::atomic_bool m_full {false};
};


} // namespace xci::term

#endif // include guard
