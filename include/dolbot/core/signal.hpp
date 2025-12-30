#pragma once

#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <algorithm>

namespace dolbot::core {

class ConnectionHandle {
public:
    ConnectionHandle() = default;
    explicit ConnectionHandle(std::function<void()> disconnect_fn) 
        : disconnect_fn_(std::move(disconnect_fn)) {}
    
    void disconnect() {
        if (disconnect_fn_) {
            disconnect_fn_();
            disconnect_fn_ = nullptr;
        }
    }
    
    ~ConnectionHandle() { disconnect(); }
    
    ConnectionHandle(ConnectionHandle&& other) noexcept 
        : disconnect_fn_(std::move(other.disconnect_fn_)) {
        other.disconnect_fn_ = nullptr;
    }
    
    ConnectionHandle& operator=(ConnectionHandle&& other) noexcept {
        if (this != &other) {
            disconnect();
            disconnect_fn_ = std::move(other.disconnect_fn_);
            other.disconnect_fn_ = nullptr;
        }
        return *this;
    }
    
    ConnectionHandle(const ConnectionHandle&) = delete;
    ConnectionHandle& operator=(const ConnectionHandle&) = delete;

private:
    std::function<void()> disconnect_fn_;
};

template<typename... Args>
class Signal {
public:
    using SlotType = std::function<void(Args...)>;
    using SlotId = std::size_t;
    
    Signal() = default;
    ~Signal() = default;
    
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
    Signal(Signal&&) = delete;
    Signal& operator=(Signal&&) = delete;
    
    ConnectionHandle connect(SlotType slot) {
        std::unique_lock lock(mutex_);
        auto id = next_id_++;
        slots_.emplace_back(id, std::move(slot));
        
        return ConnectionHandle([this, id]() {
            std::unique_lock lock(mutex_);
            slots_.erase(
                std::remove_if(slots_.begin(), slots_.end(),
                    [id](const auto& pair) { return pair.first == id; }),
                slots_.end()
            );
        });
    }
    
    void fire(Args... args) {
        std::vector<SlotType> slots_copy;
        {
            std::shared_lock lock(mutex_);
            slots_copy.reserve(slots_.size());
            for (const auto& [id, slot] : slots_) {
                slots_copy.push_back(slot);
            }
        }
        
        for (const auto& slot : slots_copy) {
            slot(args...);
        }
    }
    
    void operator()(Args... args) {
        fire(args...);
    }
    
    [[nodiscard]] std::size_t connection_count() const {
        std::shared_lock lock(mutex_);
        return slots_.size();
    }

private:
    std::vector<std::pair<SlotId, SlotType>> slots_;
    SlotId next_id_ = 0;
    mutable std::shared_mutex mutex_;
};

template<typename T>
class ObservableValue {
public:
    explicit ObservableValue(T initial = T{}) : value_(std::move(initial)) {}
    
    const T& get() const {
        std::shared_lock lock(mutex_);
        return value_;
    }
    
    void set(T new_value) {
        {
            std::unique_lock lock(mutex_);
            if (value_ == new_value) return;
            value_ = std::move(new_value);
        }
        changed_.fire(value_);
    }
    
    ConnectionHandle on_change(std::function<void(const T&)> callback) {
        return changed_.connect(std::move(callback));
    }
    
    operator const T&() const { return get(); }
    
    ObservableValue& operator=(const T& new_value) {
        set(new_value);
        return *this;
    }

private:
    T value_;
    Signal<const T&> changed_;
    mutable std::shared_mutex mutex_;
};

} // namespace dolbot::core
