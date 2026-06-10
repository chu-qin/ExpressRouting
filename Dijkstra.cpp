// ============================================================================
// Dijkstra.cpp — 最短路径算法 + 拓扑排序 实现
// ============================================================================
// 堆优化 Dijkstra 核心：
//   不实现 decrease-key（在二叉堆中降低已存在元素的优先级很复杂），
//   改为"懒惰删除"策略：
//     - 每次松弛时直接 push 新的 (距离, 节点) 进入堆
//     - pop 时检查：如果堆顶记录的 dist > 当前已知最短距离，跳过这条旧记录
//     - 每条边最多产生一次 push，总复杂度 O((V+E)logE) ≈ O((V+E)logV)
//
// 双权处理：
//   最短耗时查询 → 以 time 为第一比较维度，cost 作为辅助记录
//   最低费用查询 → 以 cost 为第一比较维度，time 作为辅助记录
//   两条 Dijkstra 完全相同，只是比较的权重维度不同
// ============================================================================

#include "Dijkstra.h"
#include <limits>
#include <iostream>

using std::string;
using std::to_string;

static const double INF = std::numeric_limits<double>::infinity();

// ---- 堆中存储的元素： (距离/费用, 节点编号) ----
struct DE { double d; int n; };
struct DECmp { bool operator()(const DE& a, const DE& b) const { return a.d < b.d; } };

// ---- 从 prev 数组回溯重建路径 ----
// prev[cur] 记录了"到达 cur 的前一个节点是谁"
// 从 dst 开始沿着 prev 倒推回 src，得到逆序路径，然后翻转
static DynArray<int> tracePath(const HashMap<int, int>& prev, int dst) {
    DynArray<int> p;
    for (int cur = dst; cur != -1; ) {
        p.push_back(cur);
        const int* pp = prev.find(cur);   // 查找 cur 的前驱
        cur = pp ? *pp : -1;
    }
    // 翻转：当前是 [dst, ..., src] → 翻成 [src, ..., dst]
    for (int l = 0, r = p.size() - 1; l < r; ++l, --r) {
        int t = p[l]; p[l] = p[r]; p[r] = t;
    }
    return p;
}

// ---- 初始化 dist / cost / prev ----
static void initMaps(const DynArray<int>& ids,
                     HashMap<int, double>& distA, HashMap<int, double>& distB,
                     HashMap<int, int>& prev) {
    for (int i = 0; i < ids.size(); ++i) {
        distA[ids[i]] = INF;
        distB[ids[i]] = INF;
        prev[ids[i]] = -1;
    }
}

// ================================================================
// shortestTimeFrom — 单源最短耗时（堆优化）
// ================================================================
HashMap<int, PathResult> Dijkstra::shortestTimeFrom(const Graph& g, int src) {
    if (!g.hasNode(src)) {
        std::cerr << "[错误] Dijkstra 起点 " << src << " 不存在\n";
        return {};
    }

    DynArray<int> ids = g.getAllNodeIds();
    HashMap<int, double> dist;   // 最短耗时
    HashMap<int, double> cost;   // 对应路径的费用（辅助）
    HashMap<int, int>    prev;   // 前驱节点
    initMaps(ids, dist, cost, prev);
    dist[src] = cost[src] = 0;

    MinHeap<DE, DECmp> pq;
    pq.push({ 0.0, src });

    while (!pq.empty()) {
        DE top = pq.top(); pq.pop();

        // 惰性删除：如果堆顶距离 > 已知最短距离，说明这条记录已过时
        double* dp = dist.find(top.n);
        if (!dp || top.d > *dp) continue;

        // 松弛所有邻居
        for (const Edge& e : g.getNeighbors(top.n)) {
            double nd = *dp + e.time;      // 经过 top.n 到 e.to 的新耗时
            double* dv = dist.find(e.to);
            if (dv && nd < *dv) {          // 找到更短路径
                *dv = nd;
                *cost.find(e.to) = *cost.find(top.n) + e.cost;
                prev[e.to] = top.n;
                pq.push({ nd, e.to });     // push 新记录（旧记录变为惰性删除）
            }
        }
    }

    // 构建返回结果
    HashMap<int, PathResult> res;
    for (int i = 0; i < ids.size(); ++i) {
        PathResult pr;
        double* dv = dist.find(ids[i]);
        if ((pr.reachable = (dv && *dv != INF))) {
            pr.totalTime = *dv;
            pr.totalCost = *cost.find(ids[i]);
            pr.path = tracePath(prev, ids[i]);
        }
        res.set(ids[i], pr);
    }
    return res;
}

