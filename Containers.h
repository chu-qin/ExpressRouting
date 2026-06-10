#pragma once
// ============================================================================
// Containers.h — 自实现容器库（替代 STL）
// ============================================================================
// 课程红线：禁止使用 STL 容器。本文件实现了四个常用数据结构：
//   DynArray<T>  — 动态数组（替代 std::vector）
//   HashMap<K,V> — 哈希表  （替代 std::map / unordered_map）
//   MinHeap<T>   — 最小堆  （替代 std::priority_queue）
//   Queue<T>     — 队列    （替代 std::queue）
//
// 设计思想：
//   - DynArray  是底层基础，MinHeap 和 Queue 都基于它
//   - HashMap   使用开放地址法 + 线性探测，支持 int 和 std::string 键
//   - 所有容器都实现了"三件套"：拷贝构造 / operator= / 析构函数
// ============================================================================

#include <utility>
#include <string>

// ============================================================================
// DynArray<T> — 动态数组
// ============================================================================
// 核心原理：
//   堆上分配 T* 数组，元素数达到容量时扩容 2 倍。
//   均摊 push_back 时间复杂度 O(1)。
//
// 和项目A MyVector 的区别：
//   - 增加了移动构造和移动赋值（C++11 右值引用）
//   - operator= 使用 copy-and-swap 惯用法，更安全
//   - 提供 begin()/end() 接口，支持范围 for 循环
//   - 提供 erase / remove_first / remove_all 多种删除方式
// ============================================================================
template<typename T>
class DynArray {
    T* _d = nullptr;   // 堆数组指针
    int _sz = 0;       // 当前元素数
    int _cap = 0;      // 容量

    // 扩容：申请 2 倍空间，拷贝旧数据，释放旧空间
    void grow() {
        int nc = _cap ? _cap * 2 : 4;
        T* nd = new T[nc];
        for (int i = 0; i < _sz; ++i) nd[i] = std::move(_d[i]);
        delete[] _d; _d = nd; _cap = nc;
    }

public:
    // ---- 构造 / 析构 / 赋值 ----
    DynArray() = default;

    // 拷贝构造：逐元素 push_back，实现深拷贝
    DynArray(const DynArray& o) { for (int i = 0; i < o._sz; ++i) push_back(o._d[i]); }

    // 移动构造：直接接管资源，源对象置空（noexcept 保证 STL 兼容）
    DynArray(DynArray&& o) noexcept : _d(o._d), _sz(o._sz), _cap(o._cap)
        { o._d = nullptr; o._sz = o._cap = 0; }

    ~DynArray() { delete[] _d; }

    // copy-and-swap 赋值：传值即拷贝，交换即接管，自动处理自赋值
    DynArray& operator=(DynArray o) noexcept {
        std::swap(_d, o._d); std::swap(_sz, o._sz); std::swap(_cap, o._cap);
        return *this;
    }

    // ---- 增删 ----
    void push_back(const T& v)  { if (_sz == _cap) grow(); _d[_sz++] = v; }
    void push_back(T&& v)       { if (_sz == _cap) grow(); _d[_sz++] = std::move(v); }
    void pop_back()             { if (_sz > 0) --_sz; }

    // 删除索引 i 处元素，后续元素前移
    void erase(int i) {
        for (; i < _sz - 1; ++i) _d[i] = std::move(_d[i + 1]); --_sz;
    }

    // 按条件删除第一个匹配元素
    template<typename P> bool remove_first(P pred) {
        for (int i = 0; i < _sz; ++i) if (pred(_d[i])) { erase(i); return true; }
        return false;
    }

    // 按条件删除所有匹配元素
    template<typename P> void remove_all(P pred) {
        for (int i = 0; i < _sz;) { if (pred(_d[i])) erase(i); else ++i; }
    }

    // ---- 访问 ----
    T&       operator[](int i)       { return _d[i]; }
    const T& operator[](int i) const { return _d[i]; }
    T&       back()                  { return _d[_sz - 1]; }

