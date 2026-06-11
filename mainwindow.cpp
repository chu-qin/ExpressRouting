// ============================================================================
// MainWindow.cpp — Qt 图形界面完整实现
// ============================================================================

#include "MainWindow.h"
#include "Dijkstra.h"

#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QStatusBar>
#include <QScrollBar>
#include <cmath>
#include <sstream>
#include <iomanip>

// ============================================================================
// GraphWidget — 交互式路网画布
// ============================================================================

GraphWidget::GraphWidget(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setMinimumSize(500, 400);
}

void GraphWidget::setGraph(const Graph* graph) { graphPtr = graph; updatePositions(); update(); }

void GraphWidget::setHighlight(const DynArray<int>& nodeList, const DynArray<int>& edgeSrcList,
                                const DynArray<int>& edgeDstList, int start, int target) {
    hlNodes = nodeList; hlEdgeSrc = edgeSrcList; hlEdgeDst = edgeDstList;
    startHL = start; targetHL = target;
    update();
}

void GraphWidget::clearHighlight() {
    hlNodes.clear(); hlEdgeSrc.clear(); hlEdgeDst.clear();
    startHL = targetHL = -1;
    update();
}

// ---- 坐标计算：经纬度 → 屏幕 ----
void GraphWidget::updatePositions() {
    if (!graphPtr || graphPtr->nodeCount() == 0) return;

    DynArray<int> ids = graphPtr->getAllNodeIds();
    double minLon = 1e9, maxLon = -1e9, minLat = 1e9, maxLat = -1e9;
    bool hasGeo = false;
    for (int i = 0; i < ids.size(); ++i) {
        const Node* node = graphPtr->findNode(ids[i]);
        if (node && (node->lon != 0 || node->lat != 0)) {
            hasGeo = true;
            if (node->lon < minLon) minLon = node->lon;
            if (node->lon > maxLon) maxLon = node->lon;
            if (node->lat < minLat) minLat = node->lat;
            if (node->lat > maxLat) maxLat = node->lat;
        }
    }

    if (!hasGeo) { minLon = 0; maxLon = 100; minLat = 0; maxLat = 100; }

    float margin = 40.0f;
    float drawW = width() - 2 * margin;
    float drawH = height() - 2 * margin;
    if (drawW < 1 || drawH < 1) return;

    float rangeX = maxLon - minLon;
    float rangeY = maxLat - minLat;
    if (rangeX < 0.01f) rangeX = 1.0f;
    if (rangeY < 0.01f) rangeY = 1.0f;

    for (int i = 0; i < ids.size(); ++i) {
        const Node* node = graphPtr->findNode(ids[i]);
        if (!node) continue;
        float screenX = (node->lon != 0 || node->lat != 0)
            ? margin + (node->lon - minLon) / rangeX * drawW
            : margin + (float)(ids[i] * 37 % (int)drawW);
        float screenY = (node->lon != 0 || node->lat != 0)
            ? margin + drawH - (node->lat - minLat) / rangeY * drawH
            : margin + (float)(ids[i] * 53 % (int)drawH);
        nodePos.set(ids[i], QPointF(screenX, screenY));
    }
}

void GraphWidget::resizeEvent(QResizeEvent*) { updatePositions(); }

// ---- 高亮判断 ----
bool GraphWidget::isNodeHL(int id) const {
    for (int i = 0; i < hlNodes.size(); ++i)
        if (hlNodes[i] == id) return true;
    return false;
}

bool GraphWidget::isEdgeHL(int from, int to) const {
    for (int i = 0; i < hlEdgeSrc.size(); ++i)
        if (hlEdgeSrc[i] == from && hlEdgeDst[i] == to) return true;
    return false;
}

// ---- 屏幕坐标 → 节点 ----
int GraphWidget::nodeAt(QPoint screenPt) const {
    if (!graphPtr) return -1;
    DynArray<int> ids = graphPtr->getAllNodeIds();
    for (int i = 0; i < ids.size(); ++i) {
        const QPointF* pos = nodePos.find(ids[i]);
        if (!pos) continue;
        float dx = screenPt.x() - pos->x(), dy = screenPt.y() - pos->y();
        if (std::sqrt(dx * dx + dy * dy) <= nodeRadius + 4) return ids[i];
    }
    return -1;
}

