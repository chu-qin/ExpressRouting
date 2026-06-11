#pragma once
// ============================================================================
// Containers.h — 自实现容器库（替代 STL）
// ============================================================================
// 课程要求：禁止使用 STL。本文件实现四个基础数据结构：
//   DynArray<T>  — 动态数组
//   HashMap<V>   — 哈希表（键固定为 int）
//   MinHeap<T>   — 最小堆
//   Queue<T>     — 队列
//
// 设计原则：
//   - 零外部依赖（不 include 任何标准库头文件）
//   - 不使用 std::move / std::swap（用普通赋值替代）
//   - 不使用模板元编程（如 if constexpr）
//   - 代码短小直白，适合初学者阅读
// ============================================================================

// ============================================================================
// DynArray<T> — 动态数组
// ============================================================================
// 原理：在堆上分配 T 数组，满了就扩容 2 倍。
//
// 为什么扩容是 2 倍？
//   假设从容量 1 开始 push n 次：
//   扩容时的总拷贝次数 = 1+2+4+...+n/2 ≈ n
//   → n 次 push 总代价 O(n)，每次均摊 O(1)
//   如果每次只加固定大小（如 +10），总拷贝会变成 O(n²)
//
// 深拷贝 vs 浅拷贝：
//   如果 T 是 string（内部有指针），直接 memcpy 会导致两个对象
//   指向同一块内存 → 一个析构后另一个变成野指针 → 程序崩溃
//   所以必须逐个元素调用赋值运算符（深拷贝）
// ============================================================================
template<typename T>
class DynArray {
    T* _d = nullptr;   // 堆数组指针
    int _sz = 0;       // 当前元素个数
    int _cap = 0;      // 容量（已分配的内存可放几个）

    // 扩容：申请 2 倍空间，拷贝旧数据，释放旧空间
    void grow() {
        int nc = _cap ? _cap * 2 : 4;   // 空数组初始给 4 个位置
        T* nd = new T[nc];
        for (int i = 0; i < _sz; ++i)
            nd[i] = _d[i];               // 直接赋值（对 string 是深拷贝）
        delete[] _d;
        _d = nd;
        _cap = nc;
    }

public:
    // 默认构造：空数组
    DynArray() = default;

    // 拷贝构造：逐元素 push_back（深拷贝）
    DynArray(const DynArray& o) {
        for (int i = 0; i < o._sz; ++i) push_back(o._d[i]);
    }

    // 析构：释放堆内存（delete[] nullptr 是安全的）
    ~DynArray() { delete[] _d; }

    // 赋值：先清空自己，再逐元素拷贝
    DynArray& operator=(const DynArray& o) {
        if (this == &o) return *this;    // 防止 a = a
        delete[] _d;
        _d = nullptr; _sz = _cap = 0;
        for (int i = 0; i < o._sz; ++i) push_back(o._d[i]);
        return *this;
    }

    // ---- 增删 ----
    void push_back(const T& v) {
        if (_sz == _cap) grow();         // 满了先扩容
        _d[_sz++] = v;
    }

    void pop_back() {
        if (_sz > 0) --_sz;              // 只减计数，不释放内存
    }

    // 删除索引 i 处元素，后面的元素全部前移
    void erase(int i) {
        for (; i < _sz - 1; ++i)
            _d[i] = _d[i + 1];           // 后继元素前移
        --_sz;
    }

    // 删除第一个满足条件的元素，返回是否成功
    template<typename P>
    bool remove_first(P pred) {
        for (int i = 0; i < _sz; ++i)
            if (pred(_d[i])) { erase(i); return true; }
        return false;
    }

    // 删除所有满足条件的元素
    template<typename P>
    void remove_all(P pred) {
        for (int i = 0; i < _sz; ) {
            if (pred(_d[i])) erase(i);   // 删了不 +i（下一个元素移到此位置）
            else ++i;
        }
    }

    // ---- 访问 ----
    T&       operator[](int i)       { return _d[i]; }
    const T& operator[](int i) const { return _d[i]; }
    T&       back()                  { return _d[_sz - 1]; }

    int  size()  const { return _sz; }
    bool empty() const { return _sz == 0; }
    void clear()       { _sz = 0; }     // 只改计数，可快速重用

    // 迭代器（支持范围 for）
    T* begin()       { return _d; }
    T* end()         { return _d + _sz; }
    const T* begin() const { return _d; }
    const T* end()   const { return _d + _sz; }
};