    int  size()  const { return _sz; }
    bool empty() const { return _sz == 0; }
    void clear()       { _sz = 0; }

    // 迭代器接口（支持范围 for）
    T* begin()       { return _d; }
    T* end()         { return _d + _sz; }
    const T* begin() const { return _d; }
    const T* end()   const { return _d + _sz; }
};

// ============================================================================
// HashMap<K,V> — 开放地址哈希表
// ============================================================================
// 核心原理：
//   使用开放地址法 + 线性探测解决哈希冲突。
//   惰性删除：删除时标记 del=true（不真正移除），插入时复用 del 槽。
//   负载因子 > 0.5 触发 rehash（扩容 2 倍）。
//
// 哈希函数：
//   int 键：k * 2654435761u % cap（乘法哈希，黄金比例 φ^-1 ≈ 0.618）
//   string 键：djb2 算法（hash = hash * 33 + c），经典字符串哈希
//
// 为什么不用链地址法？
//   链地址法需要额外的链表节点内存分配。开放地址法内存连续，缓存友好。
// ============================================================================
template<typename K, typename V>
class HashMap {
    struct Slot { K key; V val; bool used = false, del = false; };
    Slot* _s;     // 槽数组
    int _cap;      // 容量
    int _sz;       // 当前有效元素数

    // ---- 哈希函数 ----
    unsigned h(int k) const { return (unsigned)k * 2654435761u % (unsigned)_cap; }
    unsigned h(const std::string& k) const {
        unsigned r = 5381;
        for (char c : k) r = r * 33 + (unsigned char)c;
        return r % (unsigned)_cap;
    }
    unsigned slot0(const K& k) const {
        if constexpr (std::is_same_v<K, int>) return h((int)k);
        else return h(k);
    }

    // 重新哈希到新容量
    void rehash(int nc) {
        Slot* old = _s; int oc = _cap;
        _s = new Slot[nc](); _cap = nc; _sz = 0;
        for (int i = 0; i < oc; ++i)
            if (old[i].used && !old[i].del)
                (*this)[old[i].key] = std::move(old[i].val);
        delete[] old;
    }

public:
    explicit HashMap(int c = 16) : _s(new Slot[c]()), _cap(c), _sz(0) {}

    // 拷贝构造
    HashMap(const HashMap& o) : _s(new Slot[o._cap]()), _cap(o._cap), _sz(o._sz)
        { for (int i = 0; i < _cap; ++i) _s[i] = o._s[i]; }

    // 移动构造
    HashMap(HashMap&& o) noexcept : _s(o._s), _cap(o._cap), _sz(o._sz)
        { o._s = nullptr; o._cap = o._sz = 0; }

    ~HashMap() { delete[] _s; }

    HashMap& operator=(HashMap o) noexcept {
        std::swap(_s, o._s); std::swap(_cap, o._cap); std::swap(_sz, o._sz); return *this;
    }

    // operator[]：查找或插入。负载超 0.5 时自动 rehash
    V& operator[](const K& k) {
        if (_sz * 2 >= _cap) rehash(_cap * 2);
        unsigned i = slot0(k); int fd = -1;
        while (_s[i].used) {
            if (_s[i].del) { if (fd < 0) fd = (int)i; }
            else if (_s[i].key == k) return _s[i].val;
            i = (i + 1) % (unsigned)_cap;
        }
        int ins = (fd >= 0) ? fd : (int)i;
        _s[ins] = { k, V{}, true, false }; ++_sz;
        return _s[ins].val;
    }

    void set(const K& k, const V& v) { (*this)[k] = v; }
    void set(const K& k, V&& v)      { (*this)[k] = std::move(v); }

    // 查找：返回指针，不存在返回 nullptr
    V* find(const K& k) {
        unsigned i = slot0(k), s = i;
        while (_s[i].used) {
            if (!_s[i].del && _s[i].key == k) return &_s[i].val;
            if ((i = (i + 1) % (unsigned)_cap) == s) break;
        }
        return nullptr;
    }
    const V* find(const K& k) const {
        unsigned i = slot0(k), s = i;
        while (_s[i].used) {
            if (!_s[i].del && _s[i].key == k) return &_s[i].val;
            if ((i = (i + 1) % (unsigned)_cap) == s) break;
        }
        return nullptr;
    }

