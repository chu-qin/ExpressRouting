// ============================================================================
// mainwindow.cpp —— 图形界面实现
// ============================================================================
// 布局结构：
//   ┌─ QSplitter(水平) ────────────────────────────┐
//   │  leftPanel                │  QSplitter(垂直)   │
//   │  [网点管理] 按钮区         │  ┌─ canvas ─┐     │
//   │  [路网管理]               │  │  画布    │     │
//   │  [路径查询] 起止点下拉框   │  └──────────┘     │
//   │  [订单管理]               │  ┌─ logBox ─┐     │
//   │                           │  │  日志    │     │
//   │                           │  └──────────┘     │
//   └───────────────────────────┴───────────────────┘
// ============================================================================

#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QSplitter>
#include <QPainter>
#include <QMouseEvent>
#include <QFileDialog>
#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QtMath>

#include <iostream>
#include <cmath>
#include <algorithm>
#include <QStyleHints>
#include <QGuiApplication>

// ================================================================
// GraphWidget —— 路网画布
// ================================================================

GraphWidget::GraphWidget(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);              // 鼠标移动时也触发事件（不需要按下）
    setMinimumSize(500, 400);
}

void GraphWidget::setGraph(const Graph* g) {
    graph = g;
    nodePos.clear();
    if (graph) {
        int N = graph->maxNodeId();
        for (int i = 0; i < N; ++i) nodePos.push_back(QPointF());
    }
    recalcPositions();
    update();
}

void GraphWidget::highlightPath(const DynArray<int>& path) {
    clearHighlight();
    if (path.empty()) return;

    startHL  = path[0];
    targetHL = path[path.size() - 1];

    for (int i = 0; i < path.size(); ++i)
        hlNodes.push_back(path[i]);

    for (int i = 0; i + 1 < (int)path.size(); ++i) {
        int from = path[i];
        int to   = path[i + 1];
        hlEdges.push_back(from * 10000 + to);   // 编码边：from*10000+to
    }
    update();
}

void GraphWidget::clearHighlight() {
    hlNodes.clear();
    hlEdges.clear();
    startHL  = -1;
    targetHL = -1;
    update();
}

// ---- 坐标计算 ----
void GraphWidget::recalcPositions() {
    if (!graph) return;

    int N = graph->maxNodeId();
    // 找到经纬度范围
    double minLon = 1e9, maxLon = -1e9;
    double minLat = 1e9, maxLat = -1e9;
    bool hasCoord = false;

    for (int i = 0; i < N; ++i) {
        const Node* n = graph->findNode(i);
        if (!n) continue;
        if (n->lon != 0 || n->lat != 0) {
            hasCoord = true;
            if (n->lon < minLon) minLon = n->lon;
            if (n->lon > maxLon) maxLon = n->lon;
            if (n->lat < minLat) minLat = n->lat;
            if (n->lat > maxLat) maxLat = n->lat;
        }
    }

    float pad = 70.0f;
    float w   = width()  - pad * 2;
    float h   = height() - pad * 2;

    if (w < 100) w = 100;
    if (h < 100) h = 100;

    double lonRange = maxLon - minLon;
    double latRange = maxLat - minLat;

    // 没有坐标数据时用圆形布局
    if (!hasCoord || lonRange < 0.01 || latRange < 0.01) {
        int    cnt  = graph->nodeCount();
        double cx   = width()  / 2.0;
        double cy   = height() / 2.0;
        double r    = qMin(w, h) / 2.0;
        int    idx  = 0;

        for (int i = 0; i < N; ++i) {
            if (!graph->hasNode(i)) continue;
            double angle = 2 * 3.14159265 * idx / cnt - 3.14159265 / 2;
            nodePos[i] = QPointF(cx + r * std::cos(angle),
                                  cy + r * std::sin(angle));
            ++idx;
        }
        return;
    }

    // 有经纬度时做线性映射
    for (int i = 0; i < N; ++i) {
        const Node* n = graph->findNode(i);
        if (!n) continue;

        double x = pad + w * (n->lon - minLon) / lonRange;
        double y = pad + h * (1.0 - (n->lat - minLat) / latRange);
        nodePos[i] = QPointF(x, y);
    }
}

int GraphWidget::nodeAtPos(QPointF pt) const {
    if (!graph) return -1;
    int N = graph->maxNodeId();
    for (int i = 0; i < N; ++i) {
        if (!graph->hasNode(i)) continue;
        double dx = pt.x() - nodePos[i].x();
        double dy = pt.y() - nodePos[i].y();
        if (dx * dx + dy * dy <= radius * radius)
            return i;
    }
    return -1;
}