// ============================================================================
// HashMap<V> — 开放地址哈希表（键固定为 int）
// ============================================================================
// 原理：用 int 键 → 算出槽号 → 直接定位，O(1) 均摊。
//
// 一、哈希函数：乘法哈希
//   h(k) = (k × 2654435761) mod 表容量
//   2654435761 = 2^32 × (√5-1)/2，黄金比例倒数
//   能把连续整数均匀打散到各个槽
//   例：k=1→slot 2654435761%16=1, k=2→slot(2*Φ⁻¹)≈slot 8
//
// 二、冲突解决：开放地址法 + 线性探测
//   如果目标槽被占了 → 看下一个槽 → 还被占再看下一个
//   就像停车场找车位：首选位置被占了就往下一个找
//
// 三、惰性删除：删时不真清，打 del=true 标记
//   为什么不能直接清空？
//   例：key=5 → slot3, key=13冲突 → slot4
//   删了 slot3 → 找 13 时看 slot3 空 → "不存在"（错！13 在 slot4）
//   用 del 标记 → 找 13 时跳过打了标记的 slot3 → 找到 slot4 ✓
//
// 四、负载因子：有效元素数 / 容量 > 0.5 时扩容 2 倍（rehash）
//   为什么 0.5？开放地址法负载太高探测链会很长
//   0.5 保证平均探测 ≤ 2 次
// ============================================================================
template<typename V>
class HashMap {
    struct Slot {
        int  key;
        V    val;
        bool used = false;   // 槽是否被占用过
        bool del  = false;   // 惰性删除标记
    };

    Slot* _s = nullptr;   // 槽数组
    int   _cap = 0;       // 容量
    int   _sz = 0;        // 有效元素数（不含 del 的槽）

    // 乘法哈希
    unsigned h(int k) const {
        return (unsigned)k * 2654435761u % (unsigned)_cap;
    }

    // 扩容到 nc 并重新哈希所有有效元素
    void rehash(int nc) {
        Slot* old = _s;
        int   oc  = _cap;
        _s   = new Slot[nc]();     // () 让 used/del 全 false
        _cap = nc;
        _sz  = 0;
        for (int i = 0; i < oc; ++i)
            if (old[i].used && !old[i].del)
                (*this)[old[i].key] = old[i].val;  // 搬到新表
        delete[] old;
    }

public:
    // 构造：默认 16 个槽
    HashMap(int c = 16) : _s(new Slot[c]()), _cap(c), _sz(0) {}

    // 拷贝构造：深拷贝整个槽数组
    HashMap(const HashMap& o)
        : _s(new Slot[o._cap]()), _cap(o._cap), _sz(o._sz) {
        for (int i = 0; i < _cap; ++i) _s[i] = o._s[i];
    }

    ~HashMap() { delete[] _s; }

    // 赋值
    HashMap& operator=(const HashMap& o) {
        if (this == &o) return *this;
        delete[] _s;
        _s   = new Slot[o._cap]();
        _cap = o._cap;
        _sz  = o._sz;
        for (int i = 0; i < _cap; ++i) _s[i] = o._s[i];
        return *this;
    }

    // ---- operator[]：最核心的方法 ----
    // 功能：有则返回引用，无则插入默认值再返回引用
    // 示例：map[5] = "北京";  // 自动创建 key=5 的槽
    //        cout << map[5];  // 查找并返回
    V& operator[](int k) {
        if (_sz * 2 >= _cap)             // 负载 ≥ 0.5 先扩容
            rehash(_cap * 2);

        unsigned i = h(k);
        int fd = -1;                     // 遇到的第一个 del 槽位置

        while (_s[i].used) {
            if (_s[i].del) {
                if (fd < 0) fd = (int)i; // 记下，可复用
            } else if (_s[i].key == k) {
                return _s[i].val;        // 找到了！
            }
            i = (i + 1) % (unsigned)_cap; // 往下探测
        }

        // 没找到 → 插入新槽
        int ins = (fd >= 0) ? fd : (int)i;   // 优先复用 del 槽
        _s[ins] = { k, V{}, true, false };
        ++_sz;
        return _s[ins].val;
    }

    // set：设置键值对，比 operator[] 更语义化
    void set(int k, const V& v) { (*this)[k] = v; }

    // find：查找，返回值的指针。不存在 → nullptr
    V* find(int k) {
        unsigned i = h(k), s = i;
        while (_s[i].used) {
            if (!_s[i].del && _s[i].key == k)
                return &_s[i].val;
            if ((i = (i + 1) % (unsigned)_cap) == s)
                break;                   // 绕了一圈，不存在
        }
        return nullptr;
    }
    const V* find(int k) const {
        unsigned i = h(k), s = i;
        while (_s[i].used) {
            if (!_s[i].del && _s[i].key == k)
                return &_s[i].val;
            if ((i = (i + 1) % (unsigned)_cap) == s)
                break;
        }
        return nullptr;
    }

