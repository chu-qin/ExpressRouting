#pragma once
// ============================================================================
// DynArray<T> —— 动态数组（项目唯一的自实现容器）
// ============================================================================
// 功能：可自动扩容的数组，类似教科书上的顺序表
// 扩容策略：容量不够时 ×2，均摊 O(1)
// ============================================================================

template<typename T>
class DynArray {
private:
    T*  data     = nullptr;   // 堆上的数组指针
    int length   = 0;         // 当前元素个数
    int capacity = 0;         // 已分配容量

    // 扩容：新容量 = 旧容量×2（首次为4）
    void expand() {
        int newCap = capacity ? capacity * 2 : 4;
        T*  newArr = new T[newCap];
        for (int i = 0; i < length; ++i)
            newArr[i] = data[i];
        delete[] data;
        data     = newArr;
        capacity = newCap;
    }

public:
    // ---- 构造 / 析构 / 拷贝 ----
    DynArray() = default;

    ~DynArray() { delete[] data; }

    DynArray(const DynArray& other) {
        for (int i = 0; i < other.length; ++i)
            push_back(other.data[i]);
    }

    DynArray& operator=(const DynArray& other) {
        if (this == &other) return *this;
        delete[] data;
        data     = nullptr;
        length   = 0;
        capacity = 0;
        for (int i = 0; i < other.length; ++i)
            push_back(other.data[i]);
        return *this;
    }

    // ---- 增删 ----
    void push_back(const T& value) {
        if (length == capacity) expand();
        data[length++] = value;
    }

    void pop_back() {
        if (length > 0) --length;
    }

    // 删除下标为 index 的元素，后续元素前移
    void erase(int index) {
        for (int i = index; i < length - 1; ++i)
            data[i] = data[i + 1];
        --length;
    }

    // 删除第一个满足 check 的元素
    template<typename P>
    bool remove_first(P check) {
        for (int i = 0; i < length; ++i)
            if (check(data[i])) { erase(i); return true; }
        return false;
    }

    // 删除所有满足 check 的元素
    template<typename P>
    void remove_all(P check) {
        for (int i = 0; i < length; ) {
            if (check(data[i])) erase(i);   // 删完不++i，因为下一元素移过来了
            else ++i;
        }
    }

    // ---- 访问 ----
    T&       operator[](int i)       { return data[i]; }
    const T& operator[](int i) const { return data[i]; }

    T&       back()       { return data[length - 1]; }
    const T& back() const { return data[length - 1]; }

    int  size()  const { return length; }
    bool empty() const { return length == 0; }
    void clear()       { length = 0; }

    // ---- 迭代器（支持 for (auto& x : arr)）----
    T*       begin()       { return data; }
    T*       end()         { return data + length; }
    const T* begin() const { return data; }
    const T* end()   const { return data + length; }
};