    bool contains(const K& k) const { return find(k) != nullptr; }

    // 惰性删除：标记 del=true，不真正移除槽
    bool erase(const K& k) {
        unsigned i = slot0(k), s = i;
        while (_s[i].used) {
            if (!_s[i].del && _s[i].key == k) { _s[i].del = true; --_sz; return true; }
            if ((i = (i + 1) % (unsigned)_cap) == s) break;
        }
        return false;
    }

    int  size()  const { return _sz; }
    bool empty() const { return _sz == 0; }
    void clear() { for (int i = 0; i < _cap; ++i) { _s[i].used = _s[i].del = false; } _sz = 0; }

    // 遍历所有有效元素
    template<typename F> void forEach(F f) {
        for (int i = 0; i < _cap; ++i) if (_s[i].used && !_s[i].del) f(_s[i].key, _s[i].val);
    }
    template<typename F> void forEach(F f) const {
        for (int i = 0; i < _cap; ++i) if (_s[i].used && !_s[i].del) f(_s[i].key, _s[i].val);
    }
};

// ============================================================================
// MinHeap<T,Cmp> — 二叉最小堆
// ============================================================================
// 核心原理：
//   完全二叉树用数组存储：节点 i 的左孩子=2i+1，右孩子=2i+2，父=(i-1)/2。
//   push：加到数组末尾 → up(i) 上浮到合适位置
//   pop ：把末尾元素移到堆顶 → down(0) 下沉
//
// 应用场景：
//   Dijkstra 堆优化版本中作为优先队列：
//     - 不实现 decrease-key（太复杂）
//     - 改用"惰性删除"：push 新记录时旧记录留在堆中，pop 时跳过
//     - 判断条件：top.d > dist[top.n] → 跳过这条过时记录
// ============================================================================
template<typename T>
struct HeapLess { bool operator()(const T& a, const T& b) const { return a < b; } };

template<typename T, typename Cmp = HeapLess<T>>
class MinHeap {
    DynArray<T> _d;  // 底层动态数组
    Cmp _c;          // 比较器（默认 operator< = 最小堆）

    void up(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (_c(_d[i], _d[p])) { std::swap(_d[i], _d[p]); i = p; }
            else break;
        }
    }

    void down(int i) {
        int n = _d.size();
        while (true) {
            int b = i, l = 2 * i + 1, r = 2 * i + 2;
            if (l < n && _c(_d[l], _d[b])) b = l;
            if (r < n && _c(_d[r], _d[b])) b = r;
            if (b == i) break;
            std::swap(_d[i], _d[b]); i = b;
        }
    }

public:
    bool      empty() const { return _d.empty(); }
    const T&  top()   const { return _d[0]; }
    void push(const T& v)   { _d.push_back(v); up(_d.size() - 1); }

    void pop() {
        if (_d.empty()) return;
        _d[0] = std::move(_d.back());
        _d.pop_back();
        if (!_d.empty()) down(0);
    }
};

// ============================================================================
// Queue<T> — 队列（基于 DynArray 的环形缓冲）
// ============================================================================
// 核心原理：
//   不是真正的环形数组，而是用 head 下标 + DynArray 模拟。
//   push → 追加到 DynArray 末尾
//   pop  → head 下标后移，不释放内存
//   缺点：head 只增不减，长时间使用会浪费前面空间
//   应用场景：拓扑排序的 BFS 队列（短期使用，无内存浪费问题）
// ============================================================================
template<typename T>
class Queue {
    DynArray<T> _d;
    int _h = 0;  // 队头下标

public:
    bool empty() const    { return _h >= _d.size(); }
    T&   front()          { return _d[_h]; }
    void push(const T& v) { _d.push_back(v); }
    void pop()            { if (!empty()) ++_h; }
    void clear()          { _d.clear(); _h = 0; }
};
