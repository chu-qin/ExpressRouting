// ============================================================================
// MainWindow.cpp — Qt 图形界面完整实现
// ============================================================================
// 本文件包含：
//   GraphWidget  — 交互式路网画布（paintEvent + mouseMoveEvent + mousePressEvent）
//   MainWindow  — 主窗口（菜单栏 + 导航面板 + 画布 + 日志 + 状态栏）
//
// 关键 Qt 机制：
//   信号槽    connect(btn, &QPushButton::clicked, this, &MainWindow::slotFn)
//   事件重写  paintEvent / mouseMoveEvent / mousePressEvent
//   对话框    QDialog + QFormLayout + QDialogButtonBox（标准 Qt 模式）
//   文件选择  QFileDialog::getOpenFileName / getSaveFileName
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
    setMouseTracking(true);          // 启用鼠标追踪（不需要按下就能触发 moveEvent）
    setMinimumSize(500, 400);
}

void GraphWidget::setGraph(const Graph* g) { g_ = g; updatePos(); update(); }

void GraphWidget::setHL(const DynArray<int>& nodes, const DynArray<int>& edgeSrc,
                         const DynArray<int>& edgeDst, int src, int dst) {
    hlNodes_ = nodes; hlEdgeSrc_ = edgeSrc; hlEdgeDst_ = edgeDst;
    srcHL_ = src; dstHL_ = dst;
    update();
}

void GraphWidget::clearHL() {
    hlNodes_.clear(); hlEdgeSrc_.clear(); hlEdgeDst_.clear();
    srcHL_ = dstHL_ = -1;
    update();
}

// ---- 坐标计算：经纬度 → 屏幕坐标 ----
void GraphWidget::updatePos() {
    if (!g_ || g_->nodeCount() == 0) return;

    // 收集所有节点的经纬度，找 min/max
    DynArray<int> ids = g_->getAllNodeIds();
    double minLon = 1e9, maxLon = -1e9, minLat = 1e9, maxLat = -1e9;
    bool hasGeo = false;
    for (int i = 0; i < ids.size(); ++i) {
        const Node* n = g_->findNode(ids[i]);
        if (n && (n->lon != 0 || n->lat != 0)) {
            hasGeo = true;
            if (n->lon < minLon) minLon = n->lon;
            if (n->lon > maxLon) maxLon = n->lon;
            if (n->lat < minLat) minLat = n->lat;
            if (n->lat > maxLat) maxLat = n->lat;
        }
    }

    // 如果没有坐标数据，均匀分布
    if (!hasGeo) { minLon = 0; maxLon = 100; minLat = 0; maxLat = 100; }

    float margin = 40.0f;
    float W = width() - 2 * margin;
    float H = height() - 2 * margin;
    if (W < 1 || H < 1) return;

    float rangeX = maxLon - minLon;
    float rangeY = maxLat - minLat;
    if (rangeX < 0.01f) rangeX = 1.0f;
    if (rangeY < 0.01f) rangeY = 1.0f;

    for (int i = 0; i < ids.size(); ++i) {
        const Node* n = g_->findNode(ids[i]);
        if (!n) continue;
        float fx = (n->lon != 0 || n->lat != 0)
            ? margin + (n->lon - minLon) / rangeX * W
            : margin + (float)(ids[i] * 37 % (int)W);
        float fy = (n->lon != 0 || n->lat != 0)
            ? margin + H - (n->lat - minLat) / rangeY * H  // 纬度大=上方，屏幕 y 小=上方
            : margin + (float)(ids[i] * 53 % (int)H);
        pos_.set(ids[i], QPointF(fx, fy));
    }
}

void GraphWidget::resizeEvent(QResizeEvent*) { updatePos(); }

// ---- 高亮判断 ----
bool GraphWidget::isNodeHL(int id) const {
    for (int i = 0; i < hlNodes_.size(); ++i)
        if (hlNodes_[i] == id) return true;
    return false;
}

bool GraphWidget::isEdgeHL(int f, int t) const {
    for (int i = 0; i < hlEdgeSrc_.size(); ++i)
        if (hlEdgeSrc_[i] == f && hlEdgeDst_[i] == t) return true;
    return false;
}

