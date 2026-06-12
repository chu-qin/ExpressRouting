// ============================================================================
// OrderManager.cpp —— 订单管理 + 文件读写 实现
// ============================================================================

#include "OrderManager.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

using std::string;

// ================================================================
// OrderManager —— 订单管理
// ================================================================

bool OrderManager::addOrder(const Order& o) {
    // 查重
    for (int i = 0; i < orders.size(); ++i) {
        if (orders[i].orderId == o.orderId) {
            std::cerr << "[错误] 添加订单失败：订单号 " << o.orderId << " 已存在\n";
            return false;
        }
    }
    orders.push_back(o);
    std::cout << "[OK] 添加订单 " << o.orderId << ": "
              << o.srcNode << "→" << o.dstNode
              << " [" << o.goods << "]\n";
    return true;
}

bool OrderManager::deleteOrder(int id) {
    bool ok = orders.remove_first([id](const Order& o) {
        return o.orderId == id;
    });
    if (!ok) {
        std::cerr << "[错误] 删除订单失败：订单号 " << id << " 不存在\n";
        return false;
    }
    std::cout << "[OK] 删除订单 " << id << "\n";
    return true;
}

void OrderManager::listAll() const {
    if (orders.empty()) {
        std::cout << "（当前无订单）\n";
        return;
    }
    std::cout << std::left
              << std::setw(8)  << "订单号"
              << std::setw(6)  << "起点" << std::setw(6) << "终点"
              << std::setw(14) << "货物"
              << "优化目标\n"
              << string(50, '-') << "\n";

    for (int i = 0; i < orders.size(); ++i) {
        const Order& o = orders[i];
        std::cout << std::setw(8)  << o.orderId
                  << std::setw(6)  << o.srcNode
                  << std::setw(6)  << o.dstNode
                  << std::setw(14) << o.goods
                  << (o.preferTime ? "最短耗时" : "最低费用") << "\n";
    }
}

DynArray<DeliveryPlan> OrderManager::planAll(const Graph& graph) const {
    DynArray<DeliveryPlan> plans;

    for (int i = 0; i < orders.size(); ++i) {
        DeliveryPlan plan;
        plan.order = orders[i];

        if (orders[i].preferTime) {
            // 单源最短耗时 → 从结果数组中取目标节点的
            DynArray<PathResult> all = Dijkstra::shortestTime(graph, orders[i].srcNode);
            plan.result = all[orders[i].dstNode];
        } else {
            // 两点最低费用
            plan.result = Dijkstra::cheapestPath(graph, orders[i].srcNode, orders[i].dstNode);
        }

        plans.push_back(plan);
    }
    std::cout << "[OK] 批量规划完成，共 " << plans.size() << " 条配送方案\n";
    return plans;
}

TopoResult OrderManager::planBatch(const Graph& graph) const {
    // 收集所有订单涉及的节点
    DynArray<int> nodeIds;
    DynArray<int> seen;
    for (int i = 0; i < graph.maxNodeId(); ++i) seen.push_back(0);

    for (int i = 0; i < orders.size(); ++i) {
        int s = orders[i].srcNode;
        int d = orders[i].dstNode;
        if (!seen[s]) { seen[s] = 1; nodeIds.push_back(s); }
        if (!seen[d]) { seen[d] = 1; nodeIds.push_back(d); }
    }

    return TopoSort::sort(graph, nodeIds);
}

int  OrderManager::orderCount() const { return orders.size(); }
void OrderManager::clear()            { orders.clear(); }

// ================================================================
// FileManager —— 文件读写
// ================================================================

