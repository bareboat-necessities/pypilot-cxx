#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>
#include <utility>

namespace pypilot::syslib::event_loop {

class SteadyClock {
public:
    uint64_t micros() const
    {
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
    }
};

template <size_t MaxTimers = 32, typename ClockT = SteadyClock>
class EventLoop {
public:
    using Callback = std::function<void()>;

    EventLoop() = default;

    ClockT& clock() { return clock_; }
    const ClockT& clock() const { return clock_; }

    uint64_t now_us() const { return clock_.micros(); }

    int call_later_us(uint64_t delay_us, Callback cb)
    {
        return add_timer(delay_us, 0, std::move(cb));
    }

    int call_every_us(uint64_t period_us, Callback cb)
    {
        return add_timer(period_us, period_us, std::move(cb));
    }

    void cancel(int id)
    {
        for (auto& timer : timers_) {
            if (timer.active && timer.id == id) {
                timer.active = false;
                timer.callback = nullptr;
                return;
            }
        }
    }

    void run_due_timers()
    {
        const uint64_t now = now_us();
        for (auto& timer : timers_) {
            if (!timer.active || now < timer.due_us) {
                continue;
            }

            auto callback = timer.callback;
            if (timer.period_us == 0) {
                timer.active = false;
                timer.callback = nullptr;
            } else {
                do {
                    timer.due_us += timer.period_us;
                } while (now >= timer.due_us);
            }

            if (callback) {
                callback();
            }
        }
    }

    void run_once()
    {
        run_due_timers();
        // TODO: integrate Linux fd readiness and Arduino stream polling here.
    }

    void sleep_us(uint64_t us)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }

private:
    struct Timer {
        bool active = false;
        int id = 0;
        uint64_t due_us = 0;
        uint64_t period_us = 0;
        Callback callback;
    };

    int add_timer(uint64_t delay_us, uint64_t period_us, Callback cb)
    {
        for (auto& timer : timers_) {
            if (!timer.active) {
                timer.active = true;
                timer.id = next_id_++;
                timer.due_us = now_us() + delay_us;
                timer.period_us = period_us;
                timer.callback = std::move(cb);
                return timer.id;
            }
        }
        // TODO: match uploaded fixed-storage event-loop overflow policy.
        return -1;
    }

    ClockT clock_{};
    std::array<Timer, MaxTimers> timers_{};
    int next_id_ = 1;
};

} // namespace pypilot::syslib::event_loop
