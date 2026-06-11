# 快递网点配送路径规划 -- 从零构建指南

---

## 目录

1. [前置准备](#一前置准备)
2. [DynArray 动态数组](#二dynarrayt--动态数组)
3. [HashMap 哈希表](#三hashmapv--开放地址哈希表)
4. [MinHeap 最小堆 + Queue 队列](#四minheaptcmp--二叉最小堆--queuet--队列)
5. [Graph 有向图邻接表](#五graph--有向图邻接表)
6. [Dijkstra 最短路径 + 拓扑排序](#六dijkstra-最短路径--拓扑排序)
7. [OrderManager 订单 + 文件读写](#七ordermanager-订单管理--文件读写)
8. [Qt GUI 图形界面](#八qt-gui-图形界面)
9. [数据 + 构建 + 编译运行](#九数据文件--构建配置--编译运行)
10. [算法详解附录](#十算法详解附录)
11. [答辩准备](#十一答辩准备)

---

## 一、前置准备

### 1.1 我们要做什么？

我们要做一个**快递网点配送路径规划系统**。用大白话说：

- 中国有 25 个城市的快递网点（北京、上海、广州...）
- 网点之间有运输线路（比如北京到天津需要 1.5 小时，费用 60 元）
- 用户输入起点，程序算出到所有城市的最快路线
- 用户输入起点和终点，程序算出最便宜的路线
- 支持批量导入订单，自动规划所有路线
- 在图形界面用鼠标点击、高亮路线

```
┌──────────────────────────────────────────────┐
│       快递网点配送路径规划系统                    │
│                                              │
│  25 个中国城市，124 条有向边                     │
│  北京 ──→ 天津（1.5h / 60元）                  │
│  北京 ──→ 石家庄（3h / 90元）                   │
│    ...                                       │
│                                              │
│  功能：                                       │
│  1. 输入起点 → 到所有城市的最短耗时               │
│  2. 输入起点+终点 → 最低费用路径                  │
│  3. 批量导入订单 → 自动规划全部路线               │
│  4. 检测城市间是否有循环依赖（拓扑排序）            │
│  5. 画布上高亮路径，鼠标悬停看节点信息              │
└──────────────────────────────────────────────┘
```

#### 知识点：什么是"数据结构"？

> "数据结构"就是**怎么在计算机里组织数据**的方法。
>
> 打个比方：你有 100 本书。
> - 随手堆在地上 = 什么结构都没有，找一本书要翻半天
> - 按编号排成一排 = 数组
> - 按类别分开放，每类一个抽屉 = 哈希表
> - 按大小堆成金字塔形，小的在上面 = 堆
>
> 不同的组织方式，决定了"查找一本书"需要多少时间。这就是数据结构课的核心问题。

#### 知识点：为什么用图来建模快递路线？

> 快递网点之间有运输路线，这天然就是一个**图（Graph）**：
> - 网点 = 图中的**节点（Node）**，也叫**顶点（Vertex）**
> - 运输路线 = 图中的**边（Edge）**
> - 北京到天津可以走，但天津到北京不一定有同样的路线 = **有向图（Directed Graph）**
>
> 图是比数组、链表更"高级"的数据结构，因为它能表示"多对多"的关系。
> 我们之前学的二叉树也是图的一种特例（每个节点最多两个子节点）。

### 1.2 什么是 Qt？为什么用 Qt 做这个项目？

Qt（读作 "cute"）是一个 C++ 的 GUI 框架。GUI = Graphical User Interface = 图形用户界面。

对比一下：

| | 黑框控制台 | Qt 图形界面 |
|---|---|---|
| 样子 | 小黑窗，只有文字 | 有窗口、按钮、画布 |
| 操作 | 敲命令 | 鼠标点击 |
| 答辩效果 | 老师：这是什么？ | 老师：哇，有地图有路径！ |

Qt 提供了现成的**按钮（QPushButton）**、**文本框（QLineEdit）**、**画布（QWidget）**，我们只需要"拼积木"就能搭出一个像样的界面。

另外，Qt 最重要的是**自动管理内存**：你在界面上创建了一个按钮，当你关掉窗口时，Qt 会自动帮你删掉它。不需要手动 `delete`。

### 1.3 什么是 CMake？为什么不用 g++ 直接编译？

如果你之前写作业是用 `g++ main.cpp sort.cpp -o main.exe`，那你用的是**手动编译**。

但是当项目有 10+ 个源文件，还要链接 Qt 库时，手动敲 g++ 命令会变成一长串恐怖的东西。

CMake 就像一个**建筑图纸**：你告诉它"我这个项目有哪些源文件，需要哪些库"，然后它自动生成编译命令。

我们的 `CMakeLists.txt` 只有约 50 行，后面第九章会逐行解释。

### 1.4 课程红线

> **禁止使用 STL 容器**：`vector`、`list`、`queue`、`stack`、`map`、`priority_queue` 全部不能用

为什么有这个红线？

因为这门课的目的不仅是"用"数据结构，更是**理解和实现**数据结构。如果你直接 `#include <vector>` 拿来用，老师怎么知道你真的懂动态数组的原理？

所以我们要自己写四个容器：动态数组、哈希表、堆、队列。

### 1.5 我们需要什么头文件？

**核心算法层（Containers → Graph → Dijkstra → OrderManager）用的标准库**：

| 头文件 | 哪里用 | 用途 |
|--------|--------|------|
| `<string>` | Graph.h | Node 的城市名和地址 |
| `<iostream>` | Graph.cpp / Dijkstra.cpp | 打印日志 |
| `<fstream>` | OrderManager.cpp | 读写 txt 文件 |
| `<sstream>` | OrderManager.cpp | 字符串流解析 |

一共 **4 个**，都是 C++98 最基础的东西。核心容器 `Containers.h` **零外部依赖**。

### 1.6 安装 Qt

1. 下载 Qt Online Installer：https://www.qt.io/download
2. 安装时勾选 **Qt 6.x.x** → `MSVC 2022 64-bit` 或 `MinGW 64-bit`
3. 安装 **Qt Creator**（推荐，免去 VS Code 的复杂配置）

> 选择 MSVC 还是 MinGW？
> - **MSVC** = 微软的 C++ 编译器。如果你已经装了 Visual Studio 2022，用这个。
> - **MinGW** = Windows 上的 GCC 移植版。Qt 安装器自带，不需要额外装东西。
>
> 推荐：如果你不确定，就选 MinGW 64-bit。

### 1.7 文件夹结构

```
ExpressRouting/
├── Containers.h          # 4 个自实现容器（零外部依赖）
├── Graph.h / Graph.cpp     # 有向图邻接表
├── Dijkstra.h / Dijkstra.cpp  # 最短路径 + 拓扑排序
├── OrderManager.h / OrderManager.cpp  # 订单 + 文件读写
├── MainWindow.h / MainWindow.cpp      # Qt 界面
├── main.cpp              # 程序入口
├── CMakeLists.txt        # 构建配置
├── data/
│   ├── network.txt       # 25 城市路网
│   └── orders.txt        # 5 条测试订单
└── .vscode/              # VS Code 配置（可选）
```

### 1.8 开发顺序（重要！）

```
容器层 → 图层 → 算法层 → 业务层 → GUI层
```

**不要从 GUI 开始写。** 为什么？

想象你在盖楼。如果直接从第 5 层（GUI）开始砌砖，下面 4 层（数据结构、图、算法）都不存在，你一运行程序就崩溃，而且你根本不知道是哪里崩了。

正确的做法是**一层一层写，写完一层立刻测试**：
1. 写 DynArray → 写个测试 main.cpp，push 10 个数，检查访问、拷贝是否正常
2. 写 HashMap → 测试插入、查找、删除是否正常
3. 写 MinHeap → 测试 push 5 个数，pop 出来是不是从小到大
4. ...
5. 最后才写 Qt 界面

---

## 二、DynArray\<T\> -- 动态数组

### 2.1 为什么需要它？

整个项目到处需要"可变长数组"：
- 图的邻接表：每个节点存一个出边列表
- Dijkstra 的路径：存一串节点编号
- MinHeap 的底层：用数组存完全二叉树
- 拓扑排序的队列

所以 DynArray 是**地基**。地基不稳，全楼倒塌。

C++ 标准库有 `std::vector` 做这个事，但课程红线禁止使用。所以我们要自己写一个。

### 2.2 核心原理

#### 2.2.1 静态数组 vs 动态数组

先回顾最简单的常识：

```cpp
int arr[100];    // ★ 静态数组：编译时就固定大小，不能改
                 // 问题：我不知道用户要存几个数！多了溢出了，少了浪费空间
```

动态数组解决这个问题：一开始很小（甚至为空），当用户往里加东西、数组满了时，**自动变大**。

```
初始：                                扩容后：
data = nullptr                       data = [A][B][C][?]
length = 0                            length = 3
capacity = 0                          capacity = 4

push_back("D"):                      push_back("E"):
  length(3) < capacity(4) → 直接放       length(4) == capacity(4) → 满了！
  [D]                                 new T[8]（2 倍）
                                      拷贝 4 个旧元素过去
void expand() {                       delete[] 旧数组
  新容量 = capacity*2（初始给 4）       再放 E
  申请新数组 new T[新容量]
  循环拷贝 newData[i] = data[i]
  删旧数组，更新指针和容量
}
```

#### 知识点：`new` 和 `delete[]` -- 堆内存分配

C++ 中变量可以存在两个地方：

| | 栈（Stack） | 堆（Heap） |
|---|---|---|
| 怎么分配 | 编译器自动 | `new` 手动申请 |
| 怎么释放 | 作用域结束自动 | `delete` 手动归还 |
| 大小限制 | 小（几 MB） | 大（GB 级别） |
| 速度 | 快 | 相对慢 |
| 类比 | 口袋里的零钱 | 银行保险柜 |

```cpp
// 栈：自动管理
void foo() {
    int x = 42;        // x 在栈上，函数结束自动销毁
}

// 堆：手动管理
void bar() {
    int* p = new int[100];   // 从堆上借了 400 字节（100个int）
    // ... 使用 p ...
    delete[] p;              // ★ 必须归还！否则"内存泄漏"
}
```

类比：`new` 就像从图书馆借书，`delete` 就像还书。借了不还，图书馆的书（内存）就越来越少，最后系统崩溃。这叫**内存泄漏（Memory Leak）**。

#### 2.2.2 为什么扩容是 2 倍？

push n 次，扩容发生在第 4、8、16、... 次。总拷贝次数 = 4+8+16+...+n/2 ≈ n。均摊到每次 push 上 = O(1)。

如果每次只加 10 个位置，总拷贝 = 10+20+30+...+n = O(n²)。

**用排队打饭来类比**：
- 2 倍扩容 = 每次食堂阿姨发现队伍太长就开一个新窗口，窗口数翻倍。找个平均时间，你需要排多久？很快，因为窗口越来越多。
- 每次加 10 个位置 = 每次发现位置不够就加 10 个。如果来了 1000 个人，你需要重新分配 100 次！每次都把所有的人挪一遍。

### 2.3 完整代码

创建 `Containers.h`：

```cpp
#pragma once
// ============================================================================
// Containers.h -- 自实现容器库（替代 STL）
// ============================================================================
// 四个基础容器：DynArray / HashMap / MinHeap / Queue
// 零外部依赖，不使用 std::move / std::swap / if constexpr
// ============================================================================

// ============================================================================
// DynArray<T> -- 动态数组
// ============================================================================
// 核心操作：push_back 均摊 O(1)。满了扩容 2 倍。
// 拷贝：逐元素深拷贝（不能 memcpy，因为 T 可能是 string）
// ============================================================================
template<typename T>
class DynArray {
    // ===== 成员变量 =====
    T* data = nullptr;
    // ↑ 指向堆上数组的指针。T 是模板参数 -- 如果写 DynArray<int>，T 就是 int。
    //   nullptr 是 C++ 的空指针（C 语言用 NULL，但 C++ 中 nullptr 更安全）。
    //   初始为空，第一次 push_back 时才分配内存。

    int length = 0;
    // ↑ 当前存了几个元素。注意和 capacity 的区别：
    //   length ≤ capacity 永远成立。length 是从用户角度看"有几个"，
    //   capacity 是从内存角度看"能放几个"。

    int capacity = 0;
    // ↑ 已分配的数组长度。length==capacity 时触发 expand() 扩容。

    // ===== 私有辅助函数 =====
    void expand() {
        int newCap = capacity ? capacity * 2 : 4;   // 初始 4 个槽
        // ↑ 三元运算符：(条件) ? (真时的值) : (假时的值)。
        //   如果 capacity > 0，newCap = capacity × 2
        //   如果 capacity == 0（初始状态），newCap = 4
        //   为什么初始是 4 不是 1？如果初始是 1，push 2 个元素就要扩容 2 次，太频繁。

        T* newData = new T[newCap];
        // ↑ new T[n]：在堆上申请 n 个 T 类型的内存。注意是 n 个对象，不是 n 个字节。
        //   如果 T 是 int，new int[4] 就是 16 字节（4×4）。
        //   如果 T 是 string，new string[4] 就构造 4 个空 string。

        for (int i = 0; i < length; ++i) newData[i] = data[i];
        // ↑ 逐元素赋值。为什么不用 memcpy？
        //   memcpy 是"按位拷贝" -- 只复制内存中的二进制数据。
        //   对 int 没问题，但对 string 有致命问题：
        //   string 内部有一个指针指向堆上的字符数组。
        //   memcpy 只复制了指针，不复制指针指向的内容。
        //   两个 string 的指针指向同一块内存 → 一个释放后另一个读 → 崩溃。
        //   用 = 赋值运算符，string 的 = 已经写好了深拷贝逻辑。

        delete[] data;        // 释放旧数组的内存
        data = newData;       // 指向新数组
        capacity = newCap;    // 更新容量
    }

public:
    // ===== 构造函数 =====
    DynArray() = default;
    // ↑ "= default" 是什么？
    //   构造函数是对象创建时自动调用的函数。
    //   如果你不写任何构造函数，编译器会生成一个"默认构造函数"：
    //     DynArray() {}  空函数，什么都不做。
    //   "= default" 就是告诉编译器："用你生成的默认版本就行"。
    //   我们的成员变量已经初始化了（data=nullptr, length=0, capacity=0），
    //   所以让编译器自己处理就够了。

    // ===== 拷贝构造函数 =====
    // ★ 这是最容易被忽略但最致命的函数 ★
    DynArray(const DynArray& other) {
        for (int i = 0; i < other.length; ++i) push_back(other.data[i]);
    }
    // ↑ 拷贝构造函数：当执行 DynArray<int> b = a; 时会调用这个函数。
    //   参数 const DynArray& other 是被拷贝的原始数组（即 a）。
    //   我们选择逐元素 push_back，是为了**逐个拷贝** each element。
    //   这样能触发 expand() 来分配新内存。
    //
    // ★★★ 如果没有拷贝构造函数会发生什么？（重要！） ★★★
    //   编译器会自动生成一个"默认拷贝构造函数"，它做的是**浅拷贝**：
    //     b.data = a.data;      ← 只拷贝了指针！不是拷贝数组内容！
    //     b.length = a.length;
    //     b.capacity = a.capacity;
    //   结果：a.data 和 b.data 指向**同一块堆内存**。
    //
    //   浅拷贝示意图：
    //     a.data ──→ [A][B][C][?]  ←─ b.data（指向同一块！）
    //
    //   问题来了：
    //     1. 修改 b[0] = 999 → a[0] 也变成了 999（因为指向同一块内存）
    //     2. a 的析构函数 delete[] a.data（释放了 [A][B][C]）
    //     3. b 的析构函数 delete[] b.data（再次释放同一块内存！）
    //        → "double free" 错误 → 程序崩溃！
    //
    //   深拷贝示意图：
    //     a.data ──→ [A][B][C][?]
    //     b.data ──→ [A][B][C][?]（完全独立的一份！）

    // ===== 析构函数 =====
    ~DynArray() { delete[] data; }
    // ↑ ~类名() 是析构函数，对象销毁时自动调用。
    //   "销毁"的时机：离开作用域，或者执行 delete obj。
    //   职责：归还从堆上申请的资源（delete[] data）。
    //
    //   为什么必须有析构函数？
    //   因为构造函数中我们 new 了内存。C++ 不会自动帮你还书 -- 必须写析构函数。
    //   不写 = 内存泄漏。

    // ===== 赋值运算符 =====
    DynArray& operator=(const DynArray& other) {
        if (this == &other) return *this;
        // ↑ ★ 自赋值检查 ★
        //   如果有人写 arr = arr;（虽然看起来傻，但可能通过引用间接发生）
        //   如果不检查，下面的 delete[] data 会把自己的数据删掉，然后自己拷自己 → 崩溃。
        //   "this" 是指向"当前对象"的指针。&other 是参数对象的地址。
        //   如果它们相等 → 是同一个人 → 直接返回。
        //
        //   知识点：`this` 指针
        //   在类的成员函数内部，`this` 是一个隐含的指针，指向调用这个函数的对象。
        //   比如 arr.push_back(3); 在 push_back 内部，`this` 就指向 arr。

        delete[] data; data = nullptr; length = capacity = 0;
        // ↑ 1. 先把旧数据清掉
        //   2. data = nullptr 防止悬空指针

        for (int i = 0; i < other.length; ++i) push_back(other.data[i]);
        // ↑ 3. 逐元素拷贝

        return *this;
        // ↑ 4. 返回 *this（当前对象的引用），允许链式赋值：a = b = c;
    }

    // ---- 增删 ----
    void push_back(const T& value) {
        // ↑ 参数为什么是 "const T&" 而不是 "T"？
        //   如果参数是 T value：调用 push_back 时，实参会被**拷贝**一份传入函数。
        //     DynArray<string> arr; arr.push_back("hello");
        //     如果参数是 string："hello" 先被拷贝 → 得到一个临时 string → 再拷贝到数组中
        //     一共 2 次拷贝！
        //   如果参数是 const T&（常引用）：不拷贝，直接传递"原物的别名"。
        //     arr.push_back("hello");
        //     "hello" 直接被放置到数组中 → 1 次拷贝。
        //
        //   知识点：引用（Reference）
        //   int x = 42; int& ref = x;  // ref 是 x 的别名，修改 ref 就是修改 x
        //   const int& cref = x;       // cref 也是别名，但不能通过 cref 修改 x
        //   引用本质上是指针的"语法糖"，但写法更简洁。

        if (length == capacity) expand();
        // ↑ 满了就扩容。length 是当前元素数，capacity 是总槽位数。

        data[length++] = value;
        // ↑ 等价于：
        //   data[length] = value;
        //   length = length + 1;
        //   "后置++" 的含义：先取值，再自增。length++ 返回旧值，然后 length 自己加 1。
    }

    void pop_back() {
        if (length > 0) --length;
        // ↑ 只改 length，不释放内存。
        //   数据还在内存里，但对外"看不见"了。
        //   好处：如果后面又 push_back，不需要重新分配。
    }

    // 删索引 index，后面元素前移
    void erase(int index) {
        for (; index < length - 1; ++index) data[index] = data[index + 1]; --length;
        // ↑ 示意图（删除索引 1 的元素 B）：
        //   删除前：[A][B][C][D]   length=4
        //   删除后：[A][C][D][D]   length=3
        //   （[D] 还在内存里，但 length=3 所以访问不到）
        //
        //   循环体等价于：
        //     for (int i = index; i < length - 1; ++i) {
        //         data[i] = data[i + 1];   // 后面的元素往前挪一位
        //     }
        //     length = length - 1;
    }

    // 删除第一个匹配的元素
    template<typename P>
    bool remove_first(P check) {
        for (int i = 0; i < length; ++i)
            if (check(data[i])) { erase(i); return true; }
        return false;
    }
    // ↑ `template<typename P>` 是什么？为什么函数也需要模板？
    //   因为 check 可以是任何"能当函数用的东西"：
    //     - 一个函数：bool isFive(int x) { return x == 5; }
    //     - 一个 lambda 表达式（后面会讲到）：[](int x){ return x == 5; }
    //   我们不限制 P 的具体类型，只要 check(data[i]) 能编译通过就行。

    // 删除所有匹配元素（删了不 +i，因为下个元素移到此位置）
    template<typename P>
    void remove_all(P check) {
        for (int i = 0; i < length;) {
            if (check(data[i])) erase(i);   // 删除后 data[i] 变成原来的 data[i+1]，所以 i 不自增
            else ++i;                       // 没匹配上，i 自增正常前进
        }
    }

    // ---- 访问 ----
    T& operator[](int i) {
        return data[i];
    }
    // ↑ operator[] 就是让对象能像数组一样用方括号访问：
    //   DynArray<int> a; a.push_back(42); cout << a[0];
    //   编译器看到 a[0] 会自动翻译成 a.operator[](0)。

    const T& operator[](int i) const {
        return data[i];
    }
    // ↑ 这个 const 版本用于"只读"的 DynArray（前面有 const 修饰的变量）。
    //   两个版本共存，编译器会根据 context 选择：
    //     非 const 的 DynArray → 用第一个（可修改返回值）
    //     const 的 DynArray   → 用第二个（返回值也是 const，不能改）

    T& back() {
        return data[length - 1];
    }
    // ↑ 返回最后一个元素。用来配合 pop_back：先看 back() 再弹出。
    //   注意：如果 length == 0，data[-1] 会访问非法内存。调用者要保证非空。

    // ---- 查询 ----
    int size() const {
        return length;
    }
    // ↑ 注意末尾的 const 关键字！这叫做"const 成员函数"。
    //   它承诺："这个函数不会修改对象的任何成员变量"。
    //   为什么需要这个承诺？
    //   如果一个对象是 const 的（如 void foo(const DynArray<int>& arr)），
    //   你只能调用它的 const 成员函数。普通函数不允许调用，因为无法验证它不会修改。

    bool empty() const {
        return length == 0;
    }

    void clear() {
        length = 0;
    }
    // ↑ 不清空内存，只是把逻辑长度归零。capacity 不变。
    //   好处：如果后面又 push_back，直接复用已有的 capacity，不用重新分配。
    //   类比：在黑板上写字，擦黑板不拆黑板。

    // ---- 迭代器（支持 for (auto& x : arr) ...）----
    T* begin() {
        return data;
    }
    T* end() {
        return data + length;
    }
    // ↑ 指针本身就可以做迭代器！
    //   当你写 for (auto& x : arr) 时，编译器生成的代码大致是：
    //     for (T* it = arr.begin(); it != arr.end(); ++it) {
    //         auto& x = *it;
    //         // 循环体
    //     }
    //   因为 data 是连续内存（数组），指针 it++ 天然就是"下一个元素"。
    //   这就是为什么 STL 的 vector::iterator 在大多数实现中就是 T*。

    const T* begin() const { return data; }
    const T* end()   const { return data + length; }
    // ↑ const 版本：用于只读遍历
};
```

#### 2.3.1 逐步追踪：push_back 6 次发生了什么事？

让我们一步步看 `DynArray<int> arr;` 然后 `arr.push_back(10)` 六次：

```
初始：
  data = nullptr    length = 0    capacity = 0

push_back(10):
  length(0) == capacity(0) → 触发 expand()
    expand(): newCap = capacity? 0*2 : 4 → newCap = 4
              newData = [  ?,  ?,  ?,  ? ]       // 堆上分配 4 个 int
              不执行循环（length=0）
              delete[] nullptr → 安全（删除空指针是合法的）
              data = newData, capacity = 4
  data[0] = 10, length = 1
  数组: [10][?][?][?]   len=1, cap=4

push_back(20):
  length(1) < capacity(4) → 不扩容
  data[1] = 20, length = 2
  数组: [10][20][?][?]   len=2, cap=4

push_back(30):
  data[2] = 30, length = 3
  数组: [10][20][30][?]   len=3, cap=4

push_back(40):
  data[3] = 40, length = 4
  数组: [10][20][30][40]   len=4, cap=4

push_back(50):
  length(4) == capacity(4) → 触发 expand()
    expand(): newCap = 4 * 2 = 8
              newData = [  ?,  ?,  ?,  ?,  ?,  ?,  ?,  ? ]
              for i=0..3: newData[i] = data[i]
              → newData = [10][20][30][40][?][?][?][?]
              delete[] data (释放旧 [10][20][30][40])
              data = newData, capacity = 8
  data[4] = 50, length = 5
  数组: [10][20][30][40][50][?][?][?]   len=5, cap=8

push_back(60):
  length(5) < capacity(8) → 不扩容
  data[5] = 60, length = 6
  数组: [10][20][30][40][50][60][?][?]   len=6, cap=8
```

#### 2.3.2 内存布局示意图

```
栈（Stack）                         堆（Heap）
┌─────────────┐                    ┌─────────────────────────┐
│ arr         │                    │     DynArray 数据区       │
│ .data ──────┼──→指向堆上的数组─→│ [10][20][30][40][50][60] │
│ .length = 6 │                    │           ↑              │
│ .capacity=8 │                    │    还有 2 个空槽 (?)      │
└─────────────┘                    └─────────────────────────┘
```

`arr` 对象本身在栈上（3 个成员变量 ≈ 24 字节），但它 data 指针指向的数组在堆上（8 × sizeof(int) = 32 字节）。

### 2.4 测试

在 Containers.h 下方写一个测试用的 main 函数（或者单独创建 test.cpp）：

```cpp
// 临时测试代码（写完下一章节可以删掉）
#include <iostream>
using namespace std;

int main() {
    // 测试 1：基本 push_back 和访问
    DynArray<int> a;
    for (int i = 0; i < 10; ++i) a.push_back(i * 10);
    cout << "测试1 - a[3] = " << a[3] << "\n";    // 应输出 30
    cout << "测试1 - size = " << a.size() << "\n";  // 应输出 10

    // 测试 2：拷贝构造函数（深拷贝！）
    DynArray<int> b = a;
    b[0] = 999;
    cout << "测试2 - a[0] = " << a[0] << "\n";    // 应输出 0，不是 999
    // 如果输出 999，说明是浅拷贝 → 拷贝构造函数有 bug

    // 测试 3：erase
    a.erase(3);
    cout << "测试3 - a[3] = " << a[3] << "\n";    // 应输出 40（原来的 30 被删了，40前移）
    cout << "测试3 - size = " << a.size() << "\n"; // 应输出 9

    // 测试 4：范围 for（迭代器）
    cout << "测试4 - 遍历: ";
    for (auto& x : a) cout << x << " ";
    cout << "\n";

    // 测试 5：赋值运算符
    DynArray<int> c;
    c.push_back(88);
    c = a;                                   // 测试 operator=
    cout << "测试5 - c[0] = " << c[0] << "\n";   // 应输出 0（同 a）

    // 测试 6：自赋值
    a = a;                                   // 不应该崩溃
    cout << "测试6 - a 自赋值后 size = " << a.size() << "\n";   // 正常输出 9

    return 0;
}
```

如果输出都正确，恭喜你，地基打好了！

---

## 三、HashMap\<V\> -- 开放地址哈希表

### 3.1 为什么需要哈希表？

前面我们写了 DynArray，它可以通过下标快速访问。但有一个问题：**下标必须是 0, 1, 2... 连续整数**。

如果我想这样用：
```
存储"节点编号 5 对应的信息"
存储"节点编号 13 对应的信息"
存储"节点编号 200 对应的信息"
```

用 DynArray 的话，得开一个长度至少 201 的数组，大量空间浪费。

**哈希表解决的就是"用任意整数做下标，快速存取"的问题。**

它的核心思想：把一个任意大小的整数，通过一个数学函数，映射到一个小范围内的位置。

```
键（key）→ 哈希函数 → 数组下标 → 直接取数据
例：key=5   → hash(5)=3  → arr[3] → 数据
   key=13  → hash(13)=3 → 冲突！需要解决
```

#### 知识点：类比 -- 图书馆的书架

哈希表就像图书馆的分类系统：
- 每本书有一个索书号（key）
- 索书号通过分类规则（哈希函数）→ 确定放在哪个书架（数组下标）
- 如果两个不同的书被分到同一个书架（哈希冲突）→ 往旁边挪一格（开放地址法）

### 3.2 为什么键固定为 int？

项目中所有哈希表都用 int 键：
- `HashMap<Node>` -- 节点编号 → 节点信息
- `HashMap<DynArray<Edge>>` -- 节点编号 → 出边列表
- `HashMap<double>` -- 节点编号 → 最短距离
- `HashMap<int>` -- 节点编号 → 前驱节点编号
- `HashMap<bool>` -- 节点编号 → 集合中是否存在

键只有 int 这一种。所以把键类型固定为 int，代码少一个模板参数，更简单。

### 3.3 核心原理详解

#### 3.3.1 哈希函数（乘法哈希）

```
h(k) = (k × 2654435761) mod 表容量

2654435761 = 2³² × (√5-1)/2 = 黄金比例的倒数 × 2³²
```

**为什么是这个魔数 2654435761？**

假设表容量是 16，我们看看把 1, 2, 3, 4, 5 用这个公式映射到哪里：

```
k=1: (1 × 2654435761) % 16 = 2654435761 % 16 = 1     ← 模 16 的结果就是最后 4 bit
k=2: 5308871522 % 16 = 2
k=3: 7963307283 % 16 = 3
k=4: 10617743044 % 16 = 4
k=5: 13272178805 % 16 = 5
...
```

如果直接 `k % 16`（取模哈希），连续整数映射到连续槽，很均匀。但如果是 `k % 16` 且 k 是 1000, 1001, 1002... 呢？`1000%16=8, 1001%16=9, ...` 连续键还是映射到连续槽。

如果键不是均匀分布的（比如 100, 200, 300... 差 100），`100%16=4, 200%16=8, 300%16=12` 也很均匀。

但如果键是 16, 32, 48, 64... 呢？全映射到 0！

乘法哈希的好处是：它把键先乘以一个"看起来随机的"巨大奇数，然后再取模。这样即使键有规律（如都是 16 的倍数），哈希结果也是分散的。黄金比例倒数 (√5-1)/2 ≈ 0.618 在数学上被证明能让分布最均匀。

**知识点：`unsigned` 类型**

```cpp
unsigned hashIndex(int k) const {
    return (unsigned)k * 2654435761u % (unsigned)cap;
}
```

`unsigned` 是无符号整数（没有负数）。为什么哈希函数返回 unsigned？
- 因为数组下标不能是负数
- C++ 中负数取模的结果是负数（如 -3 % 16 = -3），这会导致数组下标越界
- 先转成 unsigned 再取模，保证结果 ≥ 0

#### 3.3.2 冲突解决（开放地址 + 线性探测）

```
哈希表 = 数组 + 哈希函数

k₁=5 → slot 3
k₂=13 → 也算出 slot 3（冲突！）
  → 看 slot 4，空 → 放 slot 4

就像找停车位，首选被占了就往前找一个。
"开放地址"的意思是：所有元素都放在同一张大表里，不额外拉链表。
"线性探测"的意思是：每次只往前挪 1 格。
```

**逐步追踪：插入 key=5，然后 key=13（冲突），然后删除 key=5，然后查找 key=13**

假设表容量 cap=8：

```
初始表：[空][空][空][空][空][空][空][空]

步骤 1：插入 key=5, value="A"
  hash(5) = (unsigned)5 * 2654435761u % 8
          = 13272178805u % 8
          = 5       （13272178805 / 8 的余数是 5）
  table[5] 为空 → 放入 table[5]
  表：[空][空][空][空][空][5→"A"][空][空]

步骤 2：插入 key=13, value="B"
  hash(13) = (unsigned)13 * 2654435761u % 8
           = 34507664893u % 8
           = 5       （余数还是 5 -- 冲突！）
  table[5] 已占用 → 线性探测 table[6] → 空 → 放入
  表：[空][空][空][空][空][5→"A"][13→"B"][空]
                          ↑       ↑
                       原本位置  探测后位置

步骤 3：删除 key=5（惰性删除）
  找到 table[5]，标记 removed=true
  表：[空][空][空][空][空][(删)5→"A"][13→"B"][空]
  cnt 从 2 减到 1（有效元素数）

步骤 4：查找 key=13
  hash(13) = 5
  table[5].used=true 但 table[5].removed=true → ★ 跳过，但不停止 ★
  table[6].used=true, table[6].removed=false, table[6].key==13 → 找到！
  ✓ 返回 table[6].value = "B"

如果步骤 3 用的是"真正删除"（清空 table）而不是惰性删除：
  表：[空][空][空][空][空][空][13→"B"][空]
  步骤 4：hash(13)=5 → table[5].used=false → 停止 → "不存在" ✗ 错误！
```

**这就是惰性删除存在的全部意义。**

#### 知识点：struct 和 class 的区别

```cpp
struct Slot {
    int  key;
    V    value;
    bool used = false;
    bool removed  = false;
};
```

C++ 中 `struct` 和 `class` 几乎完全一样，只有一个区别：
- `struct` 的成员**默认是 public**（公开访问）
- `class` 的成员**默认是 private**（私有访问）

为什么在 HashMap 内部用 struct 而不是 class？
因为 Slot 只是 HashMap 的内部实现细节，不暴露给外界。用 struct 写起来少打 `public:` 三个字。

### 3.4 完整代码

追加到 `Containers.h`：

```cpp
// ============================================================================
// HashMap<V> -- 开放地址哈希表（键固定为 int）
// ============================================================================
// 四个核心设计：乘法哈希 + 线性探测 + 惰性删除 + 0.5 负载扩容
// ============================================================================
template<typename V>
class HashMap {
    // ===== 内部结构 =====
    // struct 的成员默认 public（私有实现不需要封装）
    struct Slot {
        int  key;
        // ↑ 键（索引）。用户用这个来找数据。如查找 key=5 时，找 key==5 的 Slot。

        V    value;
        // ↑ 值（数据）。Key 对应的数据。V 是模板参数 -- HashMap<double> 时 V=double。

        bool used = false;
        // ↑ 这个槽位被占用过吗？包括当前有数据和曾经有数据但已惰性删除的槽。
        //   used==false 的槽位是"处女槽" -- 从未被写过，可以安心停止探测。

        bool removed  = false;
        // ↑ 被惰性删除了吗？removed==true 的槽对查找来说是"跳过"，
        //   但对插入来说是"可以复用"。
    };

    Slot* table = nullptr;
    // ↑ 堆上的 Slot 数组。与 DynArray 的 data 一样，new 来的内存。

    int   cap = 0;
    // ↑ 表的总容量（Slot 数组的长度）。不是有效元素数！

    int   cnt = 0;
    // ↑ 有效元素数（used==true 且 removed==false）。
    //   当 cnt / cap >= 0.5 时触发扩容。

    // ===== 乘法哈希函数 =====
    unsigned hashIndex(int k) const {
        return (unsigned)k * 2654435761u % (unsigned)cap;
        // ↑ 一步步拆解：
        //   1. (unsigned)k：把 int 转成 unsigned。因为负数取模结果是负数，
        //      转 unsigned 后取模保证结果 >=0。
        //   2. 2654435761u：魔数 "u" 后缀表示这是 unsigned 字面量。
        //   3. % (unsigned)cap：取模运算，cap 也转为 unsigned 统一类型。
    }

    // ===== 扩容（rehash）=====
    void rebuild(int newCap) {
        Slot* oldTable = table;
        int oldCap = cap;
        table = new Slot[newCap]();
        // ↑ new Slot[newCap]() -- 注意末尾的括号 ()！
        //   C++ 中 new T[n] 和 new T[n]() 有重要区别：
        //     new T[n]   → 默认初始化（对于基本类型，值是不确定的随机垃圾）
        //     new T[n]() → 值初始化（对于基本类型，设为 0；对于 bool，设为 false）
        //   我们写了 ()，所以所有 Slot 的 used/removed 都自动设为 false。
        //   如果忘了写 ()，used/removed 会是随机值 → 灾难！

        cap = newCap; cnt = 0;
        // ↑ cnt 归零，因为 operator[] 下面重新插入时会自增。

        for (int i = 0; i < oldCap; ++i)
            if (oldTable[i].used && !oldTable[i].removed)
                (*this)[oldTable[i].key] = oldTable[i].value;
        // ↑ 遍历旧表，把有效元素（used 且未删）重新插入新表。
        //   (*this)[key] 是 operator[]，它在更大的新表里重新计算哈希位置。
        //   因为是全新的表（cnt=0, table 全是处女槽），不会有冲突累积。

        delete[] oldTable;
        // ↑ 旧表用完，归还内存。
    }

public:
    // ===== 构造函数 =====
    HashMap(int initCap = 16) : table(new Slot[initCap]()), cap(initCap), cnt(0) {}
    // ↑ 注意冒号后面的部分：这是"初始化列表"（Member Initializer List）。
    //   它等价于把下面三行写在函数体开头：
    //     table = new Slot[16]();
    //     cap = 16;
    //     cnt = 0;
    //   但初始化列表更高效：成员在构造函数的函数体执行之前就被初始化了。
    //
    //   initCap=16 表示默认值。用户不传参时用 16。为什么是 16？
    //   16 是 2 的幂，位运算更高效。而且对于约 25 个节点的图来说，
    //   容量 16，负载 0.5 时触发扩容（即 8 个元素时扩容），刚好够用。

    // ===== 拷贝构造 =====
    HashMap(const HashMap& other)
        : table(new Slot[other.cap]()), cap(other.cap), cnt(other.cnt) {
        for (int i = 0; i < cap; ++i) table[i] = other.table[i];
        // ↑ 逐槽拷贝。Slot 是简单 struct，= 就能完成浅拷贝（成员逐个复制）。
        //   V 的值也是通过赋值运算符拷贝的 —— 如果 V 是 DynArray，会触发 DynArray 的 operator=。
        //   注意：我们不需要 operator[] 重新插入，因为哈希冲突模式已经确定。
    }

    // ===== 析构 =====
    ~HashMap() { delete[] table; }

    // ===== 赋值 =====
    HashMap& operator=(const HashMap& other) {
        if (this == &other) return *this;
        // ↑ 自赋值保护（和 DynArray 一样）

        delete[] table;
        table = new Slot[other.cap]();
        cap = other.cap;
        cnt = other.cnt;
        for (int i = 0; i < cap; ++i) table[i] = other.table[i];
        return *this;
    }

    // ==================
    // operator[]：查找或插入
    // ==================
    // ★ 这是 HashMap 最核心的函数 ★
    // 有则返回引用，无则插入默认值再返回引用
    V& operator[](int k) {
        if (cnt * 2 >= cap) rebuild(cap * 2);
        // ↑ 负载因子检查：cnt/cap >= 0.5（等价于 cnt*2 >= cap）。
        //   为什么要控制负载因子 ≤ 0.5？
        //   开放地址法的查找性能严重依赖负载因子：
        //     load=0.25 → 平均探测 1.33 次
        //     load=0.5  → 平均探测 2.0 次
        //     load=0.75 → 平均探测 4.0 次
        //     load=0.9  → 平均探测 10.0 次
        //   0.5 是一个好的折中：空间利用率尚可，性能也还好。

        unsigned i = hashIndex(k);
        int firstDel = -1;
        // ↑ firstDel：遇到的第一个 removed 槽的下标。初始 -1 表示"还没遇到"。
        //   如果整个探测链上没有空位，我们可以复用这个 removed 槽。

        while (table[i].used) {
            // ↑ 只要槽位被"用过"（有数据或惰性删除标记），就继续看。
            //   什么时候停？遇到 used==false 的处女槽。

            if (table[i].removed) {
                if (firstDel < 0) firstDel = (int)i;
                // ↑ 记下第一个 removed 槽的位置，但继续探测（key 可能在更后面）
            } else if (table[i].key == k) {
                return table[i].value;
                // ↑ 找到了！返回引用，调用者可以读取或修改。
            }
            i = (i + 1) % (unsigned)cap;
            // ↑ 线性探测：看下一个槽。到末尾回绕到开头（环形探测）。
        }
        // 循环结束 → 没找到 key，需要插入。

        int dest = (firstDel >= 0) ? firstDel : (int)i;
        // ↑ 优先复用 removed 槽，没有则用遇到的第一个处女槽。

        table[dest] = { k, V{}, true, false };
        // ↑ 聚合初始化（Aggregate Initialization）：
        //   按 Slot 中成员的声明顺序依次赋值：
        //     key=k, value=V{}, used=true, removed=false
        //
        //   V{} 是什么？
        //     V{} 是"值初始化"，创建一个用默认值的 V 对象：
        //       V=int    → 0
        //       V=double → 0.0
        //       V=string → ""（空字符串）
        //       V=DynArray<int> → 空数组（调用了 DynArray 的默认构造函数）
        //   如果直接写 V()，可能被解析成函数声明（"most vexing parse"），
        //   所以用 V{} 更安全。

        ++cnt;
        return table[dest].value;
    }

    void set(int k, const V& v) { (*this)[k] = v; }
    // ↑ 等价于 hashMap[k] = v;  —— 语法糖，方便显式设置值。

    // ==================
    // find：查找，返回指针
    // ==================
    // ★ 为什么返回指针而不是 bool？ ★
    //   指针可以同时表达两种信息：
    //     nullptr  → "不存在"
    //     有效指针  → "存在"，并且通过 *ptr 可以直接获取值
    //   如果返回 bool，调用者还需要再调一次 operator[] 来获取值，
    //   这意味着两次哈希计算。
    V* find(int k) {
        unsigned i = hashIndex(k), s = i;
        // ↑ s 记住起始位置。如果绕了一圈回到起始，说明表满了或全是 removed。

        while (table[i].used) {
            if (!table[i].removed && table[i].key == k)
                return &table[i].value;
                // ↑ 有效元素且 key 匹配 → 返回值的地址

            if ((i = (i + 1) % (unsigned)cap) == s) break;
            // ↑ 探测到下一个位置。如果绕回起点 → 停止（防止死循环）。
            //   注意这个 "赋值 + 比较" 的写法：
            //     (i = (i + 1) % cap) == s
            //   先执行 i = (i+1)%cap，然后判断新的 i 是否等于 s。
            //   这是 C 风格的紧凑写法。
        }
        return nullptr;
        // ↑ nullptr：未找到。C++11 引入的关键字，类型安全。
        //   不推荐用 NULL（NULL 本质是 #define NULL 0，在某些情况下有歧义）。
    }

    const V* find(int k) const {
        // ↑ const 版本的 find：返回 const V*，不能通过指针修改值。
        unsigned i = hashIndex(k), s = i;
        while (table[i].used) {
            if (!table[i].removed && table[i].key == k) return &table[i].value;
            if ((i = (i + 1) % (unsigned)cap) == s) break;
        }
        return nullptr;
    }

    bool contains(int k) const {
        return find(k) != nullptr;
        // ↑ 只需要知道"在不在"，用 contains 更语义化。
    }

    // ==================
    // erase：惰性删除
    // ==================
    bool erase(int k) {
        unsigned i = hashIndex(k), s = i;
        while (table[i].used) {
            if (!table[i].removed && table[i].key == k) {
                table[i].removed = true;
                // ↑ 只打标记，不清空 key 和 value。
                //   key 和 value 保留（find 时 key 还需要用来比较跳过）。
                --cnt;
                return true;
            }
            if ((i = (i + 1) % (unsigned)cap) == s) break;
        }
        return false;
    }

    // ==================
    // 辅助查询
    // ==================
    int size()  const { return cnt; }
    // ↑ cnt 是有效元素数（不包括惰性删除的槽）

    bool empty() const { return cnt == 0; }

    void clear() {
        for (int i = 0; i < cap; ++i)
            table[i].used = table[i].removed = false;
        // ↑ 全部标回"处女槽"。不释放内存（delete[] table 再 new），
        //   因为可能马上又要插入。容量保持 cap 不变。
        cnt = 0;
    }

    // ==================
    // forEach：遍历所有有效元素
    // ==================
    template<typename F>
    void forEach(F f) {
        for (int i = 0; i < cap; ++i)
            if (table[i].used && !table[i].removed)
                f(table[i].key, table[i].value);
        // ↑ f 是任意"可调用对象"（函数、函数指针、lambda 表达式）。
        //   每找到一个有效元素，就调用 f(key, value)。
        //   使用示例：
        //     nodes.forEach([](int id, const Node& node) {
        //         cout << id << ": " << node.name << "\n";
        //     });
    }

    template<typename F>
    void forEach(F f) const {
        for (int i = 0; i < cap; ++i)
            if (table[i].used && !table[i].removed)
                f(table[i].key, table[i].value);
    }
    // ↑ const 版本：用于只读遍历。
};
```

#### 知识点：Lambda 表达式

```cpp
nodes.forEach([](int id, const Node& node) {
    cout << id << ": " << node.name << "\n";
});
```

Lambda 是 C++11 引入的**匿名函数**。上面这行等价于：

```cpp
// 传统写法：先定义一个函数
void printNode(int id, const Node& node) {
    cout << id << ": " << node.name << "\n";
}
// 然后传函数指针
nodes.forEach(printNode);
```

Lambda 的语法：
```
[capture](parameters) -> return_type { body }
  ↑        ↑              ↑            ↑
捕获列表  参数列表       返回类型      函数体
```

- `[]` 空捕获：不访问外部变量
- `[=]` 按值捕获：lambda 内使用的所有外部变量都拷贝一份
- `[&]` 按引用捕获：lambda 内使用的所有外部变量都是引用
- `[id]`：只捕获 id 这一个变量（按值）
- `[&id]`：只捕获 id 这一个变量（按引用）

在我们的 Graph.cpp 中用到了 `[id]` 的捕获：

```cpp
adj.forEach([id](int, DynArray<Edge>& edgeList) {
    edgeList.remove_all([id](const Edge& edge) { return edge.to == id; });
});
// ↑ 外层 lambda 捕获 id（待删除节点编号）
//   内层 lambda 也捕获 id
```

### 3.5 测试

```cpp
HashMap<double> dist;   // 键=int，值=double，表示距离
dist[1] = 5.5;
dist[2] = 3.2;

// 测试 1：查找存在的 key
double* v = dist.find(1);
cout << "测试1 - 找到 key=1: " << *v << "\n";   // 5.5

// 测试 2：operator[] 自动创建
cout << "测试2 - key=3 自动创建: " << dist[3] << "\n";   // 0

// 测试 3：惰性删除
dist.erase(2);
cout << "测试3 - key=2 已删除, contains=" << dist.contains(2) << "\n"; // 0 (false)

// 测试 4：删除后不影响其他元素的查找
v = dist.find(1);
cout << "测试4 - 删除 key=2 后 key=1 仍在: " << *v << "\n";   // 5.5

// 测试 5：删除后重新插入
dist[2] = 9.9;
// ↑ 会优先复用之前惰性删除的槽（key=2 的 removed 槽）
cout << "测试5 - key=2 重新赋值后: " << dist[2] << "\n";    // 9.9

// 测试 6：冲突测试（手动触发）
HashMap<double> t(4);  // 很小的表，容易冲突
t[1] = 1.1; t[5] = 5.5; t[9] = 9.9; t[13] = 13.13;
// ↑ 如果 hash(1)=1, hash(5)=1 冲突, hash(9)=1 又冲突...
//   测试线性探测是否正常
cout << "测试6 - 冲突后查找 key=13: " << *t.find(13) << "\n"; // 13.13

// 测试 7：扩容
t[17] = 17.17;  // 第 5 个元素，超过负载 0.5（容量 4，5>2），触发扩容
cout << "测试7 - 扩容后 size=" << t.size() << "\n";    // 5
cout << "测试7 - 扩容后 key=1 仍在: " << *t.find(1) << "\n"; // 1.1
cout << "测试7 - 扩容后 key=17: " << *t.find(17) << "\n"; // 17.17
```

---

## 四、MinHeap\<T,Cmp\> -- 二叉最小堆 + Queue\<T\> -- 队列

### 4.1 MinHeap 原理

#### 4.1.1 什么是堆？

堆（Heap）是一种**完全二叉树**（每一层都填满，最后一层靠左），但它不用指针存储，而是用**数组**。

堆有一个"堆有序"性质：**父节点 ≤ 子节点**（最小堆）或**父节点 ≥ 子节点**（最大堆）。

```
最小堆范例：
      2          ← 堆顶（最小值）
    /   \
   5     8
  / \
 9   7

数组表示：[2, 5, 8, 9, 7]
索引：     0  1  2  3  4
```

**索引规则（不用指针，用算术）**：
- 左孩子 = 2i + 1
- 右孩子 = 2i + 2
- 父节点 = (i-1) / 2 （整数除法，自动向下取整）

验证一下：
- arr[0]=2 的左孩子 = arr[2*0+1] = arr[1] = 5 ✓
- arr[0]=2 的右孩子 = arr[2*0+2] = arr[2] = 8 ✓
- arr[1]=5 的父节点 = arr[(1-1)/2] = arr[0] = 2 ✓

#### 4.1.2 push 操作（插入 + 上浮）

```
push(3) 的完整过程：

初始：[2, 5, 8, 9, 7]
      2
    /   \
   5     8
  / \
 9   7

步骤 1：放到末尾
  数组：[2, 5, 8, 9, 7, 3]
  树形：  2
        /   \
       5     8
      / \   /
     9   7 3          ← 新元素 3，父节点是 arr[(5-1)/2]=arr[2]=8

步骤 2：上浮 -- 元素3(idx=5) 和 父节点8(idx=2) 比较
  3 < 8 → 交换
  交换后：[2, 5, 3, 9, 7, 8]
  树形：  2
        /   \
       5     3         ← 3 上浮到了 idx=2
      / \   /
     9   7 8

步骤 3：继续上浮 -- 元素3(idx=2) 和 父节点2(idx=0) 比较
  3 > 2 → 停止！
  （因为父节点更小，堆有序性质已满足）

最终：[2, 5, 3, 9, 7, 8]
```

#### 4.1.3 pop 操作（取最小值 + 下沉）

```
pop() 的完整过程（对上一步的结果操作）：

起始：[2, 5, 3, 9, 7, 8]
      2
    /   \
   5     3
  / \   /
 9   7 8

步骤 1：取出堆顶 2，末尾 8 移到堆顶
  数组：[8, 5, 3, 9, 7]    （size 从 6 变成 5）
  树形：  8              ← 8 在堆顶，但 8 > 3 > 5，需要下沉
        /   \
       5     3
      / \
     9   7

步骤 2：下沉 -- 元素8(idx=0)，左孩子5(idx=1)，右孩子3(idx=2)
  左孩子5 < 父8，右孩子3 < 父8 → 选最小的孩子 3 交换
  交换后：[3, 5, 8, 9, 7]
  树形：  3
        /   \
       5     8
      / \
     9   7

步骤 3：继续下沉 -- 元素8(idx=2)，左孩子? → 2*2+1=5 ≥ size=5，无孩子 → 停止

最小值 2 被弹出，新的堆顶是 3。
```

#### 4.1.4 逐步追踪：push 序列 5, 2, 8, 1, 3

```
push(5):
  数组：[5]
  树：  5         （只有一个节点，自然满足堆序）

push(2):
  数组：[5, 2]
  树：  5
      /
     2            （2 在 idx=1，父节点 idx=0，arr[0]=5 > 2 → 交换）
  交换后：[2, 5]
  树：  2
      /
     5            ✓

push(8):
  数组：[2, 5, 8]
  树：  2
      / \
     5   8        （8 的父节点 arr[(2-1)/2]=arr[0]=2 < 8 → 不交换 ✓）

push(1):
  数组：[2, 5, 8, 1]
  树：  2
      / \
     5   8
    /
   1              （1 的父节点 arr[(3-1)/2]=arr[1]=5 > 1 → 交换）
  交换后：[2, 1, 8, 5]
  树：  2
      / \
     1   8
    /
   5              （1 新 idx=1，父节点 arr[(1-1)/2]=arr[0]=2 > 1 → 再交换）
  交换后：[1, 2, 8, 5]
  树：  1
      / \
     2   8
    /
   5              ✓

push(3):
  数组：[1, 2, 8, 5, 3]
  树：  1
      / \
     2   8
    / \
   5   3          （3 的父节点 arr[(4-1)/2]=arr[1]=2 < 3 → 不交换 ✓）

最终堆：[1, 2, 8, 5, 3]
```

pop 出来顺序应该是：1, 2, 3, 5, 8（从小到大）✓

#### 4.1.5 在 Dijkstra 中的应用：惰性删除优先队列

Dijkstra 算法需要一个"优先队列"（每次取出当前距离最小的节点）。

标准做法是用最小堆做优先队列：堆顶 = 距离最小的节点。

```
正常做法：找到更短路径 → 在堆中找到旧记录 → decrease-key 更新 → O(n) 太慢
我们的做法：直接 push 新记录 (7,node5)，旧记录 (10,node5) 留在堆里
          每次 pop 时在外层检查：堆顶的距离 > 已知最短距离？→ 跳过
```

详见第六章。

### 4.2 代码

追加到 `Containers.h`：

```cpp
// ============================================================================
// MinHeap<T,Cmp> -- 二叉最小堆
// ============================================================================

// ===== 默认比较器 =====
template<typename T>
struct HeapLess {
    bool operator()(const T& a, const T& b) const {
        return a < b;
    }
};
// ↑ HeapLess 是什么？
//   它看起来像一个函数（有 operator()），但它是一个 struct。
//   这种"可以当函数用的对象"叫做"函数对象"或"Functor（仿函数）"。
//
//   为什么要用 Functor 而不用函数指针？
//   - Functor 可以被编译器内联优化（函数指针通常不能）
//   - Functor 可以有成员变量（虽然这里没有用）
//   - 模板参数传类型比传函数指针更方便

// ===== MinHeap =====
template<typename T, typename Cmp = HeapLess<T>>
// ↑ 第二个模板参数 Cmp 有默认值 HeapLess<T>。
//   默认情况下是最小堆（cmp(a,b) = a < b）。
//   如果想让堆变成最大堆，传入 HeapGreater：
//     template<typename T>
//     struct HeapGreater { bool operator()(const T& a, const T& b) const { return a > b; } };
//     MinHeap<int, HeapGreater<int>> maxHeap;
//
//   知识点：默认模板参数
//   就像函数的默认参数 void foo(int x = 42);
//   模板也可以有默认参数 <> 里的第二个就用了默认值。
class MinHeap {
    DynArray<T> data;   // 底层用动态数组存完全二叉树
    Cmp         cmp;    // 比较器对象

    // ===== 上浮 =====
    // 新元素放在末尾，然后和父节点比较，小则交换，直到堆序满足
    void shiftUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            // ↑ 整数除法自动向下取整。C++ 中两个 int 相除结果仍是 int。
            //   例如：3/2 = 1（不是 1.5）

            if (cmp(data[i], data[parent])) {
                // ↑ cmp(a,b) 返回 true 当"a 应该在 b 上面"。
                //   对最小堆：cmp(a,b) = a<b → 当 data[i] < data[parent] 时交换。
                //   换成人话：孩子比父亲小？→ 交换（让孩子上浮）。

                T temp = data[i];
                data[i] = data[parent];
                data[parent] = temp;
                // ↑ 手动交换两个元素。为什么不用 std::swap？
                //   因为我们要避免任何 STL 依赖。手动三行就搞定了。

                i = parent;
                // ↑ 继续看新的位置是否需要再上浮。
            } else break;
            // ↑ 堆序已满足，停止上浮。
        }
    }

    // ===== 下沉 =====
    // 堆顶元素被替换后，和左右孩子中较小的比，大则交换
    void shiftDown(int i) {
        int len = data.size();
        while (true) {
            int best = i;
            int left = 2*i + 1;
            int right = 2*i + 2;

            // 左孩子在范围内且比当前 best 更小 → best 换成 left
            if (left < len && cmp(data[left], data[best])) best = left;

            // 右孩子在范围内且比当前 best 更小 → best 换成 right
            if (right < len && cmp(data[right], data[best])) best = right;

            // best 没有变化 → 当前节点已经比所有孩子都小 → 停止
            if (best == i) break;

            // 交换
            T temp = data[i];
            data[i] = data[best];
            data[best] = temp;

            i = best;
            // ↑ 继续在 best 位置检查是否需要再下沉。
        }
    }

public:
    bool empty() const {
        return data.empty();
    }

    const T& top() const {
        return data[0];
    }
    // ↑ 注意：const T& 返回值 + const 成员函数。
    //   "const T&"：返回常引用，防止调用者修改堆顶数据。
    //   "const"（末尾）：承诺 top() 不修改 MinHeap 自身。
    //
    //   如果堆为空，data[0] 是非法的。调用者需先检查 empty()。

    void push(const T& v) {
        data.push_back(v);              // 放到末尾
        shiftUp(data.size() - 1);       // 上浮到正确位置
    }

    void pop() {
        if (data.empty()) return;
        data[0] = data.back();          // 末尾元素放堆顶
        data.pop_back();                // 删除末尾（现在它已经是复制品了）
        if (!data.empty()) shiftDown(0); // 堆顶下沉
    }
};
```

### 4.3 Queue -- 队列

```
FIFO（First In First Out）：队尾进、队头出

排队打饭：
  新人从队尾入 → [A][B][C] ← 队尾
  从队头出 → [A] 打完饭离开
  结果：[B][C]

实现方案：
  方案1（选的这个）：DynArray 存数据 + head 下标标记队头
    优点：实现极简（10 行）
    缺点：head 只增不减，前面空间不回收

  为什么"不回收"不是问题？
    拓扑排序中队列的生命周期很短 -- 用完就销毁。
    如果长期使用（比如做任务调度），需要改用循环队列。
```

```cpp
// ============================================================================
// Queue<T> -- 队列
// ============================================================================
template<typename T>
class Queue {
    DynArray<T> data;      // 存储数据
    int head = 0;          // 队头下标

public:
    bool empty() const {
        return head >= data.size();
        // ↑ 队头下标 >= 元素总数 → 说明所有人都出队了 → 空
    }

    T& front() {
        return data[head];
        // ↑ 返回队头元素。调用前需确保非空。
    }

    void push(const T& v) {
        data.push_back(v);
        // ↑ 从队尾进入。
    }

    void pop() {
        if (!empty()) ++head;
        // ↑ 队头下标后移。不真的删除元素（data[0..head-1] 的内容还在内存里）。
        //   这就是前面说的"不回收"。
    }

    void clear() {
        data.clear();
        head = 0;
        // ↑ 复用 DynArray 的 clear（length=0），head 也归零。
    }
};
```

### 4.4 测试

```cpp
// ========================
// 测试 MinHeap
// ========================
MinHeap<int> h;
h.push(5); h.push(2); h.push(8); h.push(1); h.push(3);
cout << "MinHeap pop 顺序: ";
while (!h.empty()) { cout << h.top() << " "; h.pop(); }
cout << "\n";
// 应输出：1 2 3 5 8

// ========================
// 测试 Queue
// ========================
Queue<int> q;
q.push(10); q.push(20); q.push(30);
cout << "Queue front: " << q.front() << "\n";   // 10
q.pop();
cout << "Queue front after pop: " << q.front() << "\n";   // 20
q.pop(); q.pop();
cout << "Queue empty: " << q.empty() << "\n";   // 1 (true)
```

容 器层完成！现在我们有：
- `DynArray<T>` -- 动态数组（~70 行）
- `HashMap<V>` -- 哈希表（~100 行）
- `MinHeap<T>` -- 最小堆（~45 行）
- `Queue<T>` -- 队列（~15 行）
- **全部零外部依赖**，一个标准库头文件都不需要

---

## 五、Graph -- 有向图邻接表

### 5.1 数据结构设计

#### 5.1.1 邻接矩阵 vs 邻接表

| | 邻接矩阵 | 邻接表（我们选的） |
|---|---|---|
| 结构 | `double[V][V]` | `HashMap<int, DynArray<Edge>>` |
| 空间 | O(V²) = 25² = 625 | O(V+E) = 25 + 124 |
| 遍历出边 | O(V) 扫整行 | O(degree) 只看有边的 |
| 查边 | O(1) `mat[i][j]` | O(degree) 遍历出边 |
| 插入节点 | 扩容整矩阵 O(V²) | HashMap insert O(1) |
| 删除节点 | 行列前移 O(V²) | 遍历删入边 O(E) |

我们的图：25 节点，124 条有向边，密度 124/(25×24)≈20% → 稀疏图 → 邻接表。

#### 5.1.2 具体内存消耗分析

邻接矩阵：每个元素是 double（8 字节），25×25 = 625 个 double = 5000 字节 ≈ 5KB。
邻接表：25 个 HashMap entry + 124 个 Edge 对象。Edge 有 4 个域（from, to, time, cost）+ DynArray 开销 ≈ 76 字节/边。节点本身约 15KB。总 < 20KB。

25 个节点的场景下两者差距不大，但选邻接表的原因：
1. 练习 HashMap 的实现（课程要求）
2. 算法更高效（遍历出边只扫描有连接的点）
3. 扩展性好（如果将来有 1000 个节点，邻接表仍然 O(V+E)）

#### 5.1.3 数据结构图

```
Graph 对象
├── nodes: HashMap<Node>
│   ┌────────────────────────────────┐
│   │ key=1 → Node(1, "北京", "北京市朝阳区", 116.4, 39.9) │
│   │ key=2 → Node(2, "天津", "天津市滨海新区", 117.2, 39.1) │
│   │ key=3 → Node(3, "上海", "上海市浦东新区", 121.5, 31.2) │
│   │ ...                                                    │
│   └────────────────────────────────┘
│
└── adj: HashMap<DynArray<Edge>>
    ┌────────────────────────────────┐
    │ key=1 → [Edge(1→2, 1.5h, 60元), Edge(1→3, 5h, 300元)] │
    │ key=2 → [Edge(2→1, 1.5h, 60元), Edge(2→3, 4h, 250元)] │
    │ key=3 → [Edge(3→4, 2h, 120元)]                         │
    │ ...                                                    │
    └────────────────────────────────┘
```

### 5.2 Graph.h

创建 `Graph.h`：

```cpp
#pragma once
#include "Containers.h"
#include <string>

// ===== 节点：编号 + 名称 + 地址 + 经纬度（画图用）=====
struct Node {
    int id = 0;
    std::string name;     // 城市名，如 "北京总仓"
    std::string address;  // 地址，如 "北京市朝阳区"
    double lon = 0, lat = 0;  // 经纬度

    Node() = default;
    // ↑ 默认构造函数。= default 让编译器自动生成。

    Node(int nodeId, const std::string& nodeName, const std::string& nodeAddr,
         double nodeLon = 0, double nodeLat = 0)
        : id(nodeId), name(nodeName), address(nodeAddr), lon(nodeLon), lat(nodeLat) {}
    // ↑ 带参数的构造函数。冒号后面是"成员初始化列表"。
    //   知识点：成员初始化列表 vs 函数体内赋值
    //     : id(nodeId)           ← 初始化列表：直接构造成员，一次完成
    //     { id = nodeId; }       ← 函数体内赋值：先默认构造，再赋值，多一步
    //   对于 string 这种复杂类型，初始化列表明显更高效。
    //   lon/lat 有默认值 =0，调用者可以不传（纯算法使用时不需要坐标）。
};

// ===== 有向边（双权）=====
struct Edge {
    int from = 0, to = 0;
    double time = 0;   // 运输耗时（小时）
    double cost = 0;   // 运输费用（元）

    Edge() = default;

    Edge(int edgeFrom, int edgeTo, double edgeTime, double edgeCost)
        : from(edgeFrom), to(edgeTo), time(edgeTime), cost(edgeCost) {}
};

class Graph {
public:
    // ---- 节点 CRUD ----
    bool  addNode(const Node& node);
    bool  deleteNode(int id);
    bool  updateNode(int id, const Node& node);
    Node*       findNode(int id);
    const Node* findNode(int id) const;

    // ---- 边 CRUD ----
    bool  addEdge(const Edge& edge);
    bool  deleteEdge(int from, int to);

    // ---- 查询 ----
    const DynArray<Edge>& getNeighbors(int id) const;
    DynArray<int> getAllNodeIds() const;
    bool hasNode(int id) const;
    int  nodeCount() const;
    int  edgeCount() const;
    void clear();

private:
    HashMap<Node>           nodes;   // 节点表：编号 → 节点
    HashMap<DynArray<Edge>> adj;     // 邻接表：编号 → 出边列表

    static const DynArray<Edge> emptyAdj;
    // ↑ static 类成员：所有 Graph 实例共享同一个 emptyAdj。
    //   当 getNeighbors 找不到某节点时，返回这个空数组的引用，避免返回临时对象。
    //
    //   知识点：static 成员
    //   普通成员：每个对象一份（如每个 Graph 有自己的 nodes）
    //   静态成员：整个类一份（所有 Graph 共享 emptyAdj）
    //   需要在 .cpp 文件中定义：const DynArray<Edge> Graph::emptyAdj;
};
```

### 5.3 Graph.cpp

创建 `Graph.cpp`：

```cpp
#include "Graph.h"
#include <iostream>

const DynArray<Edge> Graph::emptyAdj;
// ↑ 静态成员的定义。注意前面不能加 static（语法规定）。

// ================================
// 节点 CRUD
// ================================

bool Graph::addNode(const Node& node) {
    if (node.name.empty()) return false;
    // ↑ 名称不能为空。name 是 std::string，.empty() 检查 string 长度是否为 0。

    if (nodes.contains(node.id)) return false;
    // ↑ 编号不能重复。ID 是每个节点的唯一标识。

    nodes.set(node.id, node);
    adj[node.id];
    // ↑ adj[node.id] 利用 HashMap 的 operator[] 自动创建空邻接表。
    //   DynArray<Edge> 的默认值是空数组（size=0, capacity=0）。

    return true;
}

bool Graph::deleteNode(int id) {
    if (!nodes.contains(id)) return false;

    nodes.erase(id);
    adj.erase(id);
    // ↑ 删自己的邻接表

    // 删除所有其他节点指向此节点的入边 O(E)
    adj.forEach([id](int, DynArray<Edge>& edgeList) {
        edgeList.remove_all([id](const Edge& edge) {
            return edge.to == id;
        });
    });
    // ↑ 遍历所有邻接表，从每个节点的出边列表中删除目标是此节点的边。
    //   复杂度 O(E)：扫描所有边一次。
    //
    //   这是整个 Graph 中最复杂的操作。为什么？
    //   因为邻接表只存"出边"（from → to），不存"入边"（谁指向我）。
    //   要删除入边，只能遍历所有节点看"谁指向了我"。

    return true;
}

// updateNode 和 findNode 的实现：
bool Graph::updateNode(int id, const Node& node) {
    Node* n = nodes.find(id);
    if (!n) return false;
    *n = node;          // 整体替换
    n->id = id;         // 但保持原 ID 不变
    return true;
}

Node* Graph::findNode(int id) {
    return nodes.find(id);
}

const Node* Graph::findNode(int id) const {
    return nodes.find(id);
}

// ================================
// 边 CRUD
// ================================

bool Graph::addEdge(const Edge& edge) {
    // 防御性检查
    if (!nodes.contains(edge.from) || !nodes.contains(edge.to))
        return false;
    // ↑ 起点和终点必须已存在

    if (edge.from == edge.to) return false;
    // ↑ 不允许自环（节点指向自己）。快递不可能自己送给自己。

    if (edge.time < 0 || edge.cost < 0) return false;
    // ↑ 权重非负（Dijkstra 的前提条件）

    DynArray<Edge>& edgeList = adj[edge.from];
    for (int i = 0; i < edgeList.size(); ++i)
        if (edgeList[i].to == edge.to) return false;
    // ↑ 查重：同一对 from→to 只能有一条边。

    edgeList.push_back(edge);
    return true;
}

bool Graph::deleteEdge(int from, int to) {
    DynArray<Edge>* found = adj.find(from);
    if (!found) return false;
    return found->remove_first([to](const Edge& edge) {
        return edge.to == to;
    });
    // ↑ 利用 DynArray 的 remove_first 删除第一条匹配的边。
    //   Lambda [to] 捕获了目标节点编号 to。
}

// ================================
// 查询
// ================================

const DynArray<Edge>& Graph::getNeighbors(int id) const {
    const DynArray<Edge>* p = adj.find(id);
    return p ? *p : emptyAdj;
    // ↑ 三元运算符：如果 p 为非空 → 返回 *p（解引用）。
    //              如果 p 为空   → 返回 emptyAdj（空列表，安全）。
    //   为什么不直接 adj[id]？因为 adj 是 const 的，operator[] 会尝试插入。
}

DynArray<int> Graph::getAllNodeIds() const {
    DynArray<int> ids;
    nodes.forEach([&](int id, const Node&) {
        ids.push_back(id);
    });
    // ↑ [&] 捕获：以引用方式捕获外部变量 ids。
    //   如果用了 [=]（值捕获），ids 是拷贝，push_back 不会影响外面的 ids → bug。

    return ids;
}

bool Graph::hasNode(int id) const {
    return nodes.contains(id);
}

int Graph::nodeCount() const {
    return nodes.size();
}

int Graph::edgeCount() const {
    int total = 0;
    adj.forEach([&](int, const DynArray<Edge>& edgeList) {
        total += edgeList.size();
    });
    // ↑ 遍历所有邻接表，累加每条邻接表的长度。
    return total;
}

void Graph::clear() {
    nodes.clear();
    adj.clear();
}
```

---

## 六、Dijkstra 最短路径 + 拓扑排序

### 6.1 Dijkstra 算法原理

#### 6.1.1 问题

给定一个有向图，每条边有一个非负权重，求从起点到所有节点的最短路径。

#### 6.1.2 直观理解（贪心法）

```
1. 一开始，只确定 start 到自己距离 = 0，其他节点距离 = ∞
2. 每次从"未确定"的节点中选出距离最小的 u
3. 用 u 去"松弛"它的所有邻居 v：
   如果 dist[u] + weight(u→v) < dist[v]，更新 dist[v]
4. 标记 u 为"已确定"，回到步骤 2
```

**为什么每次选最小值就能保证正确？**

因为所有边权 ≥ 0。假设 u 是当前 pop 出的距离最小节点。如果存在一条更短路径从 start 到 u，它必然经过某个"未确定"节点 x。但 x 未确定意味着 dist[x] ≥ dist[u]（否则应该先 pop x）。加上 x→u 的路径 ≥ 0，更短路径不可能存在。

#### 6.1.3 朴素版 vs 堆优化版

| | 朴素 O(V²) | 堆优化 O((V+E)logV) |
|---|---|---|
| 选最短节点 | 每轮扫所有 V 个 | 堆顶 O(logV) |
| 松弛 | 扫所有 V 个（邻接矩阵） | 遍历出边 O(degree) |
| 优先队列 | 不需要 | MinHeap |
| 25 节点时 | 625 次比较 | 约 25 次 push/pop × log(25) |

### 6.2 完整步骤追踪：4 节点示例

用这个网：
```
北京(id=1)──5h,300元──→上海(id=2)──6h,350元──→广州(id=3)
    │                                          ↑
    └───────────10h,600元─────────────────────────┘
                          北京──→武汉(id=4, 直达 ∞, 不通)
```

**求从北京(1)出发的最短耗时（以 time 为权重）**。

```
===== 初始化 =====
dist:  { 1:0,   2:INF, 3:INF,  4:INF  }
cost:  { 1:0,   2:INF, 3:INF,  4:INF  }
prev:  { 1:-1,  2:-1,  3:-1,   4:-1   }
heap:  [(0,1)]

===== 迭代 1 =====
Pop 堆顶: (d=0, node=1)
惰性删除检查: d(0) == dist[1](0) ✓ → 不跳过

松弛 1→2 (5h,300元):
  newDist = 0 + 5 = 5
  oldDist = dist[2] = INF
  5 < INF → 更新!
  dist[2]=5, cost[2]=0+300=300, prev[2]=1
  heap.push((5,2))

松弛 1→3 (10h,600元):
  newDist = 0 + 10 = 10
  oldDist = INF
  10 < INF → 更新!
  dist[3]=10, cost[3]=0+600=600, prev[3]=1
  heap.push((10,3))

状态：
  dist:  { 1:0, 2:5, 3:10, 4:INF }
  prev:  { 1:-1, 2:1, 3:1, 4:-1 }
  heap:  [(5,2), (10,3)]

===== 迭代 2 =====
Pop 堆顶: (d=5, node=2)
惰性删除检查: d(5) == dist[2](5) ✓

松弛 2→3 (6h,350元):
  newDist = 5 + 6 = 11
  oldDist = dist[3] = 10
  11 > 10 → 不更新（现有路径北京→3直达10h更短）

状态：
  dist:  { 1:0, 2:5, 3:10, 4:INF }
  heap:  [(10,3)]

===== 迭代 3 =====
Pop 堆顶: (d=10, node=3)
惰性删除检查: d(10) == dist[3](10) ✓

松弛 3 的出边: 没有 → 跳过

状态：
  dist:  { 1:0, 2:5, 3:10, 4:INF }
  heap:  []

===== 堆空，结束 =====

最终结果：
  节点1(北京): dist=0,  path=[1]
  节点2(上海): dist=5,  path=[1,2]    ← 直达最快
  节点3(广州): dist=10, path=[1,3]    ← 直达比经上海(11h)更快
  节点4(武汉): dist=INF, path=[]     ← 不可达（没有边连接）

★ 注意：如果 edge(1→2) 的 time 是 3h 而不是 5h，
         那么 dist[2]=3, dist[3]=3+6=9（经上海更快）！
         这就是"优化"的意义。
```

### 6.3 惰性删除详解

```
场景：堆中已有 (dist=10, node=5)。
      现在通过 node=3 的松弛发现更短路径 dist[5]=7。

"正常"做法（decrease-key）：
  需要在堆中定位 node=5 → 把 10 改成 7 → 上浮 → O(n) 找位置，太慢

"惰性删除"做法：
  直接 push 新记录 (7, node=5)。旧记录 (10, node=5) 留在堆里。

  堆现在有：[(7,node5), (8,node2), (10,node5), ...]
  pop (7,node5): d=7 == dist[5]=7 → 处理 ✓
  pop (8,node2): d=8 == dist[2]=8 → 处理 ✓
  pop (10,node5): d=10 > dist[5]=7 → ★ 旧记录！跳过 ★

代价分析：
  - 每个节点可能被多个前驱松弛 → 每条边最多产生 1 个新记录
  - 堆大小从 O(V) 变成 O(E)
  - 空间增加，但总复杂度仍为 O((V+E)logE) ≈ O((V+E)logV)
  - 对于 25 节点 124 边：堆最多 124 个元素，log(124) ≈ 7，可以接受
```

### 6.4 双权处理

| | shortestTimeFrom | cheapestPath |
|---|---|---|
| 主优化目标 | time（耗时） | cost（费用） |
| 辅助记录 | cost（顺带算） | time（顺带算） |
| 比较语句 | `newDist = *curDist + e.time` | `newDist = *curDist + e.cost` |

两条 Dijkstra 结构完全相同，只有被比较的权重不同。

### 6.5 Dijkstra.h

创建 `Dijkstra.h`：

```cpp
#pragma once
#include "Graph.h"

// ===== 路径结果 =====
struct PathResult {
    DynArray<int> path;      // 节点序列 [start, ..., target]
    double totalTime = 0;    // 总耗时（小时）
    double totalCost = 0;    // 总费用（元）
    bool   reachable = false; // 是否可达
};

// ===== 拓扑排序结果 =====
struct TopoResult {
    bool          hasCycle = false;
    DynArray<int> order;        // 拓扑序列
    DynArray<int> cycleNodes;   // 环中节点（如果有环）
};

class Dijkstra {
public:
    // 单源最短耗时：返回 HashMap<节点编号 → 路径结果>
    static HashMap<PathResult> shortestTimeFrom(const Graph& g, int start);

    // 两点最低费用
    static PathResult cheapestPath(const Graph& g, int start, int target);
};

class TopoSort {
public:
    // Kahn 拓扑排序（对 ids 子集）
    static TopoResult sort(const Graph& g, const DynArray<int>& ids);
};
```

### 6.6 Dijkstra.cpp

创建 `Dijkstra.cpp`：

```cpp
#include "Dijkstra.h"
#include <iostream>

// INF 用一个极大的 double 值代替"无穷远"
static const double INF = 1e18;
// ↑ 1e18 = 1000000000000000000.0（10 的 18 次方）。
//   为什么不用 double 的 INFINITY？
//     INFINITY 是 IEEE 754 的特殊浮点值，运算规则特殊：
//       INFINITY + 5 = INFINITY（不是正常的算术结果）
//       这会导致 newDist = *curDist + e.time 永远 = INF 如果 curDist 是 INF。
//       而我们的惰性删除逻辑依赖于正常的比较。
//   1e18 足够大（地球上没有快递需要 1e18 小时），也服从正常浮点运算。

// 堆中元素：距离 + 节点编号
struct HeapItem {
    double d;
    int    n;
};

// 堆的比较器：按距离 d 升序（最小堆）
struct HeapItemCmp {
    bool operator()(const HeapItem& a, const HeapItem& b) const {
        return a.d < b.d;
    }
};

// ================================================================
// 路径回溯：从 dst 沿 prev[] 倒推回 start，再翻转
// ================================================================
static DynArray<int> rebuildPath(const HashMap<int>& prev, int dst) {
    DynArray<int> p;

    // 第一步：倒着走
    for (int cur = dst; cur != -1; ) {
        p.push_back(cur);
        const int* pp = prev.find(cur);
        cur = pp ? *pp : -1;
    }
    // ↑ 结果：p = [3, 1]（从广州倒走到北京）

    // 第二步：翻转数组，得到正向路径
    for (int l = 0, r = p.size() - 1; l < r; ++l, --r) {
        int t = p[l];
        p[l] = p[r];
        p[r] = t;
    }
    // ↑ 双指针翻转：
    //   l=0,r=1 → [1, 3]
    //   l=1,r=0 → 停止

    return p;  // [1, 3]
}

// ================================================================
// 初始化 dist/cost/prev 数组
// ================================================================
static void initMaps(const DynArray<int>& ids,
                     HashMap<double>& distA,
                     HashMap<double>& distB,
                     HashMap<int>&    prev) {
    for (int i = 0; i < ids.size(); ++i) {
        distA[ids[i]] = INF;    // 主权重距离 = 无穷
        distB[ids[i]] = INF;    // 辅权重距离 = 无穷
        prev[ids[i]] = -1;      // 前驱 = -1（无前驱）
    }
}

// ================================================================
// 单源最短耗时（以 time 为主权重）
// ================================================================
HashMap<PathResult> Dijkstra::shortestTimeFrom(const Graph& g, int start) {
    if (!g.hasNode(start)) return {};
    // ↑ 起点不存在 → 返回空结果

    DynArray<int> ids = g.getAllNodeIds();
    HashMap<double> dist;   // 最短耗时（主权重）
    HashMap<double> cost;   // 对应费用（辅权重，顺带算）
    HashMap<int>    prev;   // 前驱节点编号
    initMaps(ids, dist, cost, prev);

    dist[start] = cost[start] = 0;
    // ↑ 起点到自己距离=0

    MinHeap<HeapItem, HeapItemCmp> heap;
    heap.push({ 0.0, start });

    // ===== 主循环 =====
    while (!heap.empty()) {
        HeapItem cur = heap.top();
        heap.pop();

        // ★ 惰性删除检查 ★
        double* curDist = dist.find(cur.n);
        if (!curDist || cur.d > *curDist) continue;
        // ↑ 如果堆顶记录的 dist > 当前已知的最短 dist → 这是旧记录，跳过！
        //   这正是惰性删除的核心：不 decrease-key，pop 时判断过期。

        // 松弛所有出边
        for (const Edge& e : g.getNeighbors(cur.n)) {
            double newDist = *curDist + e.time;
            // ↑ ★ 以 time 为主权重 ★ — 这就是和 cheapestPath 的唯一关键区别

            double* oldDist = dist.find(e.to);
            if (oldDist && newDist < *oldDist) {
                // ↑ 找到更短路径 → 更新所有信息
                *oldDist = newDist;

                // 顺带更新费用
                double* curCost = cost.find(cur.n);
                double* dstCost = cost.find(e.to);
                *dstCost = *curCost + e.cost;

                // 记录前驱
                prev[e.to] = cur.n;

                // ★ 只 push 新记录，不修改旧记录（惰性删除）★
                heap.push({ newDist, e.to });
            }
        }
    }

    // ===== 构建结果 =====
    HashMap<PathResult> result;
    for (int i = 0; i < ids.size(); ++i) {
        PathResult pathRes;
        double* oldDist = dist.find(ids[i]);
        if ((pathRes.reachable = (oldDist && *oldDist != INF))) {
            // ↑ 条件赋值：先把 (oldDist != INF) 赋给 reachable，
            //   如果 reachable 为 true 再填充具体数据。
            pathRes.totalTime = *oldDist;
            pathRes.totalCost = *cost.find(ids[i]);
            pathRes.path = rebuildPath(prev, ids[i]);
        }
        result.set(ids[i], pathRes);
    }
    return result;
}

// ================================================================
// 两点最低费用（结构同最短耗时，以 cost 为主权重）
// ================================================================
PathResult Dijkstra::cheapestPath(const Graph& g, int start, int target) {
    if (!g.hasNode(start) || !g.hasNode(target)) return {};

    DynArray<int> ids = g.getAllNodeIds();
    HashMap<double> dist;   // 最小费用（主权重）
    HashMap<double> time;   // 对应耗时（辅权重）
    HashMap<int>    prev;
    initMaps(ids, dist, time, prev);

    dist[start] = time[start] = 0;

    MinHeap<HeapItem, HeapItemCmp> heap;
    heap.push({ 0.0, start });

    while (!heap.empty()) {
        HeapItem cur = heap.top(); heap.pop();

        double* curDist = dist.find(cur.n);
        if (!curDist || cur.d > *curDist) continue;  // 惰性删除

        for (const Edge& e : g.getNeighbors(cur.n)) {
            double newDist = *curDist + e.cost;
            // ↑ ★ 以 cost 为主权重 ★ — 唯一不同之处

            double* oldDist = dist.find(e.to);
            if (oldDist && newDist < *oldDist) {
                *oldDist = newDist;
                *time.find(e.to) = *time.find(cur.n) + e.time;  // 顺带算耗时
                prev[e.to] = cur.n;
                heap.push({ newDist, e.to });
            }
        }
    }

    // 构建返回结果
    PathResult result;
    double* d = dist.find(target);
    if ((result.reachable = (d && *d != INF))) {
        result.totalCost = *d;
        result.totalTime = *time.find(target);
        result.path = rebuildPath(prev, target);
    }
    return result;
}

// ================================================================
// Kahn 拓扑排序（子集版本）
// ================================================================
TopoResult TopoSort::sort(const Graph& g, const DynArray<int>& ids) {
    TopoResult result;
    if (ids.empty()) return result;

    // 步骤 1：建立子集查找表（HashMap O(1) 检查一个节点是否在子集中）
    HashMap<bool> inSubSet;
    for (int i = 0; i < ids.size(); ++i)
        inSubSet.set(ids[i], true);
    // ↑ 为什么不直接用 ids.contains()？
    //   因为 DynArray 没有 contains，我们用 HashMap 做快速查找。

    // 步骤 2：统计子集内各节点入度
    HashMap<int> ruDu;
    for (int i = 0; i < ids.size(); ++i)
        ruDu[ids[i]] = 0;  // 初始入度 = 0

    for (int i = 0; i < ids.size(); ++i) {
        for (const Edge& e : g.getNeighbors(ids[i])) {
            if (inSubSet.contains(e.to)) {
                ++ruDu[e.to];
                // ↑ 只有目标在子集内才计算入度
            }
        }
    }

    // 步骤 3：入度 = 0 的节点入队
    Queue<int> queue;
    for (int i = 0; i < ids.size(); ++i)
        if (ruDu[ids[i]] == 0)
            queue.push(ids[i]);

    // 步骤 4：BFS 出队
    while (!queue.empty()) {
        int u = queue.front();
        queue.pop();
        result.order.push_back(u);

        for (const Edge& e : g.getNeighbors(u)) {
            if (inSubSet.contains(e.to)) {
                --ruDu[e.to];
                // ↑ 模仿"删除节点 u"
                if (ruDu[e.to] == 0)
                    queue.push(e.to);
            }
        }
    }

    // 步骤 5：判环
    if (result.order.size() < ids.size()) {
        result.hasCycle = true;

        // 收集环中的节点
        HashMap<bool> sorted;
        for (int i = 0; i < result.order.size(); ++i)
            sorted.set(result.order[i], true);

        for (int i = 0; i < ids.size(); ++i)
            if (!sorted.contains(ids[i]))
                result.cycleNodes.push_back(ids[i]);
        // ↑ 入度 > 0 且未出队 → 在环中
    }
    return result;
}
```

### 6.7 测试

```cpp
void testDijkstra() {
    Graph g;

    // 建图
    g.addNode(Node(1, "北京", "北京市朝阳区", 116.4, 39.9));
    g.addNode(Node(2, "上海", "上海市浦东新区", 121.5, 31.2));
    g.addNode(Node(3, "广州", "广东省广州市", 113.3, 23.1));
    g.addNode(Node(4, "武汉", "湖北省武汉市", 114.3, 30.6));

    g.addEdge(Edge(1, 2, 5.0, 300.0));
    g.addEdge(Edge(2, 3, 6.0, 350.0));
    g.addEdge(Edge(1, 3, 10.0, 600.0));
    // 注意：武汉没有边连接 → 应该是 unreachable

    // 测试 1：最短耗时（北京出发）
    cout << "========== 单源最短耗时 ==========\n";
    auto results = Dijkstra::shortestTimeFrom(g, 1);

    PathResult* pathRes = results.find(3);
    cout << "北京→广州 最短耗时: " << pathRes->totalTime << "h\n";
    cout << "北京→广州 对应费用: " << pathRes->totalCost << "元\n";
    cout << "路径: ";
    for (int i = 0; i < pathRes->path.size(); ++i)
        cout << pathRes->path[i] << " ";
    cout << "\n";
    // 预期：10h, 600元, 路径 [1, 3]

    // 测试 2：最低费用（北京→广州）
    cout << "========== 两点最低费用 ==========\n";
    PathResult r = Dijkstra::cheapestPath(g, 1, 3);
    cout << "北京→广州 最低费用: " << r.totalCost << "元\n";
    cout << "北京→广州 对应耗时: " << r.totalTime << "h\n";
    cout << "路径: ";
    for (int i = 0; i < r.path.size(); ++i)
        cout << r.path[i] << " ";
    cout << "\n";
    // 预期：600元, 10h, 路径 [1, 3]
    // 注意：费用最低也是直达，因为 600 < 300+350=650

    // 测试 3：不可达
    PathResult* r4 = results.find(4);
    cout << "北京→武汉 可达: " << r4->reachable << "\n";
    // 预期：0 (false)

    // 测试 4：拓扑排序
    cout << "========== 拓扑排序 ==========\n";
    DynArray<int> allIds = g.getAllNodeIds();
    TopoResult topo = TopoSort::sort(g, allIds);
    cout << "有环: " << topo.hasCycle << "\n";
    cout << "序列: ";
    for (int i = 0; i < topo.order.size(); ++i)
        cout << topo.order[i] << " ";
    cout << "\n";
    // 预期：无环，序列如 1 2 3 4（武汉是孤立节点）
}
```

算法层完成。

---

## 七、OrderManager 订单管理 + 文件读写

### 7.1 数据结构

```
Order:
┌──────────────────┐
│ orderId: int     │  ← 订单号（如 1001）
│ srcNode: int     │  ← 起点城市编号
│ dstNode: int     │  ← 终点城市编号
│ goods: string    │  ← 货物名称（如 "电子产品"）
│ preferTime: bool │  ← true=按耗时, false=按费用
└──────────────────┘

DeliveryPlan:
┌──────────────────────┐
│ order: Order          │  ← 原始订单
│ result: PathResult    │  ← 规划结果（路径、耗时、费用）
└──────────────────────┘
```

### 7.2 文件格式

#### network.txt（路网数据）

```
# NODES
1 北京总仓 北京市朝阳区 116.4 39.9
2 天津仓 天津市滨海新区 117.2 39.1
# EDGES
1 2 1.5 60.00
2 1 1.5 60.00
```

格式详解：
- `# NODES` / `# EDGES` 是分段标记，表明接下来的行属于哪一段
- `#` 开头的行为注释，解析时跳过
- 节点行：`编号 名称 地址 经度 纬度`（空格分隔）
- 边行：`起点编号 终点编号 耗时(h) 费用(元)`（空格分隔）
- 编号 = 数字（1, 2, 3...），不是字符串

#### orders.txt（订单数据）

```
# 订单号 起点 终点 货物 优化目标(0=费用/1=耗时)
1001 1 20 电子产品 1
1002 10 25 服装 0
```

- 订单号：数字
- 起点/终点：城市编号（不是城市名！）
- 货物：中文字符串（不含空格）
- 优化目标：0=按费用最低，1=按耗时最短

### 7.3 核心解析逻辑（逐行详解）

以节点行 `1 北京总仓 北京市朝阳区 116.4 39.9` 为例：

```cpp
// 第一步：读取一行
string line = "1 北京总仓 北京市朝阳区 116.4 39.9";

// 第二步：用 istringstream 拆词
istringstream iss(line);
string word;
DynArray<string> words;
while (iss >> word) {  // >> 自动按空格分割
    words.push_back(word);
}
// 拆词后的 words = ["1", "北京总仓", "北京市朝阳区", "116.4", "39.9"]
```
```
拆词后的 words = ["1", "北京总仓", "北京市朝阳区", "116.4", "39.9"]
  编号 = words[0]
  名称 = words[1]
  纬度 = words[最后-1]     ← 最后两个词
  经度 = words[最后-2]
  地址 = words[2..最后-3] 拼起来  ← 中间的词（可能含空格）
```

**为什么这样设计？**

因为地址可能含空格！比如 `广西壮族自治区 南宁市`。如果按空格固定位置截取，会错误地把"广西壮族自治区"和"南宁市"解析成两个不同的字段。我们的做法是：前两个词（编号、名称）和后两个词（经纬度）位置固定，中间所有的词拼起来就是地址。

**知识点：`std::istringstream`**

```cpp
#include <sstream>

string line = "1 北京总仓 北京市朝阳区";
istringstream iss(line);  // 用 line 初始化一个"字符串流"
string word;
while (iss >> word) {     // >> 操作符：从流中读取到下一个空白字符
    cout << word << "\n";
}
// 输出：
// 1
// 北京总仓
// 北京市朝阳区
```

`istringstream` 就像一个"在字符串里移动的光标"。每次 `>>` 就往前走一段（跳过空白，读到下一个空白）。当光标走到末尾时，`>>` 返回 false，循环结束。

**知识点：`std::stoi` / `std::stod`**

```cpp
string s = "123";
int x = stoi(s);        // string to int → 123
// stoi = String TO Int

string s2 = "3.14";
double d = stod(s2);    // string to double → 3.14
// stod = String TO Double
```

### 7.4 文件 I/O 基础知识

**知识点：`std::ifstream` / `std::ofstream`**

```cpp
#include <fstream>
using namespace std;

// 读取文件
ifstream fin("data/network.txt");
// ↑ "ifstream" = Input File Stream（输入文件流）
//   从文件中读数据到程序

if (!fin.is_open()) {
    cout << "文件打开失败！\n";
    return;
}

string line;
while (getline(fin, line)) {
    // getline 读取一行（包括空格，读到 \n 为止）
    // 如果文件读完了，getline 返回 false
    cout << line << "\n";
}
fin.close();  // 关闭文件（也可以不写，fin 析构时自动关闭）

// 写入文件
ofstream fout("data/output.txt");
// ↑ "ofstream" = Output File Stream（输出文件流）
//   从程序写数据到文件

fout << "Hello World\n";
fout << "第二行\n";
fout.close();
```

### 7.5 OrderManager 核心方法

| 方法 | 功能 |
|------|------|
| `FileManager::loadNetwork()` | 从 txt 读取路网，分段解析 #NODES/#EDGES |
| `FileManager::saveNetwork()` | 保存当前路网到 txt |
| `FileManager::loadOrders()` | 从 txt 读取订单 |
| `FileManager::savePlans()` | 导出配送方案 |
| `OrderManager::planAllOrders()` | 遍历订单，每条调用 Dijkstra |
| `OrderManager::planBatchSequence()` | 收集订单涉及的节点 → 拓扑排序 |

完整代码见 `ExpressRouting/OrderManager.h` 和 `ExpressRouting/OrderManager.cpp`。

---

## 八、Qt GUI 图形界面

> 这是最复杂的部分（~500 行）。建议在 Qt Creator 中边写边调试。
> 核心思路：**先搭骨架，再加交互**。

### 8.1 Qt 基础知识

#### 8.1.1 什么是 Qt？

Qt 是一个跨平台的 C++ GUI 框架。你不用从零写 Windows API 调用来画窗口、按钮，Qt 已经帮你包装好了。

#### 知识点：事件循环 `app.exec()`

```cpp
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    // ↑ QApplication 是 Qt 程序的"总管"。它管理事件循环、命令行参数等。
    //   每个 Qt 程序有且只有一个 QApplication 对象。

    MainWindow win;
    win.show();
    // ↑ 创建并显示主窗口

    return app.exec();
    // ↑ app.exec() 启动"事件循环"（Event Loop）。
    //   程序在这里"阻塞"——不往下走，而是等待用户操作。
    //
    //   事件循环就像餐厅的服务员：
    //     等客人招手（用户点击按钮）→ 响应需求（调用槽函数）→ 继续等
    //   这个循环一直运转，直到用户关闭窗口。
    //
    //   exec() 的返回值会传给 return，作为程序的退出码。
}
```

#### 知识点：信号与槽（Signal & Slot）-- Qt 的核心机制

信号和槽是 Qt 最独特的特性。它解决了"一个组件要通知另一个组件"的问题。

**传统做法（回调函数）**：
```cpp
button->setCallback(myFunction);  // 按钮被点击时调用 myFunction
// 问题：需要手动管理函数指针，类型不安全
```

**Qt 做法（信号槽）**：
```cpp
// connect("谁发信号", "发什么信号", "谁接收", "收到后干什么")
connect(按钮, &QPushButton::clicked,  this, &MainWindow::onButtonClicked);
//       ↑ 谁发            ↑ 发什么       ↑ 谁收           ↑ 干什么
```

用人话解释：
- **信号** = 一个事情发生了。比如"按钮被点击了"。
- **槽** = 收到信号后要执行的函数。比如"点击后计算路径"。
- **connect** = 把信号和槽"连接"起来。就像把电线插进插座。

深入理解（简化的 MOC 解释）：
Qt 有一个预处理器叫 **MOC（Meta-Object Compiler，元对象编译器）**。它会在编译前扫描你的头文件，如果发现 `Q_OBJECT` 宏，就自动生成额外的代码（包括信号发射函数、元信息表等）。

```cpp
// 你写的：
class MyClass : public QObject {
    Q_OBJECT          // ← MOC 会发现这个宏
signals:
    void mySignal(int value);  // ← MOC 生成 mySignal 的实现代码
public slots:
    void mySlot(int value);    // ← MOC 注册为"可被信号调用的槽"
};
```

Lambda 也可以做槽：
```cpp
connect(画布, &GraphWidget::nodeHovered, this, [this](int id) {
    statusBar()->showMessage("悬停节点 " + id);
    // ↑ [this] 捕获：让 lambda 可以访问 MainWindow 的成员函数
});
```

#### 知识点：`Q_OBJECT` 宏

任何使用信号/槽的类，其头文件必须包含 `Q_OBJECT` 宏。如果忘了写：
- 编译会报错 "undefined reference to vtable"
- 或者 connect 静默失败（信号发出来槽收不到）

#### 知识点：父-子所有权

```cpp
QLabel* label = new QLabel("Hello", this);
//                ↑ 在堆上分配     ↑ parent = this（MainWindow）
// Qt 的对象树机制：当 this（MainWindow）被销毁时，
// 所有以 this 为 parent 的子对象会自动被 delete。
// 你不需要手动 delete label！
```

#### 知识点：`protected` 和 `override`

```cpp
protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
```

- `protected`：介于 public 和 private 之间。子类可以访问，外界不能。
  Qt 的基类（QWidget）定义了很多 `protected virtual` 函数，子类通过 `override` 来定制行为。
- `override`：显式声明"我在重写基类的虚函数"。如果函数签名写错了（比如拼错函数名），编译器会报错。这是 C++11 的安全特性。

### 8.2 界面布局

```
┌─────────────────────────────────────────────────────┐
│ 菜单栏: [文件] 导入路网 | 导出路网 | 导入订单 | 导出方案 │
├───────────────┬─────────────────────────────────────┤
│ 左侧面板(260px)│          画布（占满剩余空间）          │
│ 深蓝背景       │                                     │
│               │    · 蓝色圆 = 城市                    │
│ 统计: 25/60/5 │    · 绿色 = 起点  红色 = 终点         │
│               │    · 橙色 = 路径高亮                  │
│ [网点管理]     │    · 灰线 = 有向边（箭头）             │
│ [路网管理]     │    · 鼠标悬停 → 浮窗详情              │
│ [路径查询]     │    · 鼠标点击 → 状态栏                │
│ [批次配送]     │                                     │
│               │                                     │
│ 操作日志       │                                     │
├───────────────┴─────────────────────────────────────┤
│ 状态栏: [1] 北京总仓  北京市朝阳区                     │
└─────────────────────────────────────────────────────┘
```

导航方式：左侧按钮不是简单网页跳转，而是 **QStackedWidget** 切换页面：
- 主菜单（4 个大按钮）
- 网点管理子页（添加/删除/修改/查询/列表/返回）
- 路网管理子页
- 路径查询子页
- 批次配送子页

### 8.3 画布核心几何（GraphWidget）

#### 8.3.1 经纬度 → 屏幕坐标

```
找所有节点的 lon/lat max/min
对每个节点：
  x = 边距 + (lon - minLon) / (maxLon - minLon) × 画布宽度
  y = 边距 + 画布高度 - (lat - minLat) / (maxLat - minLat) × 画布高度
  （纬度越大越靠北 → 屏幕 y 越小 → 需要反向）
```

#### 知识点：QPainter 基础

```cpp
void GraphWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    // ↑ QPainter 是 Qt 的"画笔"。所有绘图操作都通过它完成。
    //   "this" 表示画在这个 widget 上。

    // 设置颜色和线宽
    painter.setPen(QPen(QColor(100, 100, 100), 2));  // 灰色, 2像素宽
    painter.setBrush(QColor(0, 100, 200));            // 蓝色填充

    // 画线（从 (x1,y1) 到 (x2,y2)）
    painter.drawLine(x1, y1, x2, y2);

    // 画实心圆（圆心 (cx,cy)，半径 r）
    painter.drawEllipse(QPointF(cx, cy), r, r);

    // 画文字
    painter.drawText(x, y, "北京");

    // QPainter 坐标系统：
    // (0,0) 在左上角，x 轴向右，y 轴向下（和数学坐标系相反！）
    //   (0,0) ──── x →
    //      │
    //      y
    //      ↓
    //   这就是为什么 latitude 要反转：
    //   纬度越大（越北）→ 屏幕 y 越小（越上方）
}
```

#### 8.3.2 画箭头

```
方向向量 unitDir = (终点 - 起点) / 距离    ← 归一化到长度为 1
垂直向量 normal = (-unitDir.y, unitDir.x)  ← 旋转 90 度

画线：起点圆边缘 → 终点圆边缘（减去圆半径）
  lineStart = center(from) + unitDir × nodeRadius
  lineEnd   = center(to)   - unitDir × nodeRadius

箭头三角（在终点处）：
  顶点1: lineEnd
  顶点2: lineEnd - unitDir × 箭头长度 + normal × 半宽
  顶点3: lineEnd - unitDir × 箭头长度 - normal × 半宽

  示意图：
                    normal × 半宽
     终点 ──→ ● ←── 顶点1(lineEnd)
              /│\
             / │ \
    顶点2 ← /  │  \ → 顶点3
              (箭头长)
              ↓
          unitDir方向

双向边：两条线各偏移 normal × 5，避免重叠看不清
```

#### 8.3.3 鼠标命中检测

```
对每个节点：距离 = √((鼠标x - 圆心x)² + (鼠标y - 圆心y)²)
距离 ≤ 圆半径 + 4px → 命中
```

### 8.4 事件处理 vs 信号/槽

| | 事件（Event） | 信号/槽（Signal/Slot） |
|---|---|---|
| 粒度 | 低层（鼠标移动每个像素） | 高层（按钮点击） |
| 触发 | Qt 事件系统自动 | 组件主动 emit |
| 处理 | 重写 paintEvent 等虚函数 | connect + 槽函数 |
| 例子 | mouseMoveEvent, paintEvent | clicked, nodeHovered |

### 8.5 main.cpp

```cpp
#include <QApplication>
#include <QDir>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QDir::setCurrent(QCoreApplication::applicationDirPath());
    // ↑ 设置当前工作目录为 exe 所在目录。
    //   这样 data/network.txt 的相对路径才能正确解析。

    MainWindow win;
    win.show();
    return app.exec();
}
```

### 8.6 建议阅读顺序

1. **先看 `buildUI()`** -- 界面怎么搭的（布局、按钮、QStackedWidget）
2. **再看 `GraphWidget::paintEvent()`** -- 画布怎么渲染的（边→节点→浮窗 三层绘制）
3. **然后看鼠标事件** -- 交互怎么实现的（mouseMoveEvent, mousePressEvent）
4. **最后看业务槽函数** -- 按钮怎么连通到算法

完整代码见 `ExpressRouting/MainWindow.h` 和 `ExpressRouting/MainWindow.cpp`。

---

## 九、数据文件 + 构建配置 + 编译运行

### 9.1 data/network.txt

25 个中国真实城市，124 条有向边，双权（耗时+费用）。格式和前面 7.2 节描述的一致。

参见项目 `data/network.txt`。

### 9.2 data/orders.txt

5 条测试订单，覆盖南北、东西向，不同优化目标。

参见项目 `data/orders.txt`。

### 9.3 CMakeLists.txt 逐行详解

```cmake
# 最低 CMake 版本要求
cmake_minimum_required(VERSION 3.16)
# ↑ "16" 指 CMake 3.16，不是 C++ 版本。

# 项目定义
project(ExpressRouting VERSION 1.0 LANGUAGES CXX)
# ↑ 项目名 = ExpressRouting，版本 = 1.0，语言 = C++

# Qt 自动处理
set(CMAKE_AUTOUIC ON)
# ↑ AUTOUIC：自动处理 .ui 文件（如果有 .ui 界面文件，自动生成对应的 .h）

set(CMAKE_AUTOMOC ON)
# ↑ AUTOMOC：自动调用 MOC（Meta-Object Compiler）。
#   MOC 扫描所有 .h 文件，找到 Q_OBJECT 宏后生成 moc_*.cpp。
#   ★ 这是必须的！没有 AUTOMOC，信号槽机制无法工作。

set(CMAKE_AUTORCC ON)
# ↑ AUTORCC：自动处理 .qrc 资源文件（如果有的话）

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# ↑ 要求编译器支持 C++17。REQUIRED=ON 意味着不支持就报错。

# 找 Qt 库
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
# ↑ 先找 Qt6，找不到再找 Qt5。只要 Widgets 组件（基本控件库）。
#   Qt 分很多模块：Widgets（窗口/按钮）、Core（核心）、Gui（画图）、Network 等。

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
# ↑ ${QT_VERSION_MAJOR} 是 CMake 变量 —— 上一行找到的是 6 就是 6。
#   等价于：find_package(Qt6 REQUIRED COMPONENTS Widgets)

# 源文件列表
set(PROJECT_SOURCES
    main.cpp MainWindow.cpp MainWindow.h
    Graph.cpp Graph.h Dijkstra.cpp Dijkstra.h
    OrderManager.cpp OrderManager.h Containers.h
)
# ↑ 列出项目中所有 .h 和 .cpp。Containers.h 虽然是纯头文件，
#   放在这里可以确保 AUTOMOC 扫描到它（如果需要的话）。

# 创建可执行文件
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ExpressRouting MANUAL_FINALIZATION ${PROJECT_SOURCES})
else()
    add_executable(ExpressRouting ${PROJECT_SOURCES})
endif()
# ↑ Qt6 用 qt_add_executable（会自动处理一些 Qt 特有设置）
#   Qt5 用普通 add_executable

# 头文件搜索路径
target_include_directories(ExpressRouting PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# ↑ 告诉编译器：在项目根目录找 #include 的头文件。
#   这样 #include "Graph.h" 不需要写路径前缀。

# 链接 Qt 库
target_link_libraries(ExpressRouting PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
# ↑ 将 Qt6::Widgets（或 Qt5::Widgets）链接到我们的 exe。
#   链接 = "把 Qt 的编译好的 .lib/.dll 和我们的 .exe 关联起来"。

# 编译后自动复制 data/ 到输出目录
add_custom_command(TARGET ExpressRouting POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data $<TARGET_FILE_DIR:ExpressRouting>/data
)
# ↑ POST_BUILD：在编译完成后执行。
#   copy_directory：CMake 内置的目录复制命令。
#   ${CMAKE_CURRENT_SOURCE_DIR}/data → 输出目录/data
#   这样运行程序时，data/network.txt 和 data/orders.txt 就在 exe 旁边。

# Qt6 收尾
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ExpressRouting)
endif()
```

### 9.4 编译运行

**Qt Creator（推荐）**：
1. File → Open File or Project → 选 `CMakeLists.txt`
2. 选编译套件（MSVC 或 MinGW）
3. 点左下角 构建（锤子图标），点 运行（三角图标）

**VS Code（备选）**：
项目已提供 `.vscode/tasks.json` 和 `.vscode/launch.json`，但需要手动指定 Qt 头文件路径。

### 9.5 功能测试清单

| # | 测试 | 操作 | 预期 |
|---|------|------|------|
| 1 | 启动 | 直接运行 | 自动加载 network.txt，画布显示 25 个城市 |
| 2 | 悬停 | 鼠标移节点上 | 浮窗显示编号/名称/地址/出边数 |
| 3 | 点击 | 点节点 | 状态栏显示详情 |
| 4 | 最短耗时 | 路径查询→单源→输入 1 | 从北京到各城市的高亮路径 |
| 5 | 最低费用 | 路径查询→两点→1→20 | 北京→广州最便宜路径 |
| 6 | 添加网点 | 网点→添加→输入信息 | 画布出现新节点 |
| 7 | 删网点 | 网点→删除 | 节点和相关边消失 |
| 8 | 批量规划 | 批次→规划所有 | 5 条订单全算出来 |
| 9 | 拓扑排序 | 批次→拓扑排序 | 输出排序序列 |
| 10 | 导出方案 | 批次→导出 | data/plans.txt 生成 |

---

## 十、算法详解附录

### 10.1 Dijkstra 正确性证明

**定理**：Dijkstra 算法结束时，dist[v] 等于 start 到 v 的最短距离。

**证明（归纳 + 反证）**：

每次通过堆 pop 出一个节点 u 时，我们声称 dist[u] 已经是最优的（不可再缩短）。

反证法：假设存在一条更短的路径 P：start → ... → x → ... → u。

- 情况 1：x 已经被 pop 过 → 当 x 被 pop 时，我们遍历了 x 的所有出边，其中包括 x→u → 这条边会松弛 u，dist[u] 已经是 P 上的距离 → 矛盾。
- 情况 2：x 还没有被 pop → 因为所有边的权重 ≥ 0，P 的距离 ≥ dist[x]（因为 dist[x] 是 start 到 x 的最短距离的当前估计值，可能已经是最优也可能还能更短，但 dist[x] 至少 ≤ P 中 start 到 x 的距离）。如果 P 的距离 < dist[u]，则 dist[x] ≤ P 距离 < dist[u] → 堆顶不应该是 u（x 的距离更小，应该先 pop x）→ 矛盾。

因此每次 pop 出的节点的 dist 值已是最优。算法结束时，所有可达节点的 dist 都是最优的。

**示意图**：
```
start ──→ ... ──→ x ──→ ... ──→ u
                 ↑                ↑
            假设 x 未 pop    如果 x 距离更小
            但 dist[x] <     堆顶应该是 x
            dist[u]          不是 u！
```

### 10.2 惰性删除复杂度分析

```
总 push 次数：≤ E（每条边最多触发一次松弛 = 一次 push）
总 pop 次数：≤ E（包含跳过旧记录的操作）
每次 push/pop：O(log E)
总复杂度：O((V+E) log E) ≈ O((V+E) log V)

空间：堆中最多 E 个元素（懒惰的代价）
时间：每条边被处理一次，常数 log E 开销
```

### 10.3 Kahn 拓扑排序证明

**定理**：如果所有节点出队，出队顺序是合法的拓扑序。

**证明**：
- 节点 v 入队时，其在子集内的所有前驱都已经出队
- 当 v 出队时，v 的所有前驱都在结果序列中 → u→v 满足 u 在 v 前面
- 如果有环 → 环中所有节点的入度永远 > 0（环中每点至少被环中另一个点指向）→ 永远无法入队 → 出队数 < 总数

### 10.4 其他最短路径算法对比

| 算法 | 适用条件 | 复杂度 | 特点 |
|---|---|---|---|
| Dijkstra | 边权 ≥ 0 | O((V+E)logV) | 本项 目使用 |
| Bellman-Ford | 任意边权（含负权） | O(VE) | 能检测负权环。但本项目的快递边权没有负数，不需要 |
| A* | 有启发函数 | O(E) 最优情况 | 需要知道"到终点的距离估计"（如欧几里得距离），比 Dijkstra 快但需要地理信息 |
| Floyd-Warshall | 所有点对 | O(V³) | 一次性算出所有节点对之间的最短路径。25 节点 O(15625) 也可以用，但没 Dijkstra 好讲 |

为什么选 Dijkstra 而不是 Floyd-Warshall？
- Dijkstra 在答辩时可以展示"贪心法"的策略，每一步松弛都有意义
- Floyd-Warshall 的三重循环（k,i,j）虽然简单，但不容易讲清楚"为什么这样就能算对"
- 堆优化的 Dijkstra 可以顺便展示 MinHeap 的使用，增加项目技术含量

### 10.5 复杂度总表

| 操作 | 复杂度 | 备注 |
|------|--------|------|
| DynArray push_back | 均摊 O(1) | 2 倍扩容 |
| DynArray erase | O(n) | 后续元素前移 |
| DynArray operator[] | O(1) | 指针偏移 |
| HashMap insert/find | 均摊 O(1) | 负载 ≤ 0.5 |
| HashMap erase | 均摊 O(1) | 惰性删除 |
| MinHeap push/pop | O(log n) | 上浮/下沉 |
| Queue push/pop | O(1) | 下标操作 |
| Graph addNode | O(1) | HashMap insert |
| Graph deleteNode | O(E) | 遍历所有邻接表 |
| Graph addEdge | O(degree) | 查重 + push_back |
| Graph getNeighbors | O(1) | HashMap find |
| Dijkstra (堆优化) | O((V+E)log V) | 惰性删除 |
| Kahn 拓扑排序 | O(V+E) | BFS |

---

## 十一、答辩准备

### 11.1 老师最可能问的 8 个问题

**Q1："Dijkstra 为什么用堆？朴素版不是更简单吗？"**

> 堆优化把"选最小 dist 节点"从 O(V) 降到 O(logV)。虽然 25 节点时差异不大（朴素 625 vs 堆 74），但展示了算法优化的思路。另外我用了惰性删除——不在堆里修改旧值，而是 push 新值、pop 时跳过过期记录。这比实现 decrease-key 简单得多。

**Q2："HashMap 为什么用开放地址法而不是链地址法？"**

> 开放地址法所有数据在连续内存上，缓存友好。不需要额外链表节点分配。负载因子控制在 0.5，保证探测链 ≤ 2 次。惰性删除用 del 标记，插入时可复用。哈希用乘法哈希（黄金比例倒数），对连续整数分布均匀。

**Q3："邻接表 vs 邻接矩阵？"**

> 本项目 25 节点 124 边，密度 ~20%，是稀疏图。邻接表 O(V+E) 空间，HashMap find O(1)，遍历出边 O(degree)。邻接矩阵 O(V²) 空间且扫全行，对稀疏图不利。另外 HashMap 的自实现本身也是课程要求。

**Q4："拓扑排序有环怎么处理？"**

> Kahn 算法结束后出队数 < 子集节点数 → 有环。我收集所有入度 > 0 的节点作为"环节点列表"，在 GUI 标红显示。用户不仅知道有环，还能看到哪些节点在环里。

**Q5："双权边怎么处理？"**

> 每条边存 time 和 cost。Dijkstra 代码只写一份，最短耗时时以 time 为比较权重（cost 顺带算），最低费用时以 cost 为比较权重（time 顺带算）。两函数结构完全相同，只改了一行比较语句。

**Q6："你的 DynArray 扩容为什么是 2 倍，不是 1.5 倍？"**

> 2 倍扩容使得均摊复杂度为 O(1)：总拷贝次数 = n/2 + n/4 + ... ≈ n。1.5 倍当然也可以，只是数学常数不同。Java 的 ArrayList 默认也是 1.5 倍。关键在于"指数级扩容"这个思路，具体倍数可以根据内存和性能权衡。

**Q7："为什么你自己写迭代器（begin/end），而不是让 DynArray 只提供 operator[]？"**

> 为了支持 C++11 的范围 for 循环（`for (auto& x : arr)`）。这样使用时更简洁，也更安全（不会写出越界的索引）。实现很简单——指针本身就是迭代器，因为内存是连续的。

**Q8："你的惰性删除会不会导致堆里太多垃圾？"**

> 每条边最多产生一个新记录，所以堆大小最多 O(E)。对于 25 节点 124 边的图，最多 124 个堆元素，log₂(124) ≈ 7 层。完全可以接受。如果图特别大（比如 10 万条边），可以通过"定期清理"来优化，但在我们这个规模下不需要。

### 11.2 三句话讲清每个核心算法

- **Dijkstra**：1. MinHeap 取最短节点 2. 惰性删除跳旧记录 3. prev 回溯 + 翻转得路径
- **拓扑排序**：1. 统计入度 2. 入度 0 入队 3. 出队减度 4. 入度 0 继续入队 5. 比数量判环
- **HashMap**：1. 乘法哈希定位 2. 线性探测冲突 3. 惰性删除打标记

### 11.3 现场演示指南

1. **启动程序**：直接运行，自动加载 network.txt。画布显示 25 个城市节点。

2. **展示悬停**：鼠标移到"北京"上 -- 浮窗显示状态信息。说："我实现了鼠标交互，可以快速查看网点信息。"

3. **展示最短耗时**：点击"路径查询" → "单源最短耗时" → 输入起点 1 → 选择"按耗时最短"。画布上橙色高亮路径。说："这是 Dijkstra 堆优化版，复杂度 O((V+E)logV)。"

4. **展示最低费用**：两点查询 → 输入起点 1，终点 20 → 选择"按费用最低"。对比两种优化目标的路径可能不同。

5. **展示拓扑排序**：点击"批次配送" → "拓扑排序"。说："Kahn 算法检测环路依赖，如果有环会标红显示。"

6. **展示文件导入导出**：菜单栏 → 文件 → 导入路网 / 导出方案。

### 11.4 代码行数

| 层 | 文件 | 行数 |
|----|------|------|
| 容器 | Containers.h | ~260 |
| 图 | Graph.h + .cpp | ~150 |
| 算法 | Dijkstra.h + .cpp | ~150 |
| 业务 | OrderManager.h + .cpp | ~200 |
| GUI | MainWindow.h + .cpp | ~500 |
| 入口 | main.cpp | ~20 |
| 构建 | CMakeLists.txt | ~50 |
| 数据 | network.txt + orders.txt | ~95 |
| **合计** | | **~1425** |

---

*文档版本 v3.0 | 匹配项目代码 | 2026-06-11*
