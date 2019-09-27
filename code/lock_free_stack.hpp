#include <atomic>
#include <memory>

template <typename T>
class lock_free_stack {
  struct node {
    std::shared_ptr<T> val;
    node* next;
    node(const T& x) : val(std::make_shared<T>(x)) {}
  };
  std::atomic<node*> head;
  std::atomic<unsigned> cnt;  // 调用 pop 的线程数
  std::atomic<node*> toDel;   // 待删除节点的列表的头节点
 public:
  void push(const T& x) {
    const auto newNode = new node(x);
    newNode->next = head.load();
    while (!head.compare_exchange_weak(newNode->next, newNode)) {
    }
  }
  std::shared_ptr<T> pop() {
    ++cnt;  // 调用 pop 的线程数加一，表示 oldHead 正被持有，保证可以被解引用
    node* oldHead = head.load();
    while (oldHead && !head.compare_exchange_weak(oldHead, oldHead->next)) {
    }
    std::shared_ptr<T> res;
    if (oldHead) {
      res.swap(
          oldHead->val);  // oldHead 一定能解引用，oldHead->val 设为 nullptr
    }
    try_reclaim(oldHead);  // 计数器为 1 则释放 oldHead，否则添加到待删除列表中
    return res;  // res 保存了 oldHead->val
  }

 private:
  static void deleteNodes(node* n) {  // 释放 n 及之后的所有节点
    while (n) {
      node* tmp = n->next;
      delete n;
      n = tmp;
    }
  }
  void try_reclaim(node* oldHead) {
    if (cnt == 1) {  // 调用 pop 的线程数为 1 则可以进行释放
      // exchange 返回 toDel 值，即待删除列表的头节点，再将 toDel 设为 nullptr
      node* n = toDel.exchange(nullptr);  // 获取待删除列表的头节点
      if (--cnt == 0) {  // 没有其他线程，则释放待删除列表中所有节点
        deleteNodes(n);
      } else if (n) {  // 如果多于一个线程则继续保存到待删除列表
        addToDel(n);
      }
      delete oldHead;  // 删除传入的节点
    } else {  // 调用 pop 的线程数超过 1，添加当前节点到待删除列表
      addToDel(oldHead, oldHead);
      --cnt;
    }
  }
  void addToDel(node* n) {  // 把 n 及之后的节点置于待删除列表之前
    node* last = n;
    while (const auto tmp = last->next) last = tmp;  // last 指向尾部
    addToDel(n, last);  // 添加从 n 至 last 的所有节点到待删除列表
  }
  void addToDel(node* first, node* last) {
    last->next = toDel;  // 链接到已有的待删除列表之前
    // 确保最后 last->next为toDel，再将 toDel 设为 first，first 即新的头节点
    while (!toDel.compare_exchange_weak(last->next, first)) {
    }
  }
};