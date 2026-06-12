// ============================================================================
// Dijkstra.cpp —— 最短路径 + 拓扑排序 实现
// ============================================================================
// 算法选择：
//   最短路径：朴素 Dijkstra O(V²)
//     每次手动扫描所有节点，找出"距离最小且未访问"的那个
//     25个节点只需 625 次比较，无需堆优化
//   拓扑排序：Kahn 算法（BFS + 入度）
//     用 DynArray + head 下标模拟队列，不需额外 Queue 类
// ============================================================================

#include "Dijkstra.h"
#include <iostream>

static const double INF = 1e18;     // 代表"无穷远"

// ---- 从 prev 数组回溯路径 ----
static DynArray<int> rebuildPath(const DynArray<int>& prev, int target) {
    DynArray<int> path;
    for (int cur = target; cur != -1; cur = prev[cur])
        path.push_back(cur);

    // 翻转：[终点,...,起点] → [起点,...,终点]
    for (int l = 0, r = path.size() - 1; l < r; ++l, --r) {
        int tmp   = path[l];
        path[l]   = path[r];
        path[r]   = tmp;
    }
    return path;
}

// ================================================================
// shortestTime —— 单源最短耗时（朴素 O(V²) Dijkstra）
// ================================================================
DynArray<PathResult> Dijkstra::shortestTime(const Graph& graph, int start) {
    int N = graph.maxNodeId();               // N = 最大编号+1（即 nodes.size()）
    DynArray<double> minTime;                // 最短耗时
    DynArray<double> minCost;                // 对应费用
    DynArray<int>    prev;                   // 前驱节点
    DynArray<int>    visited;                // 是否已确定最短路径

    // 初始化
    for (int i = 0; i < N; ++i) {
        minTime.push_back(INF);
        minCost.push_back(INF);
        prev.push_back(-1);
        visited.push_back(0);
    }

    if (!graph.hasNode(start)) {
        std::cerr << "[错误] Dijkstra：起点 " << start << " 不存在\n";
        DynArray<PathResult> empty;
        return empty;
    }

    minTime[start] = 0;
    minCost[start] = 0;

    // 主循环：每次确定一个节点的最短路径
    for (int round = 0; round < N; ++round) {
        // 第1步：在所有未确定节点中，找距离最小的
        int    cur  = -1;
        double best = INF;
        for (int j = 0; j < N; ++j) {
            if (!visited[j] && graph.hasNode(j) && minTime[j] < best) {
                best = minTime[j];
                cur  = j;
            }
        }

        if (cur == -1) break;   // 所有剩余节点都不可达

        visited[cur] = 1;

        // 第2步：用 cur 去松弛它的所有邻居
        const DynArray<Edge>& edges = graph.getNeighbors(cur);
        for (int j = 0; j < edges.size(); ++j) {
            const Edge& e   = edges[j];
            double newTime  = minTime[cur] + e.time;
            if (newTime < minTime[e.to]) {
                minTime[e.to] = newTime;
                minCost[e.to] = minCost[cur] + e.cost;
                prev[e.to]    = cur;
            }
        }
    }

    // 构建结果
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

// ================================================================
// cheapestPath —— 两点最低费用（朴素 O(V²) Dijkstra，以 cost 为权）
// ================================================================
PathResult Dijkstra::cheapestPath(const Graph& graph, int start, int target) {
    int N = graph.maxNodeId();
    DynArray<double> dist;
    DynArray<double> totalTime;
    DynArray<int>    prev;
    DynArray<int>    visited;

    for (int i = 0; i < N; ++i) {
        dist.push_back(INF);
        totalTime.push_back(INF);
        prev.push_back(-1);
        visited.push_back(0);
    }

    if (!graph.hasNode(start) || !graph.hasNode(target)) {
        std::cerr << "[错误] cheapestPath：节点不存在\n";
        return PathResult();
    }

    dist[start]      = 0;
    totalTime[start] = 0;

    for (int round = 0; round < N; ++round) {
        int    cur  = -1;
        double best = INF;
        for (int j = 0; j < N; ++j) {
            if (!visited[j] && graph.hasNode(j) && dist[j] < best) {
                best = dist[j];
                cur  = j;
            }
        }

        if (cur == -1 || cur == target) break;

        visited[cur] = 1;

        const DynArray<Edge>& edges = graph.getNeighbors(cur);
        for (int j = 0; j < edges.size(); ++j) {
            const Edge& e  = edges[j];
            double newCost  = dist[cur] + e.cost;
            if (newCost < dist[e.to]) {
                dist[e.to]      = newCost;
                totalTime[e.to] = totalTime[cur] + e.time;
                prev[e.to]      = cur;
            }
        }
    }

    PathResult pr;
    if (dist[target] != INF) {
        pr.reachable  = true;
        pr.totalTime  = totalTime[target];
        pr.totalCost  = dist[target];
        pr.path       = rebuildPath(prev, target);
    }
    return pr;
}

// ================================================================
// TopoSort::sort —— Kahn 拓扑排序（子集版）
// ================================================================
TopoResult TopoSort::sort(const Graph& graph, const DynArray<int>& nodeIds) {
    TopoResult result;
    if (nodeIds.empty()) return result;

    int N = graph.maxNodeId();

    // 1. 建立子集标记表：inSet[id] = 1 表示 id 在子集中
    DynArray<int> inSet;
    for (int i = 0; i < N; ++i) inSet.push_back(0);
    for (int i = 0; i < nodeIds.size(); ++i)
        inSet[nodeIds[i]] = 1;

    // 2. 统计子集内每个节点的入度
    DynArray<int> ruDu;    // 入度
    for (int i = 0; i < N; ++i) ruDu.push_back(0);
    for (int i = 0; i < nodeIds.size(); ++i) {
        int u = nodeIds[i];
        const DynArray<Edge>& edges = graph.getNeighbors(u);
        for (int j = 0; j < edges.size(); ++j) {
            int v = edges[j].to;
            if (inSet[v]) ruDu[v]++;
        }
    }

    // 3. 入度为 0 的入队（用 DynArray + head 模拟队列）
    DynArray<int> queue;
    int head = 0;
    for (int i = 0; i < nodeIds.size(); ++i) {
        int u = nodeIds[i];
        if (ruDu[u] == 0) queue.push_back(u);
    }

    // 4. BFS
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

    // 5. 判环：如果排出的节点数少于子集大小，说明有环
    if (result.order.size() < (int)nodeIds.size()) {
        result.hasCycle = true;

        // 找出哪些节点在环中（不在 order 里的就是）
        DynArray<int> sorted;
        for (int i = 0; i < N; ++i) sorted.push_back(0);
        for (int i = 0; i < result.order.size(); ++i)
            sorted[result.order[i]] = 1;

        for (int i = 0; i < nodeIds.size(); ++i) {
            int u = nodeIds[i];
            if (!sorted[u]) result.cycleNodes.push_back(u);
        }
    }
    return result;
}
