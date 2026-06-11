// ============================================================================
// Graph.cpp — 有向图邻接表实现
// ============================================================================

#include "Graph.h"
#include <iostream>

const DynArray<Edge> Graph::emptyAdj;

// ================================================================
// 节点 CRUD
// ================================================================

bool Graph::addNode(const Node& node) {
    if (node.name.empty()) {
        std::cerr << "[警告] 添加节点失败：名称为空\n";
        return false;
    }
    if (nodes.contains(node.id)) {
        std::cerr << "[警告] 添加节点失败：编号 " << node.id << " 已存在\n";
        return false;
    }
    nodes.set(node.id, node);
    adj[node.id];     // 初始化空邻接表
    std::cout << "[OK] 添加节点：[" << node.id << "] " << node.name << "\n";
    return true;
}

bool Graph::deleteNode(int id) {
    if (!nodes.contains(id)) {
        std::cerr << "[警告] 删除失败：节点 " << id << " 不存在\n";
        return false;
    }
    nodes.erase(id);
    adj.erase(id);

    // 遍历其他节点，删除指向该节点的入边
    adj.forEach([id](int, DynArray<Edge>& edges) {
        edges.remove_all([id](const Edge& e) { return e.to == id; });
    });

    std::cout << "[OK] 删除节点：" << id << "\n";
    return true;
}

bool Graph::updateNode(int id, const Node& node) {
    if (!nodes.contains(id)) {
        std::cerr << "[警告] 修改失败：节点不存在\n";
        return false;
    }
    if (node.name.empty()) {
        std::cerr << "[警告] 修改失败：名称为空\n";
        return false;
    }
    Node updated = node;
    updated.id = id;
    nodes.set(id, updated);
    std::cout << "[OK] 修改节点：" << id << "\n";
    return true;
}

Node* Graph::findNode(int id)             { return nodes.find(id); }
const Node* Graph::findNode(int id) const { return nodes.find(id); }

// ================================================================
// 边 CRUD
// ================================================================

bool Graph::addEdge(const Edge& edge) {
    if (!nodes.contains(edge.from) || !nodes.contains(edge.to)) {
        std::cerr << "[警告] 添加边失败：节点不存在\n";
        return false;
    }
    if (edge.from == edge.to) {
        std::cerr << "[警告] 添加边失败：不允许自环\n";
        return false;
    }
    if (edge.time < 0 || edge.cost < 0) {
        std::cerr << "[警告] 添加边失败：权重不能为负\n";
        return false;
    }

    // 查重
    DynArray<Edge>& edgeList = adj[edge.from];
    for (int i = 0; i < edgeList.size(); ++i)
        if (edgeList[i].to == edge.to) {
            std::cerr << "[警告] 边 " << edge.from << "→" << edge.to << " 已存在\n";
            return false;
        }

    edgeList.push_back(edge);
    std::cout << "[OK] 添加边：" << edge.from << "→" << edge.to
              << " 耗时" << edge.time << "h 费用" << edge.cost << "元\n";
    return true;
}

bool Graph::deleteEdge(int from, int to) {
    DynArray<Edge>* found = adj.find(from);
    if (!found) {
        std::cerr << "[警告] 删除边失败：起点不存在\n";
        return false;
    }
    bool ok = found->remove_first([to](const Edge& e) { return e.to == to; });
    if (!ok) {
        std::cerr << "[警告] 删除边失败：边 " << from << "→" << to << " 不存在\n";
        return false;
    }
    std::cout << "[OK] 删除边：" << from << "→" << to << "\n";
    return true;
}

// ================================================================
// 查询
// ================================================================

const DynArray<Edge>& Graph::getNeighbors(int id) const {
    const DynArray<Edge>* found = adj.find(id);
    return found ? *found : emptyAdj;
}

DynArray<int> Graph::getAllNodeIds() const {
    DynArray<int> ids;
    nodes.forEach([&](int id, const Node&) { ids.push_back(id); });
    return ids;
}

bool Graph::hasNode(int id)  const { return nodes.contains(id); }
int  Graph::nodeCount()      const { return nodes.size(); }

int  Graph::edgeCount() const {
    int total = 0;
    adj.forEach([&](int, const DynArray<Edge>& edges) { total += edges.size(); });
    return total;
}

void Graph::clear() { nodes.clear(); adj.clear(); }
