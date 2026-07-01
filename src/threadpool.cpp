

#include "threadpool.h"
#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>
ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
  for (int i = 0; i < numThreads; i++) {
    workers.emplace_back([this] {
      while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return stop || !tasks.empty(); });
        if (stop || tasks.empty())
          return;
        auto task = std::move(tasks.front());
        tasks.pop();
        lock.unlock();
        task();
      }
    });
  }
}
void ThreadPool::enqueue(std::function<void()> task) {
  // braces make it so that the lock is released without explicit mentioning
  {
    std::unique_lock<std::mutex> lock(mtx);
    tasks.push(std::move(task));
  }
  cv.notify_one();
}
ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(mtx);
    stop = true;
  }
  cv.notify_all();
  for (std::thread &worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}
