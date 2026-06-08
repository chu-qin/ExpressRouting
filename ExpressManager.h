#ifndef EXPRESSMANAGER_H
#define EXPRESSMANAGER_H

#include "ExpressGraph.h"
#include <fstream>    // ifstream / ofstream：文件读写
#include <cstdio>     // sscanf：解析文件每行内容
using namespace std;

// ============================================================================
// 结果结构体
// ============================================================================

// PathResult — Dijkstra 路径查询的返回结果
struct PathResult {
    bool reachable;         // 是否可达（起点到终点是否有路）
    double cost;            // 总运输耗时
    MyVector<int> path;     // 路径节点索引序列 [start, ..., end]
};

// TopoResult — 拓扑排序的返回结果
struct TopoResult {
    bool hasCycle;          // 是否存在环路（true = 有循环依赖）
    MyVector<int> order;    // 拓扑排序序列
};

// ============================================================================
// ExpressManager — 业务管理层（前后端之间的桥梁）
// ============================================================================
// 职责：
//   1. 文件读写：从 txt 文件加载路网数据，把结果输出到文件
//   2. 名字映射：把 "北京""上海" 等字符串转为图引擎认识的数字索引
//   3. 封装高层 API：findShortestPath / checkDependencies / batchProcessOrders
//   4. 错误处理：文件不存在、格式错误、不可达等
//
// 设计原则：
//   后端（ExpressGraph）只认 int 索引，不碰字符串
//   前端（MainWindow）只传 QString / 字符串，不碰矩阵
//   ExpressManager 做中间转换
// ============================================================================

class ExpressManager {
private:
    ExpressGraph* graph;    // 持有图引擎指针（has-a 关系）

    // ---- 字符串工具函数 ----

    // 比较两个 C 风格字符串是否相等
    bool sameStr(const char* a, const char* b) const {
        while (*a && *b) {
            if (*a != *b) return false;
            a++; b++;
        }
        return *a == '\0' && *b == '\0';
    }

    // 去掉字符串首尾的空白字符（空格、Tab、回车）
    // 用于处理文件读取的行，避免空行或前导空格影响解析
    void trim(char* str) const {
        int len = 0;
        while (str[len]) len++;

        int start = 0;
        while (str[start] == ' ' || str[start] == '\t'
            || str[start] == '\r' || str[start] == '\n') start++;

        int end = len - 1;
        while (end >= start && (str[end] == ' ' || str[end] == '\t'
            || str[end] == '\r' || str[end] == '\n')) end--;

        int j = 0;
        for (int i = start; i <= end; i++) {
            str[j++] = str[i];
        }
        str[j] = '\0';
    }

public:
    ExpressManager()  { graph = new ExpressGraph(); }
    ~ExpressManager() { delete graph; }

    // 获取图引擎指针（供 GUI 绘图时访问节点坐标和边信息）
    ExpressGraph* getGraph() { return graph; }

    // ================================================================
    // 路网导入 / 导出
    // ================================================================

    // 从文件加载路网数据
    // nodesFile: 每行 "网点名 X坐标 Y坐标"
    // edgesFile: 每行 "起点名 终点名 运输耗时"
    // 支持 # 开头注释行，支持空行
    // 返回 true 表示加载成功
    bool loadNetwork(const char* nodesFile, const char* edgesFile) {
        // ---- 读取节点文件 ----
        ifstream fn(nodesFile);
        if (!fn.is_open()) return false;

        char line[256];
        while (fn.getline(line, 256)) {
            trim(line);
            if (line[0] == '#' || line[0] == '\0') continue;  // 跳过注释和空行

            char name[64];
            double x, y;
            // sscanf: 从字符串中按格式解析变量
            // "%s %lf %lf" → 字符串 / double / double
            int n = sscanf(line, "%s %lf %lf", name, &x, &y);
            if (n == 3) {
                graph->addNode(name, x, y);
            }
        }
        fn.close();

        if (graph->getNodeCount() == 0) return false;

        // ---- 读取边文件 ----
        ifstream fe(edgesFile);
        if (!fe.is_open()) return graph->getNodeCount() > 0;  // 有节点无边的图也能用

        while (fe.getline(line, 256)) {
            trim(line);
            if (line[0] == '#' || line[0] == '\0') continue;

            char from[64], to[64];
            double w;
            int n = sscanf(line, "%s %s %lf", from, to, &w);
            if (n == 3) {
                int fi = graph->queryNode(from);
                int ti = graph->queryNode(to);
                if (fi >= 0 && ti >= 0) {
                    graph->addEdge(fi, ti, w);    // 添加单向边
                }
            }
        }
        fe.close();
        return true;
    }

