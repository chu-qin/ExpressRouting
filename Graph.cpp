// ============================================================================
// Graph.cpp — 有向图邻接表实现
// ============================================================================
// 核心数据结构回顾：
//   nodes_: HashMap<int, Node>           — 所有节点
//   adj_:   HashMap<int, DynArray<Edge>> — 每个节点的出边列表
//
// CRUD 设计要点：
//   删除节点 → 先删邻接表中的出边 → 遍历所有邻接表删指向该节点的入边 → 删节点
//   添加边   → 检查起终点存在、非自环、权重非负、不重复
//   删除边   → 线性查找邻接表 + remove_first 条件删除
// ============================================================================

#include "Graph.h"
#include <iostream>
#include <iomanip>

using std::string;
using std::to_string;

// 静态成员定义：空邻接表，无出边的节点引用此对象
const DynArray<Edge> Graph::emptyNeighbors_;

// ================================================================
// 节点 CRUD
// ================================================================

bool Graph::addNode(const Node& n) {
    if (n.name.empty()) {
        std::cerr << "[警告] 添加节点失败：名称为空\n";
        return false;
    }
    if (nodes_.contains(n.id)) {
        std::cerr << "[警告] 添加节点失败：编号 " << n.id << " 已存在\n";
        return false;
    }
    nodes_.set(n.id, n);          // 插入节点表
    adj_[n.id];                   // 初始化空邻接表（operator[] 会创建默认值）
    std::cout << "[OK] 添加节点：[" << n.id << "] " << n.name << "\n";
    return true;
}

bool Graph::deleteNode(int id) {
    if (!nodes_.contains(id)) {
        std::cerr << "[警告] 删除失败：节点 " << id << " 不存在\n";
        return false;
    }
    // 1. 删邻接表中该节点的出边
    nodes_.erase(id);
    adj_.erase(id);

    // 2. 遍历所有其他节点的邻接表，删除指向该节点的入边
    adj_.forEach([id](int, DynArray<Edge>& es) {
        es.remove_all([id](const Edge& e) { return e.to == id; });
    });

    std::cout << "[OK] 删除节点：" << id << "\n";
    return true;
}

bool Graph::updateNode(int id, const Node& n) {
    if (!nodes_.contains(id)) {
        std::cerr << "[警告] 修改失败：节点不存在\n";
        return false;
    }
    if (n.name.empty()) {
        std::cerr << "[警告] 修改失败：名称为空\n";
        return false;
    }
    Node u = n;
    u.id = id;                     // 保持原编号
    nodes_.set(id, u);
    std::cout << "[OK] 修改节点：" << id << "\n";
    return true;
}

Node* Graph::findNode(int id)             { return nodes_.find(id); }
const Node* Graph::findNode(int id) const { return nodes_.find(id); }

// ================================================================
// 边 CRUD
// ================================================================

bool Graph::addEdge(const Edge& e) {
    // 边界检查
    if (!nodes_.contains(e.from) || !nodes_.contains(e.to)) {
        std::cerr << "[警告] 添加边失败：节点不存在\n";
        return false;
    }
    if (e.from == e.to) {
        std::cerr << "[警告] 添加边失败：不允许自环\n";
        return false;
    }
    if (e.time < 0 || e.cost < 0) {
        std::cerr << "[警告] 添加边失败：权重不能为负\n";
        return false;
    }

    // 查重
    DynArray<Edge>& es = adj_[e.from];
    for (int i = 0; i < es.size(); ++i)
        if (es[i].to == e.to) {
            std::cerr << "[警告] 边 " << e.from << "→" << e.to << " 已存在\n";
            return false;
        }

    es.push_back(e);
    std::cout << "[OK] 添加边：" << e.from << "→" << e.to
              << " 耗时" << e.time << "h 费用" << e.cost << "元\n";
    return true;
}

bool Graph::deleteEdge(int from, int to) {
    DynArray<Edge>* ep = adj_.find(from);
    if (!ep) {
        std::cerr << "[警告] 删除边失败：起点不存在\n";
        return false;
    }
    bool ok = ep->remove_first([to](const Edge& e) { return e.to == to; });
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
    const DynArray<Edge>* p = adj_.find(id);
    return p ? *p : emptyNeighbors_;
}

// 获取所有节点编号（用于遍历），返回后外部自行排序
DynArray<int> Graph::getAllNodeIds() const {
    DynArray<int> ids;
    nodes_.forEach([&](int id, const Node&) { ids.push_back(id); });
    return ids;
}

bool Graph::hasNode(int id)  const { return nodes_.contains(id); }
int  Graph::nodeCount()      const { return nodes_.size(); }

int  Graph::edgeCount()      const {
    int c = 0;
    adj_.forEach([&](int, const DynArray<Edge>& e) { c += e.size(); });
    return c;
}

void Graph::clear() { nodes_.clear(); adj_.clear(); }
