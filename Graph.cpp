// ============================================================================
// Graph.cpp —— 有向图邻接表 实现
// ============================================================================
// 邻接表结构回顾：
//   nodes[0] = {name:"北京总仓", ...}    ← 0号城市的信息
//   neigh[0] = [{to:2,time:3,cost:50}, {to:5,time:8,cost:120}]  ← 出边列表
//
// 要找城市0能直达哪些城市？读 neigh[0]
// 要找城市0→城市2耗时多少？遍历 neigh[0] 找到 to==2 的那条边
// ============================================================================

#include "Graph.h"
#include <iostream>

const DynArray<Edge> Graph::emptyList;

// ================================================================
// 节点管理
// ================================================================

bool Graph::addNode(const Node& node) {
    // 异常检查：名称为空
    if (node.name.empty()) {
        std::cerr << "[错误] 添加节点失败：名称为空\n";
        return false;
    }
    // 异常检查：编号必须等于当前节点总数（保证顺序分配）
    if (node.id != nodes.size()) {
        std::cerr << "[错误] 添加节点失败：编号必须为 " << nodes.size() << "\n";
        return false;
    }

    nodes.push_back(node);
    neigh.push_back(DynArray<Edge>());   // 初始化空的出边列表
    std::cout << "[OK] 添加节点 [" << node.id << "] " << node.name << "\n";
    return true;
}

bool Graph::deleteNode(int id) {
    // 异常检查：节点不存在
    if (!hasNode(id)) {
        std::cerr << "[错误] 删除节点失败：节点 " << id << " 不存在\n";
        return false;
    }

    // 删除该节点的出边列表
    neigh[id].clear();

    // 删除所有指向该节点的入边（遍历所有节点的出边列表）
    for (int i = 0; i < (int)nodes.size(); ++i) {
        if (nodes[i].id == -1) continue;   // 跳过已删除节点
        neigh[i].remove_all([id](const Edge& e) {
            return e.to == id;
        });
    }

    // 标记节点为已删除
    nodes[id].id = -1;
    std::cout << "[OK] 删除节点 " << id << "，同时清理了所有关联边\n";
    return true;
}

bool Graph::updateNode(int id, const Node& node) {
    if (!hasNode(id)) {
        std::cerr << "[错误] 修改节点失败：节点 " << id << " 不存在\n";
        return false;
    }
    if (node.name.empty()) {
        std::cerr << "[错误] 修改节点失败：名称为空\n";
        return false;
    }
    nodes[id].name    = node.name;
    nodes[id].address = node.address;
    nodes[id].lon     = node.lon;
    nodes[id].lat     = node.lat;
    std::cout << "[OK] 修改节点 " << id << "\n";
    return true;
}

Node* Graph::findNode(int id) {
    if (!hasNode(id)) return nullptr;
    return &nodes[id];
}

const Node* Graph::findNode(int id) const {
    if (!hasNode(id)) return nullptr;
    return &nodes[id];
}

DynArray<int> Graph::getAllNodeIds() const {
    DynArray<int> ids;
    for (int i = 0; i < (int)nodes.size(); ++i)
        if (nodes[i].id != -1)           // 跳过已删除节点
            ids.push_back(i);
    return ids;
}

// ================================================================
// 边管理
// ================================================================

bool Graph::addEdge(int from, int to, double time, double cost) {
    // 异常检查：节点不存在
    if (!hasNode(from) || !hasNode(to)) {
        std::cerr << "[错误] 添加边失败：节点 " << from << " 或 " << to << " 不存在\n";
        return false;
    }
    // 异常检查：不允许自环
    if (from == to) {
        std::cerr << "[错误] 添加边失败：不允许自环（" << from << "→" << to << "）\n";
        return false;
    }
    // 异常检查：权重不能为负
    if (time < 0 || cost < 0) {
        std::cerr << "[错误] 添加边失败：权重不能为负数\n";
        return false;
    }
    // 异常检查：不允许重复边
    for (int i = 0; i < neigh[from].size(); ++i) {
        if (neigh[from][i].to == to) {
            std::cerr << "[错误] 添加边失败：边 " << from << "→" << to << " 已存在\n";
            return false;
        }
    }

    neigh[from].push_back(Edge(to, time, cost));
    std::cout << "[OK] 添加边 " << from << "→" << to
              << " 耗时" << time << "h 费用" << cost << "元\n";
    return true;
}

bool Graph::deleteEdge(int from, int to) {
    if (!hasNode(from)) {
        std::cerr << "[错误] 删除边失败：起点 " << from << " 不存在\n";
        return false;
    }
    bool ok = neigh[from].remove_first([to](const Edge& e) {
        return e.to == to;
    });
    if (!ok) {
        std::cerr << "[错误] 删除边失败：边 " << from << "→" << to << " 不存在\n";
        return false;
    }
    std::cout << "[OK] 删除边 " << from << "→" << to << "\n";
    return true;
}

// ================================================================
// 查询
// ================================================================

const DynArray<Edge>& Graph::getNeighbors(int id) const {
    if (!hasNode(id)) return emptyList;
    return neigh[id];
}

DynArray<Edge>& Graph::neighborsOf(int id) {
    return neigh[id];
}

bool Graph::hasNode(int id) const {
    return id >= 0 && id < (int)nodes.size() && nodes[id].id != -1;
}

int Graph::nodeCount() const {
    int cnt = 0;
    for (int i = 0; i < (int)nodes.size(); ++i)
        if (nodes[i].id != -1) ++cnt;
    return cnt;
}

int Graph::edgeCount() const {
    int cnt = 0;
    for (int i = 0; i < (int)neigh.size(); ++i)
        cnt += neigh[i].size();
    return cnt;
}

int Graph::maxNodeId() const {
    return nodes.size();   // 下一个可用的编号 = 当前数组长度
}

void Graph::clear() {
    nodes.clear();
    neigh.clear();
}