    // 导出当前路网到文件（保存修改后的数据）
    bool exportNetwork(const char* nodesFile, const char* edgesFile) {
        int n = graph->getNodeCount();

        // 写节点
        ofstream fn(nodesFile);
        if (!fn.is_open()) return false;
        for (int i = 0; i < n; i++) {
            fn << graph->getNodeName(i) << " "
               << graph->getNodeX(i)    << " "
               << graph->getNodeY(i)    << "\n";
        }
        fn.close();

        // 写边
        ofstream fe(edgesFile);
        if (!fe.is_open()) return false;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                double w = graph->getWeight(i, j);
                if (w != INF) {
                    fe << graph->getNodeName(i) << " "
                       << graph->getNodeName(j) << " "
                       << w << "\n";
                }
            }
        }
        fe.close();
        return true;
    }

    // ================================================================
    // 名字 ↔ 索引 映射
    // ================================================================

    int  findIndex(const char* name)   { return graph->queryNode(name); }
    const char* getName(int idx)       { return graph->getNodeName(idx); }

    // ================================================================
    // 网点 CRUD
    // ================================================================

    bool addNode(const char* name, double x, double y) {
        if (graph->queryNode(name) >= 0) return false;  // 重名拦截
        graph->addNode(name, x, y);
        return true;
    }

    bool removeNode(const char* name) {
        int idx = graph->queryNode(name);
        if (idx < 0) return false;
        return graph->removeNode(idx);
    }

    bool updateNode(const char* name, double newX, double newY) {
        int idx = graph->queryNode(name);
        if (idx < 0) return false;
        return graph->updateNode(idx, name, newX, newY);
    }

    int  queryNode(const char* name) { return graph->queryNode(name); }

    bool addEdge(const char* from, const char* to, double weight) {
        int fi = graph->queryNode(from);
        int ti = graph->queryNode(to);
        if (fi < 0 || ti < 0) return false;
        return graph->addEdge(fi, ti, weight);
    }

    bool removeEdge(const char* from, const char* to) {
        int fi = graph->queryNode(from);
        int ti = graph->queryNode(to);
        if (fi < 0 || ti < 0) return false;
        return graph->removeEdge(fi, ti);
    }

    // ================================================================
    // 最短路径查询
    // ================================================================

    PathResult findShortestPath(const char* from, const char* to) {
        PathResult r;
        r.reachable = false;
        r.cost = INF;

        // 名字 → 索引
        int si = graph->queryNode(from);
        int ei = graph->queryNode(to);
        if (si < 0 || ei < 0) return r;      // 名字不存在

        // 调用图引擎的 Dijkstra
        r.path = graph->dijkstra(si, ei, r.cost);
        r.reachable = (r.path.getSize() > 0);
        return r;
    }

    // ================================================================
    // 拓扑排序（批次配送依赖检测）
    // ================================================================
    // depFile 格式：每行 "前置网点 后置网点"
    //   表示"前置网点"必须先于"后置网点"送达
    // ================================================================

    TopoResult checkDependencies(const char* depFile) {
        TopoResult r;
        r.hasCycle = false;

        MyVector<Edge> deps;
        ifstream fd(depFile);
        if (!fd.is_open()) {
            r.hasCycle = true;    // 文件打不开当作错误处理
            return r;
        }

        char line[256];
        while (fd.getline(line, 256)) {
            trim(line);
            if (line[0] == '#' || line[0] == '\0') continue;

            char from[64], to[64];
            int n = sscanf(line, "%s %s", from, to);
            if (n == 2) {
                int fi = graph->queryNode(from);
                int ti = graph->queryNode(to);
                if (fi >= 0 && ti >= 0) {
                    Edge e;
                    e.from   = fi;
                    e.to     = ti;
                    e.weight = 1.0;     // 拓扑排序不关心权重，随便填
                    deps.push_back(e);
                }
            }
        }
        fd.close();

        r.order = graph->topologicalSort(deps, r.hasCycle);
        return r;
    }

    // ================================================================
    // 批量配送订单处理
    // ================================================================
    // orderFile 格式：每行 "包裹编号 起点网点 终点网点"
    // outFile   格式：每个包裹的路径 + 耗时 + 成功/失败状态
    // 返回值：成功处理的订单数，-1 表示文件打开失败
    // ================================================================

    int batchProcessOrders(const char* orderFile, const char* outFile) {
        ifstream fo(orderFile);
        if (!fo.is_open()) return -1;

        ofstream out(outFile);
        if (!out.is_open()) { fo.close(); return -1; }

        out << "========================================\n";
        out << "  批量配送方案\n";
        out << "========================================\n\n";

        int success = 0;
        int fail    = 0;
        char line[256];

        while (fo.getline(line, 256)) {
            trim(line);
            if (line[0] == '#' || line[0] == '\0') continue;

            char pid[64], from[64], to[64];
            int n = sscanf(line, "%s %s %s", pid, from, to);
            if (n != 3) continue;    // 格式不对的行跳过

            PathResult r = findShortestPath(from, to);

            out << "[" << pid << "] " << from << " -> " << to << "\n";
            if (r.reachable) {
                out << "  路径: ";
                for (int i = 0; i < r.path.getSize(); i++) {
                    if (i > 0) out << " -> ";
                    out << graph->getNodeName(r.path[i]);
                }
                out << "\n  总耗时: " << r.cost << " 小时\n";
                out << "  状态: 配送成功\n\n";
                success++;
            } else {
                out << "  路径: (无路径)\n";
                out << "  状态: 不可达\n\n";
                fail++;
            }
        }

        out << "========================================\n";
        out << "  汇总: 成功 " << success << " / 失败 " << fail << "\n";
        out << "========================================\n";

        fo.close();
        out.close();
        return success;
    }
};

#endif
