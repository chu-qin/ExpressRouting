# 快递网点配送路径规划系统 —— 从零构建指南

> 适用读者：C++ 初学者，数据结构与算法课程设计  
> 难度等级：适合大一下/大二上学生  
> 预计阅读时间：2-3 小时  

---

## 目录

1. [项目简介](#1-项目简介)
2. [开发环境搭建](#2-开发环境搭建)
3. [数据结构设计：DynArray 动态数组](#3-数据结构设计dynarray-动态数组)
4. [图的存储：邻接表](#4-图的存储邻接表)
5. [图的 CRUD 操作](#5-图的-crud-操作)
6. [最短路径算法：Dijkstra](#6-最短路径算法dijkstra)
7. [最低费用路径](#7-最低费用路径)
8. [拓扑排序：Kahn 算法](#8-拓扑排序kahn-算法)
9. [订单管理与文件读写](#9-订单管理与文件读写)
10. [Qt 图形界面](#10-qt-图形界面)
11. [测试流程](#11-测试流程)
12. [答辩要点](#12-答辩要点)
13. [代码索引](#13-代码索引)

---

## 1. 项目简介

### 1.1 项目目标

实现一个**快递网点配送路径规划系统**，核心功能：

- 管理全国 25 个快递网点（城市），支持增删改查
- 管理网点之间的运输路线（有向边），含耗时和费用
- 查询任意两个网点之间的最短耗时路径
- 查询任意两个网点之间的最低费用路径
- 批量导入订单，自动规划配送路径，检测环路
- 图形化界面：在地图上可视化路网和路径

### 1.2 技术要求

根据课程设计要求（题目 21）：

- **不能使用 STL 模板库**（vector/list/stack/queue/map 等），必须自己实现数据结构
- 基于面向对象思想设计
- 节点数量 ≥ 25 个
- 完整封装图结构、算法类、业务类
- 完善异常处理、日志输出
- 图形化界面

### 1.3 项目文件总览

```
highway_network_planning/
├── DynArray.h          ← 自实现动态数组（唯一的手写容器）
├── Graph.h             ← 图的数据结构声明
├── Graph.cpp           ← 图的增删改查实现
├── Dijkstra.h          ← 最短路径 + 拓扑排序声明
├── Dijkstra.cpp        ← Dijkstra 算法 + Kahn 拓扑排序
├── OrderManager.h      ← 订单管理 + 文件读写声明
├── OrderManager.cpp    ← 业务逻辑实现
├── mainwindow.h        ← Qt 界面声明
├── mainwindow.cpp      ← Qt 界面实现（布局+画布+信号槽）
├── main.cpp            ← 程序入口
├── CMakeLists.txt      ← CMake 构建配置
└── data/
    ├── network.txt     ← 路网数据（25 节点 + 100+ 条边）
    └── orders.txt      ← 测试订单
```

### 1.4 设计哲学：为什么这么简单？

本项目刻意避免了工业级项目中常见的高级技巧：

| 高级写法 | 本项目替代方案 | 为什么 |
|----------|---------------|--------|
| 手写 HashMap | 顺序编号，用 DynArray 下标替代 | 节点 0~24，下标就是编号 |
| 手写 MinHeap | 朴素 O(V²) Dijkstra | 25 节点 = 625 次比较，毫秒完成 |
| 手写 Queue | DynArray + head 下标 | 10 行代码模拟队列 |
| 惰性删除 + 堆优化 | 朴素算法一次性确定 | 不求性能求易懂 |
| 模板元编程 | 一个模板类 DynArray | 只学一次模板语法 |

**核心原则：用大一学过的知识完成这个项目。** 你只需要理解数组、指针、for 循环、类。

---

## 2. 开发环境搭建

### 2.1 所需软件

- **Qt 6.x**（含 Qt Creator）— 用于图形界面
- **CMake 3.16+** — 构建系统（Qt 自带）
- **MSVC 2022** 或 **MinGW/GCC** — C++17 编译器

### 2.2 创建项目

1. 打开 Qt Creator
2. 文件 → 新建项目 → Qt Widgets Application
3. 项目名：`highway_network_planning`
4. 构建系统：CMake
5. 基类：QMainWindow
6. 创建完成后，在左侧项目树中删除自动生成的 `mainwindow.ui`（我们手写 UI）

### 2.3 CMakeLists.txt 配置

CMakeLists.txt 负责告诉编译器哪些文件需要参与编译：

```cmake
cmake_minimum_required(VERSION 3.16)
project(highway_network_planning VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)          # 自动处理 Qt 信号槽
set(CMAKE_CXX_STANDARD 17)     # 使用 C++17 标准

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES            # ★ 所有源文件清单
    main.cpp
    mainwindow.cpp mainwindow.h
    Graph.cpp Graph.h
    Dijkstra.cpp Dijkstra.h
    OrderManager.cpp OrderManager.h
    DynArray.h
)

qt_add_executable(highway_network_planning ${PROJECT_SOURCES})
target_link_libraries(highway_network_planning PRIVATE Qt6::Widgets)
```

**关键点：** 每次添加新的 `.cpp`/`.h` 文件后，都要在 `PROJECT_SOURCES` 里加一行，否则 CMake 不知道这个文件需要编译。

---

## 3. 数据结构设计：DynArray 动态数组

> **文件：** `DynArray.h` | **行数：** ~100 行 | **难度：** ⭐⭐⭐

### 3.1 为什么需要它？

课程要求不能使用 STL 的 `vector`，但项目中到处需要"能自动扩容的数组"。所以我们自己写一个。

### 3.2 核心原理

DynArray 的原理和教科书上的**顺序表**完全一样：在堆上申请一块连续内存，元素放满了就申请一块 2 倍大的新内存，把旧数据拷过去，释放旧内存。

```
初始状态：
data → [ _ , _ , _ , _ ]   capacity=4  length=0

push_back("北京")：
data → [北京, _ , _ , _ ]   capacity=4  length=1

push_back 3次填满后：
data → [北京,上海,广州,深圳] capacity=4  length=4

再 push_back("成都") 时触发 expand()：
1. newData → [  ,  ,  ,  ,  ,  ,  ,  ]   newCap=8
2. 拷贝旧数据 → [北京,上海,广州,深圳,  ,  ,  ,  ]
3. delete[] data（释放旧内存）
4. data = newData
5. data → [北京,上海,广州,深圳,成都,  ,  ,  ]  cap=8  len=5
```

### 3.3 关键成员变量

```cpp
template<typename T>
class DynArray {
private:
    T*  data     = nullptr;   // 指向堆上数组的指针
    int length   = 0;         // 当前有多少个元素
    int capacity = 0;         // 数组总共多大
```

**理解 `T*` 和 `T` 的区别：**

- `T data[100]` — 在栈上分配 100 个 T（编译时就要知道大小）
- `T* data` — 只存一个指针（8 字节），指向堆上的数组（运行时决定大小）

**length vs capacity：**
- `length`："你能看见几个元素"（`size()` 返回值）
- `capacity`："背后实际申请了多大空间"（内部使用，外部不关心）
- 当 `length == capacity` 时触发扩容

### 3.4 扩容函数 expand()

```cpp
void expand() {
    int newCap = capacity ? capacity * 2 : 4;  // ★ 首次 4，之后翻倍
    T*  newArr = new T[newCap];                // 堆上申请新数组

    for (int i = 0; i < length; ++i)           // 逐元素拷贝
        newArr[i] = data[i];

    delete[] data;     // ★ 释放旧数组内存（必须有[]，因为是数组）
    data     = newArr; // 指针指向新数组
    capacity = newCap; // 更新容量记录
}
```

> **⚠️ 关键知识点：** `new T[n]` 配 `delete[]`，`new T` 配 `delete`，不能混用！
> - `delete[]` 会调用每个元素的析构函数再释放整块内存
> - `delete` 只释放一个对象，用于数组会导致未定义行为

**为什么扩容 2 倍？** 数学上可以证明，这样 push_back N 次的均摊时间是 O(1)。如果每次只扩 1 个位置，push_back N 次是 O(N²)。

### 3.5 拷贝构造函数（深拷贝 vs 浅拷贝）

```cpp
DynArray(const DynArray& other) {
    for (int i = 0; i < other.length; ++i)
        push_back(other.data[i]);      // ★ 逐元素拷贝
}
```

**这是整个项目最容易出 bug 的地方。** 必须理解深浅拷贝：

```cpp
// ❌ 默认拷贝（浅拷贝）— 编译器自动生成的
DynArray(const DynArray& other)
    : data(other.data), length(other.length), capacity(other.capacity) {}
// 问题：两个对象的 data 指向同一块内存！
// 析构时：A 先 delete[] data，B 再 delete[] data → 同一块内存被释放两次 → 崩溃！

// ✅ 正确的深拷贝
DynArray(const DynArray& other) {
    for (int i = 0; i < other.length; ++i)
        push_back(other.data[i]);
    // 每个元素单独拷贝到新内存，两个对象互不影响
}
```

**答辩可以用图示解释：**

```
浅拷贝：
  a.data ──→ [北京,上海,广州]
               ↑
  b.data ──────┘  （两个指针指向同一块！释放两次崩溃）

深拷贝：
  a.data ──→ [北京,上海,广州]  （独立内存）
  b.data ──→ [北京,上海,广州]  （独立内存）
```

### 3.6 赋值运算符（operator=）

```cpp
DynArray& operator=(const DynArray& other) {
    if (this == &other) return *this;    // ★ 防止 a=a 把自己删了

    delete[] data;                        // 先清掉自己的旧数据
    data     = nullptr;
    length   = 0;
    capacity = 0;

    for (int i = 0; i < other.length; ++i)
        push_back(other.data[i]);         // 逐元素拷贝

    return *this;                         // ★ 返回自己，支持 a=b=c
}
```

> **`this == &other` 是什么？**  
> `this` 是"当前对象的地址"，`&other` 是"参数对象的地址"。  
> 如果 `a = a`（自己赋值给自己），`this == &other` 为 true。  
> 此时如果执行 `delete[] data`，就把自己的数据删了，后面拷贝时访问的是已释放内存 → 崩溃。

### 3.7 删除操作

```cpp
// 删除下标为 index 的元素，后面元素依次前移
void erase(int index) {
    for (int i = index; i < length - 1; ++i)
        data[i] = data[i + 1];      // 前移
    --length;
}

// 删除第一个满足条件的元素
template<typename P>
bool remove_first(P check) {
    for (int i = 0; i < length; ++i)
        if (check(data[i])) { erase(i); return true; }
    return false;
}

// 删除所有满足条件的元素
template<typename P>
void remove_all(P check) {
    for (int i = 0; i < length; ) {
        if (check(data[i]))
            erase(i);        // ★ 删完不 i++，因为下一个元素移到当前位置了
        else
            ++i;
    }
}
```

> **`template<typename P>` 是什么意思？**  
> `P` 可以是任何"能当函数用"的东西：函数指针、lambda 表达式、函数对象。  
> 例如 `remove_first([](const Edge& e){ return e.to == 5; })` — 删除第一个 to==5 的边。

### 3.8 迭代器

```cpp
T* begin() { return data; }
T* end()   { return data + length; }
```

**为什么指针能当迭代器？** 因为数组元素连续存放在内存中，`data` 指向第一个元素，`data+1` 指向第二个……所以 `T*` 天然支持 `++` 操作。

```cpp
// for (auto& x : arr) 编译器会展开为：
for (T* it = arr.begin(); it != arr.end(); ++it) {
    auto& x = *it;   // *it 就是 data[i]
}
```

---

## 4. 图的存储：邻接表

> **文件：** `Graph.h` | **行数：** ~55 行 | **难度：** ⭐⭐

### 4.1 什么是邻接表？

**一句话：给每个城市记一张"我能直达哪里"的清单。**

> 假设有 3 个城市，2 条路：
> ```
> 北京(0) → 上海(1)  耗时 6h  费用 450 元
> 北京(0) → 广州(2)  耗时 10h 费用 850 元
> 上海(1) → 广州(2)  耗时 5h  费用 380 元
> ```
>
> 邻接表就是：
> ```
> neigh[0] = [ {to:1, time:6, cost:450}, {to:2, time:10, cost:850} ]
> neigh[1] = [ {to:2, time:5, cost:380} ]
> neigh[2] = [  ]   ← 空列表，广州没有出边
> ```

### 4.2 数据结构定义

```cpp
// 节点（快递网点）
struct Node {
    int         id;
    std::string name;        // 城市名（如"北京总仓"）
    std::string address;
    double      lon, lat;    // 经纬度（在地图上显示位置用）

    Node() = default;
    Node(int i, const std::string& n, const std::string& a,
         double lo = 0, double la = 0)
        : id(i), name(n), address(a), lon(lo), lat(la) {}
};

// 有向边（运输线路）
struct Edge {
    int    to;        // ☆ 只有 to，没有 from！
    double time;      // 运输耗时（小时）
    double cost;      // 运输费用（元）

    Edge() = default;
    Edge(int t, double ti, double c) : to(t), time(ti), cost(c) {}
};
```

> **⚠️ 为什么 Edge 没有 `from` 字段？**  
> 因为边存在 `neigh[from]` 这个列表里——`from` 已经被邻接表的外层下标表达了。  
> 这样省一个 int 字段，也避免了数据不一致（`from` 和列表归属矛盾）。

### 4.3 Graph 类

```cpp
class Graph {
private:
    DynArray<Node>            nodes;     // nodes[i] = i号节点的信息
    DynArray< DynArray<Edge> > neigh;    // neigh[i] = 从i出发的所有边
    //             ↑
    //    "DynArray 套 DynArray"——每个节点一个出边列表
```

**节点的编号规则（本文最关键的设计决策）：**

> 节点编号 = 数组下标。添加时自动分配 `nodes.size()` 作为新编号。
>
> ```
> 添加北京 → nodes[0] = {name:"北京总仓", ...}
> 添加上海 → nodes[1] = {name:"上海分仓", ...}
> 添加广州 → nodes[2] = {name:"广州分仓", ...}
> ```
>
> 好处：给一个编号 `id`，直接 `nodes[id]` 就能查到节点信息，完全不需要 HashMap。

**删除节点后怎么办？** 把 `nodes[id].id` 设为 -1 标记为"已删除"，保留数组位置不重用。这样其他节点的编号不会变，所有引用边也不会断裂。

### 4.4 邻接表 vs 邻接矩阵

| | 邻接矩阵 | 邻接表 |
|---|---|---|
| 存储方式 | N×N 二维数组 | N 个列表 |
| 空间 | O(N²) | O(N+E) |
| 查边 `(i,j)` | O(1)：`matrix[i][j]` | O(deg)：遍历 `neigh[i]` |
| 遍历 `i` 的所有邻居 | O(N)：扫描整行 | O(deg)：遍历列表 |
| 适合 | 稠密图（边多） | 稀疏图（边少） |

本项目 25 节点 ~100 条边，是稀疏图，邻接表更合适。

---

## 5. 图的 CRUD 操作

> **文件：** `Graph.cpp` | **行数：** ~130 行 | **难度：** ⭐⭐⭐

### 5.1 添加节点

```cpp
bool Graph::addNode(const Node& node) {
    // ① 异常检查：名称不能为空
    if (node.name.empty()) {
        std::cerr << "[错误] 添加节点失败：名称为空\n";
        return false;
    }
    // ② 异常检查：编号必须按顺序分配
    if (node.id != nodes.size()) {
        std::cerr << "[错误] 添加节点失败：编号必须为 " << nodes.size() << "\n";
        return false;
    }

    nodes.push_back(node);                // 存节点信息
    neigh.push_back(DynArray<Edge>());    // 初始化空的出边列表
    return true;
}
```

> **问：为什么 `push_back(DynArray<Edge>())`？**  
> 让 `neigh` 数组和 `nodes` 数组长度保持同步。即使节点还没有出边，也要给它一个空列表占位，这样 `neigh[id]` 访问不会越界。

### 5.2 删除节点

```cpp
bool Graph::deleteNode(int id) {
    if (!hasNode(id)) return false;

    // ① 清空该节点的出边列表
    neigh[id].clear();

    // ② 遍历所有节点，删除指向 id 的边（清理入边）
    for (int i = 0; i < (int)nodes.size(); ++i) {
        if (nodes[i].id == -1) continue;   // 跳过已删除节点
        neigh[i].remove_all([id](const Edge& e) {
            return e.to == id;
        });
    }

    // ③ 标记节点为已删除
    nodes[id].id = -1;
    return true;
}
```

> **问：为什么要清理入边？**  
> 假如删了上海（id=1），但北京的 `neigh[0]` 里还有 `{to:1, ...}` 这条边。如果不清理，后续遍历 `neigh[0]` 时就会指向一个不存在的节点。

### 5.3 添加边

```cpp
bool Graph::addEdge(int from, int to, double time, double cost) {
    // ① 两端节点必须存在
    if (!hasNode(from) || !hasNode(to)) return false;
    // ② 不能自己连自己
    if (from == to) return false;
    // ③ 权重不能为负
    if (time < 0 || cost < 0) return false;
    // ④ 不能重复添加同一方向
    for (int i = 0; i < neigh[from].size(); ++i)
        if (neigh[from][i].to == to) return false;

    neigh[from].push_back(Edge(to, time, cost));
    return true;
}
```

**边界检查清单：** 每项异常都对应一行 if，老师检查时会重点看这个。

### 5.4 查询

```cpp
// 获取节点 id 的所有出边
const DynArray<Edge>& Graph::getNeighbors(int id) const {
    if (!hasNode(id)) return emptyList;   // 查不到返回空列表
    return neigh[id];
}

// 判断节点是否存在
bool Graph::hasNode(int id) const {
    return id >= 0 && id < (int)nodes.size() && nodes[id].id != -1;
}

// 统计边数（遍历所有节点的出边列表求和）
int Graph::edgeCount() const {
    int cnt = 0;
    for (int i = 0; i < neigh.size(); ++i)
        cnt += neigh[i].size();
    return cnt;
}
```

---

## 6. 最短路径算法：Dijkstra

> **文件：** `Dijkstra.cpp` | **行数：** ~50 行（最短耗时部分） | **难度：** ⭐⭐⭐⭐

### 6.1 算法思想（一句话）

**从起点出发，每次选一个"当前知道的最短距离"且"还没确定"的节点，用它去更新它所有邻居的距离。**

### 6.2 为什么不用堆优化？

教材上的 Dijkstra 通常用优先队列（堆）优化到 O((V+E)logV)。但本项目刻意用朴素版：

- 25 个节点，每次选最小要扫描 25 个，共 25 轮 = 625 次比较——**毫秒级完成**
- 朴素版不需要 MinHeap，少写 100 行代码
- 答辩时更容易讲清楚每行代码的作用

### 6.3 算法步骤（关键三行注释）

```
初始化：所有节点距离 = ∞，起点的距离 = 0

重复 N 次：
  ① 在所有"未确定"节点中，找距离最小的 → 叫它 cur
  ② 标记 cur 为"已确定"
  ③ 用 cur 去松弛它的所有邻居：如果 dist[cur] + 边权 < dist[邻居]，就更新

回溯：从终点沿着 prev 数组往回走到起点，再翻转
```

### 6.4 完整代码逐行解读

```cpp
DynArray<PathResult> Dijkstra::shortestTime(const Graph& graph, int start) {
    int N = graph.maxNodeId();       // N = 25（节点总数）

    // ===== 初始化三个数组 =====
    DynArray<double> minTime;     // minTime[i] = 从起点到 i 的最短耗时
    DynArray<double> minCost;     // minCost[i] = 对应路径上的总费用
    DynArray<int>    prev;        // prev[i] = i 的前驱节点（用于回溯路径）
    DynArray<int>    visited;     // visited[i] = 1 表示 i 已确定最短路径

    for (int i = 0; i < N; ++i) {
        minTime.push_back(INF);   // INF = 1e18（代表无穷远）
        minCost.push_back(INF);
        prev.push_back(-1);       // -1 代表没有前驱
        visited.push_back(0);     // 0 代表未确定
    }

    if (!graph.hasNode(start)) return {};  // 起点不存在就返回空

    minTime[start] = 0;           // 起点到自己的距离为0
    minCost[start] = 0;

    // ===== 主循环：重复 N 次 =====
    for (int round = 0; round < N; ++round) {

        // ★★★ 第一步：找"未确定节点中距离最小"的那个 ★★★
        int    cur  = -1;
        double best = INF;
        for (int j = 0; j < N; ++j) {
            if (!visited[j] && graph.hasNode(j) && minTime[j] < best) {
                best = minTime[j];
                cur  = j;
            }
        }
        if (cur == -1) break;     // 剩余节点都不可达，提前结束

        // ★★★ 第二步：标记为已确定 ★★★
        visited[cur] = 1;

        // ★★★ 第三步：松弛 cur 的所有邻居 ★★★
        const DynArray<Edge>& edges = graph.getNeighbors(cur);
        for (int j = 0; j < edges.size(); ++j) {
            const Edge& e  = edges[j];
            double newTime = minTime[cur] + e.time;     // 经过 cur 到 e.to
            if (newTime < minTime[e.to]) {               // 如果这条路更短
                minTime[e.to] = newTime;                  // 更新最短耗时
                minCost[e.to] = minCost[cur] + e.cost;    // 更新累计费用
                prev[e.to]    = cur;                       // 记录前驱
            }
        }
    }

    // ===== 构建结果 =====
    DynArray<PathResult> results;
    for (int i = 0; i < N; ++i) {
        PathResult pr;
        if (graph.hasNode(i) && minTime[i] != INF) {
            pr.reachable  = true;
            pr.totalTime  = minTime[i];
            pr.totalCost  = minCost[i];
            pr.path       = rebuildPath(prev, i);
        }
        results.push_back(pr);
    }
    return results;
}
```

### 6.5 算法执行过程（手工跟踪）

以 4 个节点为例，起点为 0：

```
边：0→1(time:4)  0→2(time:2)  1→3(time:2)  2→1(time:1)  2→3(time:5)

初始状态：
  minTime = [0, ∞, ∞, ∞]
  visited = [0, 0, 0, 0]
  prev    = [-1,-1,-1,-1]

第1轮（找最小）：
  扫描 minTime[0..3]：0(未访,值0) 1(未访,值∞) 2(未访,值∞) 3(未访,值∞)
  → cur=0（最小值0）
  visited[0]=1
  松弛邻居 1：newTime=0+4=4 < ∞ → minTime[1]=4, prev[1]=0
  松弛邻居 2：newTime=0+2=2 < ∞ → minTime[2]=2, prev[2]=0
  minTime = [0✓, 4, 2, ∞]

第2轮：
  扫描 minTime[0..3]：0(已访) 1(未访,4) 2(未访,2) 3(未访,∞)
  → cur=2（最小值2）
  visited[2]=1
  松弛邻居 1：newTime=2+1=3 < 4 → minTime[1]=3, prev[1]=2
  松弛邻居 3：newTime=2+5=7 < ∞ → minTime[3]=7, prev[3]=2
  minTime = [0✓, 3, 2✓, 7]

第3轮：
  扫描 minTime[0..3]：0(已访) 1(未访,3) 2(已访) 3(未访,7)
  → cur=1（最小值3）
  visited[1]=1
  松弛邻居 3：newTime=3+2=5 < 7 → minTime[3]=5, prev[3]=1
  minTime = [0✓, 3✓, 2✓, 5]

第4轮：
  → cur=3（最后确定）
  最终：minTime = [0✓, 3✓, 2✓, 5✓]

回溯路径 0→3：prev[3]=1 → prev[1]=2 → prev[2]=0 → prev[0]=-1
翻转后：[0, 2, 1, 3]，总耗时=5
```

### 6.6 路径回溯函数

```cpp
static DynArray<int> rebuildPath(const DynArray<int>& prev, int target) {
    DynArray<int> path;
    // 从终点往回走，沿着 prev 链一直走到 -1
    for (int cur = target; cur != -1; cur = prev[cur])
        path.push_back(cur);

    // 翻转：[终点,...,起点] → [起点,...,终点]
    for (int l = 0, r = path.size() - 1; l < r; ++l, --r) {
        int tmp = path[l];
        path[l] = path[r];
        path[r] = tmp;
    }
    return path;
}
```

---

## 7. 最低费用路径

> **文件：** `Dijkstra.cpp` | **难度：** ⭐⭐

### 7.1 和最短耗时的唯一区别

代码结构和 `shortestTime` **完全一样**，只改了一行：

```cpp
// shortestTime：以 time 为权
double newTime = minTime[cur] + e.time;
if (newTime < minTime[e.to]) { ... }

// cheapestPath：以 cost 为权
double newCost = dist[cur] + e.cost;
if (newCost < dist[e.to]) { ... }
```

其余逻辑：初始化、找最小、松弛、回溯——100% 相同。

### 7.2 另一个区别：cheapestPath 是两点查询

```cpp
PathResult Dijkstra::cheapestPath(const Graph& graph, int start, int target) {
    // ... 初始化 ...
    for (int round = 0; round < N; ++round) {
        // 找最小未确定节点
        // ...
        if (cur == -1 || cur == target) break;  // ★ 找到终点立即停止
        // 松弛邻居
    }
    // 只返回 target 的结果
}
```

---

## 8. 拓扑排序：Kahn 算法

> **文件：** `Dijkstra.cpp`（底部） | **行数：** ~50 行 | **难度：** ⭐⭐⭐

### 8.1 应用场景

当有多个订单时，需要检查是否存在**配送环路**（A→B→C→A），如果有环则无法确定先后顺序。

- 无环 → 输出配送先后顺序
- 有环 → 输出环中节点列表

### 8.2 算法思想

**不断删除"入度为 0"的节点，直到删完或删不动（有环）。**

```
入度 = 有多少条边指向这个节点

例如：A→B→C→B（C 又指回 B）
  A 入度=0  B 入度=2  C 入度=1

  第1步：删 A（入度0），B 入度变 1
  第2步：入度0的节点没了！还剩 B(入1) C(入1)
  → 有环！环包含 B 和 C
```

### 8.3 队列的模拟

不需要单独的 Queue 类。用一个 `DynArray<int>` 加一个 `int head` 下标就能模拟队列：

```cpp
DynArray<int> queue;    // 队列本体
int head = 0;           // 队头下标

// 入队
queue.push_back(x);

// 出队
int x = queue[head];
head++;

// 判断队空
bool empty = (head >= queue.size());
```

### 8.4 完整算法

```cpp
TopoResult TopoSort::sort(const Graph& graph, const DynArray<int>& nodeIds) {
    int N = graph.maxNodeId();

    // 1. 标记哪些节点在子集中（inSet[id]=1）
    DynArray<int> inSet;
    for (int i = 0; i < N; ++i) inSet.push_back(0);
    for (int i = 0; i < nodeIds.size(); ++i)
        inSet[nodeIds[i]] = 1;

    // 2. 统计子集内每个节点的入度
    DynArray<int> ruDu;
    for (int i = 0; i < N; ++i) ruDu.push_back(0);
    for (int i = 0; i < nodeIds.size(); ++i) {
        const DynArray<Edge>& edges = graph.getNeighbors(nodeIds[i]);
        for (int j = 0; j < edges.size(); ++j)
            if (inSet[edges[j].to]) ruDu[edges[j].to]++;
    }

    // 3. 入度为 0 的入队
    DynArray<int> queue;
    int head = 0;
    for (int i = 0; i < nodeIds.size(); ++i)
        if (ruDu[nodeIds[i]] == 0) queue.push_back(nodeIds[i]);

    // 4. BFS：不断弹出队头，删掉它的出边（减少邻居入度）
    while (head < queue.size()) {
        int u = queue[head++];
        result.order.push_back(u);

        const DynArray<Edge>& edges = graph.getNeighbors(u);
        for (int j = 0; j < edges.size(); ++j) {
            int v = edges[j].to;
            if (inSet[v]) {
                ruDu[v]--;
                if (ruDu[v] == 0) queue.push_back(v);
            }
        }
    }

    // 5. 判环：排序结果数 < 子集大小 → 有环
    if (result.order.size() < nodeIds.size()) {
        result.hasCycle = true;
        // 不在 order 中的节点就在环中
        ...
    }
    return result;
}
```

---

## 9. 订单管理与文件读写

> **文件：** `OrderManager.h/cpp` | **行数：** ~160 行 | **难度：** ⭐⭐

### 9.1 数据模型

```cpp
struct Order {
    int         orderId;       // 订单编号
    int         srcNode;       // 起点网点
    int         dstNode;       // 终点网点
    std::string goods;         // 货物名称
    int         preferTime;    // 1=最短耗时  0=最低费用
};

struct DeliveryPlan {
    Order      order;          // 原始订单
    PathResult result;         // 规划结果
};
```

### 9.2 批量规划

```cpp
DynArray<DeliveryPlan> OrderManager::planAll(const Graph& graph) const {
    DynArray<DeliveryPlan> plans;
    for (int i = 0; i < orders.size(); ++i) {
        DeliveryPlan plan;
        plan.order = orders[i];

        if (orders[i].preferTime)
            plan.result = Dijkstra::shortestTime(graph, orders[i].srcNode)
                          [orders[i].dstNode];   // 取对应目标的结果
        else
            plan.result = Dijkstra::cheapestPath(graph,
                          orders[i].srcNode, orders[i].dstNode);
        plans.push_back(plan);
    }
    return plans;
}
```

### 9.3 文件格式

**路网文件 (network.txt)：** 分段标记 `# NODES` / `# EDGES`

```
# NODES
0 北京总仓 北京市大兴区 116.40 39.90
1 上海分仓 上海市浦东新区 121.47 31.23
...
# EDGES
0 1 6.0 450
0 2 10.0 850
...
```

**订单文件 (orders.txt)：** 每行一个订单

```
1001 0 16 电子产品 1
#  ↑  ↑  ↑  ↑      ↑
#  ID 起 终 货物  1=耗时优先
```

### 9.4 文件解析核心代码

```cpp
// 拆词：把一行文字按空格拆成多个词
std::istringstream ss(line);
string w;
DynArray<string> words;
while (ss >> w) words.push_back(w);

// 对于节点行：末两词是经纬度，首词是编号，第二词是名称，中间是地址
int    id   = std::stoi(words[0]);
string name = words[1];
double lon  = std::stod(words[words.size() - 2]);
double lat  = std::stod(words[words.size() - 1]);
```

---

## 10. Qt 图形界面

> **文件：** `mainwindow.h/cpp` | **行数：** ~500 行 | **难度：** ⭐⭐⭐⭐⭐

### 10.1 布局结构

```
┌── QSplitter(水平) ──────────────────────┐
│  leftPanel(160px) │  QSplitter(垂直)     │
│  ┌─ 网点管理 ─┐   │  ┌─ canvas(画布) ─┐ │
│  │ [添加网点] │   │  │                │ │
│  │ [删除网点] │   │  │  城市圆点+连线  │ │
│  │ [修改网点] │   │  │                │ │
│  │ [查询网点] │   │  └────────────────┘ │
│  ├─ 路网管理 ─┤   │  ┌─ logBox(日志) ─┐ │
│  │ [添加路线] │   │  │ [OK] 路径: ... │ │
│  │ [删除路线] │   │  └────────────────┘ │
│  │ [导入路网] │   │                      │
│  │ [导出路网] │   │                      │
│  ├─ 路径查询 ─┤   │                      │
│  │ 起点:[▼]  │   │                      │
│  │ 终点:[▼]  │   │                      │
│  │[最短耗时] │   │                      │
│  │[最低费用] │   │                      │
│  │[清除高亮] │   │                      │
│  ├─ 订单管理 ─┤   │                      │
│  │ [添加订单] │   │                      │
│  │ ...       │   │                      │
└──────────────────┴──────────────────────┘
```

### 10.2 画布绘制：paintEvent

```cpp
void GraphWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);  // 抗锯齿

    p.fillRect(rect(), QColor(30, 30, 30));   // 深色背景

    // ★ 先画边（边在节点下面）
    for (每个节点 i) {
        for (neigh[i] 的每条边) {
            // 判断高亮颜色
            QColor clr = 高亮 ? 黄色 : 灰色;
            drawEdge(p, nodePos[i], nodePos[边.to], clr, ...);
        }
    }

    // ★ 再画节点（节点在边上面）
    for (每个有效节点 i) {
        QColor clr = 默认蓝;
        if (悬停) clr = 亮色;
        if (起点) clr = 绿;
        if (终点) clr = 红;
        if (路径中) clr = 黄;
        drawNode(p, i, nodePos[i], clr);
    }
}
```

### 10.3 箭头绘制

```cpp
void GraphWidget::drawEdge(QPainter& p, QPointF a, QPointF b, ...) {
    // 计算方向向量
    double dx = b.x() - a.x(), dy = b.y() - a.y();
    double len = std::sqrt(dx*dx + dy*dy);
    double ux = dx/len, uy = dy/len;     // 单位方向

    // 从圆边缘开始画（不减掉 radius 会画到圆心）
    QPointF from(a.x() + ux*radius, a.y() + uy*radius);
    QPointF to(b.x() - ux*radius, b.y() - uy*radius);
    p.drawLine(from, to);

    // 箭头三角形（由单位向量旋转 ±30° 得到）
    double arrowLen = 10;
    QPointF arrow1(to.x() - arrowLen*(ux*0.866 - uy*0.5),
                    to.y() - arrowLen*(uy*0.866 + ux*0.5));
    QPointF arrow2(to.x() - arrowLen*(ux*0.866 + uy*0.5),
                    to.y() - arrowLen*(uy*0.866 - ux*0.5));
    QPointF tri[3] = { to, arrow1, arrow2 };
    p.drawPolygon(tri, 3);

    // 权重标签（边中点处写耗时和费用）
    QPointF mid((from.x()+to.x())/2, (from.y()+to.y())/2);
    p.drawText(QRectF(mid.x()-30, mid.y()-18, 70, 16), ...);
}
```

### 10.4 鼠标交互

```cpp
// 鼠标移动 → 高亮悬停节点
void mouseMoveEvent(QMouseEvent* ev) {
    hoverNode = nodeAtPos(ev->position());  // 计算鼠标在哪个节点上
    update();                                // 触发重绘
    emit nodeHovered(hoverNode, "节点信息");  // 信号通知状态栏
}

// 鼠标点击 → 选中节点（填入起止点下拉框）
void mousePressEvent(QMouseEvent* ev) {
    int id = nodeAtPos(ev->position());
    if (id >= 0) emit nodeClicked(id);
}
```

### 10.5 信号槽（Qt 核心机制）

```cpp
// 信号：画布发出 "节点被点击了"
// 槽：主窗口处理 "把点击的节点填入下拉框"
connect(canvas, &GraphWidget::nodeClicked, this, [this](int id) {
    srcCombo->setCurrentIndex(srcCombo->findText(QString::number(id)));
});
```

**信号槽通俗理解：** "当某件事发生时，自动调用某个函数"。不需要写 `while(true) { if(点击) 处理(); }`，Qt 框架帮你做了这件事。

---

## 11. 测试流程

### 11.1 启动程序

1. 在 Qt Creator 中打开 `CMakeLists.txt`
2. 点击左下角绿色三角形运行
3. 程序启动显示空白画布和暗色主题界面

### 11.2 导入路网数据

1. 点击「导入路网」按钮
2. 选择 `data/network.txt`
3. 画布上出现 25 个节点和路线箭头
4. 下拉框自动填入所有节点

### 11.3 测试最短路径

1. 在下拉框选起点「0 北京总仓」、终点「16 昆明分仓」
2. 点击「最短耗时」
3. 画布高亮路径，日志显示耗时和费用

### 11.4 测试订单

1. 点击「导入订单」→ 选择 `data/orders.txt`
2. 点击「批量规划」→ 自动为每条订单计算最优路径
3. 点击「拓扑排序」→ 检查是否有配送环路
4. 点击「导出方案」→ 保存配送方案到文件

### 11.5 测试异常处理

| 操作 | 预期结果 |
|------|---------|
| 添加空名称节点 | 提示"名称为空" |
| 添加重复编号节点 | 提示"编号必须为 N" |
| 添加自环边 | 提示"不允许自环" |
| 添加负权边 | 提示"权重不能为负" |
| 删除不存在的节点 | 提示"节点不存在" |
| 在无订单时点批量规划 | 提示"当前无订单" |
| 查询不存在的节点 | 提示"节点不存在" |

---

## 12. 答辩要点

### 12.1 老师常问的问题

**Q1: 图的数据结构是什么？**
> 邻接表。每个节点一个出边列表，存在 `DynArray< DynArray<Edge> > neigh` 中。
> `neigh[i]` 就是从节点 i 出发的所有边。空间复杂度 O(N+E)。
> （指 `Graph.h` 第 N 行的 `neigh` 成员变量）

**Q2: 为什么用邻接表不用邻接矩阵？**
> 本项目 25 节点 ~100 边，是稀疏图。邻接表空间 O(N+E)=125，邻接矩阵空间 O(N²)=625。稀疏图用邻接表省空间，遍历邻居也快。
> （指 `Graph.cpp` 第 N 行 `getNeighbors` 函数）

**Q3: 如何存储节点编号的？**
> 顺序编号 0~24，编号即数组下标。这样 `nodes[id]` 直接 O(1) 查到节点，不需要哈希表。
> （指 `Graph.h` 中 `DynArray<Node> nodes` 的声明）

**Q4: 删除节点时做了什么？**
> 1) 清空自己的出边列表；2) 遍历所有其他节点的出边，删除指向自己的边；3) 标记自己为已删除（id=-1）。
> （指 `Graph.cpp` 中 `deleteNode` 函数）

**Q5: Dijkstra 的时间复杂度？**
> O(V²) = 25² = 625 次比较。本项目用了朴素版而不是堆优化版，因为 V 很小，代码更清晰。
> （指 `Dijkstra.cpp` 中查找最小的 for 循环）

**Q6: 如果时间相同，按什么选路径？**
> 本项目不处理这种情况。Dijkstra 用 `<` 严格小于判断，相等时保留先遇到的路径。
> （指 `Dijkstra.cpp` 中 `if (newTime < minTime[e.to])` 这行）

**Q7: 如何回溯路径？**
> 维护 `prev` 数组记录每个节点的前驱。从终点沿着 `prev` 走回起点，再翻转数组。
> （指 `Dijkstra.cpp` 中 `rebuildPath` 函数）

**Q8: 如何检测配送环路？**
> Kahn 拓扑排序。统计入度，反复删除入度为 0 的节点。如果删不完就有环。
> （指 `Dijkstra.cpp` 底部 `TopoSort::sort` 函数）

**Q9: 为什么用自实现 DynArray 不用 STL？**
> 课程要求不能使用 STL，必须自己实现数据结构。DynArray 本质上就是教科书上的顺序表。
> （指 `DynArray.h`）

**Q10: 异常处理做了哪些？**
> 每个 CRUD 操作都检查：空名称、越界编号、重复编号、不存在的节点、自环、负权、重复边。
> （逐一指向 `Graph.cpp` 中各个函数的 if 检查）

### 12.2 快速定位表

| 老师想看什么 | 文件 | 定位方法 |
|-------------|------|---------|
| 顺序表实现 | DynArray.h | `class DynArray` → `expand()` |
| 邻接表结构 | Graph.h | `DynArray< DynArray<Edge> > neigh` |
| 添加节点 | Graph.cpp | `addNode()` |
| 删除节点的边缘清理 | Graph.cpp | `deleteNode()` 中间 forEach |
| Dijkstra 算法 | Dijkstra.cpp | `shortestTime()` |
| 路径回溯 | Dijkstra.cpp | `rebuildPath()` |
| 拓扑排序 | Dijkstra.cpp | `TopoSort::sort()` |
| 文件格式解析 | OrderManager.cpp | `loadNetwork()` |
| 画布绘制 | mainwindow.cpp | `paintEvent()` |
| 暗色主题 | mainwindow.cpp | `applyStyle()` |
| 信号槽连接 | mainwindow.cpp | `setupUI()` 里的 connect |

### 12.3 检查时的注意事项

> **⚠️ 检查时代码不能有注释！** 所以平时写代码时就要做到"看变量名就能懂"：
> - `neigh` 不是 `adj`——一看就知是"邻居表"
> - `minTime` 不是 `dist`——一看就知是"最短耗时"
> - `ruDu` 不是 `deg`——一看就知是"入度"

---

## 13. 代码索引

### 13.1 按功能查找

| 功能 | 文件 | 关键类/函数 |
|------|------|-----------|
| 动态数组 | DynArray.h | `class DynArray<T>` |
| 图的存储 | Graph.h | `class Graph`, `neigh` 成员 |
| 节点 CRUD | Graph.cpp | `addNode()`, `deleteNode()`, `updateNode()`, `findNode()` |
| 边 CRUD | Graph.cpp | `addEdge()`, `deleteEdge()` |
| 最短耗时 | Dijkstra.cpp | `Dijkstra::shortestTime()` |
| 最低费用 | Dijkstra.cpp | `Dijkstra::cheapestPath()` |
| 拓扑排序 | Dijkstra.cpp | `TopoSort::sort()` |
| 订单管理 | OrderManager.h/cpp | `class OrderManager` |
| 文件读写 | OrderManager.cpp | `FileManager::loadNetwork()` 等 |
| 画布 | mainwindow.cpp | `GraphWidget::paintEvent()` |
| 按钮事件 | mainwindow.cpp | `onAddNode()`, `onShortestTime()` 等 |
| 暗色样式 | mainwindow.cpp | `MainWindow::applyStyle()` |
| 异常处理 | Graph.cpp | 各函数入口 `if` 检查 |

### 13.2 按学习顺序

| 顺序 | 内容 | 文件 | 建议时间 |
|------|------|------|---------|
| 1 | 理解动态数组 | DynArray.h | 30 分钟 |
| 2 | 理解邻接表结构 | Graph.h + Graph.cpp | 45 分钟 |
| 3 | 理解 Dijkstra 算法 | Dijkstra.h + Dijkstra.cpp 上半 | 60 分钟 |
| 4 | 理解拓扑排序 | Dijkstra.cpp 下半 | 30 分钟 |
| 5 | 理解文件读写 | OrderManager.cpp | 20 分钟 |
| 6 | 理解 Qt 界面 | mainwindow.cpp | 60 分钟 |
| 7 | 整体调试 | 全部 | 90 分钟 |

---

> **写在最后**
>
> 这个项目的代码量看起来不少，但核心逻辑只有三块：
> 1. **数组**（DynArray）—— 顺序表，大一上就学过
> 2. **邻接表**（Graph）—— 给每个节点配一个出边列表
> 3. **Dijkstra** —— 每次找最近的未确定节点，用它更新邻居
>
> 其余全是这三块的组合和包装。不要被模板语法 `<T>` 吓到——把 `DynArray<int>` 当成 `int数组` 读就行。
>
> 祝你答辩顺利！
