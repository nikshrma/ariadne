

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
class ThreadPool {
private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;

  std::mutex mtx;
  std::condition_variable cv;

  bool stop;

public:
  ThreadPool(size_t numThreads);
  void enqueue(std::function<void()> task);
  ~ThreadPool();
};
