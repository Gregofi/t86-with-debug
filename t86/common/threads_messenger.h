#pragma once
#include "messenger.h"
#include <mutex>
#include <condition_variable>
#include <queue>

/// Packs together needed things for shared data queue.
template<typename T>
struct ThreadQueue {
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv;
};

class ThreadMessenger: public Messenger {
public:
    /// @param in - Sends the messages to this queue
    /// @param out - Receives messages from this queue
    ThreadMessenger(ThreadQueue<std::string> &in, ThreadQueue<std::string>& out): in(in), out(out) {
    }

    void Send(const std::string& message) override {
        std::lock_guard l(in.m);
        in.q.push(message);
        in.cv.notify_one();
    }

    std::optional<std::string> Receive() override {
        std::unique_lock<std::mutex> l(out.m);
        out.cv.wait(l, [this]{ return !this->out.q.empty();});
        auto response = out.q.front();
        out.q.pop();
        return response;
    }

private:
    ThreadQueue<std::string>& in;
    ThreadQueue<std::string>& out;
};