// ---- 画边（箭头 + 双向偏移） ----
void GraphWidget::drawEdge(QPainter& painter, QPointF fromPt, QPointF toPt,
                            QColor color, float width, bool isBidir) {
    QPointF dir = toPt - fromPt;
    float length = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    if (length < 1) return;

    QPointF unitDir(dir.x() / length, dir.y() / length);
    QPointF normal(-unitDir.y(), unitDir.x());

    float offset = isBidir ? 5.0f : 0;
    QPointF startPt = fromPt + unitDir * nodeRadius + normal * offset;
    QPointF endPt   = toPt - unitDir * nodeRadius + normal * offset;

    painter.setPen(QPen(color, width));
    painter.drawLine(startPt, endPt);

    // 箭头（三角形）
    float arrowSize = 10.0f;
    QPolygonF arrowHead;
    arrowHead << endPt
              << (endPt - unitDir * arrowSize + normal * (arrowSize * 0.42f))
              << (endPt - unitDir * arrowSize - normal * (arrowSize * 0.42f));
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPolygon(arrowHead);
}

// ---- 画节点（渐变圆 + 编号 + 名称） ----
void GraphWidget::drawNode(QPainter& painter, int id, QPointF pos, QColor color) {
    // 阴影
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 45));
    painter.drawEllipse(pos + QPointF(2, 2), (double)nodeRadius + 2, (double)nodeRadius + 2);

    // 主体
    painter.setPen(QPen(Qt::white, 1.5));
    painter.setBrush(color);
    painter.drawEllipse(pos, (double)nodeRadius, (double)nodeRadius);

    // 编号
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(pos.x() - nodeRadius, pos.y() - nodeRadius,
                            nodeRadius * 2, nodeRadius * 2),
                     Qt::AlignCenter, QString::number(id));

    // 名称标签
    if (graphPtr) {
        const Node* node = graphPtr->findNode(id);
        if (node && !node->name.empty()) {
            QString name = QString::fromStdString(node->name);
            font.setPointSize(8);
            font.setBold(false);
            painter.setFont(font);
            painter.setPen(QColor(30, 45, 60));
            painter.drawText(QRectF(pos.x() - 45, pos.y() + nodeRadius + 2, 90, 30),
                           Qt::AlignHCenter | Qt::TextWordWrap, name);
        }
    }
}

// ---- 主绘制 ----
void GraphWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(240, 245, 250));

    if (!graphPtr || graphPtr->nodeCount() == 0) {
        painter.setPen(QColor(150, 160, 170));
        painter.drawText(rect(), Qt::AlignCenter, "（路网为空，请导入数据）");
        return;
    }

    DynArray<int> ids = graphPtr->getAllNodeIds();

    // 1. 画所有边
    for (int i = 0; i < ids.size(); ++i) {
        const QPointF* posFrom = nodePos.find(ids[i]);
        if (!posFrom) continue;

        const DynArray<Edge>& neighbors = graphPtr->getNeighbors(ids[i]);
        for (int j = 0; j < neighbors.size(); ++j) {
            const QPointF* posTo = nodePos.find(neighbors[j].to);
            if (!posTo) continue;

            // 检查双向边
            bool isBidir = false;
            const DynArray<Edge>& reverseEdges = graphPtr->getNeighbors(neighbors[j].to);
            for (int k = 0; k < reverseEdges.size(); ++k)
                if (reverseEdges[k].to == ids[i]) { isBidir = true; break; }

            bool highlight = isEdgeHL(ids[i], neighbors[j].to);
            drawEdge(painter, *posFrom, *posTo,
                     highlight ? QColor(230, 100, 30) : QColor(160, 180, 200, 180),
                     highlight ? 3.0f : 1.5f, isBidir);

            // 边权标签
            if (!highlight) {
                QPointF mid = (*posFrom + *posTo) / 2.0;
                QPointF dir = *posTo - *posFrom;
                QPointF normal(-dir.y(), dir.x());
                float nLen = std::sqrt(normal.x() * normal.x() + normal.y() * normal.y());
                if (nLen > 0) {
                    normal = QPointF(normal.x() / nLen * 12, normal.y() / nLen * 12);
                    QFont saveFont = painter.font(); saveFont.setPointSize(7); painter.setFont(saveFont);
                    painter.setPen(QColor(100, 120, 140));
                    painter.drawText(QRectF(mid.x() + normal.x() - 30, mid.y() + normal.y() - 10, 60, 20),
                                   Qt::AlignCenter,
                                   QString("%1h/%2").arg(neighbors[j].time, 0, 'f', 1).arg(neighbors[j].cost, 0, 'f', 0));
                }
            }
        }
    }

    // 2. 画所有节点
    for (int i = 0; i < ids.size(); ++i) {
        const QPointF* pos = nodePos.find(ids[i]);
        if (!pos) continue;

        QColor color = QColor(74, 144, 217);         // 默认蓝色
        if (ids[i] == startHL)        color = QColor(39, 174, 96);    // 起点绿色
        else if (ids[i] == targetHL)  color = QColor(231, 76, 60);    // 终点红色
        else if (isNodeHL(ids[i]))    color = QColor(243, 156, 18);   // 路径橙色
        else if (ids[i] == hoverNode) color = QColor(100, 180, 255);  // 悬停亮蓝

        drawNode(painter, ids[i], *pos, color);
    }

    // 3. 悬停浮窗
    if (hoverNode > 0 && graphPtr) {
        const Node* node = graphPtr->findNode(hoverNode);
        const QPointF* pos = nodePos.find(hoverNode);
        if (node && pos) {
            float boxX = pos->x() + nodeRadius + 8, boxY = pos->y() - 35;
            if (boxX + 200 > width()) boxX = pos->x() - 208;
            if (boxY < 5) boxY = 5;

            QRectF box(boxX, boxY, 200, 70);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(18, 32, 50, 220));
            painter.drawRoundedRect(box, 6, 6);
            painter.setPen(QColor(70, 130, 190));
            painter.drawRoundedRect(box, 6, 6);

            QFont font = painter.font(); font.setPointSize(9); painter.setFont(font);
            painter.setPen(QColor(90, 200, 255));
            painter.drawText(box.adjusted(6, 5, -4, -45), Qt::AlignLeft,
                           QString("[%1] %2").arg(node->id).arg(QString::fromStdString(node->name)));
            painter.setPen(QColor(175, 195, 215));
            painter.drawText(box.adjusted(6, 25, -4, -25), Qt::AlignLeft,
                           QString::fromStdString(node->address));
            painter.setPen(QColor(140, 180, 210));
            painter.drawText(box.adjusted(6, 48, -4, -5), Qt::AlignLeft,
                           QString("出边: %1").arg(graphPtr->getNeighbors(node->id).size()));
        }
    }
}

