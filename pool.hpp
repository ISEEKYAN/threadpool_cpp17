#pragma once
#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

namespace pool {
class ThreadPool {
 public:
  ThreadPool(int num_thread = 0) {
    if (num_thread == 0) num_thread = std::thread::hardware_concurrency();
    for (int i = 0; i < num_thread; ++i) {
      threads_.push_back(std::thread([this] { Routine(); }));
    }
  }

  template <typename T>
  void push_task(T&& task) {
    std::unique_lock lk(mtx_);
    tasks_.push_back(std::forward<T>(task));
    ready_ = true;
    cv_.notify_one();
  }

  template <typename T>
  auto submit(T&& t) {
    using R = std::invoke_result_t<std::decay_t<T>>;
    std::shared_ptr<std::promise<R>> task_promise(new std::promise<R>);
    std::future<R> future = task_promise->get_future();
    push_task([t, task_promise] {
      try {
        if constexpr (std::is_same_v<void, R>) {
          t();
          task_promise->set_value();
        } else {
          task_promise->set_value(t());
        }
      } catch (...) {
        try {
          task_promise->set_exception(std::current_exception());
        } catch (...) {
        }
      }
    });
    return future;
  }

  void Sync() {
    while (running_cnt > 0 || tasks_.size() > 0) {
      std::this_thread::yield();
    }
  }

  ~ThreadPool() {
    Close();
    for (auto& t : threads_) {
      t.join();
    }
  }

  void Close() {
    Sync();
    std::unique_lock lk(mtx_);
    close_ = true;
    ready_ = true;
    cv_.notify_all();
  }

 private:
  void Routine() {
    while (!close_) {
      std::unique_lock lk(mtx_);
      cv_.wait(lk, [this] { return ready_; });
      if (close_) return;
      if (tasks_.size() == 0) {
        ready_ = false;
        continue;
      }
      auto task = tasks_[0];
      tasks_.pop_front();
      lk.unlock();
      running_cnt++;
      task();
      running_cnt--;
    }
  }
  bool close_ = false;
  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<std::thread> threads_;
  std::deque<std::function<void()>> tasks_;
  std::atomic<int> running_cnt{0};
  bool ready_ = false;
};
}  // namespace pool
