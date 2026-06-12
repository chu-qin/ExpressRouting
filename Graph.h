#pragma once
// ============================================================================
// Graph.h —— 有向加权图（邻接表实现）
// ============================================================================
// 存储结构：
//   nodes[i]  = i号节点的信息（Node结构体）
//   neigh[i]  = 从i号节点出发的所有边（Edge列表）
//
// 节点编号规则：顺序分配 0,1,2,...，编号即数组下标，不需要 HashMap
// 删除节点后该位置留空（id设为-1），不重用编号，不影响其他节点
// ============================================================================

#include "DynArray.h"
#include <string>

// ---- 节点（快递网点）----
struct Node {
    int         id   = 0;
    std::string name;          // 网点名称
    std::string address;       // 地址
    double      lon   = 0;     // 经度（画布显示用）
    double      lat   = 0;     // 纬度（画布显示用）

    Node() = default;
    Node(int i, const std::string& n, const std::string& a,
         double lo = 0, double la = 0)
        : id(i), name(n), address(a), lon(lo), lat(la) {}
};

// ---- 有向边（运输线路）----
struct Edge {
    int    to   = 0;           // 目标节点编号
    double time = 0;           // 运输耗时（小时）
    double cost = 0;           // 运输费用（元）

    Edge() = default;
    Edge(int t, double ti, double c) : to(t), time(ti), cost(c) {}
};

// ---- 图 ----
class Graph {
public:
    // 节点管理
    bool  addNode(const Node& node);
    bool  deleteNode(int id);
    bool  updateNode(int id, const Node& node);
    Node*       findNode(int id);
    const Node* findNode(int id) const;
    DynArray<int> getAllNodeIds() const;

    // 边管理
    bool addEdge(int from, int to, double time, double cost);
    bool deleteEdge(int from, int to);

    // 查询
    const DynArray<Edge>& getNeighbors(int id) const;
    bool hasNode(int id) const;
    int  nodeCount() const;
    int  edgeCount() const;
    int  maxNodeId()   const;     // 已分配的最大编号

    void clear();

private:
    DynArray<Node>            nodes;      // nodes[i] = i号节点
    DynArray< DynArray<Edge> > neigh;     // neigh[i] = i号节点的出边列表

    // 非 const 版本的 getNeighbors
    DynArray<Edge>& neighborsOf(int id);

    // getNeighbors 查不到时返回的空列表
    static const DynArray<Edge> emptyList;
};
