#pragma once
// ============================================================================
// OrderManager.h — 订单管理 + 文件读写
// ============================================================================
// OrderManager — 订单 CRUD + 批量规划 + 批次排序
// FileManager  — 静态工具类：路网/订单文件导入导出
//
// 文件格式（network.txt 分段格式）：
//   # NODES
//   1 北京总仓 北京市朝阳区 116.4 39.9
//   # EDGES
//   1 2 1.5 60.00
//
// 文件格式（orders.txt）：
//   # 订单号 起点 终点 货物 优化目标(0=费用/1=耗时)
//   1001 1 20 电子产品 1
// ============================================================================

#include "Dijkstra.h"
#include <string>

// ---- 订单 ----
struct Order {
    int orderId;
    int srcNode, dstNode;
    std::string goods;       // 货物描述
    bool byTime = false;     // true=按耗时优化, false=按费用优化

    Order() = default;
    Order(int oid, int s, int d, const std::string& g, bool bt)
        : orderId(oid), srcNode(s), dstNode(d), goods(g), byTime(bt) {}
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
    bool addOrder(const Order& o);
    bool removeOrder(int id);
    void listOrders() const;
    void clear() { orders_.clear(); }

    // 批量规划所有订单路径
    DynArray<DeliveryPlan> planAllOrders(const Graph& g) const;

    // 基于订单起终点生成批次配送拓扑排序
    TopoResult planBatchSequence(const Graph& g) const;

    const DynArray<Order>& getOrders() const { return orders_; }

private:
    DynArray<Order> orders_;
};

// ============================================================================
// FileManager — 文件读写（静态工具类）
// ============================================================================
class FileManager {
public:
    // 从 network.txt 加载路网（节点 + 边）
    static bool loadNetwork(const std::string& path, Graph& g);

    // 保存路网到文件
    static bool saveNetwork(const std::string& path, const Graph& g);

    // 从 orders.txt 加载订单
    static bool loadOrders(const std::string& path, OrderManager& om);

    // 导出配送方案到文件
    static bool savePlans(const std::string& path, const Graph& g,
                          const DynArray<DeliveryPlan>& plans);
};
