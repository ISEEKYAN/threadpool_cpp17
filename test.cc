#include <chrono>
#include <iostream>

#include "pool.hpp"
using namespace std;

int64_t fibonacci(int64_t i) {
  if (i == 1) return 1;
  if (i == 2) return 2;
  return fibonacci(i - 1) + fibonacci(i - 2);
}

struct Timer {
  auto Now() { return chrono::steady_clock::now(); };
  Timer() { t0 = Now(); }
  ~Timer() {
    auto t = Now();
    cout << "time in ms: "
         << chrono::duration_cast<chrono::milliseconds>(t - t0).count() << "\n";
  }
  decltype(chrono::steady_clock::now()) t0;
};

int main() {
  int i1 = 40;
  int i2 = 44;
  cout << "single thread: \n";
  {
    Timer _;
    for (int i = i1; i < i2; ++i) {
      cout << fibonacci(i) << " \n";
    }
  }
  cout << "example without return\n";
  {
    Timer _;
    using namespace pool;
    ThreadPool T(4);
    for (int i = i1; i < i2; ++i) {
      T.push_task([i] { cout << fibonacci(i) << " \n"; });
    }
  }
  cout << "example returning future<int64_t>\n";
  {
    Timer _;
    using namespace pool;
    ThreadPool T(4);
    vector<future<int64_t>> futs;
    for (int i = i1; i < i2; ++i) {
      futs.push_back(T.submit([i]() { return fibonacci(i); }));
    }
    for (auto& f : futs) {
      cout << f.get() << endl;
    }
  }
  cout << "example returning promise<void>\n";
  {
    Timer _;
    using namespace pool;
    ThreadPool T(4);
    vector<future<void>> futs;
    for (int i = i1; i < i2; ++i) {
      futs.push_back(T.submit([i]() { cout << fibonacci(i) << " \n"; }));
    }
    for (auto& f : futs) {
      f.get();
    }
  }
}