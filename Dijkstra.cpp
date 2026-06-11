// ============================================================================
// Dijkstra.cpp — 最短路径算法 + 拓扑排序 实现
// ============================================================================

#include "Dijkstra.h"
#include <iostream>

// 极大值代表"无穷远"
static const double INF = 1e18;

// ---- 堆元素：(距离/费用, 节点编号) ----
struct HeapItem { double dist; int node; };
struct HeapItemCmp { bool operator()(const HeapItem& a, const HeapItem& b) const { return a.dist < b.dist; } };

// ---- 从 prev 回溯重建路径 ----
static DynArray<int> rebuildPath(const HashMap<int>& prev, int target) {
    DynArray<int> path;
    for (int cur = target; cur != -1; ) {
        path.push_back(cur);
        const int* pp = prev.find(cur);
        cur = pp ? *pp : -1;
    }
    // 翻转：[终点, ..., 起点] → [起点, ..., 终点]
    for (int l = 0, r = path.size() - 1; l < r; ++l, --r) {
        int temp = path[l]; path[l] = path[r]; path[r] = temp;
    }
    return path;
}

// ---- 初始化 dist / aux / prev ----
static void initMaps(const DynArray<int>& ids,
                     HashMap<double>& dist, HashMap<double>& aux,
                     HashMap<int>& prev) {
    for (int i = 0; i < ids.size(); ++i) {
        dist[ids[i]] = INF;
        aux[ids[i]] = INF;
        prev[ids[i]] = -1;
    }
}

// ================================================================
// shortestTimeFrom — 单源最短耗时
// ================================================================
HashMap<PathResult> Dijkstra::shortestTimeFrom(const Graph& graph, int start) {
    if (!graph.hasNode(start)) {
        std::cerr << "[错误] Dijkstra 起点 " << start << " 不存在\n";
        return {};
    }

    DynArray<int> ids = graph.getAllNodeIds();
    HashMap<double> minTime;    // 最短耗时
    HashMap<double> minCost;    // 对应费用
    HashMap<int>    prev;
    initMaps(ids, minTime, minCost, prev);
    minTime[start] = minCost[start] = 0;

    MinHeap<HeapItem, HeapItemCmp> heap;
    heap.push({ 0.0, start });

    while (!heap.empty()) {
        HeapItem cur = heap.top(); heap.pop();

        // 惰性删除：堆顶距离 > 已知最短 → 过时记录
        double* curDist = minTime.find(cur.node);
        if (!curDist || cur.dist > *curDist) continue;

        // 松弛邻居
        for (const Edge& edge : graph.getNeighbors(cur.node)) {
            double newDist = *curDist + edge.time;
            double* oldDist = minTime.find(edge.to);
            if (oldDist && newDist < *oldDist) {
                *oldDist = newDist;
                *minCost.find(edge.to) = *minCost.find(cur.node) + edge.cost;
                prev[edge.to] = cur.node;
                heap.push({ newDist, edge.to });
            }
        }
    }

    // 构建结果
    HashMap<PathResult> result;
    for (int i = 0; i < ids.size(); ++i) {
        PathResult pr;
        double* d = minTime.find(ids[i]);
        if ((pr.reachable = (d && *d != INF))) {
            pr.totalTime = *d;
            pr.totalCost = *minCost.find(ids[i]);
            pr.path = rebuildPath(prev, ids[i]);
        }
        result.set(ids[i], pr);
    }
    return result;
}

// ================================================================
// cheapestPath — 两点最低费用
// ================================================================
PathResult Dijkstra::cheapestPath(const Graph& graph, int start, int target) {
    if (!graph.hasNode(start) || !graph.hasNode(target)) {
        std::cerr << "[错误] cheapestPath：节点不存在\n";
        return {};
    }

    DynArray<int> ids = graph.getAllNodeIds();
    HashMap<double> minCost;    // 最低费用
    HashMap<double> totalTime;  // 对应耗时
    HashMap<int>    prev;
    initMaps(ids, minCost, totalTime, prev);
    minCost[start] = totalTime[start] = 0;

    MinHeap<HeapItem, HeapItemCmp> heap;
    heap.push({ 0.0, start });

    while (!heap.empty()) {
        HeapItem cur = heap.top(); heap.pop();
        double* curDist = minCost.find(cur.node);
        if (!curDist || cur.dist > *curDist) continue;

        for (const Edge& edge : graph.getNeighbors(cur.node)) {
            double newDist = *curDist + edge.cost;
            double* oldDist = minCost.find(edge.to);
            if (oldDist && newDist < *oldDist) {
                *oldDist = newDist;
                *totalTime.find(edge.to) = *totalTime.find(cur.node) + edge.time;
                prev[edge.to] = cur.node;
                heap.push({ newDist, edge.to });
            }
        }
    }

    PathResult pr;
    double* d = minCost.find(target);
    if ((pr.reachable = (d && *d != INF))) {
        pr.totalCost = *d;
        pr.totalTime = *totalTime.find(target);
        pr.path = rebuildPath(prev, target);
    }
    return pr;
}

// ================================================================
// TopoSort::sort — Kahn 拓扑排序
// ================================================================
TopoResult TopoSort::sort(const Graph& graph, const DynArray<int>& nodeIds) {
    TopoResult result;
    if (nodeIds.empty()) return result;

    // 1. 建立子集查找表
    HashMap<bool> inSubSet;
    for (int i = 0; i < nodeIds.size(); ++i)
        inSubSet.set(nodeIds[i], true);

    // 2. 统计子集内入度
    HashMap<int> ruDu;    // 入度（拼音更直观）
    for (int i = 0; i < nodeIds.size(); ++i) ruDu[nodeIds[i]] = 0;
    for (int i = 0; i < nodeIds.size(); ++i)
        for (const Edge& edge : graph.getNeighbors(nodeIds[i]))
            if (inSubSet.contains(edge.to)) ++ruDu[edge.to];

    // 3. 入度为 0 的入队
    Queue<int> queue;
    for (int i = 0; i < nodeIds.size(); ++i)
        if (ruDu[nodeIds[i]] == 0) queue.push(nodeIds[i]);

    // 4. BFS
    while (!queue.empty()) {
        int node = queue.front(); queue.pop();
        result.order.push_back(node);
        for (const Edge& edge : graph.getNeighbors(node))
            if (inSubSet.contains(edge.to) && --ruDu[edge.to] == 0)
                queue.push(edge.to);
    }

    // 5. 判环
    if (result.order.size() < nodeIds.size()) {
        result.hasCycle = true;
        HashMap<bool> sorted;
        for (int i = 0; i < result.order.size(); ++i)
            sorted.set(result.order[i], true);
        for (int i = 0; i < nodeIds.size(); ++i)
            if (!sorted.contains(nodeIds[i]))
                result.cycleNodes.push_back(nodeIds[i]);
    }
    return result;
}