// ---- 鼠标坐标 → 节点（距离 < R+4 像素即为命中） ----
int GraphWidget::nodeAt(QPoint p) const {
    if (!g_) return -1;
    DynArray<int> ids = g_->getAllNodeIds();
    for (int i = 0; i < ids.size(); ++i) {
        const QPointF* q = pos_.find(ids[i]);
        if (!q) continue;
        float dx = p.x() - q->x(), dy = p.y() - q->y();
        if (std::sqrt(dx * dx + dy * dy) <= R + 4) return ids[i];
    }
    return -1;
}

// ---- 画边（箭头 + 偏移处理双向边） ----
void GraphWidget::drawEdge(QPainter& p, QPointF a, QPointF b, QColor col, float w, bool bi) {
    QPointF d = b - a;
    float len = std::sqrt(d.x() * d.x() + d.y() * d.y());
    if (len < 1) return;

    QPointF u(d.x() / len, d.y() / len);        // 方向单位向量
    QPointF perp(-u.y(), u.x());                 // 垂直方向（用于双向边偏移）

    float off = bi ? 5.0f : 0;                    // 双向边偏移量
    QPointF p1 = a + u * R + perp * off;          // 起点：圆边缘
    QPointF p2 = b - u * R + perp * off;          // 终点：圆边缘

    // 画线段
    p.setPen(QPen(col, w));
    p.drawLine(p1, p2);

    // 画箭头（三角形，边长 10px）
    float sz = 10.0f;
    QPolygonF tri;
    tri << p2
        << (p2 - u * sz + perp * (sz * 0.42f))
        << (p2 - u * sz - perp * (sz * 0.42f));
    p.setPen(Qt::NoPen);
    p.setBrush(col);
    p.drawPolygon(tri);
}

// ---- 画节点（渐变圆 + 阴影 + 编号 + 名称） ----
void GraphWidget::drawNode(QPainter& p, int id, QPointF pos, QColor col) {
    // 阴影（右下偏移 2px）
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 45));
    p.drawEllipse(pos + QPointF(2, 2), (double)R + 2, (double)R + 2);

    // 主体圆
    p.setPen(QPen(Qt::white, 1.5));
    p.setBrush(col);
    p.drawEllipse(pos, (double)R, (double)R);

    // 编号（白色加粗居中）
    QFont f = p.font();
    f.setPointSize(10);
    f.setBold(true);
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(QRectF(pos.x() - R, pos.y() - R, R * 2, R * 2), Qt::AlignCenter, QString::number(id));

    // 名称标签（圆下方）
    if (g_) {
        const Node* n = g_->findNode(id);
        if (n && !n->name.empty()) {
            QString nm = QString::fromStdString(n->name);
            f.setPointSize(8);
            f.setBold(false);
            p.setFont(f);
            p.setPen(QColor(30, 45, 60));
            p.drawText(QRectF(pos.x() - 45, pos.y() + R + 2, 90, 30),
                       Qt::AlignHCenter | Qt::TextWordWrap, nm);
        }
    }
}