bool FileManager::loadNetwork(const string& path, Graph& graph) {
    std::ifstream fin(path);
    if (!fin) {
        std::cerr << "[错误] 无法打开路网文件：" << path << "\n";
        return false;
    }

    graph.clear();
    string line, section;
    int nodeCnt = 0, edgeCnt = 0;

    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        // 检测段落标记
        if (line[0] == '#') {
            if      (line.find("NODES")  != string::npos) section = "N";
            else if (line.find("EDGES")  != string::npos) section = "E";
            continue;
        }

        if (section == "N") {
            // 格式：编号 名称 地址 经度 纬度
            // 名称和地址可能含空格，末两词为经纬度
            DynArray<string> words;
            std::istringstream ss(line);
            string w;
            while (ss >> w) words.push_back(w);

            if (words.size() >= 4) {
                int    id    = std::stoi(words[0]);
                string name  = words[1];
                double lon   = std::stod(words[words.size() - 2]);
                double lat   = std::stod(words[words.size() - 1]);

                // 拼接地址（名称和经纬度中间的部分）
                string addr;
                for (int j = 2; j + 2 < (int)words.size(); ++j) {
                    if (j > 2) addr += " ";
                    addr += words[j];
                }

                if (graph.addNode(Node(id, name, addr, lon, lat)))
                    ++nodeCnt;
            }
        } else if (section == "E") {
            // 格式：起点 终点 耗时 费用
            std::istringstream ss(line);
            int from, to; double time, cost;
            if (ss >> from >> to >> time >> cost) {
                if (graph.addEdge(from, to, time, cost)) ++edgeCnt;
            }
        }
    }

    std::cout << "[OK] 路网导入完成：" << nodeCnt << " 节点 / "
              << edgeCnt << " 边（文件：" << path << "）\n";
    return true;
}

bool FileManager::saveNetwork(const string& path, const Graph& graph) {
    std::ofstream fout(path);
    if (!fout) {
        std::cerr << "[错误] 无法写入路网文件：" << path << "\n";
        return false;
    }

    // 输出节点
    fout << "# NODES\n";
    DynArray<int> ids = graph.getAllNodeIds();
    for (int i = 0; i < ids.size(); ++i) {
        const Node* n = graph.findNode(ids[i]);
        if (n) {
            fout << n->id << " " << n->name << " " << n->address
                 << " " << n->lon << " " << n->lat << "\n";
        }
    }

    // 输出边
    fout << "# EDGES\n";
    fout << std::fixed << std::setprecision(2);
    for (int i = 0; i < ids.size(); ++i) {
        const DynArray<Edge>& edges = graph.getNeighbors(ids[i]);
        for (int j = 0; j < edges.size(); ++j) {
            fout << ids[i] << " " << edges[j].to << " "
                 << edges[j].time << " " << edges[j].cost << "\n";
        }
    }

    std::cout << "[OK] 路网已保存：" << path << "\n";
    return true;
}

bool FileManager::loadOrders(const string& path, OrderManager& mgr) {
    std::ifstream fin(path);
    if (!fin) {
        std::cerr << "[错误] 无法打开订单文件：" << path << "\n";
        return false;
    }

    int cnt = 0;
    string line;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        Order o;
        if (ss >> o.orderId >> o.srcNode >> o.dstNode >> o.goods >> o.preferTime) {
            if (mgr.addOrder(o)) ++cnt;
        }
    }

    std::cout << "[OK] 导入订单 " << cnt << " 条（文件：" << path << "）\n";
    return true;
}

bool FileManager::savePlans(const string& path,
                            const DynArray<DeliveryPlan>& plans) {
    std::ofstream fout(path);
    if (!fout) {
        std::cerr << "[错误] 无法写入配送方案文件：" << path << "\n";
        return false;
    }

    fout << std::fixed << std::setprecision(2)
         << "# 批量配送方案\n\n";

    for (int i = 0; i < plans.size(); ++i) {
        const Order&      o  = plans[i].order;
        const PathResult& pr = plans[i].result;

        fout << "订单 " << o.orderId << " | " << o.goods << " | "
             << o.srcNode << "→" << o.dstNode << " | ";

        if (o.preferTime)
            fout << "优先耗时 | ";
        else
            fout << "优先费用 | ";

        if (!pr.reachable) {
            fout << "不可达\n";
            continue;
        }

        fout << "耗时" << pr.totalTime << "h 费用" << pr.totalCost << "元\n"
             << "  路径：";

        for (int j = 0; j < pr.path.size(); ++j) {
            fout << pr.path[j];
            if (j + 1 < pr.path.size()) fout << " → ";
        }
        fout << "\n\n";
    }

    std::cout << "[OK] 配送方案已保存：" << path << "\n";
    return true;
}