    bool contains(int k) const { return find(k) != nullptr; }

    // erase：惰性删除（只打 del 标记）
    bool erase(int k) {
        unsigned i = h(k), s = i;
        while (_s[i].used) {
            if (!_s[i].del && _s[i].key == k) {
                _s[i].del = true;
                --_sz;
                return true;
            }
            if ((i = (i + 1) % (unsigned)_cap) == s) break;
        }
        return false;
    }

    int  size()  const { return _sz; }
    bool empty() const { return _sz == 0; }

    void clear() {
        for (int i = 0; i < _cap; ++i)
            _s[i].used = _s[i].del = false;
        _sz = 0;
    }

    // forEach：遍历所有有效元素
    // 用法：map.forEach([](int key, Node& val) { ... });
    template<typename F>
    void forEach(F f) {
        for (int i = 0; i < _cap; ++i)
            if (_s[i].used && !_s[i].del)
                f(_s[i].key, _s[i].val);
    }
    template<typename F>
    void forEach(F f) const {
        for (int i = 0; i < _cap; ++i)
            if (_s[i].used && !_s[i].del)
                f(_s[i].key, _s[i].val);
    }
};

// ============================================================================
// MinHeap<T,Cmp> — 二叉最小堆
// ============================================================================
// 原理：完全二叉树用数组存，父节点 ≤ 子节点。
//
// 索引关系（不用指针，用下标算术）：
//   节点 i 的左孩子 = 2i + 1
//   节点 i 的右孩子 = 2i + 2
//   节点 i 的父节点 = (i-1) / 2
//
// push：加到数组末尾 → 和父节点比，小了就交换（上浮）
// pop ：把末尾元素放到堆顶 → 和孩子中较小的比，大了就交换（下沉）
//
// 在 Dijkstra 中做优先队列：
//   不实现 decrease-key（太难），改用"惰性删除"
//   → push 新距离时旧距离留在堆里，pop 时跳过过时的记录
// ============================================================================
template<typename T>
struct HeapLess {
    bool operator()(const T& a, const T& b) const { return a < b; }
};

template<typename T, typename Cmp = HeapLess<T>>
class MinHeap {
    DynArray<T> _d;   // 底层数组（存完全二叉树）
    Cmp         _c;   // 比较器

    // 上浮：索引 i 的元素和父节点比较
    void up(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;         // 父节点索引
            if (_c(_d[i], _d[p])) {      // 孩子 < 父 → 交换
                T t = _d[i]; _d[i] = _d[p]; _d[p] = t;
                i = p;
            } else break;
        }
    }

    // 下沉：索引 i 的元素和左右孩子中较小的比较
    void down(int i) {
        int n = _d.size();
        while (true) {
            int b = i;                    // 当前最小值的索引
            int l = 2 * i + 1;            // 左孩子
            int r = 2 * i + 2;            // 右孩子
            if (l < n && _c(_d[l], _d[b])) b = l;
            if (r < n && _c(_d[r], _d[b])) b = r;
            if (b == i) break;            // 已是最小
            T t = _d[i]; _d[i] = _d[b]; _d[b] = t;
            i = b;
        }
    }

public:
    bool      empty() const { return _d.empty(); }
    const T&  top()   const { return _d[0]; }

    void push(const T& v) {
        _d.push_back(v);
        up(_d.size() - 1);
    }

    void pop() {
        if (_d.empty()) return;
        _d[0] = _d.back();          // 末尾元素放堆顶
        _d.pop_back();
        if (!_d.empty()) down(0);   // 下沉
    }
};

// ============================================================================
// Queue<T> — 队列
// ============================================================================
// 原理：FIFO，队尾进、队头出
// 用 DynArray 存数据 + head 下标标记队头
// 缺点：head 只增不减，前面空间不回收（拓扑排序中寿命很短，没关系）
// ============================================================================
template<typename T>
class Queue {
    DynArray<T> _d;
    int _h = 0;

public:
    bool empty() const    { return _h >= _d.size(); }
    T&   front()          { return _d[_h]; }
    void push(const T& v) { _d.push_back(v); }
    void pop()            { if (!empty()) ++_h; }
    void clear()          { _d.clear(); _h = 0; }
};
