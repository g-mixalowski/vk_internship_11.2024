#pragma once

#include <functional>

// TODO

class ThreadPool {
public:

    ThreadPool(int max_tasks):max_tasks{max_tasks} { }

    void add(std::function<void()> task) {
        task();
    }

private:
    int max_tasks;
};