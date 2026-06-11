#pragma once
// ============================================================================
// OrderManager.h — 订单管理 + 文件读写
// ============================================================================

#include "Dijkstra.h"
#include <string>

// ---- 订单 ----
struct Order {
    int orderId;
    int srcNode, dstNode;
    std::string goods;       // 货物描述
    bool preferTime = false; // true=最短耗时, false=最低费用

    Order() = default;
    Order(int id, int src, int dst, const std::string& goodsName, bool useTime)
        : orderId(id), srcNode(src), dstNode(dst), goods(goodsName), preferTime(useTime) {}
};

// ---- 配送计划（订单 + 计算结果） ----
struct DeliveryPlan {
    Order order;
    PathResult result;
};

// ============================================================================
// OrderManager — 订单管理
// ============================================================================
class OrderManager {
public:
    bool addOrder(const Order& order);
    bool removeOrder(int id);
    void listOrders() const;
    void clear() { orderList.clear(); }

    // 批量规划所有订单路径
    DynArray<DeliveryPlan> planAllOrders(const Graph& graph) const;

    // 基于订单起终点生成批次拓扑排序
    TopoResult planBatchSequence(const Graph& graph) const;

    const DynArray<Order>& getOrders() const { return orderList; }

private:
    DynArray<Order> orderList;
};

// ============================================================================
// FileManager — 文件读写（静态工具类）
// ============================================================================
class FileManager {
public:
    static bool loadNetwork(const std::string& filePath, Graph& graph);
    static bool saveNetwork(const std::string& filePath, const Graph& graph);
    static bool loadOrders(const std::string& filePath, OrderManager& orderMgr);
    static bool savePlans(const std::string& filePath, const Graph& graph,
                          const DynArray<DeliveryPlan>& plans);
};
