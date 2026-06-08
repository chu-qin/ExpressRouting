#ifndef EXPRESSGRAPH_H
#define EXPRESSGRAPH_H

#include "MyVector.h"
#include "MyQueue.h"

// ============================================================================
// 常量与辅助结构体
// ============================================================================

// INF — 表示两个节点之间没有边（无穷大）
// 为什么是 1e18？
//   1. 足够大：真实运输耗时不会超过 10^6
//   2. 不溢出：double 加法 dist[u] + INF 不会溢出
//   3. 可比较：dist[v] < INF 可以正确判断"是否已找到路径"
const double INF = 1e18;

// Edge — 表示一条有向边
//   from   : 起始节点索引
//   to     : 目标节点索引
//   weight : 边权（运输耗时，单位：小时）
struct Edge {
    int from;
    int to;
    double weight;
};

// ============================================================================
// ExpressGraph — 快递路网有向图（核心算法层）
// ============================================================================
// 数据存储方式：
//   邻接矩阵（Adjacency Matrix）
//     double** mat → mat[i][j] 表示从节点 i 到节点 j 的运输耗时
//     如果 i 到 j 没有直达路线，mat[i][j] = INF
//     因为是「有向图」，mat[i][j] 和 mat[j][i] 是独立设置的
//
// 为什么用邻接矩阵而不是邻接表？
//   1. 节点数 V ≈ 30，矩阵只占 30×30×8 = 7.2KB，完全可接受
//   2. 矩阵 O(1) 查边权，Dijkstra 松弛时极快
//   3. 答辩容易讲：二维数组就是"距离表"，行号=起点，列号=终点
//
// 节点名字存储：
//   char names[MAX_NODES][NAME_LEN] 固定二维数组
//   为什么不动态分配？
//     因为动态分配 char* 数组容易出悬空指针 bug。
//     固定数组 64×64 只需 4KB，安全且简单。
// ============================================================================

class ExpressGraph {
private:
    static const int MAX_NODES = 64;   // 最大节点数（设计上限）
    static const int NAME_LEN  = 64;   // 每个名字最多 63 个字符

    int numNodes;          // 当前实际节点数
    int cap;               // 矩阵当前容量（>= numNodes）
    double** mat;          // 邻接矩阵：mat[from][to] = 边权 或 INF
    char names[MAX_NODES][NAME_LEN];  // 索引 → 名字
    double* posX;          // 索引 → 绘图 X 坐标
    double* posY;          // 索引 → 绘图 Y 坐标

    // ---- 内部工具函数 ----

    // 安全拷贝字符串（带长度限制，防止溢出）
    void cpStr(char* dst, const char* src) {
        int i = 0;
        while (src[i] && i < NAME_LEN - 1) {
            dst[i] = src[i];
            i++;
        }
        dst[i] = '\0';    // 保证以 \0 结尾
    }

    // 分配 n×n 矩阵 + 坐标数组，全部初始化为 INF / 0
    void alloc(int n) {
        mat = new double*[n];
        for (int i = 0; i < n; i++) {
            mat[i] = new double[n];
            for (int j = 0; j < n; j++) {
                mat[i][j] = INF;      // 初始：所有节点间都不连通
            }
        }
        posX = new double[n];
        posY = new double[n];
        for (int i = 0; i < n; i++) {
            posX[i] = 0;
            posY[i] = 0;
        }
    }

    // 释放矩阵 + 坐标数组
    void freeMat() {
        if (mat) {
            for (int i = 0; i < cap; i++) {
                delete[] mat[i];      // 先释放每一行
            }
            delete[] mat;             // 再释放行指针数组
            mat = nullptr;
        }
        if (posX) { delete[] posX; posX = nullptr; }
        if (posY) { delete[] posY; posY = nullptr; }
    }

    // 扩容：申请更大的矩阵，保留原有数据
    void expand(int newCap) {
        double** newMat = new double*[newCap];
        double*  newX   = new double[newCap];
        double*  newY   = new double[newCap];

        // 初始化新矩阵
        for (int i = 0; i < newCap; i++) {
            newMat[i] = new double[newCap];
            for (int j = 0; j < newCap; j++) {
                newMat[i][j] = INF;
            }
            newX[i] = 0;
            newY[i] = 0;
        }

        // 拷贝旧数据到新矩阵
        for (int i = 0; i < numNodes; i++) {
            newX[i] = posX[i];
            newY[i] = posY[i];
            for (int j = 0; j < numNodes; j++) {
                newMat[i][j] = mat[i][j];
            }
        }

        freeMat();          // 释放旧矩阵
        mat  = newMat;
        posX = newX;
        posY = newY;
        cap  = newCap;
    }

public:
    // ---- 构造 / 析构 ----

    ExpressGraph() {
        numNodes = 0;
        cap      = MAX_NODES;
        mat      = nullptr;
        posX     = nullptr;
        posY     = nullptr;
        alloc(cap);         // 预分配 64×64 矩阵
    }