bool GraphWidget::isNodeHL(int id) const {
    for (int i = 0; i < hlNodes.size(); ++i)
        if (hlNodes[i] == id) return true;
    return false;
}

bool GraphWidget::isEdgeHL(int from, int to) const {
    int code = from * 10000 + to;
    for (int i = 0; i < hlEdges.size(); ++i)
        if (hlEdges[i] == code) return true;
    return false;
}

// ---- 判断系统是否为暗色主题 ----
static bool isSystemDark() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
#endif
}

// ---- 绘制 ----
void GraphWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    bool dark = isSystemDark();

    // 背景：跟随系统主题
    p.fillRect(rect(), dark ? QColor(30, 30, 30) : QColor(245, 245, 245));

    if (!graph) return;

    QColor edgeNormal  = dark ? QColor(110, 110, 110) : QColor(160, 160, 160);
    QColor edgeHL      = QColor(255, 180, 40);

    // 先画所有边
    int N = graph->maxNodeId();
    for (int i = 0; i < N; ++i) {
        if (!graph->hasNode(i)) continue;
        const DynArray<Edge>& edges = graph->getNeighbors(i);
        for (int j = 0; j < edges.size(); ++j) {
            QColor clr = isEdgeHL(i, edges[j].to) ? edgeHL : edgeNormal;
            float  wid  = isEdgeHL(i, edges[j].to) ? 2.5f : 1.0f;
            drawEdge(p, nodePos[i], nodePos[edges[j].to],
                     clr, wid, edges[j].time, edges[j].cost);
        }
    }

    // 再画所有节点（在边上面）
    for (int i = 0; i < N; ++i) {
        if (!graph->hasNode(i)) continue;

        QColor color(80, 160, 220);    // 默认蓝色
        if (i == hoverNode)            color = QColor(200, 220, 100);  // 悬停
        if (i == startHL)              color = QColor(80, 220, 80);    // 起点绿
        if (i == targetHL)             color = QColor(255, 80, 80);    // 终点红
        if (isNodeHL(i) && i != startHL && i != targetHL)
            color = QColor(255, 180, 40);    // 路径中节点黄

        drawNode(p, i, nodePos[i], color);
    }
}

void GraphWidget::drawEdge(QPainter& p, QPointF a, QPointF b,
                           QColor color, float width,
                           double time, double cost) {
    p.save();
    QPen pen(color, width);
    p.setPen(pen);

    // 计算方向向量
    double dx = b.x() - a.x();
    double dy = b.y() - a.y();
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1) { p.restore(); return; }

    double ux = dx / len;   // 单位方向
    double uy = dy / len;

    // 从圆边缘开始画（而非圆心）
    QPointF from(a.x() + ux * radius, a.y() + uy * radius);
    QPointF to(b.x() - ux * radius, b.y() - uy * radius);

    p.drawLine(from, to);

    // 箭头
    double arrowLen = 10.0;
    QPointF arrow1(to.x() - arrowLen * (ux * 0.866 - uy * 0.5),
                    to.y() - arrowLen * (uy * 0.866 + ux * 0.5));
    QPointF arrow2(to.x() - arrowLen * (ux * 0.866 + uy * 0.5),
                    to.y() - arrowLen * (uy * 0.866 - ux * 0.5));

    QPen arrowPen(color, width);
    p.setPen(arrowPen);
    p.setBrush(color);
    QPointF tri[3] = { to, arrow1, arrow2 };
    p.drawPolygon(tri, 3);

    // 权重标签——从线段中点垂直偏移，避免叠在线上
    double offsetDist = 14.0;
    double labelX = (from.x() + to.x()) / 2.0 - uy * offsetDist;
    double labelY = (from.y() + to.y()) / 2.0 + ux * offsetDist;

    p.setFont(QFont("Consolas", 10));
    QString label = QString("%1h ¥%2").arg(time, 0, 'f', 1).arg(cost, 0, 'f', 0);

    // 半透明背景（根据主题切换底色）
    bool dark = isSystemDark();
    QFontMetrics fm(p.font());
    int textW = fm.horizontalAdvance(label) + 8;
    int textH = fm.height() + 4;
    p.setPen(Qt::NoPen);
    p.setBrush(dark ? QColor(0, 0, 0, 150) : QColor(255, 255, 255, 170));
    p.drawRoundedRect(QRectF(labelX - textW/2.0, labelY - textH/2.0, textW, textH), 4, 4);

    // 文字
    p.setPen(dark ? QColor(220, 220, 220) : QColor(40, 40, 40));
    p.drawText(QRectF(labelX - textW/2.0, labelY - textH/2.0, textW, textH),
               Qt::AlignCenter, label);

    p.restore();
}

