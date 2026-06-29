#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <thread>

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

    EventLoop& scheduler() { return *this; }
    const EventLoop& scheduler() const { return *this; }

    ClockT& clock() { return clock_; }
    const ClockT& clock() const { return clock_; }

    uint64_t now_us() const { return clock_.micros(); }

    int call_later_us(uint64_t delay_us, Callback cb) { return add_timer(delay_us, 0, std::move(cb)); }
    int call_every_us(uint64_t period_us, Callback cb) { return add_timer(period_us, period_us, std::move(cb)); }
    int on_delay(uint32_t delay_ms, Callback cb) { return call_later_us(static_cast<uint64_t>(delay_ms) * 1000ULL, std::move(cb)); }
    int on_repeat(uint32_t period_ms, Callback cb) { return call_every_us(static_cast<uint64_t>(period_ms) * 1000ULL, std::move(cb)); }

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
            if (!timer.active || now < timer.due_us) continue;
            auto callback = timer.callback;
            if (timer.period_us == 0) {
                timer.active = false;
                timer.callback = nullptr;
            } else {
                do { timer.due_us += timer.period_us; } while (now >= timer.due_us);
            }
            if (callback) callback();
        }
    }

    void run_once() { run_due_timers(); }
    void tick() { run_once(); }
    void run_forever() { for (;;) { run_once(); sleep_us(1000); } }
    void request_exit() { /* TODO: add exit flag once long-running loop is used. */ }
    void sleep_us(uint64_t us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }

private:
    struct Timer { bool active = false; int id = 0; uint64_t due_us = 0; uint64_t period_us = 0; Callback callback; };

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
        return -1;
    }

    ClockT clock_{};
    std::array<Timer, MaxTimers> timers_{};
    int next_id_ = 1;
};

struct TcpConnectOptions { const char* host = "127.0.0.1"; uint16_t port = 0; };
struct TcpPeerInfo { char host[64]{}; uint16_t port = 0; };

class ITcpConnection {
public:
    virtual ~ITcpConnection() = default;
    virtual bool valid() const = 0;
    virtual void close() = 0;
    virtual const TcpPeerInfo& peer() const = 0;
    virtual size_t input_size() const = 0;
    virtual size_t output_size() const = 0;
    virtual int read(uint8_t* dst, size_t max_len) = 0;
    virtual int write(const uint8_t* src, size_t len) = 0;
    virtual bool read_line(char* dst, size_t max_len, bool strip_cr = true) = 0;
};

class ITcpClientHandler {
public:
    virtual ~ITcpClientHandler() = default;
    virtual void on_connect(ITcpConnection& connection, const TcpPeerInfo& peer) { (void)connection; (void)peer; }
    virtual void on_data(ITcpConnection& connection) { (void)connection; }
    virtual void on_write_ready(ITcpConnection& connection) { (void)connection; }
    virtual void on_close(ITcpConnection& connection) { (void)connection; }
    virtual void on_error(int error_code) { (void)error_code; }
};

template <typename LoopT = EventLoop<> >
class NativeTcpClient {
public:
    explicit NativeTcpClient(LoopT& loop) : loop_(loop) {}

    bool connect(const TcpConnectOptions& options, ITcpClientHandler& handler)
    {
        options_ = options;
        handler_ = &handler;
        // TODO: bind this facade to the Linux/Arduino async-event-loop native TCP backends.
        // The client protocol is already registered as the ITcpClientHandler listener.
        return false;
    }

    bool connected() const { return connection_ && connection_->valid(); }
    ITcpConnection* connection() { return connection_; }
    void close() { if (connection_) connection_->close(); connection_ = nullptr; }

    void attach_for_test(ITcpConnection& connection)
    {
        connection_ = &connection;
        if (handler_) handler_->on_connect(connection, connection.peer());
    }

    void notify_data_for_test()
    {
        if (handler_ && connection_) handler_->on_data(*connection_);
    }

private:
    LoopT& loop_;
    TcpConnectOptions options_{};
    ITcpClientHandler* handler_ = nullptr;
    ITcpConnection* connection_ = nullptr;
};

} // namespace pypilot::syslib::event_loop
