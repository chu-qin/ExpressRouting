#pragma once
// ============================================================================
// Containers.h — 自实现容器库（替代 STL）
// ============================================================================
// 四个基础数据结构：DynArray / HashMap / MinHeap / Queue
// 零外部依赖，不使用 std::move / std::swap / if constexpr
// ============================================================================

// ============================================================================
// DynArray<T> — 动态数组
// ============================================================================
// 堆上分配 T 数组，满了扩容 2 倍。扩容 2 倍保证均摊 O(1)。
template<typename T>
class DynArray {
    T*  data = nullptr;   // 堆数组指针
    int length = 0;       // 当前元素个数
    int capacity = 0;     // 已分配容量

    void expand() {
        int newCap = capacity ? capacity * 2 : 4;
        T*  newData = new T[newCap];
        for (int i = 0; i < length; ++i)
            newData[i] = data[i];
        delete[] data;
        data = newData;
        capacity = newCap;
    }

public:
    DynArray() = default;

    DynArray(const DynArray& other) {
        for (int i = 0; i < other.length; ++i)
            push_back(other.data[i]);
    }

    ~DynArray() { delete[] data; }

    DynArray& operator=(const DynArray& other) {
        if (this == &other) return *this;
        delete[] data;
        data = nullptr; length = capacity = 0;
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

    void erase(int index) {
        for (int i = index; i < length - 1; ++i)
            data[i] = data[i + 1];
        --length;
    }

    template<typename P>
    bool remove_first(P check) {
        for (int i = 0; i < length; ++i)
            if (check(data[i])) { erase(i); return true; }
        return false;
    }

    template<typename P>
    void remove_all(P check) {
        for (int i = 0; i < length; ) {
            if (check(data[i])) erase(i);
            else ++i;
        }
    }

    // ---- 访问 ----
    T&       operator[](int i)       { return data[i]; }
    const T& operator[](int i) const { return data[i]; }
    T&       back()                  { return data[length - 1]; }
    const T& back() const            { return data[length - 1]; }

    int  size()  const { return length; }
    bool empty() const { return length == 0; }
    void clear()       { length = 0; }

    T* begin()             { return data; }
    T* end()               { return data + length; }
    const T* begin() const { return data; }
    const T* end()   const { return data + length; }
};

// ============================================================================
// HashMap<V> — 开放地址哈希表（键固定为 int）
// ============================================================================
// 乘法哈希 + 线性探测 + 惰性删除，负载因子 > 0.5 扩容 2 倍
template<typename V>
class HashMap {
    struct Slot {
        int  key;
        V    value;
        bool used    = false;
        bool deleted = false;
    };

    Slot* slots = nullptr;
    int   cap   = 0;       // 容量
    int   cnt   = 0;       // 有效元素数

    unsigned hashIndex(int k) const {
        return (unsigned)k * 2654435761u % (unsigned)cap;
    }

    void rebuild(int newCap) {
        Slot* oldSlots = slots;
        int   oldCap   = cap;
        slots = new Slot[newCap]();
        cap   = newCap;
        cnt   = 0;
        for (int i = 0; i < oldCap; ++i)
            if (oldSlots[i].used && !oldSlots[i].deleted)
                (*this)[oldSlots[i].key] = oldSlots[i].value;
        delete[] oldSlots;
    }

public:
    HashMap(int initCap = 16) : slots(new Slot[initCap]()), cap(initCap), cnt(0) {}

    HashMap(const HashMap& other)
        : slots(new Slot[other.cap]()), cap(other.cap), cnt(other.cnt) {
        for (int i = 0; i < cap; ++i) slots[i] = other.slots[i];
    }

    ~HashMap() { delete[] slots; }

    HashMap& operator=(const HashMap& other) {
        if (this == &other) return *this;
        delete[] slots;
        slots = new Slot[other.cap]();
        cap   = other.cap;
        cnt   = other.cnt;
        for (int i = 0; i < cap; ++i) slots[i] = other.slots[i];
        return *this;
    }