void GraphWidget::mouseMoveEvent(QMouseEvent* ev) {
    int id = nodeAt(ev->pos());
    if (id != hoverNode) {
        hoverNode = id;
        emit nodeHovered(id);
        update();
    }
}

void GraphWidget::mousePressEvent(QMouseEvent* ev) {
    if (ev->button() == Qt::LeftButton) {
        int id = nodeAt(ev->pos());
        if (id > 0) emit nodeClicked(id);
    }
}

// ============================================================================
// MainWindow — 主窗口
// ============================================================================

static const char* BTN_PRIMARY   = "background:#3498db;color:white;border:none;border-radius:4px;padding:6px;";
static const char* BTN_SECONDARY = "background:#5a6a7a;color:white;border:none;border-radius:4px;padding:6px;";

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("快递网点配送路径规划系统");
    resize(1280, 760);

    buildUI();

    if (FileManager::loadNetwork("data/network.txt", graph)) {
        logOK(QString("路网加载完成：%1 节点 / %2 条边")
              .arg(graph.nodeCount()).arg(graph.edgeCount()));
        refreshStats();
        refreshCanvas();
    } else {
        logErr("未找到 data/network.txt，请通过菜单导入路网数据");
    }
}

// ---- 快捷按钮 ----
QPushButton* MainWindow::makeBtn(const QString& text, bool secondary) {
    auto* btn = new QPushButton(text);
    btn->setFixedHeight(40);
    btn->setStyleSheet(secondary ? BTN_SECONDARY : BTN_PRIMARY);
    return btn;
}

// ---- 页面批量创建 ----
QWidget* MainWindow::makePage(std::initializer_list<std::pair<QString, void(MainWindow::*)()>> items) {
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    layout->setSpacing(6);
    for (auto& [text, slot] : items) {
        auto* btn = makeBtn(text, text.startsWith("←"));
        connect(btn, &QPushButton::clicked, this, slot);
        layout->addWidget(btn);
    }
    layout->addStretch();
    return widget;
}