// ---- 主绘制 ----
void GraphWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(240, 245, 250));

    if (!g_ || g_->nodeCount() == 0) {
        p.setPen(QColor(150, 160, 170));
        p.drawText(rect(), Qt::AlignCenter, "（路网为空，请导入数据）");
        return;
    }

    DynArray<int> ids = g_->getAllNodeIds();

    // 1. 画所有边
    for (int i = 0; i < ids.size(); ++i) {
        const QPointF* pa = pos_.find(ids[i]);
        if (!pa) continue;

        const DynArray<Edge>& ns = g_->getNeighbors(ids[i]);
        for (int j = 0; j < ns.size(); ++j) {
            const QPointF* pb = pos_.find(ns[j].to);
            if (!pb) continue;

            // 检查是否为双向边
            bool bi = false;
            const DynArray<Edge>& rev = g_->getNeighbors(ns[j].to);
            for (int k = 0; k < rev.size(); ++k)
                if (rev[k].to == ids[i]) { bi = true; break; }

            bool hl = isEdgeHL(ids[i], ns[j].to);
            drawEdge(p, *pa, *pb,
                     hl ? QColor(230, 100, 30) : QColor(160, 180, 200, 180),
                     hl ? 3.0f : 1.5f, bi);

            // 边权标签（中点偏移处显示 耗时/费用）
            if (!hl) {
                QPointF mid = (*pa + *pb) / 2.0;
                QPointF d = *pb - *pa;
                QPointF perp(-d.y(), d.x());
                float plen = std::sqrt(perp.x() * perp.x() + perp.y() * perp.y());
                if (plen > 0) {
                    perp = QPointF(perp.x() / plen * 12, perp.y() / plen * 12);
                    QFont sf = p.font(); sf.setPointSize(7); p.setFont(sf);
                    p.setPen(QColor(100, 120, 140));
                    p.drawText(QRectF(mid.x() + perp.x() - 30, mid.y() + perp.y() - 10, 60, 20),
                               Qt::AlignCenter,
                               QString("%1h/%2").arg(ns[j].time, 0, 'f', 1).arg(ns[j].cost, 0, 'f', 0));
                }
            }
        }
    }

    // 2. 画所有节点（按颜色分级）
    for (int i = 0; i < ids.size(); ++i) {
        const QPointF* pos = pos_.find(ids[i]);
        if (!pos) continue;

        QColor col = QColor(74, 144, 217);       // 默认蓝色
        if (ids[i] == srcHL_)      col = QColor(39, 174, 96);   // 起点绿色
        else if (ids[i] == dstHL_) col = QColor(231, 76, 60);   // 终点红色
        else if (isNodeHL(ids[i])) col = QColor(243, 156, 18);  // 路径节点橙色
        else if (ids[i] == hovered_) col = QColor(100, 180, 255); // 悬停亮蓝

        drawNode(p, ids[i], *pos, col);
    }

    // 3. 悬停浮窗（节点详情）
    if (hovered_ > 0 && g_) {
        const Node* n = g_->findNode(hovered_);
        const QPointF* pos = pos_.find(hovered_);
        if (n && pos) {
            float bx = pos->x() + R + 8, by = pos->y() - 35;
            if (bx + 200 > width()) bx = pos->x() - 208;
            if (by < 5) by = 5;

            QRectF box(bx, by, 200, 70);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(18, 32, 50, 220));
            p.drawRoundedRect(box, 6, 6);
            p.setPen(QColor(70, 130, 190));
            p.drawRoundedRect(box, 6, 6);

            QFont f = p.font(); f.setPointSize(9); p.setFont(f);
            p.setPen(QColor(90, 200, 255));
            p.drawText(box.adjusted(6, 5, -4, -45), Qt::AlignLeft,
                       QString("[%1] %2").arg(n->id).arg(QString::fromStdString(n->name)));
            p.setPen(QColor(175, 195, 215));
            p.drawText(box.adjusted(6, 25, -4, -25), Qt::AlignLeft,
                       QString::fromStdString(n->address));
            p.setPen(QColor(140, 180, 210));
            p.drawText(box.adjusted(6, 48, -4, -5), Qt::AlignLeft,
                       QString("出边: %1").arg(g_->getNeighbors(n->id).size()));
        }
    }
}