// ================================================================
// cheapestPath — 两点最低费用路径
// ================================================================
PathResult Dijkstra::cheapestPath(const Graph& g, int src, int dst) {
    if (!g.hasNode(src) || !g.hasNode(dst)) {
        std::cerr << "[错误] cheapestPath：节点不存在\n";
        return {};
    }

    DynArray<int> ids = g.getAllNodeIds();
    HashMap<int, double> dist;   // 最低费用（主优化目标）
    HashMap<int, double> tm;     // 对应路径的耗时（辅助记录）
    HashMap<int, int>    prev;
    initMaps(ids, dist, tm, prev);
    dist[src] = tm[src] = 0;

    MinHeap<DE, DECmp> pq;
    pq.push({ 0.0, src });

    while (!pq.empty()) {
        DE top = pq.top(); pq.pop();
        double* dp = dist.find(top.n);
        if (!dp || top.d > *dp) continue;  // 惰性删除

        for (const Edge& e : g.getNeighbors(top.n)) {
            double nd = *dp + e.cost;      // 这次比较的是费用
            double* dv = dist.find(e.to);
            if (dv && nd < *dv) {
                *dv = nd;
                *tm.find(e.to) = *tm.find(top.n) + e.time;
                prev[e.to] = top.n;
                pq.push({ nd, e.to });
            }
        }
    }

    PathResult pr;
    double* dv = dist.find(dst);
    if ((pr.reachable = (dv && *dv != INF))) {
        pr.totalCost = *dv;
        pr.totalTime = *tm.find(dst);
        pr.path = tracePath(prev, dst);
    }
    return pr;
}

// ================================================================
// TopoSort::sort — Kahn 拓扑排序（子集版本）
// ================================================================
TopoResult TopoSort::sort(const Graph& g, const DynArray<int>& ids) {
    TopoResult res;
    if (ids.empty()) return res;

    // 1. 建立 ids 子集的快速查找表
    HashMap<int, bool> inSet;
    for (int i = 0; i < ids.size(); ++i)
        inSet.set(ids[i], true);

    // 2. 统计 ids 子集内各节点的入度（只看 ids 内部的边）
    HashMap<int, int> deg;
    for (int i = 0; i < ids.size(); ++i) deg[ids[i]] = 0;
    for (int i = 0; i < ids.size(); ++i)
        for (const Edge& e : g.getNeighbors(ids[i]))
            if (inSet.contains(e.to)) ++deg[e.to];

    // 3. 入度为 0 的节点入队
    Queue<int> q;
    for (int i = 0; i < ids.size(); ++i)
        if (deg[ids[i]] == 0) q.push(ids[i]);

    // 4. BFS：不断出队 → 减邻居入度 → 入度=0 入队
    while (!q.empty()) {
        int u = q.front(); q.pop();
        res.order.push_back(u);
        for (const Edge& e : g.getNeighbors(u))
            if (inSet.contains(e.to) && --deg[e.to] == 0)
                q.push(e.to);
    }

    // 5. 判环
    if (res.order.size() < ids.size()) {
        res.hasCycle = true;
        // 入度仍 > 0 的节点就是环中节点
        HashMap<int, bool> done;
        for (int i = 0; i < res.order.size(); ++i)
            done.set(res.order[i], true);
        for (int i = 0; i < ids.size(); ++i)
            if (!done.contains(ids[i]))
                res.cycleNodes.push_back(ids[i]);
    }
    return res;
}