    // operator[]：有则返回引用，无则插入默认值再返回
    V& operator[](int k) {
        if (cnt * 2 >= cap) rebuild(cap * 2);

        unsigned i = hashIndex(k);
        int firstDel = -1;

        while (slots[i].used) {
            if (slots[i].deleted) {
                if (firstDel < 0) firstDel = (int)i;
            } else if (slots[i].key == k) {
                return slots[i].value;
            }
            i = (i + 1) % (unsigned)cap;
        }

        int dest = (firstDel >= 0) ? firstDel : (int)i;
        slots[dest] = { k, V{}, true, false };
        ++cnt;
        return slots[dest].value;
    }

    void set(int k, const V& v) { (*this)[k] = v; }

    V* find(int k) {
        unsigned i = hashIndex(k), start = i;
        while (slots[i].used) {
            if (!slots[i].deleted && slots[i].key == k)
                return &slots[i].value;
            if ((i = (i + 1) % (unsigned)cap) == start) break;
        }
        return nullptr;
    }
    const V* find(int k) const {
        unsigned i = hashIndex(k), start = i;
        while (slots[i].used) {
            if (!slots[i].deleted && slots[i].key == k)
                return &slots[i].value;
            if ((i = (i + 1) % (unsigned)cap) == start) break;
        }
        return nullptr;
    }

    bool contains(int k) const { return find(k) != nullptr; }

    bool erase(int k) {
        unsigned i = hashIndex(k), start = i;
        while (slots[i].used) {
            if (!slots[i].deleted && slots[i].key == k) {
                slots[i].deleted = true;
                --cnt;
                return true;
            }
            if ((i = (i + 1) % (unsigned)cap) == start) break;
        }
        return false;
    }

    int  size()  const { return cnt; }
    bool empty() const { return cnt == 0; }

    void clear() {
        for (int i = 0; i < cap; ++i)
            slots[i].used = slots[i].deleted = false;
        cnt = 0;
    }

    template<typename F>
    void forEach(F f) {
        for (int i = 0; i < cap; ++i)
            if (slots[i].used && !slots[i].deleted)
                f(slots[i].key, slots[i].value);
    }
    template<typename F>
    void forEach(F f) const {
        for (int i = 0; i < cap; ++i)
            if (slots[i].used && !slots[i].deleted)
                f(slots[i].key, slots[i].value);
    }
};

// ============================================================================
// MinHeap<T,Cmp> — 二叉最小堆
// ============================================================================
// Dijkstra 优先队列：push 新距离 + pop 时惰性跳过旧记录
template<typename T>
struct HeapLess {
    bool operator()(const T& a, const T& b) const { return a < b; }
};

template<typename T, typename Cmp = HeapLess<T>>
class MinHeap {
    DynArray<T> data;
    Cmp         cmp;

    void shiftUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (cmp(data[i], data[parent])) {
                T temp = data[i]; data[i] = data[parent]; data[parent] = temp;
                i = parent;
            } else break;
        }
    }

    void shiftDown(int i) {
        int len = data.size();
        while (true) {
            int best  = i;
            int left  = 2 * i + 1;
            int right = 2 * i + 2;
            if (left < len && cmp(data[left], data[best]))   best = left;
            if (right < len && cmp(data[right], data[best])) best = right;
            if (best == i) break;
            T temp = data[i]; data[i] = data[best]; data[best] = temp;
            i = best;
        }
    }

public:
    bool      empty() const { return data.empty(); }
    const T&  top()   const { return data[0]; }

    void push(const T& value) {
        data.push_back(value);
        shiftUp(data.size() - 1);
    }

    void pop() {
        if (data.empty()) return;
        data[0] = data.back();
        data.pop_back();
        if (!data.empty()) shiftDown(0);
    }
};

// ============================================================================
// Queue<T> — 队列（FIFO）
// ============================================================================
template<typename T>
class Queue {
    DynArray<T> data;
    int head = 0;

public:
    bool empty() const    { return head >= data.size(); }
    T&   front()          { return data[head]; }
    void push(const T& v) { data.push_back(v); }
    void pop()            { if (!empty()) ++head; }
    void clear()          { data.clear(); head = 0; }
};
