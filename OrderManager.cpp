// ============================================================================
// OrderManager.cpp — 订单管理 + 文件读写 实现
// ============================================================================
// 文件格式说明：
//   network.txt 使用分段标记：
//     # NODES       — 下行为节点数据
//     # EDGES       — 下行为边数据
//   每行空格分隔：节点 "编号 名称 地址 经度 纬度"
//                边   "起点 终点 耗时 费用"
//
//   orders.txt：
//     订单号 起点编号 终点编号 货物描述 优化目标(0=费用/1=耗时)
//     # 开头为注释行
// ============================================================================

#include "OrderManager.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

using std::string;

// ================================================================
// OrderManager — 订单管理
// ================================================================

bool OrderManager::addOrder(const Order& o) {
    // 查重
    for (int i = 0; i < orders_.size(); ++i)
        if (orders_[i].orderId == o.orderId) {
            std::cerr << "[警告] 订单 " << o.orderId << " 已存在\n";
            return false;
        }
    orders_.push_back(o);
    std::cout << "[OK] 添加订单 " << o.orderId << ": "
              << o.srcNode << "→" << o.dstNode << "\n";
    return true;
}

bool OrderManager::removeOrder(int id) {
    bool ok = orders_.remove_first([id](const Order& o) { return o.orderId == id; });
    if (!ok) {
        std::cerr << "[警告] 订单 " << id << " 不存在\n";
        return false;
    }
    std::cout << "[OK] 删除订单 " << id << "\n";
    return true;
}

void OrderManager::listOrders() const {
    if (orders_.empty()) {
        std::cout << "  （当前无订单）\n";
        return;
    }
    std::cout << std::left << std::setw(8) << "订单号"
              << std::setw(8) << "起点" << std::setw(8) << "终点"
              << std::setw(16) << "货物" << "优化目标\n"
              << std::string(50, '-') << "\n";
    for (int i = 0; i < orders_.size(); ++i) {
        const Order& o = orders_[i];
        std::cout << std::setw(8) << o.orderId
                  << std::setw(8) << o.srcNode
                  << std::setw(8) << o.dstNode
                  << std::setw(16) << o.goods
                  << (o.byTime ? "最短耗时" : "最低费用") << "\n";
    }
}

DynArray<DeliveryPlan> OrderManager::planAllOrders(const Graph& g) const {
    DynArray<DeliveryPlan> plans;
    for (int i = 0; i < orders_.size(); ++i) {
        DeliveryPlan p;
        p.order = orders_[i];

        if (orders_[i].byTime) {
            // 按耗时：先算全源最短耗时，再取目标节点
            auto all = Dijkstra::shortestTimeFrom(g, orders_[i].srcNode);
            PathResult* pr = all.find(orders_[i].dstNode);
            p.result = pr ? *pr : PathResult{};
        } else {
            // 按费用：直接算两点最低费用
            p.result = Dijkstra::cheapestPath(g, orders_[i].srcNode, orders_[i].dstNode);
        }
        plans.push_back(p);
    }
    std::cout << "[OK] 批量规划完成，共 " << plans.size() << " 条\n";
    return plans;
}

TopoResult OrderManager::planBatchSequence(const Graph& g) const {
    // 收集所有订单涉及的节点编号（去重）
    HashMap<bool> seen;
    DynArray<int> ids;
    for (int i = 0; i < orders_.size(); ++i) {
        if (!seen.contains(orders_[i].srcNode)) {
            seen.set(orders_[i].srcNode, true);
            ids.push_back(orders_[i].srcNode);
        }
        if (!seen.contains(orders_[i].dstNode)) {
            seen.set(orders_[i].dstNode, true);
            ids.push_back(orders_[i].dstNode);
        }
    }
    return TopoSort::sort(g, ids);
}

// ================================================================
// FileManager — 文件读写
// ================================================================

