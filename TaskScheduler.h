#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class TaskScheduler {
  public:
    TaskScheduler(const unsigned int max_simultaneous_tasks = 8)
        : stop_flag{false} {
        threads.reserve(max_simultaneous_tasks);
        for (int i = 0; i < max_simultaneous_tasks; ++i) {
            threads.emplace_back(&TaskScheduler::worker, this);
        }
    }

    ~TaskScheduler() {
        stop_flag = true;
        cv.notify_all();

        for (auto &thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void Add(const std::function<void()> task, const std::time_t timestamp) {
        std::lock_guard<std::mutex> queue_lock(queue_mutex);
        tasks_queue.emplace(std::make_unique<std::function<void()>>(task),
                            timestamp);
        cv.notify_one();
    }

  private:
    class Task {
      public:
        Task(std::unique_ptr<std::function<void()>> func, std::time_t timestamp)
            : func{func}, timestamp{timestamp} {}

        void operator()() { (*func)(); }

        std::unique_ptr<std::function<void()>> func;
        std::time_t timestamp;
    };

    class Compare {
      public:
        bool operator()(const Task &a, const Task &b) {
            return a.timestamp > b.timestamp;
        }
    };

    std::vector<std::thread> threads;

    std::priority_queue<Task, std::vector<Task>, Compare> tasks_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;

    std::atomic<bool> stop_flag;

    void worker() {
        while (true) {
            std::unique_lock<std::mutex> queue_lock(queue_mutex);

            if (stop_flag && tasks_queue.empty()) {
                return;
            }

            if (tasks_queue.empty()) {
                cv.wait(queue_lock);
            } else {
                auto now = std::time(nullptr);
                if (tasks_queue.top().timestamp <= now) {
                    auto task = std::move(tasks_queue.top());
                    tasks_queue.pop();
                    queue_lock.unlock();
                    task();
                } else {
                    cv.wait_until(queue_lock,
                                  std::chrono::system_clock::from_time_t(
                                      tasks_queue.top().timestamp));
                }
            }
        }
    }
};