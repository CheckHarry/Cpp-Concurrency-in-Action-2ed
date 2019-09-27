#include <thread>

class thread_guard {
 public:
  explicit thread_guard(std::thread& t) : t_(t) {}
  ~thread_guard() {
    if (t_.joinable()) {
      t_.join();
    }
  }
  thread_guard(const thread_guard&) = delete;
  thread_guard& operator=(const thread_guard&) = delete;

 private:
  std::thread& t_;
};

int main() {
  std::thread t{[] {}};
  thread_guard g{t};
}