bool FileManager::loadNetwork(const string& path, Graph& g) {
    std::ifstream fin(path);
    if (!fin) {
        std::cerr << "[错误] 无法打开路网文件：" << path << "\n";
        return false;
    }

    g.clear();
    string line, sec;   // sec: "N"=节点段, "E"=边段
    int nc = 0, ec = 0;

    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        // 检测段落标记
        if (line[0] == '#') {
            if (line.find("NODES") != string::npos)      sec = "N";
            else if (line.find("EDGES") != string::npos) sec = "E";
            continue;
        }

        if (sec == "N") {
            // 格式：编号 名称 地址... 经度 纬度
            // 地址可能含空格，采用"拆词 + 取头取尾"策略：
            //   第一个词=编号，第二个词=名称，最后两个词=经纬度
            //   中间所有词=地址
            DynArray<string> toks;
            string t;
            std::istringstream ss(line);
            while (ss >> t) toks.push_back(t);

            if (toks.size() >= 4) {
                int id = std::stoi(toks[0]);
                string name = toks[1];
                double lon = std::stod(toks[toks.size() - 2]);
                double lat = std::stod(toks[toks.size() - 1]);
                string addr;
                for (int j = 2; j + 2 < toks.size(); ++j) {
                    if (j > 2) addr += " ";
                    addr += toks[j];
                }
                if (g.addNode(Node(id, name, addr, lon, lat))) ++nc;
            }
        } else if (sec == "E") {
            std::istringstream ss(line);
            int f, t; double ti, co;
            if (ss >> f >> t >> ti >> co) {
                if (g.addEdge(Edge(f, t, ti, co))) ++ec;
            }
        }
    }

    std::cout << "[OK] 路网导入：" << nc << " 节点 / " << ec << " 边，来自 " << path << "\n";
    return true;
}

bool FileManager::saveNetwork(const string& path, const Graph& g) {
    std::ofstream fout(path);
    if (!fout) {
        std::cerr << "[错误] 无法写入：" << path << "\n";
        return false;
    }

    // 收集并排序节点编号
    DynArray<int> ids = g.getAllNodeIds();
    for (int i = 0; i < ids.size() - 1; ++i)
        for (int j = i + 1; j < ids.size(); ++j)
            if (ids[i] > ids[j]) { int t = ids[i]; ids[i] = ids[j]; ids[j] = t; }

    fout << "# NODES\n";
    for (int i = 0; i < ids.size(); ++i) {
        const Node* n = g.findNode(ids[i]);
        if (n) fout << n->id << " " << n->name << " " << n->address
                    << " " << n->lon << " " << n->lat << "\n";
    }

    fout << "# EDGES\n";
    fout << std::fixed << std::setprecision(2);
    for (int i = 0; i < ids.size(); ++i) {
        const DynArray<Edge>& es = g.getNeighbors(ids[i]);
        for (int j = 0; j < es.size(); ++j)
            fout << es[j].from << " " << es[j].to << " "
                 << es[j].time << " " << es[j].cost << "\n";
    }

    std::cout << "[OK] 路网已保存：" << path << "\n";
    return true;
}

bool FileManager::loadOrders(const string& path, OrderManager& om) {
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
        int oid, src, dst, bt;
        string goods;
        if (ss >> oid >> src >> dst >> goods >> bt) {
            if (om.addOrder(Order(oid, src, dst, goods, bt != 0))) ++cnt;
        }
    }

    std::cout << "[OK] 导入订单：" << cnt << " 条，来自 " << path << "\n";
    return true;
}

bool FileManager::savePlans(const string& path, const Graph&,
                             const DynArray<DeliveryPlan>& plans) {
    std::ofstream fout(path);
    if (!fout) {
        std::cerr << "[错误] 无法写入配送方案：" << path << "\n";
        return false;
    }

    fout << std::fixed << std::setprecision(2) << "# 配送方案\n";
    for (int i = 0; i < plans.size(); ++i) {
        const Order& o = plans[i].order;
        const PathResult& r = plans[i].result;

        fout << o.orderId << " " << o.goods << " "
             << (o.byTime ? "最短耗时" : "最低费用") << " ";

        if (!r.reachable) {
            fout << "不可达\n";
            continue;
        }

        fout << "耗时" << r.totalTime << "h 费用" << r.totalCost << "元 路径: ";
        for (int j = 0; j < r.path.size(); ++j) {
            fout << r.path[j];
            if (j + 1 < r.path.size()) fout << "→";
        }
        fout << "\n";
    }

    std::cout << "[OK] 方案已保存：" << path << "\n";
    return true;
}