// ---- UI 构建 ----
void MainWindow::buildUI() {
    // === 菜单栏 ===
    auto* fileMenu = menuBar()->addMenu("文件(&F)");
    connect(fileMenu->addAction("导入路网..."),   &QAction::triggered, this, &MainWindow::onImportNet);
    connect(fileMenu->addAction("导出路网..."),   &QAction::triggered, this, &MainWindow::onExportNet);
    fileMenu->addSeparator();
    connect(fileMenu->addAction("导入订单..."),   &QAction::triggered, this, &MainWindow::onImportOrd);
    connect(fileMenu->addAction("导出方案..."),   &QAction::triggered, this, &MainWindow::onExportPlans);
    fileMenu->addSeparator();
    connect(fileMenu->addAction("退出(&X)"),      &QAction::triggered, this, &QMainWindow::close);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // === 左侧导航面板 ===
    auto* leftPanel = new QWidget;
    leftPanel->setFixedWidth(260);
    leftPanel->setStyleSheet("background:#1e2d3d;");

    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(8, 8, 8, 8);
    leftLayout->setSpacing(6);

    // 标题
    auto* title = new QLabel("🚚 快递路径规划系统");
    title->setStyleSheet("color:#5ab4ff;font-size:15px;font-weight:bold;padding:6px 0;");
    title->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(title);

    // 统计信息
    statsLabel = new QLabel;
    statsLabel->setStyleSheet("color:#56d78a;font-size:12px;background:#16263a;border-radius:4px;padding:6px;");
    statsLabel->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(statsLabel);

    // 当前页面指示
    modeLabel = new QLabel("▶ 主菜单");
    modeLabel->setStyleSheet("color:#a0c8f0;font-size:12px;padding:2px 4px;");
    leftLayout->addWidget(modeLabel);

    // 多页面导航
    pageStack = new QStackedWidget;
    pageStack->addWidget(makePage({
        {"网点管理",   &MainWindow::goNodePage},
        {"路网管理",   &MainWindow::goNetworkPage},
        {"路径查询",   &MainWindow::goPathPage},
        {"批次配送",   &MainWindow::goDeliveryPage},
    }));
    pageStack->addWidget(makePage({
        {"添加网点",     &MainWindow::onAddNode},
        {"删除网点",     &MainWindow::onDeleteNode},
        {"修改网点",     &MainWindow::onUpdateNode},
        {"查询网点",     &MainWindow::onFindNode},
        {"显示所有网点", &MainWindow::onListNodes},
        {"← 返回主菜单", &MainWindow::goMain},
    }));
    pageStack->addWidget(makePage({
        {"添加路段",     &MainWindow::onAddEdge},
        {"删除路段",     &MainWindow::onDeleteEdge},
        {"显示所有路段", &MainWindow::onListEdges},
        {"导入路网...",  &MainWindow::onImportNet},
        {"导出路网...",  &MainWindow::onExportNet},
        {"← 返回主菜单", &MainWindow::goMain},
    }));
    pageStack->addWidget(makePage({
        {"单源最短耗时",  &MainWindow::onShortestTime},
        {"两点最低费用",  &MainWindow::onCheapestPath},
        {"清除高亮",      &MainWindow::onClearHL},
        {"← 返回主菜单", &MainWindow::goMain},
    }));
    pageStack->addWidget(makePage({
        {"添加配送订单",  &MainWindow::onAddOrder},
        {"删除订单",      &MainWindow::onDelOrder},
        {"显示所有订单",  &MainWindow::onListOrders},
        {"批量导入订单",  &MainWindow::onImportOrd},
        {"规划所有订单",  &MainWindow::onPlanAll},
        {"拓扑批次排序",  &MainWindow::onTopoSort},
        {"导出配送方案",  &MainWindow::onExportPlans},
        {"← 返回主菜单", &MainWindow::goMain},
    }));
    leftLayout->addWidget(pageStack);

    // 日志区
    leftLayout->addWidget([]() {
        auto* label = new QLabel("── 操作日志 ──");
        label->setStyleSheet("color:#3a5060;font-size:11px;");
        return label;
    }());
    logBox = new QTextEdit;
    logBox->setReadOnly(true);
    logBox->setMaximumHeight(180);
    logBox->setStyleSheet("background:#0f1a26;color:#b0c8e0;font-size:12px;"
                           "border:1px solid #243040;border-radius:3px;");
    leftLayout->addWidget(logBox);

    mainLayout->addWidget(leftPanel);

    // === 右侧画布 ===
    canvas = new GraphWidget;
    connect(canvas, &GraphWidget::nodeHovered, this, [this](int id) {
        if (id > 0) {
            const Node* node = graph.findNode(id);
            if (node) statusBar()->showMessage(
                QString("[%1] %2  %3")
                    .arg(id)
                    .arg(QString::fromStdString(node->name))
                    .arg(QString::fromStdString(node->address)));
        } else {
            statusBar()->clearMessage();
        }
    });
    mainLayout->addWidget(canvas, 1);

    refreshStats();
}

