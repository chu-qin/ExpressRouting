#ifndef MYVECTOR_H
#define MYVECTOR_H

// ============================================================================
// MyVector<T> — 手写动态数组（替代 std::vector）
// ============================================================================
// 为什么手写？
//   课程红线：禁止使用任何 STL 容器（<vector>, <queue>, <map> 等）。
//   所以必须从零实现一个支持动态扩容的泛型数组。
//
// 核心原理：
//   内部维护一个堆数组 T* data。当元素数量 size 达到容量 capacity 时，
//   申请一块 2 倍大的新内存，把旧数据拷贝过去，释放旧内存。
//   这样 push_back 的均摊时间复杂度是 O(1)。
//
// 内存管理三件套（C++ 类的基本素养）：
//   1. 拷贝构造函数 — 深拷贝，分配独立内存，避免两个对象共享同一块 data
//   2. operator=       — 同样深拷贝，并处理"自己赋值给自己"的边界情况
//   3. 析构函数        — delete[] data 释放堆内存，防止内存泄漏
// ============================================================================

template <typename T>
class MyVector {
private:
    T*  data;      // 指向堆数组的指针
    int size;      // 当前已存储的元素个数
    int capacity;  // 当前已分配的空间能容纳的元素个数

    // 扩容：申请 2 倍容量新数组，拷贝旧数据，释放旧数组
    void expand() {
        int newCap = capacity * 2;
        if (newCap < 4) newCap = 4;   // 兜底：至少 4 个
        T* newData = new T[newCap];   // 新数组
        for (int i = 0; i < size; i++) {
            newData[i] = data[i];     // 逐元素拷贝（浅拷贝语义，对 int/double 安全）
        }
        delete[] data;                // 释放旧内存
        data = newData;
        capacity = newCap;
    }

public:
    // ---- 构造 / 析构 / 拷贝 ----

    MyVector() {
        capacity = 4;
        size = 0;
        data = new T[capacity];       // 初始分配 4 个空间
    }

    // 拷贝构造函数：创建一个与 other 完全独立的副本
    MyVector(const MyVector<T>& other) {
        capacity = other.capacity;
        size = other.size;
        data = new T[capacity];       // 分配独立内存
        for (int i = 0; i < size; i++) {
            data[i] = other.data[i];  // 逐元素拷贝
        }
    }

    // 赋值运算符：释放旧资源，深拷贝新资源
    MyVector<T>& operator=(const MyVector<T>& other) {
        if (this != &other) {         // 防止自赋值（a = a）
            delete[] data;            // 释放旧内存
            capacity = other.capacity;
            size = other.size;
            data = new T[capacity];
            for (int i = 0; i < size; i++) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }

    ~MyVector() {
        delete[] data;                // 释放堆内存
    }

    // ---- 增删 ----

    // 在尾部追加元素。空间不够时自动触发 expand()
    void push_back(const T& val) {
        if (size >= capacity) {
            expand();                 // 满了 → 扩容
        }
        data[size] = val;
        size++;
    }

    // 删除尾部元素（不释放空间，仅 size--）
    void pop_back() {
        if (size > 0) size--;
    }

    // 删除索引 idx 处的元素，后续元素前移一位
    void removeAt(int idx) {
        if (idx < 0 || idx >= size) return;
        for (int i = idx; i < size - 1; i++) {
            data[i] = data[i + 1];    // 前移
        }
        size--;
    }

    // 清空（size 归零，不释放空间）
    void clear() {
        size = 0;
    }

    // ---- 访问 ----

    // 下标访问（可读写）
    T& operator[](int i) {
        return data[i];
    }

    // 下标访问（只读）
    const T& operator[](int i) const {
        return data[i];
    }

    // 获取当前元素个数
    int getSize() const {
        return size;
    }

    // 获取当前容量
    int getCapacity() const {
        return capacity;
    }

    // 顺序查找 val，找到返回索引，找不到返回 -1
    int find(const T& val) const {
        for (int i = 0; i < size; i++) {
            if (data[i] == val) return i;
        }
        return -1;
    }
};

#endif
