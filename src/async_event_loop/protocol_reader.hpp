#pragma once

#include <cstddef>
#include <cstdint>

#include "async_event_loop/event_loop.hpp"

namespace async_event_loop {

struct LineView {
    const char* data = nullptr;
    size_t size = 0;
};

struct LineProtocolOptions {
    constexpr LineProtocolOptions() = default;
    constexpr LineProtocolOptions(char delimiter_value, bool strip_cr_value = true, bool drop_overflow_value = true)
        : delimiter(delimiter_value), strip_cr(strip_cr_value), drop_overflow(drop_overflow_value) {}

    char delimiter = '\n';
    bool strip_cr = true;
    bool drop_overflow = true;
};

struct ProtocolReaderStats {
    uint32_t messages = 0;
    uint32_t bytes = 0;
    uint32_t overflows = 0;
};

template <size_t BufferSize = 256>
class LineProtocolReader {
public:
    using Callback = void (*)(void* context, LineView line);

    explicit LineProtocolReader(IByteStream& stream, LineProtocolOptions options = {})
        : stream_(stream), options_(options) {}

    void on_data_ready(void* context, Callback callback)
    {
        context_ = context;
        callback_ = callback;
    }

    void reset()
    {
        size_ = 0;
        stats_ = {};
    }

    void poll(uint64_t)
    {
        uint8_t byte = 0;
        while (stream_.readable()) {
            const int n = stream_.read(&byte, 1);
            if (n <= 0) break;
            ++stats_.bytes;
            consume(static_cast<char>(byte));
        }
    }

    const ProtocolReaderStats& stats() const { return stats_; }

private:
    void consume(char c)
    {
        if (c == options_.delimiter) {
            emit_line();
            return;
        }
        if (size_ >= BufferSize) {
            ++stats_.overflows;
            if (options_.drop_overflow) size_ = 0;
            return;
        }
        buffer_[size_++] = c;
    }

    void emit_line()
    {
        size_t len = size_;
        if (options_.strip_cr && len > 0 && buffer_[len - 1] == '\r') --len;
        if (callback_) callback_(context_, LineView{buffer_, len});
        ++stats_.messages;
        size_ = 0;
    }

    IByteStream& stream_;
    LineProtocolOptions options_{};
    char buffer_[BufferSize]{};
    size_t size_ = 0;
    ProtocolReaderStats stats_{};
    void* context_ = nullptr;
    Callback callback_ = nullptr;
};

} // namespace async_event_loop