// ---- 页面导航 ----
void MainWindow::goMain()         { pageStack->setCurrentIndex(0); modeLabel->setText("▶ 主菜单"); }
void MainWindow::goNodePage()     { pageStack->setCurrentIndex(1); modeLabel->setText("▶ 网点管理"); }
void MainWindow::goNetworkPage()  { pageStack->setCurrentIndex(2); modeLabel->setText("▶ 路网管理"); }
void MainWindow::goPathPage()     { pageStack->setCurrentIndex(3); modeLabel->setText("▶ 路径查询"); }
void MainWindow::goDeliveryPage() { pageStack->setCurrentIndex(4); modeLabel->setText("▶ 批次配送"); }

// ---- 辅助 ----
void MainWindow::refreshStats() {
    statsLabel->setText(QString("网点: %1  路段: %2  订单: %3")
                    .arg(graph.nodeCount()).arg(graph.edgeCount()).arg(orderMgr.getOrders().size()));
}

void MainWindow::refreshCanvas() {
    canvas->setGraph(&graph);
    canvas->setHighlight(hlNodes, hlEdgeSrc, hlEdgeDst, startHL, targetHL);
}

void MainWindow::log(const QString& msg, const QString& color) {
    logBox->append(QString("<span style='color:%1'>%2</span>").arg(color).arg(msg));
    logBox->verticalScrollBar()->setValue(logBox->verticalScrollBar()->maximum());
}

void MainWindow::logOK(const QString& msg)  { log(msg, "#56d78a"); }
void MainWindow::logErr(const QString& msg) { log(msg, "#e05050"); }

void MainWindow::applyPathToHL(const DynArray<int>& path) {
    for (int i = 0; i + 1 < path.size(); ++i) {
        hlEdgeSrc.push_back(path[i]);
        hlEdgeDst.push_back(path[i + 1]);
        hlNodes.push_back(path[i]);
        hlNodes.push_back(path[i + 1]);
    }
    refreshCanvas();
}

void MainWindow::clearHL() {
    hlNodes.clear(); hlEdgeSrc.clear(); hlEdgeDst.clear();
    startHL = targetHL = -1;
    canvas->clearHighlight();
}

// ---- 对话框辅助宏 ----
#define MAKE_DLG(title) \
    QDialog dlg(this); dlg.setWindowTitle(title); \
    auto* form = new QFormLayout(&dlg); \
    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); \
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept); \
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

#define SPIN(var, lo, hi) auto* var = new QSpinBox; var->setRange(lo, hi);
#define DSPIN(var, lo, hi) auto* var = new QDoubleSpinBox; var->setRange(lo, hi); var->setDecimals(1);

// ================================================================
// 网点管理
// ================================================================
void MainWindow::onAddNode() {
    MAKE_DLG("添加网点");
    SPIN(idSpin, 1, 9999);
    auto* nameEdit = new QLineEdit;
    auto* addrEdit = new QLineEdit;
    DSPIN(lonSpin, 0, 200); DSPIN(latSpin, 0, 100);
    form->addRow("编号:", idSpin);
    form->addRow("名称:", nameEdit);
    form->addRow("地址:", addrEdit);
    form->addRow("经度:", lonSpin);
    form->addRow("纬度:", latSpin);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (graph.addNode(Node(idSpin->value(), nameEdit->text().toStdString(), addrEdit->text().toStdString(),
                            lonSpin->value(), latSpin->value()))) {
        logOK(QString("网点 [%1] %2 添加成功").arg(idSpin->value()).arg(nameEdit->text()));
        refreshStats(); refreshCanvas();
    } else {
        logErr("添加失败（编号已存在或名称为空）");
    }
}

void MainWindow::onDeleteNode() {
    MAKE_DLG("删除网点");
    SPIN(idSpin, 1, 9999);
    form->addRow("编号:", idSpin);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (graph.deleteNode(idSpin->value())) {
        logOK(QString("网点 [%1] 已删除").arg(idSpin->value()));
        clearHL(); refreshStats(); refreshCanvas();
    } else {
        logErr("网点不存在");
    }
}

