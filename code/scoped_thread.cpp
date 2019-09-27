#include <stdexcept>
#include <thread>
#include <utility>

class scoped_thread {
 public:
  explicit scoped_thread(std::thread x) : t_(std::move(x)) {
    if (!t_.joinable()) {
      throw std::logic_error("no thread");
    }
  }
  ~scoped_thread() { t_.join(); }
  scoped_thread(const scoped_thread&) = delete;
  scoped_thread& operator=(const scoped_thread&) = delete;

 private:
  std::thread t_;
};

int main() {
  scoped_thread t{std::thread{[] {}}};
}