// ============================================================================
// OrderManager.cpp — 订单管理 + 文件读写 实现
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

bool OrderManager::addOrder(const Order& order) {
    for (int i = 0; i < orderList.size(); ++i)
        if (orderList[i].orderId == order.orderId) {
            std::cerr << "[警告] 订单 " << order.orderId << " 已存在\n";
            return false;
        }
    orderList.push_back(order);
    std::cout << "[OK] 添加订单 " << order.orderId << ": "
              << order.srcNode << "→" << order.dstNode << "\n";
    return true;
}

bool OrderManager::removeOrder(int id) {
    bool ok = orderList.remove_first([id](const Order& o) { return o.orderId == id; });
    if (!ok) {
        std::cerr << "[警告] 订单 " << id << " 不存在\n";
        return false;
    }
    std::cout << "[OK] 删除订单 " << id << "\n";
    return true;
}

void OrderManager::listOrders() const {
    if (orderList.empty()) {
        std::cout << "  （当前无订单）\n";
        return;
    }
    std::cout << std::left << std::setw(8) << "订单号"
              << std::setw(8) << "起点" << std::setw(8) << "终点"
              << std::setw(16) << "货物" << "优化目标\n"
              << std::string(50, '-') << "\n";
    for (int i = 0; i < orderList.size(); ++i) {
        const Order& order = orderList[i];
        std::cout << std::setw(8) << order.orderId
                  << std::setw(8) << order.srcNode
                  << std::setw(8) << order.dstNode
                  << std::setw(16) << order.goods
                  << (order.preferTime ? "最短耗时" : "最低费用") << "\n";
    }
}

DynArray<DeliveryPlan> OrderManager::planAllOrders(const Graph& graph) const {
    DynArray<DeliveryPlan> plans;
    for (int i = 0; i < orderList.size(); ++i) {
        DeliveryPlan plan;
        plan.order = orderList[i];

        if (orderList[i].preferTime) {
            auto allPaths = Dijkstra::shortestTimeFrom(graph, orderList[i].srcNode);
            PathResult* found = allPaths.find(orderList[i].dstNode);
            plan.result = found ? *found : PathResult{};
        } else {
            plan.result = Dijkstra::cheapestPath(graph, orderList[i].srcNode, orderList[i].dstNode);
        }
        plans.push_back(plan);
    }
    std::cout << "[OK] 批量规划完成，共 " << plans.size() << " 条\n";
    return plans;
}

TopoResult OrderManager::planBatchSequence(const Graph& graph) const {
    HashMap<bool> seen;
    DynArray<int> nodeIds;
    for (int i = 0; i < orderList.size(); ++i) {
        if (!seen.contains(orderList[i].srcNode)) {
            seen.set(orderList[i].srcNode, true);
            nodeIds.push_back(orderList[i].srcNode);
        }
        if (!seen.contains(orderList[i].dstNode)) {
            seen.set(orderList[i].dstNode, true);
            nodeIds.push_back(orderList[i].dstNode);
        }
    }
    return TopoSort::sort(graph, nodeIds);
}

// ================================================================
// FileManager — 文件读写
// ================================================================

bool FileManager::loadNetwork(const string& filePath, Graph& graph) {
    std::ifstream inFile(filePath);
    if (!inFile) {
        std::cerr << "[错误] 无法打开路网文件：" << filePath << "\n";
        return false;
    }

    graph.clear();
    string line, section;    // section: "N"=节点段, "E"=边段
    int nodeCnt = 0, edgeCnt = 0;

    while (std::getline(inFile, line)) {
        if (line.empty()) continue;

        // 检测段落标记
        if (line[0] == '#') {
            if (line.find("NODES") != string::npos)       section = "N";
            else if (line.find("EDGES") != string::npos)  section = "E";
            continue;
        }

        if (section == "N") {
            // 格式：编号 名称 地址... 经度 纬度
            // 拆词：第一词=编号，第二词=名称，末两词=经纬度，中间=地址
            DynArray<string> words;
            string word;
            std::istringstream stream(line);
            while (stream >> word) words.push_back(word);

            if (words.size() >= 4) {
                int    nodeId   = std::stoi(words[0]);
                string nodeName = words[1];
                double nodeLon  = std::stod(words[words.size() - 2]);
                double nodeLat  = std::stod(words[words.size() - 1]);
                string nodeAddr;
                for (int j = 2; j + 2 < words.size(); ++j) {
                    if (j > 2) nodeAddr += " ";
                    nodeAddr += words[j];
                }
                if (graph.addNode(Node(nodeId, nodeName, nodeAddr, nodeLon, nodeLat)))
                    ++nodeCnt;
            }
        } else if (section == "E") {
            std::istringstream stream(line);
            int from, to; double time, cost;
            if (stream >> from >> to >> time >> cost) {
                if (graph.addEdge(Edge(from, to, time, cost))) ++edgeCnt;
            }
        }
    }

    std::cout << "[OK] 路网导入：" << nodeCnt << " 节点 / " << edgeCnt << " 边，来自 " << filePath << "\n";
    return true;
}