    ~ExpressGraph() {
        freeMat();
    }

    // ================================================================
    // 节点 CRUD（增删改查）
    // ================================================================

    // 添加节点：返回新节点索引
    int addNode(const char* name, double x, double y) {
        if (numNodes >= cap) expand(cap * 2);
        int idx = numNodes;
        cpStr(names[idx], name);    // 拷贝名字（防止外部指针失效）
        posX[idx] = x;
        posY[idx] = y;
        // 新行/列初始化为 INF（不与任何节点相连）
        for (int j = 0; j <= numNodes; j++) {
            mat[idx][j] = INF;
            mat[j][idx] = INF;
        }
        numNodes++;
        return idx;
    }

    // 删除节点：把后面的行/列向前移动，覆盖掉被删的索引
    bool removeNode(int idx) {
        if (idx < 0 || idx >= numNodes) return false;

        // 名字、坐标、矩阵行 前移
        for (int i = idx; i < numNodes - 1; i++) {
            cpStr(names[i], names[i + 1]);     // 名字前移
            posX[i] = posX[i + 1];             // X 坐标前移
            posY[i] = posY[i + 1];             // Y 坐标前移
            for (int j = 0; j < numNodes; j++) {
                mat[i][j] = mat[i + 1][j];     // 矩阵行前移
            }
        }
        // 矩阵列前移
        for (int j = idx; j < numNodes - 1; j++) {
            for (int i = 0; i < numNodes - 1; i++) {
                mat[i][j] = mat[i][j + 1];     // 矩阵列前移
            }
        }
        numNodes--;
        return true;
    }

    // 修改节点信息
    bool updateNode(int idx, const char* newName, double newX, double newY) {
        if (idx < 0 || idx >= numNodes) return false;
        cpStr(names[idx], newName);
        posX[idx] = newX;
        posY[idx] = newY;
        return true;
    }

    // 按名称查找节点索引（顺序遍历，O(V)）
    // 因为 V≈30，线性查找完全够用，不需要手写哈希表
    int queryNode(const char* name) const {
        for (int i = 0; i < numNodes; i++) {
            const char* a = names[i];
            const char* b = name;
            bool same = true;
            while (*a && *b) {
                if (*a != *b) { same = false; break; }
                a++; b++;
            }
            if (same && *a == '\0' && *b == '\0') return i;
        }
        return -1;  // 未找到
    }

    // 获取节点名称（返回内部存储的常量指针，安全）
    const char* getNodeName(int idx) const {
        if (idx < 0 || idx >= numNodes) return nullptr;
        return names[idx];
    }

    int  getNodeCount()           const { return numNodes; }
    double getNodeX(int idx) const { return (idx >= 0 && idx < numNodes) ? posX[idx] : 0; }
    double getNodeY(int idx) const { return (idx >= 0 && idx < numNodes) ? posY[idx] : 0; }

    // ================================================================
    // 边 CRUD
    // ================================================================

    // 添加有向边：from → to，权重 weight
    // 如果是双向路，需要调用两次（A→B 和 B→A）
    bool addEdge(int from, int to, double weight) {
        if (from < 0 || from >= numNodes) return false;
        if (to   < 0 || to   >= numNodes) return false;
        if (weight < 0) return false;     // 运费不能为负
        mat[from][to] = weight;
        return true;
    }

    // 删除有向边（设置为 INF = 不连通）
    bool removeEdge(int from, int to) {
        if (from < 0 || from >= numNodes) return false;
        if (to   < 0 || to   >= numNodes) return false;
        mat[from][to] = INF;
        return true;
    }

    double getWeight(int from, int to) const {
        if (from < 0 || from >= numNodes || to < 0 || to >= numNodes) return INF;
        return mat[from][to];
    }

    bool hasEdge(int from, int to) const {
        return getWeight(from, to) != INF;
    }

