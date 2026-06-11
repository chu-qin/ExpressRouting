#pragma once
// ============================================================================
// Graph.h — 有向加权图（邻接表实现）
// ============================================================================
// nodes: HashMap<Node>            — 节点映射
// adj:   HashMap<DynArray<Edge>>  — 邻接表
// ============================================================================

#include "Containers.h"
#include <string>

// ---- 节点 ----
struct Node {
    int id = 0;
    std::string name;       // 城市名（如"北京总仓"）
    std::string address;    // 地址
    double lon = 0;         // 经度
    double lat = 0;         // 纬度

    Node() = default;
    Node(int nodeId, const std::string& nodeName, const std::string& nodeAddr,
         double nodeLon = 0, double nodeLat = 0)
        : id(nodeId), name(nodeName), address(nodeAddr), lon(nodeLon), lat(nodeLat) {}
};

// ---- 有向边 ----
struct Edge {
    int    from = 0, to = 0;
    double time = 0;        // 运输耗时（小时）
    double cost = 0;        // 运输费用（元）

    Edge() = default;
    Edge(int edgeFrom, int edgeTo, double edgeTime, double edgeCost)
        : from(edgeFrom), to(edgeTo), time(edgeTime), cost(edgeCost) {}
};

// ---- 图 ----
class Graph {
public:
    // 节点 CRUD
    bool  addNode(const Node& node);
    bool  deleteNode(int id);
    bool  updateNode(int id, const Node& node);
    Node*       findNode(int id);
    const Node* findNode(int id) const;

    // 边 CRUD
    bool addEdge(const Edge& edge);
    bool deleteEdge(int from, int to);

    // 查询
    const DynArray<Edge>& getNeighbors(int id) const;
    DynArray<int>         getAllNodeIds() const;
    bool hasNode(int id) const;
    int  nodeCount() const;
    int  edgeCount() const;
    void clear();

private:
    HashMap<Node>           nodes;           // 节点表
    HashMap<DynArray<Edge>> adj;             // 邻接表
    static const DynArray<Edge> emptyAdj;    // 空邻接表引用
};