void GraphWidget::mouseMoveEvent(QMouseEvent* ev) {
    int id = nodeAt(ev->pos());
    if (id != hovered_) {
        hovered_ = id;
        emit nodeHovered(id);    // 通知 MainWindow 更新状态栏
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

// QSS 按钮样式
static const char* BTN_PRIMARY = "background:#3498db;color:white;border:none;border-radius:4px;padding:6px;";
static const char* BTN_SECONDARY = "background:#5a6a7a;color:white;border:none;border-radius:4px;padding:6px;";

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("快递网点配送路径规划系统");
    resize(1280, 760);

    buildUI();

    // 自动加载默认数据
    if (FileManager::loadNetwork("data/network.txt", graph_)) {
        logOK(QString("路网加载完成：%1 节点 / %2 条边")
              .arg(graph_.nodeCount()).arg(graph_.edgeCount()));
        refreshStats();
        refreshCanvas();
    } else {
        logErr("未找到 data/network.txt，请通过菜单导入路网数据");
    }
}

// ---- 快捷按钮创建 ----
QPushButton* MainWindow::makeBtn(const QString& text, bool secondary) {
    auto* b = new QPushButton(text);
    b->setFixedHeight(40);
    b->setStyleSheet(secondary ? BTN_SECONDARY : BTN_PRIMARY);
    return b;
}

// ---- 页面批量创建 ----
QWidget* MainWindow::makePage(std::initializer_list<std::pair<QString, void(MainWindow::*)()>> items) {
    auto* w = new QWidget;
    auto* l = new QVBoxLayout(w);
    l->setSpacing(6);
    for (auto& [text, slot] : items) {
        auto* b = makeBtn(text, text.startsWith("←"));
        connect(b, &QPushButton::clicked, this, slot);
        l->addWidget(b);
    }
    l->addStretch();
    return w;
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
    auto* ml = new QHBoxLayout(central);
    ml->setContentsMargins(0, 0, 0, 0);
    ml->setSpacing(0);

    // === 左侧导航面板（深色主题） ===
    auto* left = new QWidget;
    left->setFixedWidth(260);
    left->setStyleSheet("background:#1e2d3d;");

    auto* ll = new QVBoxLayout(left);
    ll->setContentsMargins(8, 8, 8, 8);
    ll->setSpacing(6);

    // 标题
    auto* title = new QLabel("🚚 快递路径规划系统");
    title->setStyleSheet("color:#5ab4ff;font-size:15px;font-weight:bold;padding:6px 0;");
    title->setAlignment(Qt::AlignCenter);
    ll->addWidget(title);

    // 统计信息
    stats_ = new QLabel;
    stats_->setStyleSheet("color:#56d78a;font-size:12px;background:#16263a;border-radius:4px;padding:6px;");
    stats_->setAlignment(Qt::AlignCenter);
    ll->addWidget(stats_);

    // 当前页面指示
    modeLabel_ = new QLabel("▶ 主菜单");
    modeLabel_->setStyleSheet("color:#a0c8f0;font-size:12px;padding:2px 4px;");
    ll->addWidget(modeLabel_);

    // 多页面导航
    stack_ = new QStackedWidget;
    stack_->addWidget(makePage({
        {"网点管理",   &MainWindow::goNode},
        {"路网管理",   &MainWindow::goNetwork},
        {"路径查询",   &MainWindow::goPath},
        {"批次配送",   &MainWindow::goDelivery},
    }));
    stack_->addWidget(makePage({    // 网点管理子页
        {"添加网点",     &MainWindow::onAddNode},
        {"删除网点",     &MainWindow::onDeleteNode},
        {"修改网点",     &MainWindow::onUpdateNode},
        {"查询网点",     &MainWindow::onFindNode},
        {"显示所有网点", &MainWindow::onListNodes},
        {"← 返回主菜单", &MainWindow::goMain},
    }));
    stack_->addWidget(makePage({    // 路网管理子页
        {"添加路段",     &MainWindow::onAddEdge},
        {"删除路段",     &MainWindow::onDeleteEdge},
        {"显示所有路段", &MainWindow::onListEdges},
        {"导入路网...",  &MainWindow::onImportNet},
        {"导出路网...",  &MainWindow::onExportNet},
        {"← 返回主菜单", &MainWindow::goMain},
    }));
    stack_->addWidget(makePage({    // 路径查询子页
        {"单源最短耗时",  &MainWindow::onShortestTime},
        {"两点最低费用",  &MainWindow::onCheapestPath},
        {"清除高亮",      &MainWindow::onClearHL},
        {"← 返回主菜单", &MainWindow::goMain},
    }));
    stack_->addWidget(makePage({    // 批次配送子页
        {"添加配送订单",  &MainWindow::onAddOrder},
        {"删除订单",      &MainWindow::onDelOrder},
        {"显示所有订单",  &MainWindow::onListOrders},
        {"批量导入订单",  &MainWindow::onImportOrd},
        {"规划所有订单",  &MainWindow::onPlanAll},
        {"拓扑批次排序",  &MainWindow::onTopoSort},
        {"导出配送方案",  &MainWindow::onExportPlans},
        {"← 返回主菜单", &MainWindow::goMain},
    }));
    ll->addWidget(stack_);

    // 日志区
    ll->addWidget([]() {
        auto* l = new QLabel("── 操作日志 ──");
        l->setStyleSheet("color:#3a5060;font-size:11px;");
        return l;
    }());
    logBox_ = new QTextEdit;
    logBox_->setReadOnly(true);
    logBox_->setMaximumHeight(180);
    logBox_->setStyleSheet("background:#0f1a26;color:#b0c8e0;font-size:12px;"
                           "border:1px solid #243040;border-radius:3px;");
    ll->addWidget(logBox_);

    ml->addWidget(left);

    // === 右侧画布 ===
    canvas_ = new GraphWidget;
    connect(canvas_, &GraphWidget::nodeHovered, this, [this](int id) {
        if (id > 0) {
            const Node* n = graph_.findNode(id);
            if (n) statusBar()->showMessage(
                QString("[%1] %2  %3")
                    .arg(id)
                    .arg(QString::fromStdString(n->name))
                    .arg(QString::fromStdString(n->address)));
        } else {
            statusBar()->clearMessage();
        }
    });
    ml->addWidget(canvas_, 1);   // stretch=1，占满剩余空间

    refreshStats();
}

// ---- 页面导航 ----
void MainWindow::goMain()     { stack_->setCurrentIndex(0); modeLabel_->setText("▶ 主菜单"); }
void MainWindow::goNode()     { stack_->setCurrentIndex(1); modeLabel_->setText("▶ 网点管理"); }
void MainWindow::goNetwork()  { stack_->setCurrentIndex(2); modeLabel_->setText("▶ 路网管理"); }
void MainWindow::goPath()     { stack_->setCurrentIndex(3); modeLabel_->setText("▶ 路径查询"); }
void MainWindow::goDelivery() { stack_->setCurrentIndex(4); modeLabel_->setText("▶ 批次配送"); }

// ---- 辅助 ----
void MainWindow::refreshStats() {
    stats_->setText(QString("网点: %1  路段: %2  订单: %3")
                    .arg(graph_.nodeCount()).arg(graph_.edgeCount()).arg(orders_.getOrders().size()));
}

void MainWindow::refreshCanvas() {
    canvas_->setGraph(&graph_);
    canvas_->setHL(hlNodes_, hlEdgeSrc_, hlEdgeDst_, srcHL_, dstHL_);
}

void MainWindow::log(const QString& msg, const QString& color) {
    logBox_->append(QString("<span style='color:%1'>%2</span>").arg(color).arg(msg));
    logBox_->verticalScrollBar()->setValue(logBox_->verticalScrollBar()->maximum());
}

void MainWindow::logOK(const QString& msg)  { log(msg, "#56d78a"); }
void MainWindow::logErr(const QString& msg) { log(msg, "#e05050"); }

void MainWindow::applyPathToHL(const DynArray<int>& path) {
    for (int i = 0; i + 1 < path.size(); ++i) {
        hlEdgeSrc_.push_back(path[i]);
        hlEdgeDst_.push_back(path[i + 1]);
        hlNodes_.push_back(path[i]);
        hlNodes_.push_back(path[i + 1]);
    }
    refreshCanvas();
}

void MainWindow::clearHL() {
    hlNodes_.clear(); hlEdgeSrc_.clear(); hlEdgeDst_.clear();
    srcHL_ = dstHL_ = -1;
    canvas_->clearHL();
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
    SPIN(id, 1, 9999);
    auto* nm = new QLineEdit;
    auto* addr = new QLineEdit;
    DSPIN(lon, 0, 200); DSPIN(lat, 0, 100);
    form->addRow("编号:", id);
    form->addRow("名称:", nm);
    form->addRow("地址:", addr);
    form->addRow("经度:", lon);
    form->addRow("纬度:", lat);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (graph_.addNode(Node(id->value(), nm->text().toStdString(), addr->text().toStdString(),
                            lon->value(), lat->value()))) {
        logOK(QString("网点 [%1] %2 添加成功").arg(id->value()).arg(nm->text()));
        refreshStats(); refreshCanvas();
    } else {
        logErr("添加失败（编号已存在或名称为空）");
    }
}

void MainWindow::onDeleteNode() {
    MAKE_DLG("删除网点");
    SPIN(id, 1, 9999);
    form->addRow("编号:", id);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (graph_.deleteNode(id->value())) {
        logOK(QString("网点 [%1] 已删除").arg(id->value()));
        clearHL(); refreshStats(); refreshCanvas();
    } else {
        logErr("网点不存在");
    }
}

void MainWindow::onUpdateNode() {
    MAKE_DLG("修改网点");
    SPIN(id, 1, 9999);
    auto* nm = new QLineEdit;
    auto* addr = new QLineEdit;
    form->addRow("编号:", id);
    form->addRow("新名称:", nm);
    form->addRow("新地址:", addr);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (graph_.updateNode(id->value(), Node(id->value(), nm->text().toStdString(), addr->text().toStdString()))) {
        logOK("修改成功");
        refreshCanvas();
    } else {
        logErr("修改失败");
    }
}

void MainWindow::onFindNode() {
    MAKE_DLG("查询网点");
    SPIN(id, 1, 9999);
    form->addRow("编号:", id);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    const Node* n = graph_.findNode(id->value());
    if (n) {
        logOK(QString("[%1] %2  %3").arg(n->id)
              .arg(QString::fromStdString(n->name))
              .arg(QString::fromStdString(n->address)));
        clearHL();
        hlNodes_.push_back(n->id);
        refreshCanvas();
    } else {
        logErr("网点不存在");
    }
}

void MainWindow::onListNodes() {
    DynArray<int> ids = graph_.getAllNodeIds();
    for (int i = 0; i < ids.size() - 1; ++i)
        for (int j = i + 1; j < ids.size(); ++j)
            if (ids[i] > ids[j]) { int t = ids[i]; ids[i] = ids[j]; ids[j] = t; }
    log(QString("── 所有网点（%1）──").arg(ids.size()), "#5ab4ff");
    for (int i = 0; i < ids.size(); ++i) {
        const Node* n = graph_.findNode(ids[i]);
        if (n) log(QString("[%1] %2  %3").arg(n->id)
                   .arg(QString::fromStdString(n->name))
                   .arg(QString::fromStdString(n->address)));
    }
}

// ================================================================
// 路网管理
// ================================================================
void MainWindow::onAddEdge() {
    MAKE_DLG("添加路段");
    SPIN(f, 1, 9999); SPIN(t, 1, 9999);
    DSPIN(ti, 0, 999); DSPIN(co, 0, 99999);
    form->addRow("起点:", f);
    form->addRow("终点:", t);
    form->addRow("耗时(h):", ti);
    form->addRow("费用(元):", co);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (graph_.addEdge(Edge(f->value(), t->value(), ti->value(), co->value()))) {
        logOK(QString("路段 %1→%2 添加成功").arg(f->value()).arg(t->value()));
        refreshStats(); refreshCanvas();
    } else {
        logErr("添加失败");
    }
}

void MainWindow::onDeleteEdge() {
    MAKE_DLG("删除路段");
    SPIN(f, 1, 9999); SPIN(t, 1, 9999);
    form->addRow("起点:", f);
    form->addRow("终点:", t);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (graph_.deleteEdge(f->value(), t->value())) {
        logOK(QString("路段 %1→%2 已删除").arg(f->value()).arg(t->value()));
        clearHL(); refreshStats(); refreshCanvas();
    } else {
        logErr("路段不存在");
    }
}

void MainWindow::onListEdges() {
    DynArray<int> ids = graph_.getAllNodeIds();
    for (int i = 0; i < ids.size() - 1; ++i)
        for (int j = i + 1; j < ids.size(); ++j)
            if (ids[i] > ids[j]) { int t = ids[i]; ids[i] = ids[j]; ids[j] = t; }
    log(QString("── 所有路段（%1）──").arg(graph_.edgeCount()), "#5ab4ff");
    for (int i = 0; i < ids.size(); ++i) {
        const DynArray<Edge>& es = graph_.getNeighbors(ids[i]);
        for (int j = 0; j < es.size(); ++j) {
            std::ostringstream s;
            s << std::fixed << std::setprecision(1)
              << es[j].from << "→" << es[j].to
              << "  耗时" << es[j].time << "h  费用" << es[j].cost << "元";
            log(QString::fromStdString(s.str()));
        }
    }
}

void MainWindow::onImportNet() {
    QString p = QFileDialog::getOpenFileName(this, "导入路网", "data", "文本文件 (*.txt)");
    if (p.isEmpty()) return;
    graph_.clear();
    clearHL();
    if (FileManager::loadNetwork(p.toStdString(), graph_)) {
        logOK(QString("导入成功：%1 节点 / %2 路段").arg(graph_.nodeCount()).arg(graph_.edgeCount()));
        refreshStats(); refreshCanvas();
    } else {
        logErr("导入失败");
    }
}

void MainWindow::onExportNet() {
    QString p = QFileDialog::getSaveFileName(this, "导出路网", "data/network.txt", "文本文件 (*.txt)");
    if (p.isEmpty()) return;
    if (FileManager::saveNetwork(p.toStdString(), graph_))
        logOK("已保存：" + p);
    else
        logErr("保存失败");
}

// ================================================================
// 路径查询
// ================================================================
void MainWindow::onShortestTime() {
    MAKE_DLG("单源最短耗时查询");
    SPIN(src, 1, 9999);
    form->addRow("起点编号:", src);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;

    clearHL();
    auto res = Dijkstra::shortestTimeFrom(graph_, src->value());
    if (res.empty()) { logErr("起点不存在"); return; }

    srcHL_ = src->value();
    log(QString("── 从 [%1] 出发的最短耗时 ──").arg(src->value()), "#5ac4ff");

    DynArray<int> ids = graph_.getAllNodeIds();
    for (int i = 0; i < ids.size(); ++i) {
        if (ids[i] == src->value()) continue;
        PathResult* pr = res.find(ids[i]);
        if (!pr || !pr->reachable) continue;

        // 收集高亮边
        for (int j = 0; j + 1 < pr->path.size(); ++j) {
            hlEdgeSrc_.push_back(pr->path[j]);
            hlEdgeDst_.push_back(pr->path[j + 1]);
        }
        hlNodes_.push_back(ids[i]);

        const Node* n = graph_.findNode(ids[i]);
        log(QString("[%1] %2  耗时%3h  费用%4元")
            .arg(ids[i])
            .arg(n ? QString::fromStdString(n->name) : "?")
            .arg(pr->totalTime, 0, 'f', 1)
            .arg(pr->totalCost, 0, 'f', 0),
            "#c8deb0");
    }
    refreshCanvas();
}

void MainWindow::onCheapestPath() {
    MAKE_DLG("两点最低费用路径");
    SPIN(s, 1, 9999); SPIN(d, 1, 9999);
    form->addRow("起点:", s);
    form->addRow("终点:", d);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;

    clearHL();
    PathResult r = Dijkstra::cheapestPath(graph_, s->value(), d->value());
    if (!r.reachable) { logErr("不可达"); return; }

    srcHL_ = s->value();
    dstHL_ = d->value();
    applyPathToHL(r.path);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "路径: ";
    for (int i = 0; i < r.path.size(); ++i) {
        const Node* n = graph_.findNode(r.path[i]);
        ss << "[" << r.path[i] << "]"
           << (n ? n->name : "?");
        if (i + 1 < r.path.size()) ss << " → ";
    }
    logOK(QString::fromStdString(ss.str()));
    logOK(QString("费用: %1元  耗时: %2h").arg(r.totalCost, 0, 'f', 0).arg(r.totalTime, 0, 'f', 1));
}

void MainWindow::onClearHL() { clearHL(); logOK("已清除高亮"); }

// ================================================================
// 批次配送
// ================================================================
void MainWindow::onAddOrder() {
    MAKE_DLG("添加配送订单");
    SPIN(oid, 1, 99999);
    SPIN(s, 1, 9999);
    SPIN(d, 1, 9999);
    auto* goods = new QLineEdit;
    auto* opt = new QComboBox;
    opt->addItem("最低费用");
    opt->addItem("最短耗时");
    form->addRow("订单号:", oid);
    form->addRow("起点:", s);
    form->addRow("终点:", d);
    form->addRow("货物:", goods);
    form->addRow("优化目标:", opt);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (orders_.addOrder(Order(oid->value(), s->value(), d->value(),
                               goods->text().toStdString(), opt->currentIndex() == 1))) {
        logOK(QString("订单 %1 添加成功").arg(oid->value()));
        refreshStats();
    } else {
        logErr("订单号已存在");
    }
}

void MainWindow::onDelOrder() {
    MAKE_DLG("删除订单");
    SPIN(id, 1, 99999);
    form->addRow("订单号:", id);
    form->addRow(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    if (orders_.removeOrder(id->value())) {
        logOK(QString("订单 %1 已删除").arg(id->value()));
        refreshStats();
    } else {
        logErr("订单不存在");
    }
}

void MainWindow::onListOrders() {
    const DynArray<Order>& os = orders_.getOrders();
    log(QString("── 所有订单（%1）──").arg(os.size()), "#5ab4ff");
    for (int i = 0; i < os.size(); ++i)
        log(QString("[%1] %2→%3  %4  %5")
            .arg(os[i].orderId)
            .arg(os[i].srcNode)
            .arg(os[i].dstNode)
            .arg(QString::fromStdString(os[i].goods))
            .arg(os[i].byTime ? "最短耗时" : "最低费用"));
}

void MainWindow::onImportOrd() {
    QString p = QFileDialog::getOpenFileName(this, "导入订单", "data", "文本文件 (*.txt)");
    if (p.isEmpty()) return;
    if (FileManager::loadOrders(p.toStdString(), orders_)) {
        logOK(QString("导入：%1 条订单").arg(orders_.getOrders().size()));
        refreshStats();
    } else {
        logErr("导入失败");
    }
}

void MainWindow::onPlanAll() {
    clearHL();
    DynArray<DeliveryPlan> plans = orders_.planAllOrders(graph_);
    log(QString("── 批量规划结果 ──"), "#5ac4ff");
    for (int i = 0; i < plans.size(); ++i) {
        const DeliveryPlan& p = plans[i];
        if (!p.result.reachable) {
            logErr(QString("订单 %1 不可达").arg(p.order.orderId));
            continue;
        }
        std::ostringstream s;
        s << std::fixed << std::setprecision(1)
          << "[" << p.order.orderId << "] " << p.order.goods << ": ";
        for (int j = 0; j < p.result.path.size(); ++j) {
            s << p.result.path[j];
            if (j + 1 < p.result.path.size()) s << "→";
        }
        s << "  " << (p.order.byTime ? p.result.totalTime : p.result.totalCost)
          << (p.order.byTime ? "h" : "元");
        logOK(QString::fromStdString(s.str()));
        applyPathToHL(p.result.path);
    }
    logOK(QString("完成，共 %1 条").arg(plans.size()));
}

void MainWindow::onTopoSort() {
    clearHL();
    TopoResult res = orders_.planBatchSequence(graph_);
    if (res.hasCycle) {
        logErr("检测到环路！涉及节点：");
        for (int i = 0; i < res.cycleNodes.size(); ++i) {
            hlNodes_.push_back(res.cycleNodes[i]);
            const Node* n = graph_.findNode(res.cycleNodes[i]);
            log(QString("  %1（%2）").arg(res.cycleNodes[i])
                .arg(n ? QString::fromStdString(n->name) : "?"), "#e09050");
        }
    } else {
        log(QString("── 拓扑排序配送顺序 ──"), "#5ac4ff");
        QString line;
        for (int i = 0; i < res.order.size(); ++i) {
            int id = res.order[i];
            hlNodes_.push_back(id);
            const Node* n = graph_.findNode(id);
            QString nm = n ? QString::fromStdString(n->name) : "?";
            if (nm.size() > 3) nm = nm.left(3);
            line += QString("%1(%2)").arg(id).arg(nm);
            if (i + 1 < res.order.size()) line += "→";
            if (line.size() > 48) {
                log(line, "#c8d8b0");
                line.clear();
            }
        }
        if (!line.isEmpty()) log(line, "#c8d8b0");
        logOK(QString::number(res.order.size()) + " 个节点，无环路");
    }
    refreshCanvas();
}

void MainWindow::onExportPlans() {
    QString p = QFileDialog::getSaveFileName(this, "导出方案", "data/plans.txt", "文本文件 (*.txt)");
    if (p.isEmpty()) return;
    DynArray<DeliveryPlan> plans = orders_.planAllOrders(graph_);
    if (FileManager::savePlans(p.toStdString(), graph_, plans))
        logOK("已导出：" + p);
    else
        logErr("导出失败");
}
