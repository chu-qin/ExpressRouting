#pragma once
// ============================================================================
// Dijkstra.h — 最短路径算法（堆优化版） + 拓扑排序
// ============================================================================
// 包含两个核心算法：
//   Dijkstra::shortestTimeFrom() — 单源最短耗时（O((V+E)logV) 堆优化）
//   Dijkstra::cheapestPath()     — 两点最低费用路径
//   TopoSort::sort()             — Kahn 拓扑排序（判环 + 排序）
//
// Dijkstra 堆优化 vs 朴素版：
//   朴素版每轮 O(V) 找最小 dist 节点，总复杂度 O(V²)
//   堆优化用 MinHeap 维护"候选节点"，每次取堆顶 O(log V)，总复杂度 O((V+E)logV)
//   懒惰删除：不在堆中更新旧记录，而是 push 新记录 + pop 时跳过过时记录
// ============================================================================

#include "Graph.h"

// ---- 路径结果 ----
struct PathResult {
    DynArray<int> path;         // 路径节点序列 [src, ..., dst]
    double totalTime = 0;       // 总耗时（小时）
    double totalCost = 0;       // 总费用（元）
    bool   reachable = false;   // 是否可达
};

// ---- 拓扑排序结果 ----
struct TopoResult {
    bool          hasCycle = false;   // 是否有环路
    DynArray<int> order;             // 拓扑序列
    DynArray<int> cycleNodes;        // 环路涉及的节点（hasCycle=true 时有值）
};

// ============================================================================
// Dijkstra — 最短路径算法
// ============================================================================
class Dijkstra {
public:
    // 单源最短耗时：计算从 src 到所有可达节点的路径
    // 返回 HashMap<节点编号, PathResult>
    static HashMap<PathResult> shortestTimeFrom(const Graph& g, int src);

    // 两点最低费用：计算从 src 到 dst 的费用最优路径
    static PathResult cheapestPath(const Graph& g, int src, int dst);
};

// ============================================================================
// TopoSort — Kahn 拓扑排序
// ============================================================================
// 算法步骤：
//   1. 对给定节点子集 ids[]，统计入度（只看 ids 内部的边）
//   2. 入度为 0 的节点入队
//   3. 出队 → 加入序列 → 邻居入度-1 → 入度=0 的新节点入队
//   4. 循环直到队列为空
//   5. 如果出队数 < ids.size()：存在环路，剩余入度>0 的节点就是环中节点
//
// 为什么只对子集排序？
//   实际场景中，一次批次配送只涉及部分网点（订单的起终点）。
//   对所有 25 个网点全局排序意义不大——只对相关节点排序更高效。
// ============================================================================
class TopoSort {
public:
    static TopoResult sort(const Graph& g, const DynArray<int>& ids);
};