void GraphWidget::drawNode(QPainter& p, int id, QPointF pos, QColor color) {
    p.save();
    // 外圈
    p.setPen(QPen(color.darker(130), 2.5));
    p.setBrush(color);
    p.drawEllipse(pos, radius, radius);

    // 编号（圈内大字）
    p.setPen(Qt::white);
    p.setFont(QFont("Consolas", 12, QFont::Bold));
    p.drawText(QRectF(pos.x() - radius, pos.y() - radius,
                       radius * 2, radius * 2),
               Qt::AlignCenter, QString::number(id));

    // 城市名（圈下方）
    if (graph && graph->hasNode(id)) {
        const Node* n = graph->findNode(id);
        QString name = QString::fromStdString(n->name);
        if (name.length() > 6) name = name.left(5) + "...";

        bool dark = isSystemDark();
        p.setFont(QFont("Microsoft YaHei", 9));
        QFontMetrics fm(p.font());
        int nw = fm.horizontalAdvance(name) + 6;
        int nh = fm.height() + 2;
        double ny = pos.y() + radius + 3;
        p.setPen(Qt::NoPen);
        p.setBrush(dark ? QColor(0, 0, 0, 140) : QColor(255, 255, 255, 170));
        p.drawRoundedRect(QRectF(pos.x() - nw/2.0, ny, nw, nh), 3, 3);
        p.setPen(dark ? QColor(220, 220, 220) : QColor(50, 50, 50));
        p.drawText(QRectF(pos.x() - nw/2.0, ny, nw, nh), Qt::AlignCenter, name);
    }
    p.restore();
}

// ---- 鼠标事件 ----
void GraphWidget::mouseMoveEvent(QMouseEvent* ev) {
    int prev = hoverNode;
    hoverNode = nodeAtPos(ev->position());
    if (hoverNode != prev) {
        update();
        if (hoverNode >= 0 && graph->hasNode(hoverNode)) {
            const Node* n = graph->findNode(hoverNode);
            emit nodeHovered(hoverNode,
                QString("[%1] %2").arg(hoverNode).arg(QString::fromStdString(n->name)));
        } else {
            emit nodeHovered(-1, "");
        }
    }
}

void GraphWidget::mousePressEvent(QMouseEvent* ev) {
    int id = nodeAtPos(ev->position());
    if (id >= 0) emit nodeClicked(id);
}

void GraphWidget::resizeEvent(QResizeEvent*) {
    recalcPositions();
    update();
}

// ================================================================
// MainWindow —— 主窗口
// ================================================================

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    applyStyle();
    resize(1200, 750);
    setWindowTitle("快递网点配送路径规划系统");
}

// ---- 辅助：创建按钮 ----
QPushButton* MainWindow::btn(const QString& text, QWidget* parent) {
    QPushButton* b = new QPushButton(text, parent);
    b->setMinimumHeight(30);
    return b;
}