void MainWindow::onUpdateNode() {
    MAKE_DLG("修改网点");
    SPIN(idSpin, 1, 9999);
    auto* nameEdit = new QLineEdit;
    auto* addrEdit = new QLineEdit;
    form->addRow("编号:", idSpin);
    form->addRow("新名称:", nameEdit);
    form->addRow("新地址:", addrEdit);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (graph.updateNode(idSpin->value(), Node(idSpin->value(), nameEdit->text().toStdString(), addrEdit->text().toStdString()))) {
        logOK("修改成功");
        refreshCanvas();
    } else {
        logErr("修改失败");
    }
}

void MainWindow::onFindNode() {
    MAKE_DLG("查询网点");
    SPIN(idSpin, 1, 9999);
    form->addRow("编号:", idSpin);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    const Node* node = graph.findNode(idSpin->value());
    if (node) {
        logOK(QString("[%1] %2  %3").arg(node->id)
              .arg(QString::fromStdString(node->name))
              .arg(QString::fromStdString(node->address)));
        clearHL();
        hlNodes.push_back(node->id);
        refreshCanvas();
    } else {
        logErr("网点不存在");
    }
}

void MainWindow::onListNodes() {
    DynArray<int> ids = graph.getAllNodeIds();
    for (int i = 0; i < ids.size() - 1; ++i)
        for (int j = i + 1; j < ids.size(); ++j)
            if (ids[i] > ids[j]) { int temp = ids[i]; ids[i] = ids[j]; ids[j] = temp; }
    log(QString("── 所有网点（%1）──").arg(ids.size()), "#5ab4ff");
    for (int i = 0; i < ids.size(); ++i) {
        const Node* node = graph.findNode(ids[i]);
        if (node) log(QString("[%1] %2  %3").arg(node->id)
                   .arg(QString::fromStdString(node->name))
                   .arg(QString::fromStdString(node->address)));
    }
}

// ================================================================
// 路网管理
// ================================================================
void MainWindow::onAddEdge() {
    MAKE_DLG("添加路段");
    SPIN(fromSpin, 1, 9999); SPIN(toSpin, 1, 9999);
    DSPIN(timeSpin, 0, 999); DSPIN(costSpin, 0, 99999);
    form->addRow("起点:", fromSpin);
    form->addRow("终点:", toSpin);
    form->addRow("耗时(h):", timeSpin);
    form->addRow("费用(元):", costSpin);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (graph.addEdge(Edge(fromSpin->value(), toSpin->value(), timeSpin->value(), costSpin->value()))) {
        logOK(QString("路段 %1→%2 添加成功").arg(fromSpin->value()).arg(toSpin->value()));
        refreshStats(); refreshCanvas();
    } else {
        logErr("添加失败");
    }
}

void MainWindow::onDeleteEdge() {
    MAKE_DLG("删除路段");
    SPIN(fromSpin, 1, 9999); SPIN(toSpin, 1, 9999);
    form->addRow("起点:", fromSpin);
    form->addRow("终点:", toSpin);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (graph.deleteEdge(fromSpin->value(), toSpin->value())) {
        logOK(QString("路段 %1→%2 已删除").arg(fromSpin->value()).arg(toSpin->value()));
        clearHL(); refreshStats(); refreshCanvas();
    } else {
        logErr("路段不存在");
    }
}

void MainWindow::onListEdges() {
    DynArray<int> ids = graph.getAllNodeIds();
    for (int i = 0; i < ids.size() - 1; ++i)
        for (int j = i + 1; j < ids.size(); ++j)
            if (ids[i] > ids[j]) { int temp = ids[i]; ids[i] = ids[j]; ids[j] = temp; }
    log(QString("── 所有路段（%1）──").arg(graph.edgeCount()), "#5ab4ff");
    for (int i = 0; i < ids.size(); ++i) {
        const DynArray<Edge>& edgeList = graph.getNeighbors(ids[i]);
        for (int j = 0; j < edgeList.size(); ++j) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1)
               << edgeList[j].from << "→" << edgeList[j].to
               << "  耗时" << edgeList[j].time << "h  费用" << edgeList[j].cost << "元";
            log(QString::fromStdString(ss.str()));
        }
    }
}

void MainWindow::onImportNet() {
    QString filePath = QFileDialog::getOpenFileName(this, "导入路网", "data", "文本文件 (*.txt)");
    if (filePath.isEmpty()) return;
    graph.clear();
    clearHL();
    if (FileManager::loadNetwork(filePath.toStdString(), graph)) {
        logOK(QString("导入成功：%1 节点 / %2 路段").arg(graph.nodeCount()).arg(graph.edgeCount()));
        refreshStats(); refreshCanvas();
    } else {
        logErr("导入失败");
    }
}

