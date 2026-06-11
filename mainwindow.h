#pragma once
// ============================================================================
// MainWindow.h — Qt 图形界面
// ============================================================================
// 界面结构：
//  ┌─────────────────────────────────────────────────────┐
//  │ 菜单栏: [文件] 导入路网 | 导出路网 | 导入订单 | 导出方案 │
//  ├───────────────┬─────────────────────────────────────┤
//  │ 左侧导航面板   │                                     │
//  │  (深色主题)    │         GraphWidget 绘图区           │
//  │               │         (交互式画布)                 │
//  │  统计信息      │                                     │
//  │  [按钮列表]    │    - 鼠标悬停 → 节点详情浮窗         │
//  │  (多页切换)    │    - 鼠标点击 → 选中节点             │
//  │               │    - 箭头 + 高亮路径                 │
//  │  操作日志      │                                     │
//  ├───────────────┴─────────────────────────────────────┤
//  │ 状态栏: 悬停提示 | 就绪                               │
//  └─────────────────────────────────────────────────────┘
//
// 导航方式：QStackedWidget 多页面（主菜单 / 网点管理 / 路网管理 / 路径查询 / 批次配送）
// 操作方式：弹出对话框（QDialog + QFormLayout）
// 画布交互：GraphWidget 支持鼠标悬停（浮窗详情）和点击（选中节点）
// ============================================================================

#include <QMainWindow>
#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QStackedWidget>
#include <QPointF>

#include "Graph.h"
#include "OrderManager.h"

// ============================================================================
// GraphWidget — 路网画布（交互式）
// ============================================================================
// 绘图功能：
//   - 灰色箭头边 + 高亮路径（橙色粗线）
//   - 渐变圆形节点 + 编号 + 名称标签
//   - 起点绿色 / 终点红色 / 路径节点橙色 / 默认蓝色
//
// 交互功能：
//   - 鼠标悬停：节点变亮蓝 + 弹出详情浮窗（编号/名称/地址/出边数）
//   - 鼠标点击：发射 nodeClicked 信号
//
// 坐标系统：
//   节点的 (lon, lat) 经纬度 → 屏幕坐标 (x, y)
//   自动缩放：找所有节点的经纬度 min/max → 等比映射到画布区域 + 10px 边距
// ============================================================================
class GraphWidget : public QWidget {
    Q_OBJECT

public:
    explicit GraphWidget(QWidget* parent = nullptr);

    // 设置图数据指针并刷新坐标
    void setGraph(const Graph* g);

    // 设置高亮：（路径节点, 路径边源列表, 路径边目标列表, 起点, 终点）
    void setHL(const DynArray<int>& nodes, const DynArray<int>& edgeSrc,
               const DynArray<int>& edgeDst, int src, int dst);
    void clearHL();

signals:
    void nodeHovered(int id);   // 鼠标悬停在节点上
    void nodeClicked(int id);   // 鼠标点击节点

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    const Graph* g_ = nullptr;

    // 节点编号 → 屏幕坐标（由 updatePos 根据经纬度计算）
    HashMap<QPointF> pos_;

    // 高亮状态
    DynArray<int> hlNodes_;      // 高亮节点
    DynArray<int> hlEdgeSrc_;    // 高亮边起点
    DynArray<int> hlEdgeDst_;    // 高亮边终点
    int srcHL_ = -1;             // 起点（绿色）
    int dstHL_ = -1;             // 终点（红色）
    int hovered_ = -1;           // 当前悬停节点

    static constexpr float R = 18.0f;  // 节点圆半径

    void updatePos();                    // 根据经纬度重新计算所有节点屏幕坐标
    bool isNodeHL(int id) const;         // 节点是否高亮
    bool isEdgeHL(int f, int t) const;   // 边是否高亮
    int  nodeAt(QPoint p) const;         // 屏幕坐标 → 节点编号（-1=无）
    void drawEdge(class QPainter& p, QPointF a, QPointF b, QColor col, float w, bool bidirectional);
    void drawNode(class QPainter& p, int id, QPointF pos, QColor col);
};

// ============================================================================
// MainWindow — 主窗口
// ============================================================================
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    // 页面导航
    void goMain();
    void goNode();
    void goNetwork();
    void goPath();
    void goDelivery();

    // 网点管理
    void onAddNode();
    void onDeleteNode();
    void onUpdateNode();
    void onFindNode();
    void onListNodes();

    // 路网管理
    void onAddEdge();
    void onDeleteEdge();
    void onListEdges();
    void onImportNet();
    void onExportNet();

    // 路径查询
    void onShortestTime();
    void onCheapestPath();
    void onClearHL();

    // 批次配送
    void onAddOrder();
    void onDelOrder();
    void onListOrders();
    void onImportOrd();
    void onPlanAll();
    void onTopoSort();
    void onExportPlans();

private:
    // 数据
    Graph        graph_;
    OrderManager orders_;

    // 高亮状态
    DynArray<int> hlNodes_, hlEdgeSrc_, hlEdgeDst_;
    int srcHL_ = -1, dstHL_ = -1;

    // 控件
    QStackedWidget* stack_ = nullptr;
    QLabel*         stats_ = nullptr;
    QTextEdit*      logBox_ = nullptr;
    GraphWidget*    canvas_ = nullptr;
    QLabel*         modeLabel_ = nullptr;

    // 构建函数
    void buildUI();
    class QPushButton* makeBtn(const QString& text, bool secondary = false);
    QWidget* makePage(std::initializer_list<std::pair<QString, void(MainWindow::*)()>> items);

    // 辅助函数
    void refreshStats();
    void refreshCanvas();
    void log(const QString& msg, const QString& color = "#dcdcdc");
    void logOK(const QString& msg);
    void logErr(const QString& msg);
    void applyPathToHL(const DynArray<int>& path);
    void clearHL();
};
