// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

namespace spdlog {
namespace details {
inline backtracer::backtracer(const backtracer &other)
{
    std::lock_guard<std::mutex> lock(other.mutex_);
    enabled_ = other.enabled();
    messages_ = other.messages_;
}

inline backtracer::backtracer(backtracer &&other) noexcept
{
    std::lock_guard<std::mutex> lock(other.mutex_);
    enabled_ = other.enabled();
    messages_ = std::move(other.messages_);
}

inline backtracer &backtracer::operator=(backtracer other)
{
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = other.enabled();
    messages_ = std::move(other.messages_);
    return *this;
}

inline void backtracer::enable(size_t size)
{
    std::lock_guard<std::mutex> lock{mutex_};
    enabled_.store(true, std::memory_order_relaxed);
    messages_ = circular_q<log_msg_buffer>{size};
}

inline void backtracer::disable()
{
    std::lock_guard<std::mutex> lock{mutex_};
    enabled_.store(false, std::memory_order_relaxed);
}

inline bool backtracer::enabled() const
{
    return enabled_.load(std::memory_order_relaxed);
}

inline void backtracer::push_back(const log_msg &msg)
{
    std::lock_guard<std::mutex> lock{mutex_};
    messages_.push_back(log_msg_buffer{msg});
}

// pop all items in the q and apply the given fun on each of them.
inline void backtracer::foreach_pop(std::function<void(const details::log_msg &)> fun)
{
    std::lock_guard<std::mutex> lock{mutex_};
    while (!messages_.empty())
    {
        auto &front_msg = messages_.front();
        fun(front_msg);
        messages_.pop_front();
    }
}
} // namespace details
} // namespace spdlog
