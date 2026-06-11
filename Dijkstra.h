#pragma once
// ============================================================================
// Dijkstra.h — 最短路径算法（堆优化） + 拓扑排序
// ============================================================================

#include "Graph.h"

// ---- 路径结果 ----
struct PathResult {
    DynArray<int> path;         // [起点, ..., 终点]
    double totalTime = 0;       // 总耗时（小时）
    double totalCost = 0;       // 总费用（元）
    bool   reachable = false;   // 是否可达
};

// ---- 拓扑排序结果 ----
struct TopoResult {
    bool          hasCycle = false;   // 是否有环路
    DynArray<int> order;             // 拓扑序列
    DynArray<int> cycleNodes;        // 环路涉及节点
};

// ============================================================================
// Dijkstra — 最短路径
// ============================================================================
class Dijkstra {
public:
    // 单源最短耗时
    static HashMap<PathResult> shortestTimeFrom(const Graph& graph, int start);

    // 两点最低费用
    static PathResult cheapestPath(const Graph& graph, int start, int target);
};

// ============================================================================
// TopoSort — Kahn 拓扑排序（子集版）
// ============================================================================
class TopoSort {
public:
    static TopoResult sort(const Graph& graph, const DynArray<int>& nodeIds);
};
