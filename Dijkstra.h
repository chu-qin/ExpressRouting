#pragma once
// ============================================================================
// Dijkstra.h —— 最短路径算法 + 拓扑排序
// ============================================================================

#include "Graph.h"

// ---- 路径结果 ----
struct PathResult {
    DynArray<int> path;         // 路径节点序列（起点→终点）
    double        totalTime = 0;
    double        totalCost = 0;
    bool          reachable = false;
};

// ---- 拓扑排序结果 ----
struct TopoResult {
    bool          hasCycle   = false;
    DynArray<int> order;             // 拓扑序列
    DynArray<int> cycleNodes;        // 环路涉及的节点
};

// ============================================================================
// Dijkstra
// ============================================================================
class Dijkstra {
public:
    // 单源最短耗时：返回从 start 到所有节点的最短路径
    static DynArray<PathResult> shortestTime(const Graph& graph, int start);

    // 两点最低费用：返回从 start 到 target 的最低费用路径
    static PathResult cheapestPath(const Graph& graph, int start, int target);
};

// ============================================================================
// TopoSort —— Kahn 拓扑排序（仅考虑指定子集的节点）
// ============================================================================
class TopoSort {
public:
    static TopoResult sort(const Graph& graph, const DynArray<int>& nodeIds);
};