bool FileManager::saveNetwork(const string& filePath, const Graph& graph) {
    std::ofstream outFile(filePath);
    if (!outFile) {
        std::cerr << "[错误] 无法写入：" << filePath << "\n";
        return false;
    }

    DynArray<int> ids = graph.getAllNodeIds();
    for (int i = 0; i < ids.size() - 1; ++i)
        for (int j = i + 1; j < ids.size(); ++j)
            if (ids[i] > ids[j]) { int temp = ids[i]; ids[i] = ids[j]; ids[j] = temp; }

    outFile << "# NODES\n";
    for (int i = 0; i < ids.size(); ++i) {
        const Node* node = graph.findNode(ids[i]);
        if (node) outFile << node->id << " " << node->name << " " << node->address
                          << " " << node->lon << " " << node->lat << "\n";
    }

    outFile << "# EDGES\n";
    outFile << std::fixed << std::setprecision(2);
    for (int i = 0; i < ids.size(); ++i) {
        const DynArray<Edge>& edgeList = graph.getNeighbors(ids[i]);
        for (int j = 0; j < edgeList.size(); ++j)
            outFile << edgeList[j].from << " " << edgeList[j].to << " "
                    << edgeList[j].time << " " << edgeList[j].cost << "\n";
    }

    std::cout << "[OK] 路网已保存：" << filePath << "\n";
    return true;
}

bool FileManager::loadOrders(const string& filePath, OrderManager& orderMgr) {
    std::ifstream inFile(filePath);
    if (!inFile) {
        std::cerr << "[错误] 无法打开订单文件：" << filePath << "\n";
        return false;
    }

    int count = 0;
    string line;
    while (std::getline(inFile, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream stream(line);
        int orderId, src, dst, useTime;
        string goodsName;
        if (stream >> orderId >> src >> dst >> goodsName >> useTime) {
            if (orderMgr.addOrder(Order(orderId, src, dst, goodsName, useTime != 0)))
                ++count;
        }
    }

    std::cout << "[OK] 导入订单：" << count << " 条，来自 " << filePath << "\n";
    return true;
}

bool FileManager::savePlans(const string& filePath, const Graph&,
                             const DynArray<DeliveryPlan>& plans) {
    std::ofstream outFile(filePath);
    if (!outFile) {
        std::cerr << "[错误] 无法写入配送方案：" << filePath << "\n";
        return false;
    }

    outFile << std::fixed << std::setprecision(2) << "# 配送方案\n";
    for (int i = 0; i < plans.size(); ++i) {
        const Order& order = plans[i].order;
        const PathResult& pathRes = plans[i].result;

        outFile << order.orderId << " " << order.goods << " "
                << (order.preferTime ? "最短耗时" : "最低费用") << " ";

        if (!pathRes.reachable) {
            outFile << "不可达\n";
            continue;
        }

        outFile << "耗时" << pathRes.totalTime << "h 费用" << pathRes.totalCost << "元 路径: ";
        for (int j = 0; j < pathRes.path.size(); ++j) {
            outFile << pathRes.path[j];
            if (j + 1 < pathRes.path.size()) outFile << "→";
        }
        outFile << "\n";
    }

    std::cout << "[OK] 方案已保存：" << filePath << "\n";
    return true;
}