void MainWindow::onExportNet() {
    QString filePath = QFileDialog::getSaveFileName(this, "导出路网", "data/network.txt", "文本文件 (*.txt)");
    if (filePath.isEmpty()) return;
    if (FileManager::saveNetwork(filePath.toStdString(), graph))
        logOK("已保存：" + filePath);
    else
        logErr("保存失败");
}

// ================================================================
// 路径查询
// ================================================================
void MainWindow::onShortestTime() {
    MAKE_DLG("单源最短耗时查询");
    SPIN(startSpin, 1, 9999);
    form->addRow("起点编号:", startSpin);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;

    clearHL();
    auto allPaths = Dijkstra::shortestTimeFrom(graph, startSpin->value());
    if (allPaths.empty()) { logErr("起点不存在"); return; }

    startHL = startSpin->value();
    log(QString("── 从 [%1] 出发的最短耗时 ──").arg(startSpin->value()), "#5ac4ff");

    DynArray<int> ids = graph.getAllNodeIds();
    for (int i = 0; i < ids.size(); ++i) {
        if (ids[i] == startSpin->value()) continue;
        PathResult* pathRes = allPaths.find(ids[i]);
        if (!pathRes || !pathRes->reachable) continue;

        for (int j = 0; j + 1 < pathRes->path.size(); ++j) {
            hlEdgeSrc.push_back(pathRes->path[j]);
            hlEdgeDst.push_back(pathRes->path[j + 1]);
        }
        hlNodes.push_back(ids[i]);

        const Node* node = graph.findNode(ids[i]);
        log(QString("[%1] %2  耗时%3h  费用%4元")
            .arg(ids[i])
            .arg(node ? QString::fromStdString(node->name) : "?")
            .arg(pathRes->totalTime, 0, 'f', 1)
            .arg(pathRes->totalCost, 0, 'f', 0),
            "#c8deb0");
    }
    refreshCanvas();
}

void MainWindow::onCheapestPath() {
    MAKE_DLG("两点最低费用路径");
    SPIN(srcSpin, 1, 9999); SPIN(dstSpin, 1, 9999);
    form->addRow("起点:", srcSpin);
    form->addRow("终点:", dstSpin);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;

    clearHL();
    PathResult pathRes = Dijkstra::cheapestPath(graph, srcSpin->value(), dstSpin->value());
    if (!pathRes.reachable) { logErr("不可达"); return; }

    startHL = srcSpin->value();
    targetHL = dstSpin->value();
    applyPathToHL(pathRes.path);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "路径: ";
    for (int i = 0; i < pathRes.path.size(); ++i) {
        const Node* node = graph.findNode(pathRes.path[i]);
        ss << "[" << pathRes.path[i] << "]"
           << (node ? node->name : "?");
        if (i + 1 < pathRes.path.size()) ss << " → ";
    }
    logOK(QString::fromStdString(ss.str()));
    logOK(QString("费用: %1元  耗时: %2h").arg(pathRes.totalCost, 0, 'f', 0).arg(pathRes.totalTime, 0, 'f', 1));
}

void MainWindow::onClearHL() { clearHL(); logOK("已清除高亮"); }

// ================================================================
// 批次配送
// ================================================================
void MainWindow::onAddOrder() {
    MAKE_DLG("添加配送订单");
    SPIN(orderIdSpin, 1, 99999);
    SPIN(srcSpin, 1, 9999);
    SPIN(dstSpin, 1, 9999);
    auto* goodsEdit = new QLineEdit;
    auto* optimizeCombo = new QComboBox;
    optimizeCombo->addItem("最低费用");
    optimizeCombo->addItem("最短耗时");
    form->addRow("订单号:", orderIdSpin);
    form->addRow("起点:", srcSpin);
    form->addRow("终点:", dstSpin);
    form->addRow("货物:", goodsEdit);
    form->addRow("优化目标:", optimizeCombo);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (orderMgr.addOrder(Order(orderIdSpin->value(), srcSpin->value(), dstSpin->value(),
                                 goodsEdit->text().toStdString(), optimizeCombo->currentIndex() == 1))) {
        logOK(QString("订单 %1 添加成功").arg(orderIdSpin->value()));
        refreshStats();
    } else {
        logErr("订单号已存在");
    }
}

