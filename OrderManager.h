#pragma once
// ============================================================================
// OrderManager.h —— 订单管理 + 文件读写
// ============================================================================

#include "Dijkstra.h"
#include <string>

// ---- 配送订单 ----
struct Order {
    int         orderId    = 0;      // 订单编号
    int         srcNode    = 0;      // 起点网点
    int         dstNode    = 0;      // 终点网点
    std::string goods;               // 货物名称
    int         preferTime = 1;      // 1=最短耗时  0=最低费用
};

// ---- 配送方案（一个订单 + 一条路径）----
struct DeliveryPlan {
    Order      order;
    PathResult result;
};

// ============================================================================
// OrderManager —— 订单管理
// ============================================================================
class OrderManager {
public:
    bool addOrder(const Order& o);
    bool deleteOrder(int id);
    void listAll() const;

    // 为所有订单规划路径
    DynArray<DeliveryPlan> planAll(const Graph& graph) const;

    // 对订单涉及的所有节点做拓扑排序，检测配送环路
    TopoResult planBatch(const Graph& graph) const;

    int  orderCount() const;
    void clear();

private:
    DynArray<Order> orders;     // 订单列表
};

// ============================================================================
// FileManager —— 文件读写
// ============================================================================
class FileManager {
public:
    static bool loadNetwork(const std::string& path, Graph& graph);
    static bool saveNetwork(const std::string& path, const Graph& graph);

    static bool loadOrders(const std::string& path, OrderManager& mgr);
    static bool savePlans(const std::string& path,
                          const DynArray<DeliveryPlan>& plans);
};
