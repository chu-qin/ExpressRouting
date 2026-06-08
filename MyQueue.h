#ifndef MYQUEUE_H
#define MYQUEUE_H

// ============================================================================
// MyQueue<T> — 手写链式队列（替代 std::queue）
// ============================================================================
// 为什么手写？
//   和 MyVector 一样，STL 容器全线禁止。拓扑排序（Kahn 算法）需要一个
//   队列来存放"当前入度为 0 的节点"，所以必须自己实现一个队列。
//
// 为什么用链表而不用环形数组？
//   链表实现最简单：不需要处理数组满/空的边界判断，不需要循环下标。
//   每个节点 new 出来，pop 时 delete 掉即可。
//
// 队列的 FIFO 特性：
//   push → 从队尾 tail 插入
//   pop  → 从队头 head 删除
//   先进先出
//
// 数据结构示意：
//   head → [A] → [B] → [C] → nullptr
//   tail 指向最后一个节点 [C]
// ============================================================================

template <typename T>
class MyQueue {
private:
    // 链表节点
    struct Node {
        T data;        // 存储的数据
        Node* next;    // 指向下一个节点的指针
        Node(const T& val) : data(val), next(nullptr) {}
    };

    Node* head;   // 队头指针（最早入队的元素）
    Node* tail;   // 队尾指针（最晚入队的元素）

public:
    // 构造函数：空队列
    MyQueue() {
        head = nullptr;
        tail = nullptr;
    }

    // 析构函数：逐个释放所有节点，防止内存泄漏
    ~MyQueue() {
        while (!empty()) {
            pop();
        }
    }

    // 入队：在队尾追加新节点
    void push(const T& val) {
        Node* node = new Node(val);   // 创建新节点
        if (tail) {
            tail->next = node;        // 旧尾指向新尾
        } else {
            head = node;              // 队列为空时，新节点也是 head
        }
        tail = node;                  // 更新尾指针
    }

    // 出队：删除队头节点
    void pop() {
        if (head == nullptr) return;  // 空队列不操作
        Node* tmp = head;
        head = head->next;            // head 后移
        if (head == nullptr) tail = nullptr;  // 队列变空时 tail 也要置空
        delete tmp;                   // 释放旧头节点
    }

    // 访问队头元素（可读写）
    T& front() {
        return head->data;
    }

    // 访问队头元素（只读）
    const T& front() const {
        return head->data;
    }

    // 判断队列是否为空
    bool empty() const {
        return head == nullptr;
    }

    // 注意：这个队列故意没有提供拷贝构造和 operator=，
    // 因为这个项目中队列只作为局部变量使用，不需要拷贝。
    // 如果强行拷贝会出现双重释放（两个对象的 head 指向同一链表）。
};

#endif
