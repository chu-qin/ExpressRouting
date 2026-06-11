#pragma once
// ============================================================================
// MainWindow.h — Qt 图形界面
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
class GraphWidget : public QWidget {
    Q_OBJECT

public:
    explicit GraphWidget(QWidget* parent = nullptr);

    void setGraph(const Graph* graph);
    void setHighlight(const DynArray<int>& nodeList, const DynArray<int>& edgeSrcList,
                      const DynArray<int>& edgeDstList, int start, int target);
    void clearHighlight();

signals:
    void nodeHovered(int id);
    void nodeClicked(int id);

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    const Graph* graphPtr = nullptr;

    // 节点编号 → 屏幕坐标
    HashMap<QPointF> nodePos;

    // 高亮状态
    DynArray<int> hlNodes;       // 高亮节点
    DynArray<int> hlEdgeSrc;     // 高亮边起点
    DynArray<int> hlEdgeDst;     // 高亮边终点
    int startHL = -1;             // 起点（绿色）
    int targetHL = -1;            // 终点（红色）
    int hoverNode = -1;           // 当前悬停节点

    static constexpr float nodeRadius = 18.0f;

    void updatePositions();
    bool isNodeHL(int id) const;
    bool isEdgeHL(int from, int to) const;
    int  nodeAt(QPoint screenPt) const;
    void drawEdge(class QPainter& painter, QPointF fromPt, QPointF toPt, QColor color, float width, bool isBidir);
    void drawNode(class QPainter& painter, int id, QPointF pos, QColor color);
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
    void goNodePage();
    void goNetworkPage();
    void goPathPage();
    void goDeliveryPage();

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
    Graph        graph;
    OrderManager orderMgr;

    // 高亮
    DynArray<int> hlNodes, hlEdgeSrc, hlEdgeDst;
    int startHL = -1, targetHL = -1;

    // 控件
    QStackedWidget* pageStack = nullptr;
    QLabel*         statsLabel = nullptr;
    QTextEdit*      logBox = nullptr;
    GraphWidget*    canvas = nullptr;
    QLabel*         modeLabel = nullptr;

    // 构建
    void buildUI();
    class QPushButton* makeBtn(const QString& text, bool secondary = false);
    QWidget* makePage(std::initializer_list<std::pair<QString, void(MainWindow::*)()>> items);

    // 辅助
    void refreshStats();
    void refreshCanvas();
    void log(const QString& msg, const QString& color = "#dcdcdc");
    void logOK(const QString& msg);
    void logErr(const QString& msg);
    void applyPathToHL(const DynArray<int>& path);
    void clearHL();
};
