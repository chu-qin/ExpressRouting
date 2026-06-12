#pragma once
// ============================================================================
// mainwindow.h —— 图形界面
// ============================================================================

#include <QMainWindow>
#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QSplitter>
#include <QComboBox>
#include <QPointF>
#include <QPainter>
#include <QColor>
#include <QPushButton>

#include "OrderManager.h"

// ============================================================================
// GraphWidget —— 路网画布
// ============================================================================
class GraphWidget : public QWidget {
    Q_OBJECT

public:
    explicit GraphWidget(QWidget* parent = nullptr);

    void setGraph(const Graph* g);
    void highlightPath(const DynArray<int>& path);
    void clearHighlight();

signals:
    void nodeHovered(int id, const QString& info);
    void nodeClicked(int id);

protected:
    void paintEvent(QPaintEvent*)   override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    const Graph* graph = nullptr;

    // 节点 → 屏幕坐标
    DynArray<QPointF> nodePos;
    float radius = 22.0f;      // 节点圆半径（加大，便于看清）

    // 高亮状态
    DynArray<int> hlNodes;       // 高亮节点列表
    DynArray<int> hlEdges;       // 高亮边列表（边编号 = from*10000+to）
    int startHL   = -1;
    int targetHL  = -1;
    int hoverNode = -1;

    void recalcPositions();
    int  nodeAtPos(QPointF pt) const;
    bool isNodeHL(int id) const;
    bool isEdgeHL(int from, int to) const;

    void drawEdge(QPainter& p, QPointF a, QPointF b,
                  QColor color, float width, double time, double cost);
    void drawNode(QPainter& p, int id, QPointF pos, QColor color);
};

// ============================================================================
// MainWindow —— 主窗口
// ============================================================================
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    // 网点管理
    void onAddNode();
    void onDeleteNode();
    void onUpdateNode();
    void onFindNode();

    // 路网管理
    void onAddEdge();
    void onDeleteEdge();
    void onImportNetwork();
    void onExportNetwork();

    // 路径查询
    void onShortestTime();
    void onCheapestPath();
    void onClearHighlight();

    // 订单管理
    void onAddOrder();
    void onDeleteOrder();
    void onPlanAll();
    void onTopoSort();
    void onImportOrders();
    void onExportPlans();

private:
    // ---- 数据 ----
    Graph         graph;
    OrderManager  orders;

    // ---- 控件 ----
    QSplitter*   splitter   = nullptr;
    QWidget*     leftPanel  = nullptr;
    GraphWidget* canvas     = nullptr;
    QTextEdit*   logBox     = nullptr;
    QLabel*      statusLabel = nullptr;
    QComboBox*   srcCombo   = nullptr;
    QComboBox*   dstCombo   = nullptr;

    // ---- 辅助 ----
    void setupUI();
    void applyStyle();
    void refreshCombo();
    void refreshCanvas();
    void refreshStatus();

    void log(const QString& msg, const QString& color = "#c8c8c8");
    void logOK(const QString& msg);
    void logErr(const QString& msg);

    void applyPathHighlight(const PathResult& pr);

    // 创建按钮的快捷函数
    QPushButton* btn(const QString& text, QWidget* parent);
};