// ---- 构建 UI ----
void MainWindow::setupUI() {
    // ==== 左侧面板 ====
    leftPanel = new QWidget;
    leftPanel->setFixedWidth(160);
    QVBoxLayout* leftLay = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(6, 6, 6, 6);
    leftLay->setSpacing(5);

    // — 网点管理 —
    QGroupBox* gbNode = new QGroupBox("网点管理");
    QVBoxLayout* nodeLay = new QVBoxLayout(gbNode);
    nodeLay->setSpacing(3);
    nodeLay->addWidget(btn("添加网点", gbNode));
    nodeLay->addWidget(btn("删除网点", gbNode));
    nodeLay->addWidget(btn("修改网点", gbNode));
    nodeLay->addWidget(btn("查询网点", gbNode));
    leftLay->addWidget(gbNode);

    // 连接信号
    QList<QPushButton*> nodeBtns = gbNode->findChildren<QPushButton*>();
    connect(nodeBtns[0], &QPushButton::clicked, this, &MainWindow::onAddNode);
    connect(nodeBtns[1], &QPushButton::clicked, this, &MainWindow::onDeleteNode);
    connect(nodeBtns[2], &QPushButton::clicked, this, &MainWindow::onUpdateNode);
    connect(nodeBtns[3], &QPushButton::clicked, this, &MainWindow::onFindNode);

    // — 路网管理 —
    QGroupBox* gbEdge = new QGroupBox("路网管理");
    QVBoxLayout* edgeLay = new QVBoxLayout(gbEdge);
    edgeLay->setSpacing(3);
    edgeLay->addWidget(btn("添加路线", gbEdge));
    edgeLay->addWidget(btn("删除路线", gbEdge));
    edgeLay->addWidget(btn("导入路网", gbEdge));
    edgeLay->addWidget(btn("导出路网", gbEdge));
    leftLay->addWidget(gbEdge);

    QList<QPushButton*> edgeBtns = gbEdge->findChildren<QPushButton*>();
    connect(edgeBtns[0], &QPushButton::clicked, this, &MainWindow::onAddEdge);
    connect(edgeBtns[1], &QPushButton::clicked, this, &MainWindow::onDeleteEdge);
    connect(edgeBtns[2], &QPushButton::clicked, this, &MainWindow::onImportNetwork);
    connect(edgeBtns[3], &QPushButton::clicked, this, &MainWindow::onExportNetwork);

    // — 路径查询 —
    QGroupBox* gbPath = new QGroupBox("路径查询");
    QVBoxLayout* pathLay = new QVBoxLayout(gbPath);
    pathLay->setSpacing(3);

    pathLay->addWidget(new QLabel("起点:"));
    srcCombo = new QComboBox;
    pathLay->addWidget(srcCombo);

    pathLay->addWidget(new QLabel("终点:"));
    dstCombo = new QComboBox;
    pathLay->addWidget(dstCombo);

    pathLay->addWidget(btn("最短耗时", gbPath));
    pathLay->addWidget(btn("最低费用", gbPath));
    pathLay->addWidget(btn("清除高亮", gbPath));
    leftLay->addWidget(gbPath);

    QList<QPushButton*> pathBtns = gbPath->findChildren<QPushButton*>();
    connect(pathBtns[0], &QPushButton::clicked, this, &MainWindow::onShortestTime);
    connect(pathBtns[1], &QPushButton::clicked, this, &MainWindow::onCheapestPath);
    connect(pathBtns[2], &QPushButton::clicked, this, &MainWindow::onClearHighlight);

    // — 订单管理 —
    QGroupBox* gbOrder = new QGroupBox("订单管理");
    QVBoxLayout* orderLay = new QVBoxLayout(gbOrder);
    orderLay->setSpacing(3);
    orderLay->addWidget(btn("添加订单", gbOrder));
    orderLay->addWidget(btn("删除订单", gbOrder));
    orderLay->addWidget(btn("批量规划", gbOrder));
    orderLay->addWidget(btn("拓扑排序", gbOrder));
    orderLay->addWidget(btn("导入订单", gbOrder));
    orderLay->addWidget(btn("导出方案", gbOrder));
    leftLay->addWidget(gbOrder);

    QList<QPushButton*> orderBtns = gbOrder->findChildren<QPushButton*>();
    connect(orderBtns[0], &QPushButton::clicked, this, &MainWindow::onAddOrder);
    connect(orderBtns[1], &QPushButton::clicked, this, &MainWindow::onDeleteOrder);
    connect(orderBtns[2], &QPushButton::clicked, this, &MainWindow::onPlanAll);
    connect(orderBtns[3], &QPushButton::clicked, this, &MainWindow::onTopoSort);
    connect(orderBtns[4], &QPushButton::clicked, this, &MainWindow::onImportOrders);
    connect(orderBtns[5], &QPushButton::clicked, this, &MainWindow::onExportPlans);

    leftLay->addStretch();

    // 左侧面板放 ScrollArea
    QScrollArea* scroll = new QScrollArea;
    scroll->setWidget(leftPanel);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    // ==== 右侧：画布 + 日志 ====
    canvas = new GraphWidget;
    canvas->setMinimumHeight(300);

    // 点击画布节点 → 填入下拉框
    connect(canvas, &GraphWidget::nodeClicked, this, [this](int id) {
        if (id >= 0) {
            if (srcCombo->currentIndex() < 0 || QApplication::keyboardModifiers() & Qt::ShiftModifier)
                dstCombo->setCurrentIndex(dstCombo->findText(QString::number(id)));
            else
                srcCombo->setCurrentIndex(srcCombo->findText(QString::number(id)));
            logOK(QString("选中节点 %1").arg(id));
        }
    });
    connect(canvas, &GraphWidget::nodeHovered, this, [this](int, const QString& info) {
        if (!info.isEmpty()) statusLabel->setText(info);
    });

    logBox = new QTextEdit;
    logBox->setReadOnly(true);
    logBox->setMinimumHeight(100);
    logBox->setMaximumHeight(180);
    logBox->setFont(QFont("Consolas", 9));

    QSplitter* rightSplit = new QSplitter(Qt::Vertical);
    rightSplit->addWidget(canvas);
    rightSplit->addWidget(logBox);
    rightSplit->setStretchFactor(0, 3);
    rightSplit->setStretchFactor(1, 1);

    // ==== 水平分割 ====
    splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(scroll);
    splitter->addWidget(rightSplit);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);

    // ==== 状态栏 ====
    statusLabel = new QLabel("就绪");
    statusBar()->addWidget(statusLabel);

    // ==== 菜单栏 ====
    QMenu* fileMenu = menuBar()->addMenu("文件");
    fileMenu->addAction("导入路网", this, &MainWindow::onImportNetwork);
    fileMenu->addAction("导出路网", this, &MainWindow::onExportNetwork);
    fileMenu->addSeparator();
    fileMenu->addAction("导入订单", this, &MainWindow::onImportOrders);
    fileMenu->addAction("导出方案", this, &MainWindow::onExportPlans);
    fileMenu->addSeparator();
    fileMenu->addAction("退出", this, &QWidget::close);

    QMenu* helpMenu = menuBar()->addMenu("帮助");
    helpMenu->addAction("关于", this, [this]() {
        QMessageBox::about(this, "关于",
            "快递网点配送路径规划系统\n"
            "数据结构与算法 综合实验\n\n"
            "核心算法：Dijkstra 最短路径 + Kahn 拓扑排序");
    });
}