    // ================================================================
    // Dijkstra 单源最短路径（朴素 O(V²) 实现）
    // ================================================================
    // 算法核心思想：
    //   维护三个数组：
    //     dist[i]    — 从起点到 i 的当前最短距离
    //     prev[i]    — 最短路径上 i 的前驱节点（用于重建路径）
    //     visited[i] — i 是否已确定最短距离
    //
    //   每轮从未访问节点中选 dist 最小的 u，
    //   标记 u 为已访问，然后用 u 作为"跳板"去更新它的邻居：
    //     if (dist[u] + w(u,v) < dist[v])   ← 这就是"松弛"操作
    //        dist[v] = dist[u] + w(u,v)
    //        prev[v] = u
    //
    // 参数：
    //   start   — 起点索引
    //   end     — 终点索引
    //   outCost — 输出参数，存放最短路径总耗时
    // 返回：
    //   最短路径经过的节点索引序列 [start, ..., end]
    //   不可达时返回空 MyVector
    // ================================================================
    MyVector<int> dijkstra(int start, int end, double& outCost) {
        MyVector<int> empty;
        outCost = INF;

        // 边界检查
        if (start < 0 || start >= numNodes) return empty;
        if (end   < 0 || end   >= numNodes) return empty;

        // 起点=终点：直接返回
        if (start == end) {
            outCost = 0;
            empty.push_back(start);
            return empty;
        }

        // 三个辅助数组（new 在堆上分配，因为 numNodes 是运行时变量）
        double* dist    = new double[numNodes];
        int*    prev    = new int[numNodes];
        bool*   visited = new bool[numNodes];

        // 初始化
        for (int i = 0; i < numNodes; i++) {
            dist[i]    = INF;
            prev[i]    = -1;       // -1 表示没有前驱
            visited[i] = false;
        }
        dist[start] = 0;           // 起点到自己的距离为 0

        // 主循环：最多 numNodes 轮
        for (int k = 0; k < numNodes; k++) {
            // ---- 第一步：找当前未访问节点中 dist 最小的 u ----
            int    u     = -1;
            double minD  = INF;
            for (int i = 0; i < numNodes; i++) {
                if (!visited[i] && dist[i] < minD) {
                    minD = dist[i];
                    u    = i;
                }
            }
            if (u == -1) break;    // 所有可达节点都已访问

            // ---- 第二步：标记 u 为已访问 ----
            visited[u] = true;

            // ---- 第三步：用 u 松弛所有邻居 v ----
            for (int v = 0; v < numNodes; v++) {
                if (!visited[v] && mat[u][v] != INF) {
                    double nd = dist[u] + mat[u][v];   // 经过 u 到 v 的距离
                    if (nd < dist[v]) {                 // 如果更短，更新
                        dist[v] = nd;
                        prev[v] = u;                    // 记录 v 是从 u 来的
                    }
                }
            }
        }

        outCost = dist[end];

        // 不可达
        if (dist[end] == INF) {
            delete[] dist; delete[] prev; delete[] visited;
            return empty;   // 返回空 MyVector
        }

        // ---- 重建路径：从 end 沿 prev[] 回溯到 start ----
        MyVector<int> path;
        int cur = end;
        while (cur != -1) {
            path.push_back(cur);    // 此时是逆序 [end, ..., start]
            cur = prev[cur];
        }

        // 翻转为正序 [start, ..., end]
        int n = path.getSize();
        for (int i = 0; i < n / 2; i++) {
            int tmp = path[i];
            path[i] = path[n - 1 - i];
            path[n - 1 - i] = tmp;
        }

        delete[] dist; delete[] prev; delete[] visited;
        return path;
    }

    // ================================================================
    // 拓扑排序（Kahn 算法）— 用于批次配送顺序规划
    // ================================================================
    // 算法核心思想：
    //   1. 统计每个节点的入度（有多少条边指向它）
    //   2. 把入度=0 的节点放入队列（它们是一开始就能配送的）
    //   3. 出队一个节点，把它指向的邻居入度-1
    //      如果某个邻居入度减到 0，说明它的前置依赖全解决了，入队
    //   4. 重复直到队列为空
    //   5. 如果出队数量 < 总节点数 → 存在环路（循环依赖）
    //
    // 参数：
    //   deps     — 依赖边列表（Edge.from 必须先于 Edge.to）
    //   hasCycle — 输出参数，true 表示存在环路
    // 返回：
    //   拓扑排序结果（节点索引序列），有环时返回空
    // ================================================================
    MyVector<int> topologicalSort(const MyVector<Edge>& deps, bool& hasCycle) {
        MyVector<int> order;
        hasCycle = false;

        // 统计入度
        int* indegree = new int[numNodes];
        for (int i = 0; i < numNodes; i++) indegree[i] = 0;

        for (int i = 0; i < deps.getSize(); i++) {
            int t = deps[i].to;
            if (t >= 0 && t < numNodes) indegree[t]++;
        }

        // 入度为 0 的节点入队
        MyQueue<int> q;
        for (int i = 0; i < numNodes; i++) {
            if (indegree[i] == 0) q.push(i);
        }

        // 不断出队、减度、入队
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            order.push_back(u);              // u 加入拓扑序列

            for (int i = 0; i < deps.getSize(); i++) {
                if (deps[i].from == u) {      // u → v 的依赖
                    int v = deps[i].to;
                    indegree[v]--;            // 解除一个前置依赖
                    if (indegree[v] == 0) {   // 所有前置都解决了
                        q.push(v);
                    }
                }
            }
        }

        // 判环：如果出队数 ≠ 节点总数，说明有环
        hasCycle = (order.getSize() != numNodes);
        if (hasCycle) order.clear();

        delete[] indegree;
        return order;
    }
};

#endif
