#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <thread>
#include <utility>

namespace async_event_loop {

class IByteStream {
public:
    virtual ~IByteStream() = default;
    virtual int read(uint8_t* dst, size_t max_len) = 0;
    virtual int write(const uint8_t* src, size_t len) = 0;
    virtual bool readable() const = 0;
    virtual bool writable() const = 0;
    virtual bool valid() const = 0;
    virtual int native_fd() const { return -1; }
};

class SteadyClock {
public:
    uint64_t micros() const
    {
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
    }
};

using EventHandle = int;

template <size_t MaxCallbacks = 64, typename ClockT = SteadyClock>
class EventLoop {
public:
    using Callback = std::function<void()>;

    EventLoop& scheduler() { return *this; }
    const EventLoop& scheduler() const { return *this; }
    ClockT& clock() { return clock_; }
    const ClockT& clock() const { return clock_; }
    uint64_t now_us() const { return clock_.micros(); }

    EventHandle on_repeat_us(uint64_t period_us, Callback cb) { return add_timer(period_us, period_us, std::move(cb)); }
    EventHandle on_delay_us(uint64_t delay_us, Callback cb) { return add_timer(delay_us, 0, std::move(cb)); }
    EventHandle on_repeat(uint32_t period_ms, Callback cb) { return on_repeat_us(static_cast<uint64_t>(period_ms) * 1000ULL, std::move(cb)); }
    EventHandle on_delay(uint32_t delay_ms, Callback cb) { return on_delay_us(static_cast<uint64_t>(delay_ms) * 1000ULL, std::move(cb)); }

    EventHandle call_later_us(uint64_t delay_us, Callback cb) { return on_delay_us(delay_us, std::move(cb)); }
    EventHandle call_every_us(uint64_t period_us, Callback cb) { return on_repeat_us(period_us, std::move(cb)); }

    template <typename Callable>
    EventHandle on_bytes_ready(IByteStream& stream, Callable callable, uint64_t cooperative_poll_us = 1000)
    {
        (void)cooperative_poll_us;
        return add_poll([&stream, callable]() mutable {
            if (stream.valid() && stream.readable()) callable();
        });
    }

    void cancel(EventHandle handle)
    {
        if (handle <= 0) return;
        for (auto& slot : slots_) {
            if (slot.active && slot.id == handle) {
                slot.active = false;
                slot.cb = nullptr;
                return;
            }
        }
    }

    void tick() { run_once(); }
    void run_once()
    {
        const uint64_t now = now_us();
        for (auto& slot : slots_) {
            if (!slot.active || slot.cb == nullptr) continue;
            if (slot.periodic_poll || now >= slot.due_us) {
                auto cb = slot.cb;
                if (slot.period_us == 0 && !slot.periodic_poll) {
                    slot.active = false;
                    slot.cb = nullptr;
                } else if (!slot.periodic_poll) {
                    do { slot.due_us += slot.period_us; } while (now >= slot.due_us);
                }
                cb();
            }
        }
    }

    void run_forever()
    {
        for (;;) {
            run_once();
            sleep_us(1000);
        }
    }

    void sleep_us(uint64_t us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }

private:
    struct Slot {
        bool active = false;
        bool periodic_poll = false;
        EventHandle id = 0;
        uint64_t due_us = 0;
        uint64_t period_us = 0;
        Callback cb;
    };

    EventHandle add_timer(uint64_t delay_us, uint64_t period_us, Callback cb)
    {
        for (auto& slot : slots_) {
            if (!slot.active) {
                slot.active = true;
                slot.periodic_poll = false;
                slot.id = next_id_++;
                slot.due_us = now_us() + delay_us;
                slot.period_us = period_us;
                slot.cb = std::move(cb);
                return slot.id;
            }
        }
        return 0;
    }

    EventHandle add_poll(Callback cb)
    {
        for (auto& slot : slots_) {
            if (!slot.active) {
                slot.active = true;
                slot.periodic_poll = true;
                slot.id = next_id_++;
                slot.cb = std::move(cb);
                return slot.id;
            }
        }
        return 0;
    }

    ClockT clock_{};
    std::array<Slot, MaxCallbacks> slots_{};
    EventHandle next_id_ = 1;
};

} // namespace async_event_loop