// ---- 暗色主题 ----
void MainWindow::applyStyle() {
    bool dark = isSystemDark();

    if (dark) {
        setStyleSheet(R"(
            QMainWindow, QWidget { background-color: #1e1e1e; color: #c8c8c8; font-size: 13px; }
            QGroupBox { border: 1px solid #444; border-radius: 4px; margin-top: 10px; padding-top: 14px; font-weight: bold; color: #88c0ff; }
            QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
            QPushButton { background: #333; border: 1px solid #555; border-radius: 3px; padding: 4px 8px; color: #c8c8c8; }
            QPushButton:hover { background: #444; border-color: #88c0ff; }
            QPushButton:pressed { background: #2a2a2a; }
            QComboBox { background: #2a2a2a; border: 1px solid #555; border-radius: 3px; padding: 2px 6px; color: #c8c8c8; }
            QComboBox:hover { border-color: #88c0ff; }
            QComboBox QAbstractItemView { background: #2a2a2a; color: #c8c8c8; selection-background-color: #444; }
            QTextEdit { background: #1a1a1a; border: 1px solid #444; color: #a0a0a0; }
            QSplitter::handle { background: #333; width: 2px; height: 2px; }
            QScrollArea { border: none; }
            QStatusBar { background: #111; color: #888; }
            QMenuBar { background: #111; color: #c8c8c8; }
            QMenuBar::item:selected { background: #333; }
            QMenu { background: #222; color: #c8c8c8; border: 1px solid #444; }
            QMenu::item:selected { background: #444; }
            QLabel { color: #aaa; font-size: 12px; }
        )");
    } else {
        setStyleSheet(R"(
            QMainWindow, QWidget { background-color: #f5f5f5; color: #333; font-size: 13px; }
            QGroupBox { border: 1px solid #ccc; border-radius: 4px; margin-top: 10px; padding-top: 14px; font-weight: bold; color: #2060a0; }
            QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
            QPushButton { background: #e8e8e8; border: 1px solid #bbb; border-radius: 3px; padding: 4px 8px; color: #333; }
            QPushButton:hover { background: #ddd; border-color: #2060a0; }
            QPushButton:pressed { background: #ccc; }
            QComboBox { background: #fff; border: 1px solid #bbb; border-radius: 3px; padding: 2px 6px; color: #333; }
            QComboBox:hover { border-color: #2060a0; }
            QComboBox QAbstractItemView { background: #fff; color: #333; selection-background-color: #c0d8f0; }
            QTextEdit { background: #fafafa; border: 1px solid #ccc; color: #444; }
            QSplitter::handle { background: #ccc; width: 2px; height: 2px; }
            QScrollArea { border: none; }
            QStatusBar { background: #e8e8e8; color: #666; }
            QMenuBar { background: #e8e8e8; color: #333; }
            QMenuBar::item:selected { background: #d0d0d0; }
            QMenu { background: #fff; color: #333; border: 1px solid #ccc; }
            QMenu::item:selected { background: #c0d8f0; }
            QLabel { color: #555; font-size: 12px; }
        )");
    }
}

// ---- 辅助 ----
void MainWindow::refreshCombo() {
    srcCombo->clear();
    dstCombo->clear();
    DynArray<int> ids = graph.getAllNodeIds();
    for (int i = 0; i < ids.size(); ++i) {
        QString label = QString::number(ids[i]);
        const Node* n = graph.findNode(ids[i]);
        if (n) label += " " + QString::fromStdString(n->name);
        srcCombo->addItem(label);
        dstCombo->addItem(label);
    }
}

void MainWindow::refreshCanvas() {
    canvas->setGraph(&graph);
}

void MainWindow::refreshStatus() {
    statusLabel->setText(
        QString("节点: %1 | 边: %2 | 订单: %3")
            .arg(graph.nodeCount()).arg(graph.edgeCount()).arg(orders.orderCount()));
}

void MainWindow::log(const QString& msg, const QString& color) {
    logBox->append(QString("<span style='color:%1'>%2</span>").arg(color, msg));
}

void MainWindow::logOK(const QString& msg)  { log("<font color='#80d080'>[OK]</font> " + msg); }
void MainWindow::logErr(const QString& msg) { log("<font color='#ff6060'>[错误]</font> " + msg); }

void MainWindow::applyPathHighlight(const PathResult& pr) {
    if (!pr.reachable) {
        logErr("起点和终点之间不可达");
        return;
    }
    canvas->highlightPath(pr.path);
    logOK(QString("路径: %1 个节点 | 耗时 %2h | 费用 %3元")
              .arg(pr.path.size())
              .arg(pr.totalTime, 0, 'f', 1)
              .arg(pr.totalCost, 0, 'f', 0));
}

// ================================================================
// 网点管理 slot
// ================================================================

void MainWindow::onAddNode() {
    QDialog dlg(this);
    dlg.setWindowTitle("添加网点");
    QFormLayout* form = new QFormLayout(&dlg);

    QLineEdit* nameEdit = new QLineEdit;
    QLineEdit* addrEdit = new QLineEdit;
    QDoubleSpinBox* lonSpin = new QDoubleSpinBox;
    lonSpin->setRange(70, 140); lonSpin->setDecimals(4); lonSpin->setValue(116.4);
    QDoubleSpinBox* latSpin = new QDoubleSpinBox;
    latSpin->setRange(15, 55); latSpin->setDecimals(4); latSpin->setValue(39.9);

    form->addRow("名称:", nameEdit);
    form->addRow("地址:", addrEdit);
    form->addRow("经度:", lonSpin);
    form->addRow("纬度:", latSpin);

    QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(btns);

    if (dlg.exec() == QDialog::Accepted) {
        int nextId = graph.maxNodeId();
        Node n(nextId, nameEdit->text().toStdString(),
               addrEdit->text().toStdString(),
               lonSpin->value(), latSpin->value());

        if (graph.addNode(n)) {
            logOK(QString("添加网点 [%1] %2").arg(nextId).arg(nameEdit->text()));
            refreshCombo();
            refreshCanvas();
            refreshStatus();
        } else {
            logErr("添加失败：名称不能为空");
        }
    }
}

void MainWindow::onDeleteNode() {
    bool ok;
    int id = QInputDialog::getInt(this, "删除网点", "请输入要删除的网点编号:", 0, 0, graph.maxNodeId()-1, 1, &ok);
    if (!ok) return;

    if (graph.deleteNode(id)) {
        logOK(QString("已删除网点 %1").arg(id));
        refreshCombo();
        refreshCanvas();
        refreshStatus();
    } else {
        logErr(QString("删除失败：网点 %1 不存在").arg(id));
    }
}

void MainWindow::onUpdateNode() {
    bool ok;
    int id = QInputDialog::getInt(this, "修改网点", "请输入要修改的网点编号:", 0, 0, graph.maxNodeId()-1, 1, &ok);
    if (!ok || !graph.hasNode(id)) {
        logErr(QString("网点 %1 不存在").arg(id));
        return;
    }

    const Node* old = graph.findNode(id);

    QDialog dlg(this);
    dlg.setWindowTitle(QString("修改网点 %1").arg(id));
    QFormLayout* form = new QFormLayout(&dlg);

    QLineEdit* nameEdit = new QLineEdit(QString::fromStdString(old->name));
    QLineEdit* addrEdit = new QLineEdit(QString::fromStdString(old->address));
    QDoubleSpinBox* lonSpin = new QDoubleSpinBox;
    lonSpin->setRange(70, 140); lonSpin->setDecimals(4); lonSpin->setValue(old->lon);
    QDoubleSpinBox* latSpin = new QDoubleSpinBox;
    latSpin->setRange(15, 55); latSpin->setDecimals(4); latSpin->setValue(old->lat);

    form->addRow("名称:", nameEdit);
    form->addRow("地址:", addrEdit);
    form->addRow("经度:", lonSpin);
    form->addRow("纬度:", latSpin);

    QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(btns);

    if (dlg.exec() == QDialog::Accepted) {
        Node updated(id, nameEdit->text().toStdString(),
                     addrEdit->text().toStdString(),
                     lonSpin->value(), latSpin->value());
        graph.updateNode(id, updated);
        logOK(QString("已修改网点 %1").arg(id));
        refreshCombo();
        refreshCanvas();
    }
}

void MainWindow::onFindNode() {
    bool ok;
    int id = QInputDialog::getInt(this, "查询网点", "请输入网点编号:", 0, 0, graph.maxNodeId()-1, 1, &ok);
    if (!ok) return;

    const Node* n = graph.findNode(id);
    if (n) {
        QString info = QString("网点 [%1] %2\n地址: %3\n坐标: (%4, %5)")
                           .arg(n->id)
                           .arg(QString::fromStdString(n->name))
                           .arg(QString::fromStdString(n->address))
                           .arg(n->lon, 0, 'f', 4).arg(n->lat, 0, 'f', 4);
        logOK(info);
        QMessageBox::information(this, "网点信息", info);
    } else {
        logErr(QString("网点 %1 不存在").arg(id));
    }
}

// ================================================================
// 路网管理 slot
// ================================================================

void MainWindow::onAddEdge() {
    QDialog dlg(this);
    dlg.setWindowTitle("添加路线");
    QFormLayout* form = new QFormLayout(&dlg);

    QSpinBox* fromSpin = new QSpinBox;
    fromSpin->setRange(0, graph.maxNodeId());
    QSpinBox* toSpin = new QSpinBox;
    toSpin->setRange(0, graph.maxNodeId());
    QDoubleSpinBox* timeSpin = new QDoubleSpinBox;
    timeSpin->setRange(0, 999); timeSpin->setDecimals(1); timeSpin->setValue(1.0);
    QDoubleSpinBox* costSpin = new QDoubleSpinBox;
    costSpin->setRange(0, 99999); costSpin->setDecimals(0); costSpin->setValue(50);

    form->addRow("起点编号:", fromSpin);
    form->addRow("终点编号:", toSpin);
    form->addRow("耗时(h):", timeSpin);
    form->addRow("费用(元):", costSpin);

    QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(btns);

    if (dlg.exec() == QDialog::Accepted) {
        if (graph.addEdge(fromSpin->value(), toSpin->value(),
                          timeSpin->value(), costSpin->value())) {
            refreshCanvas();
            refreshStatus();
        }
    }
}

void MainWindow::onDeleteEdge() {
    QDialog dlg(this);
    dlg.setWindowTitle("删除路线");
    QFormLayout* form = new QFormLayout(&dlg);

    QSpinBox* fromSpin = new QSpinBox;
    fromSpin->setRange(0, graph.maxNodeId());
    QSpinBox* toSpin = new QSpinBox;
    toSpin->setRange(0, graph.maxNodeId());

    form->addRow("起点编号:", fromSpin);
    form->addRow("终点编号:", toSpin);

    QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(btns);

    if (dlg.exec() == QDialog::Accepted) {
        if (graph.deleteEdge(fromSpin->value(), toSpin->value())) {
            refreshCanvas();
            refreshStatus();
        }
    }
}

void MainWindow::onImportNetwork() {
    QString path = QFileDialog::getOpenFileName(this, "导入路网", "", "文本文件 (*.txt)");
    if (path.isEmpty()) return;

    if (FileManager::loadNetwork(path.toStdString(), graph)) {
        logOK("路网导入成功");
        refreshCombo();
        refreshCanvas();
        refreshStatus();
    } else {
        logErr("路网导入失败");
    }
}

void MainWindow::onExportNetwork() {
    QString path = QFileDialog::getSaveFileName(this, "导出路网", "network_export.txt", "文本文件 (*.txt)");
    if (path.isEmpty()) return;

    if (FileManager::saveNetwork(path.toStdString(), graph))
        logOK("路网导出成功 → " + path);
    else
        logErr("路网导出失败");
}

// ================================================================
// 路径查询 slot
// ================================================================

void MainWindow::onShortestTime() {
    if (srcCombo->currentIndex() < 0 || dstCombo->currentIndex() < 0) {
        logErr("请先选择起点和终点");
        return;
    }

    int src = srcCombo->currentText().split(" ")[0].toInt();
    int dst = dstCombo->currentText().split(" ")[0].toInt();

    DynArray<PathResult> all = Dijkstra::shortestTime(graph, src);
    PathResult pr = all[dst];
    applyPathHighlight(pr);
    refreshStatus();
}

void MainWindow::onCheapestPath() {
    if (srcCombo->currentIndex() < 0 || dstCombo->currentIndex() < 0) {
        logErr("请先选择起点和终点");
        return;
    }

    int src = srcCombo->currentText().split(" ")[0].toInt();
    int dst = dstCombo->currentText().split(" ")[0].toInt();

    PathResult pr = Dijkstra::cheapestPath(graph, src, dst);
    applyPathHighlight(pr);
    refreshStatus();
}

void MainWindow::onClearHighlight() {
    canvas->clearHighlight();
    logOK("已清除路径高亮");
}

// ================================================================
// 订单管理 slot
// ================================================================

void MainWindow::onAddOrder() {
    QDialog dlg(this);
    dlg.setWindowTitle("添加订单");
    QFormLayout* form = new QFormLayout(&dlg);

    QSpinBox* idSpin = new QSpinBox;
    idSpin->setRange(1, 99999); idSpin->setValue(orders.orderCount() + 1001);
    QSpinBox* srcSpin = new QSpinBox;
    srcSpin->setRange(0, graph.maxNodeId());
    QSpinBox* dstSpin = new QSpinBox;
    dstSpin->setRange(0, graph.maxNodeId());
    QLineEdit* goodsEdit = new QLineEdit("普通包裹");
    QComboBox* optCombo = new QComboBox;
    optCombo->addItem("最短耗时", 1);
    optCombo->addItem("最低费用", 0);

    form->addRow("订单号:", idSpin);
    form->addRow("起点:", srcSpin);
    form->addRow("终点:", dstSpin);
    form->addRow("货物:", goodsEdit);
    form->addRow("优化目标:", optCombo);

    QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(btns);

    if (dlg.exec() == QDialog::Accepted) {
        Order o;
        o.orderId    = idSpin->value();
        o.srcNode    = srcSpin->value();
        o.dstNode    = dstSpin->value();
        o.goods      = goodsEdit->text().toStdString();
        o.preferTime = optCombo->currentData().toInt();

        if (orders.addOrder(o)) {
            logOK(QString("已添加订单 %1").arg(o.orderId));
            refreshStatus();
        }
    }
}

void MainWindow::onDeleteOrder() {
    bool ok;
    int id = QInputDialog::getInt(this, "删除订单", "请输入订单号:", 1001, 1, 99999, 1, &ok);
    if (!ok) return;

    if (orders.deleteOrder(id)) {
        logOK(QString("已删除订单 %1").arg(id));
        refreshStatus();
    }
}

void MainWindow::onPlanAll() {
    if (orders.orderCount() == 0) {
        logErr("当前无订单，请先添加或导入订单");
        return;
    }

    DynArray<DeliveryPlan> plans = orders.planAll(graph);

    int reachable = 0;
    for (int i = 0; i < plans.size(); ++i)
        if (plans[i].result.reachable) ++reachable;

    logOK(QString("批量规划完成：%1/%2 条可送达").arg(reachable).arg(plans.size()));
    refreshStatus();
}

void MainWindow::onTopoSort() {
    if (orders.orderCount() == 0) {
        logErr("当前无订单，请先添加或导入订单");
        return;
    }

    TopoResult tr = orders.planBatch(graph);

    if (tr.hasCycle) {
        QString nodes;
        for (int i = 0; i < tr.cycleNodes.size(); ++i) {
            if (i > 0) nodes += ", ";
            nodes += QString::number(tr.cycleNodes[i]);
        }
        logErr(QString("检测到配送环路！涉及节点: %1").arg(nodes));
    } else {
        QString seq;
        for (int i = 0; i < tr.order.size(); ++i) {
            if (i > 0) seq += " → ";
            seq += QString::number(tr.order[i]);
        }
        logOK(QString("拓扑排序完成，配送顺序: %1").arg(seq));
    }
    refreshStatus();
}

void MainWindow::onImportOrders() {
    QString path = QFileDialog::getOpenFileName(this, "导入订单", "", "文本文件 (*.txt)");
    if (path.isEmpty()) return;

    FileManager::loadOrders(path.toStdString(), orders);
    refreshStatus();
}

void MainWindow::onExportPlans() {
    if (orders.orderCount() == 0) {
        logErr("当前无订单，请先规划配送方案");
        return;
    }

    DynArray<DeliveryPlan> plans = orders.planAll(graph);

    QString path = QFileDialog::getSaveFileName(this, "导出配送方案", "plans.txt", "文本文件 (*.txt)");
    if (path.isEmpty()) return;

    FileManager::savePlans(path.toStdString(), plans);
    logOK("配送方案已导出 → " + path);
}
