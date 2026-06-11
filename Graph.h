#pragma once
// ============================================================================
// Graph.h — 有向加权图（邻接表实现）
// ============================================================================
// 存储结构：
//   nodes: HashMap<Node>               — 节点映射（编号 → 节点信息）
//   adj:   HashMap<DynArray<Edge>>     — 邻接表（节点 → 出边列表）
//
// 为什么用邻接表而不是邻接矩阵？
//   1. HashMap O(1) 查找节点 / O(degree) 遍历出边
//   2. 内存占用 O(V+E)，远小于邻接矩阵的 O(V²)
//   3. 稀疏图（E << V²）时优势明显
//
// 节点标识：
//   使用 int 编号（非名字字符串）作为唯一标识。
//   原因：HashMap 天然支持 int → value 的 O(1) 映射。
//   名字作为 Node 的属性存储，用户通过 ComboBox 选名字 → 内部转编号。
// ============================================================================

#include "Containers.h"
#include <string>

// ---- 节点 ----
struct Node {
    int id = 0;
    std::string name;     // 城市名（如"北京总仓"）
    std::string address;  // 地址（如"北京市朝阳区"）
    double lon = 0;       // 经度（绘图用）
    double lat = 0;       // 纬度（绘图用）

    Node() = default;
    Node(int i, const std::string& n, const std::string& a, double lo = 0, double la = 0)
        : id(i), name(n), address(a), lon(lo), lat(la) {}
};

// ---- 有向边 ----
struct Edge {
    int    from = 0, to = 0;
    double time = 0;     // 运输耗时（小时）
    double cost = 0;     // 运输费用（元）

    Edge() = default;
    Edge(int f, int t, double ti, double co) : from(f), to(t), time(ti), cost(co) {}
};

// ---- 图 ----
class Graph {
public:
    // 节点 CRUD
    bool  addNode(const Node& n);
    bool  deleteNode(int id);
    bool  updateNode(int id, const Node& n);
    Node*       findNode(int id);
    const Node* findNode(int id) const;

    // 边 CRUD
    bool  addEdge(const Edge& e);
    bool  deleteEdge(int from, int to);

    // 查询
    const DynArray<Edge>& getNeighbors(int id) const;
    DynArray<int>         getAllNodeIds() const;
    bool hasNode(int id) const;
    int  nodeCount() const;
    int  edgeCount() const;
    void clear();

private:
    HashMap<Node>           nodes_;          // 节点表
    HashMap<DynArray<Edge>> adj_;            // 邻接表
    static const DynArray<Edge>  emptyNeighbors_;  // 空邻接表（无出边的节点返回它）
};