void MainWindow::onDelOrder() {
    MAKE_DLG("删除订单");
    SPIN(idSpin, 1, 99999);
    form->addRow("订单号:", idSpin);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (orderMgr.removeOrder(idSpin->value())) {
        logOK(QString("订单 %1 已删除").arg(idSpin->value()));
        refreshStats();
    } else {
        logErr("订单不存在");
    }
}

void MainWindow::onListOrders() {
    const DynArray<Order>& orderList = orderMgr.getOrders();
    log(QString("── 所有订单（%1）──").arg(orderList.size()), "#5ab4ff");
    for (int i = 0; i < orderList.size(); ++i)
        log(QString("[%1] %2→%3  %4  %5")
            .arg(orderList[i].orderId)
            .arg(orderList[i].srcNode)
            .arg(orderList[i].dstNode)
            .arg(QString::fromStdString(orderList[i].goods))
            .arg(orderList[i].preferTime ? "最短耗时" : "最低费用"));
}

void MainWindow::onImportOrd() {
    QString filePath = QFileDialog::getOpenFileName(this, "导入订单", "data", "文本文件 (*.txt)");
    if (filePath.isEmpty()) return;
    if (FileManager::loadOrders(filePath.toStdString(), orderMgr)) {
        logOK(QString("导入：%1 条订单").arg(orderMgr.getOrders().size()));
        refreshStats();
    } else {
        logErr("导入失败");
    }
}

void MainWindow::onPlanAll() {
    clearHL();
    DynArray<DeliveryPlan> plans = orderMgr.planAllOrders(graph);
    log(QString("── 批量规划结果 ──"), "#5ac4ff");
    for (int i = 0; i < plans.size(); ++i) {
        const DeliveryPlan& plan = plans[i];
        if (!plan.result.reachable) {
            logErr(QString("订单 %1 不可达").arg(plan.order.orderId));
            continue;
        }
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1)
           << "[" << plan.order.orderId << "] " << plan.order.goods << ": ";
        for (int j = 0; j < plan.result.path.size(); ++j) {
            ss << plan.result.path[j];
            if (j + 1 < plan.result.path.size()) ss << "→";
        }
        ss << "  " << (plan.order.preferTime ? plan.result.totalTime : plan.result.totalCost)
           << (plan.order.preferTime ? "h" : "元");
        logOK(QString::fromStdString(ss.str()));
        applyPathToHL(plan.result.path);
    }
    logOK(QString("完成，共 %1 条").arg(plans.size()));
}

void MainWindow::onTopoSort() {
    clearHL();
    TopoResult topoRes = orderMgr.planBatchSequence(graph);
    if (topoRes.hasCycle) {
        logErr("检测到环路！涉及节点：");
        for (int i = 0; i < topoRes.cycleNodes.size(); ++i) {
            hlNodes.push_back(topoRes.cycleNodes[i]);
            const Node* node = graph.findNode(topoRes.cycleNodes[i]);
            log(QString("  %1（%2）").arg(topoRes.cycleNodes[i])
                .arg(node ? QString::fromStdString(node->name) : "?"), "#e09050");
        }
    } else {
        log(QString("── 拓扑排序配送顺序 ──"), "#5ac4ff");
        QString line;
        for (int i = 0; i < topoRes.order.size(); ++i) {
            int id = topoRes.order[i];
            hlNodes.push_back(id);
            const Node* node = graph.findNode(id);
            QString name = node ? QString::fromStdString(node->name) : "?";
            if (name.size() > 3) name = name.left(3);
            line += QString("%1(%2)").arg(id).arg(name);
            if (i + 1 < topoRes.order.size()) line += "→";
            if (line.size() > 48) {
                log(line, "#c8d8b0");
                line.clear();
            }
        }
        if (!line.isEmpty()) log(line, "#c8d8b0");
        logOK(QString::number(topoRes.order.size()) + " 个节点，无环路");
    }
    refreshCanvas();
}

void MainWindow::onExportPlans() {
    QString filePath = QFileDialog::getSaveFileName(this, "导出方案", "data/plans.txt", "文本文件 (*.txt)");
    if (filePath.isEmpty()) return;
    DynArray<DeliveryPlan> plans = orderMgr.planAllOrders(graph);
    if (FileManager::savePlans(filePath.toStdString(), graph, plans))
        logOK("已导出：" + filePath);
    else
        logErr("导出失败");
}
