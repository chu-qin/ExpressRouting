#include "mainwindow.h"

// =====================================================================
// GraphWidget::paintEvent — 图绘制核心（每次 update() 时被 Qt 调用）
// =====================================================================
// 绘制顺序（背景→边→高亮→节点）决定了图层叠放关系：
//   先画的在底层，后画的在上层。
//   所以节点画在最后——始终盖在边和路径上面。
// =====================================================================
void GraphWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);                         // 创建画笔
    p.setRenderHint(QPainter::Antialiasing);   // 开启抗锯齿（让线更平滑）

    if (!mgr) return;                         // 没数据就不画
    ExpressGraph* g = mgr->getGraph();
    int N = g->getNodeCount();
    if (N == 0) return;

    // -------------------------------------------------------
    // 第一步：扫描所有节点的 X/Y 坐标，找出最小/最大值
    //   用于计算"真实坐标→屏幕像素"的映射比例
    // -------------------------------------------------------
    double xMin = INF, xMax = -INF, yMin = INF, yMax = -INF;
    for (int i = 0; i < N; i++) {
        double x = g->getNodeX(i);
        double y = g->getNodeY(i);
        if (x < xMin) xMin = x;
        if (x > xMax) xMax = x;
        if (y < yMin) yMin = y;
        if (y > yMax) yMax = y;
    }
    // 防止除零：如果所有点坐标相同，设一个默认范围
    if (xMax - xMin < 1) xMax = xMin + 1;
    if (yMax - yMin < 1) yMax = yMin + 1;

    // -------------------------------------------------------
    // 第二步：计算缩放比例（保持宽高比，留 55px 边距）
    // -------------------------------------------------------
    const int M = 55;                         // 边距（留白）
    int W = width();                          // 控件的像素宽度
    int H = height();                         // 控件的像素高度
    double scaleX = (W - 2.0 * M) / (xMax - xMin);
    double scaleY = (H - 2.0 * M) / (yMax - yMin);
    double scale  = scaleX < scaleY ? scaleX : scaleY;  // 取较小值，保证不超出

    // 世界坐标 → 屏幕像素 的转换函数（Lambda 表达式）
    auto toX = [&](double wx) { return M + (wx - xMin) * scale; };
    auto toY = [&](double wy) { return M + (wy - yMin) * scale; };

    // 预计算所有节点的屏幕坐标（避免后面重复计算）
    double* sx = new double[N];
    double* sy = new double[N];
    for (int i = 0; i < N; i++) {
        sx[i] = toX(g->getNodeX(i));
        sy[i] = toY(g->getNodeY(i));
    }

    // -------------------------------------------------------
    // 第三步：画背景网格（浅灰色，间距 40px）
    //   模仿地图或专业绘图工具的网格背景
    // -------------------------------------------------------
    p.setPen(QPen(QColor(240, 240, 240), 1));
    for (int gx = M; gx < W - M; gx += 40)
        p.drawLine(gx, M, gx, H - M);
    for (int gy = M; gy < H - M; gy += 40)
        p.drawLine(M, gy, W - M, gy);

    // -------------------------------------------------------
    // 第四步：画所有有向边（灰色线 + 箭头 + 边权数字）
    //   遍历整个邻接矩阵，mat[i][j] != INF 说明 i→j 有边
    // -------------------------------------------------------
    p.setFont(QFont("Consolas", 7));          // 边权数字用等宽小字

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            double w = g->getWeight(i, j);
            if (w == INF) continue;           // 无边，跳过

            double x1 = sx[i], y1 = sy[i];    // 起点屏幕坐标
            double x2 = sx[j], y2 = sy[j];    // 终点屏幕坐标

            // 判断这条边是否属于高亮路径的一段
            bool onHL = false;
            for (int k = 0; k < highlight.getSize() - 1; k++) {
                if (highlight[k] == i && highlight[k + 1] == j) {
                    onHL = true;
                    break;
                }
            }

            if (!onHL) {
                // ---- 普通边：灰色细线 ----
                p.setPen(QPen(QColor(190, 190, 190), 0.8));
                p.drawLine(QPointF(x1, y1), QPointF(x2, y2));
            }
            // 注：高亮边不在这里画，等第五步统一画红色粗线

            // ---- 画箭头（三角形，方向指向终点） ----
            double ang = atan2(y2 - y1, x2 - x1);  // 线段角度
            double asz = 8;                         // 箭头大小
            QPointF tip(x2, y2);
            // 箭头的两条"翅膀"：从 tip 向两侧偏移 0.5 弧度
            QPointF al(x2 - asz * cos(ang - 0.5), y2 - asz * sin(ang - 0.5));
            QPointF ar(x2 - asz * cos(ang + 0.5), y2 - asz * sin(ang + 0.5));
            p.drawLine(tip, al);
            p.drawLine(tip, ar);

            // ---- 画边权数字（线段中点偏上方） ----
            p.setPen(QColor(140, 140, 140));
            double mx = (x1 + x2) / 2;           // 中点 X
            double my = (y1 + y2) / 2;           // 中点 Y
            p.drawText(QPointF(mx + 4, my - 4),
                       QString::number(w, 'f', 1));  // 保留 1 位小数
        }
    }

    // -------------------------------------------------------
    // 第五步：画高亮路径（红色粗线，画在普通边上面）
    //   单独循环确保高亮线在最上层（不会被灰线盖住）
    // -------------------------------------------------------
    if (highlight.getSize() >= 2) {
        p.setPen(QPen(QColor(220, 50, 50), 5));  // 红色，线宽 5px
        for (int k = 0; k < highlight.getSize() - 1; k++) {
            int a = highlight[k];
            int b = highlight[k + 1];
            p.drawLine(QPointF(sx[a], sy[a]), QPointF(sx[b], sy[b]));
        }
    }

    // -------------------------------------------------------
    // 第六步：画节点（圆 + 标签）
    //   每个节点：阴影 → 渐变圆 → 标签文字
    //   在高亮路径上的节点：红色实心圆
    // -------------------------------------------------------
    p.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));

    for (int i = 0; i < N; i++) {
        // 判断当前节点是否在高亮路径上
        bool onPath = false;
        for (int k = 0; k < highlight.getSize(); k++) {
            if (highlight[k] == i) { onPath = true; break; }
        }

        double cx = sx[i], cy = sy[i];   // 圆心
        double r  = 11;                   // 半径（px）

        // ---- 阴影（半透明灰色圆，偏移 2px） ----
        p.setBrush(QColor(0, 0, 0, 40));    // RGBA: 黑色，alpha=40/255
        p.setPen(Qt::NoPen);                 // 无边框
        p.drawEllipse(QPointF(cx + 2, cy + 2), r, r);

        // ---- 主体圆 ----
        if (onPath) {
            // 高亮节点：红色实心
            p.setBrush(QColor(255, 80, 80));
            p.setPen(QPen(QColor(180, 20, 20), 2.5));
        } else {
            // 普通节点：蓝色渐变（浅蓝 → 深蓝）
            QLinearGradient grad(cx - r, cy - r, cx + r, cy + r);
            grad.setColorAt(0, QColor(80, 150, 240));   // 左上角浅蓝
            grad.setColorAt(1, QColor(40, 100, 200));   // 右下角深蓝
            p.setBrush(grad);
            p.setPen(QPen(QColor(30, 70, 150), 2));     // 深蓝边框
        }
        p.drawEllipse(QPointF(cx, cy), r, r);

        // ---- 城市名标签（圆上方居中） ----
        p.setPen(Qt::black);
        QString name = QString::fromUtf8(g->getNodeName(i));
        // 文字范围：圆上方的一个矩形框，居中显示
        QRectF br(cx - 30, cy - r - 22, 60, 20);
        p.drawText(br, Qt::AlignCenter, name);
    }

    // 释放临时数组
    delete[] sx;
    delete[] sy;
}

// =====================================================================
// MainWindow 构造函数
// =====================================================================
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)       // 先调用父类 QMainWindow 的构造
{
    mgr = new ExpressManager();  // 创建业务管理层（后端）

    // 窗口基本属性
    setWindowTitle("快递网点配送路径规划系统");
    resize(1200, 750);           // 默认窗口大小
    setMinimumSize(900, 550);    // 最小窗口（防止控件挤压变形）

    // ========== 全局样式表（QSS，类似 CSS） ==========
    // 设置整个窗口的颜色、边框、圆角等视觉效果
    setStyleSheet(
        // 主窗口背景
        "QMainWindow { background: #f5f6fa; }"

        // QGroupBox：分组框 → 白色背景、圆角边框
        "QGroupBox { font-weight: bold; border: 1px solid #d0d0d0; "
        "  border-radius: 6px; margin-top: 12px; padding-top: 16px; "
        "  background: #ffffff; }"
        // QGroupBox 标题 → 深灰色，在边框上方
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; "
        "  padding: 0 6px; color: #2c3e50; }"

        // QPushButton：默认蓝色
        "QPushButton { background: #3498db; color: white; border: none; "
        "  border-radius: 4px; padding: 6px 14px; font-size: 12px; "
        "  min-height: 24px; }"
        // 鼠标悬浮时变深
        "QPushButton:hover { background: #2980b9; }"
        // 按下时更深
        "QPushButton:pressed { background: #1a5276; }"
        // 删除按钮（objectName="btnDel"）用红色
        "QPushButton#btnDel { background: #e74c3c; }"
        "QPushButton#btnDel:hover { background: #c0392b; }"

        // QComboBox：下拉框
        "QComboBox { border: 1px solid #c0c0c0; border-radius: 3px; "
        "  padding: 3px 6px; background: white; min-height: 22px; }"

        // QLineEdit：输入框
        "QLineEdit { border: 1px solid #c0c0c0; border-radius: 3px; "
        "  padding: 3px 6px; background: white; }"

        // QTextEdit：日志框
        "QTextEdit { border: 1px solid #c0c0c0; border-radius: 4px; "
        "  background: #fafbfc; }"

        // QTableWidget：结果表 → 交替行颜色
        "QTableWidget { border: 1px solid #d0d0d0; gridline-color: #e0e0e0; "
        "  background: white; alternate-background-color: #f7f8fc; }"
        // 表头 → 浅灰背景 + 底部边框
        "QHeaderView::section { background: #e8ecf2; padding: 4px; "
        "  border: none; border-bottom: 2px solid #d0d0d0; font-weight: bold; }"

        // QToolBar：工具栏 → 白色背景
        "QToolBar { background: #ffffff; border-bottom: 1px solid #d0d0d0; "
        "  spacing: 6px; padding: 3px; }"

        // QStatusBar：状态栏 → 浅灰背景
        "QStatusBar { background: #f0f1f5; border-top: 1px solid #d0d0d0; "
        "  font-size: 11px; }"
    );

    // 构建界面的三个部分
    buildToolBar();
    buildPanel();
    buildStatusBar();

    // 最后加载数据
    loadData();
}

MainWindow::~MainWindow() {
    delete mgr;   // 释放业务管理层（其析构会自动释放 ExpressGraph）
}

// =====================================================================
// 构建顶部工具栏
// =====================================================================
void MainWindow::buildToolBar() {
    QToolBar* tb = addToolBar("主工具栏");
    tb->setMovable(false);              // 不允许用户拖动工具栏
    tb->setIconSize(QSize(18, 18));

    btnPath   = new QPushButton(" 最短路径 ");
    btnTopo   = new QPushButton(" 批次排班 ");
    btnOrders = new QPushButton(" 批量订单 ");

    tb->addWidget(btnPath);
    tb->addSeparator();                // 竖线分隔
    tb->addWidget(btnTopo);
    tb->addSeparator();
    tb->addWidget(btnOrders);

    // 信号与槽：按钮点击 → 调用对应的槽函数
    connect(btnPath,   &QPushButton::clicked, this, &MainWindow::doFindPath);
    connect(btnTopo,   &QPushButton::clicked, this, &MainWindow::doTopo);
    connect(btnOrders, &QPushButton::clicked, this, &MainWindow::doOrders);
}

// =====================================================================
// 构建左侧控制面板（放在 QScrollArea 中，通过 QSplitter 连接右侧绘图区）
// =====================================================================
void MainWindow::buildPanel() {
    // 面板主体
    QWidget* pan = new QWidget();
    pan->setMinimumWidth(260);       // 最小宽度（防止被拉到看不见）
    pan->setMaximumWidth(420);       // 最大宽度
    QVBoxLayout* pl = new QVBoxLayout(pan);
    pl->setContentsMargins(8, 4, 8, 4);
    pl->setSpacing(6);

    // ===== 第一组：路径查询 =====
    QGroupBox* gb1 = new QGroupBox("路径查询");
    QFormLayout* fl = new QFormLayout(gb1);    // 表单布局："起点:" + 下拉框
    fl->setSpacing(6);
    cmbFrom = new QComboBox();
    cmbTo   = new QComboBox();
    fl->addRow("起点:", cmbFrom);    // addRow 自动把第一个参数变 QLabel
    fl->addRow("终点:", cmbTo);
    pl->addWidget(gb1);

    // ===== 第二组：网点管理（增删） =====
    QGroupBox* gb2 = new QGroupBox("网点管理");
    QFormLayout* fl2 = new QFormLayout(gb2);
    fl2->setSpacing(4);
    editName = new QLineEdit();
    editX    = new QLineEdit();
    editY    = new QLineEdit();
    btnAdd   = new QPushButton(" 添加网点 ");
    btnDel   = new QPushButton(" 删除网点 ");
    btnDel->setObjectName("btnDel");           // 用 objectName 让 QSS 样式表定位
    fl2->addRow("名称:", editName);
    fl2->addRow("X:", editX);
    fl2->addRow("Y:", editY);
    QHBoxLayout* hb = new QHBoxLayout();        // 添加/删除按钮横排
    hb->addWidget(btnAdd);
    hb->addWidget(btnDel);
    fl2->addRow(hb);                            // 整行放置按钮组
    pl->addWidget(gb2);

    // ===== 第三组：运行日志 =====
    QGroupBox* gb3 = new QGroupBox("运行日志");
    QVBoxLayout* vl3 = new QVBoxLayout(gb3);
    txtLog = new QTextEdit();
    txtLog->setReadOnly(true);                  // 只读，用户不能编辑
    txtLog->setMaximumHeight(120);              // 限制高度
    vl3->addWidget(txtLog);
    pl->addWidget(gb3);

    // ===== 第四组：结果详情表 =====
    QGroupBox* gb4 = new QGroupBox("结果详情");
    QVBoxLayout* vl4 = new QVBoxLayout(gb4);
    tblRes = new QTableWidget();
    tblRes->setColumnCount(3);                  // 三列：序号 | 网点 | 说明
    tblRes->setHorizontalHeaderLabels({"#", "网点", "说明"});
    tblRes->horizontalHeader()->setStretchLastSection(true);  // 最后一列自动填满
    tblRes->setAlternatingRowColors(true);      // 交替行颜色
    tblRes->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止编辑
    tblRes->setSelectionBehavior(QAbstractItemView::SelectRows); // 点击选中整行
    tblRes->verticalHeader()->setVisible(false); // 隐藏左侧行号
    vl4->addWidget(tblRes);
    pl->addWidget(gb4);

    // 把面板放入滚动区域（面板内容超出时可滚）
    QScrollArea* sa = new QScrollArea();
    sa->setWidget(pan);
    sa->setWidgetResizable(true);               // 面板随滚动区宽度变化
    sa->setFrameShape(QFrame::NoFrame);         // 无边框

    // 创建绘图控件
    graph = new GraphWidget();

    // 用 QSplitter 把滚动面板和绘图区左右分割
    splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(sa);    // 左侧：面板
    splitter->addWidget(graph); // 右侧：绘图区
    splitter->setStretchFactor(0, 0);           // 面板不拉伸
    splitter->setStretchFactor(1, 1);           // 绘图区随窗口拉伸
    splitter->setSizes({300, 900});             // 初始比例

    setCentralWidget(splitter);                 // 设为窗口的中央控件

    // 网点管理的信号绑定
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::doAddNode);
    connect(btnDel, &QPushButton::clicked, this, &MainWindow::doDelNode);
}

// =====================================================================
// 构建底部状态栏
// =====================================================================
void MainWindow::buildStatusBar() {
    lblStatus = new QLabel("就绪");
    // addPermanentWidget: 放在状态栏右侧
    // stretch=1: 占满剩余空间
    statusBar()->addPermanentWidget(lblStatus, 1);
}

// =====================================================================
// 加载路网数据
// =====================================================================
void MainWindow::loadData() {
    if (mgr->loadNetwork("nodes.txt", "edges.txt")) {
        graph->mgr = mgr;             // 把数据指针传给绘图控件
        refreshComboBoxes();          // 填充下拉框

        // 统计边数（用于状态栏显示）
        int n = mgr->getGraph()->getNodeCount();
        int e = 0;
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                if (mgr->getGraph()->hasEdge(i, j)) e++;

        lblStatus->setText(
            QString("节点: %1  |  边: %2  |  就绪").arg(n).arg(e));
        log(QString("路网加载成功 (%1 节点, %2 边)").arg(n).arg(e));
    } else {
        logErr("加载失败: nodes.txt / edges.txt 未找到");
    }
}

// 刷新起点/终点下拉框（节点增删后调用）
void MainWindow::refreshComboBoxes() {
    cmbFrom->clear();
    cmbTo->clear();
    ExpressGraph* g = mgr->getGraph();
    for (int i = 0; i < g->getNodeCount(); i++) {
        QString s = QString::fromUtf8(g->getNodeName(i));
        cmbFrom->addItem(s);
        cmbTo->addItem(s);
    }
}

// ---- 日志辅助 ----
void MainWindow::log(const QString& s) {
    // HTML 富文本：灰色 [INFO] 前缀
    txtLog->append("<span style='color:#2c3e50'>[INFO]</span> " + s);
}

void MainWindow::logErr(const QString& s) {
    // HTML 富文本：红色 [ERR] 前缀 + 加粗
    txtLog->append(
        "<span style='color:#c0392b;font-weight:bold'>[ERR]</span> " + s);
}

// =====================================================================
// 槽函数：计算最短路径（Dijkstra）
// =====================================================================
void MainWindow::doFindPath() {
    graph->clearPath();    // 清除上次的高亮

    // 从 QComboBox 获取用户选择的起点/终点，转为 UTF-8 字节
    QByteArray fa = cmbFrom->currentText().toUtf8();
    QByteArray ta = cmbTo->currentText().toUtf8();

    // 调用后端核心算法
    PathResult r = mgr->findShortestPath(fa.constData(), ta.constData());

    if (!r.reachable) {
        logErr(QString("不可达: %1 -> %2")
               .arg(cmbFrom->currentText(), cmbTo->currentText()));
        QMessageBox::warning(this, "无路径", "两点间没有连通路径。");
        lblStatus->setText("上次查询: 不可达");
        return;
    }

    // 日志输出
    log(QString("路径: %1 → %2  |  耗时 %3 h")
        .arg(cmbFrom->currentText(), cmbTo->currentText())
        .arg(r.cost, 0, 'f', 1));    // 保留 1 位小数

    // 填充结果表格
    tblRes->setRowCount(0);           // 清空旧行
    ExpressGraph* g = mgr->getGraph();
    for (int i = 0; i < r.path.getSize(); i++) {
        tblRes->insertRow(i);
        // 列 0: 序号
        tblRes->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        // 列 1: 网点名
        tblRes->setItem(i, 1, new QTableWidgetItem(
            QString::fromUtf8(g->getNodeName(r.path[i]))));
        // 列 2: 到达 or 途经
        tblRes->setItem(i, 2, new QTableWidgetItem(
            i == r.path.getSize() - 1 ? "✓ 到达" : "→ 途经"));
    }

    // 绘图区高亮路径
    graph->showPath(r.path);

    // 状态栏更新
    lblStatus->setText(QString("路径: %1 → %2  |  %3 h  |  %4 站")
        .arg(cmbFrom->currentText(), cmbTo->currentText())
        .arg(r.cost, 0, 'f', 1)
        .arg(r.path.getSize()));
}

// =====================================================================
// 槽函数：批次排班检测（拓扑排序）
// =====================================================================
void MainWindow::doTopo() {
    graph->clearPath();

    TopoResult r = mgr->checkDependencies("dependencies.txt");

    if (r.hasCycle) {
        logErr("拓扑排序: 检测到配送环路冲突!");
        QMessageBox::critical(this, "环路错误",
            "依赖关系中存在循环!\n请检查 dependencies.txt。");
        lblStatus->setText("拓扑排序: 存在环路");
        return;
    }

    log(QString("拓扑排序: %1 节点, 无环路").arg(r.order.getSize()));

    // 填充结果表格
    tblRes->setRowCount(0);
    ExpressGraph* g = mgr->getGraph();
    for (int i = 0; i < r.order.getSize(); i++) {
        tblRes->insertRow(i);
        tblRes->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        tblRes->setItem(i, 1, new QTableWidgetItem(
            QString::fromUtf8(g->getNodeName(r.order[i]))));
        tblRes->setItem(i, 2, new QTableWidgetItem(
            QString("第 %1 批次").arg(i + 1)));
    }

    // 在绘图区用高亮展示拓扑排序顺序
    graph->showPath(r.order);
    lblStatus->setText(
        QString("拓扑排序: %1 批次").arg(r.order.getSize()));
}

// =====================================================================
// 槽函数：批量处理订单
// =====================================================================
void MainWindow::doOrders() {
    graph->clearPath();

    int n = mgr->batchProcessOrders("orders.txt", "result.txt");

    if (n < 0) {
        logErr("批量订单: 无法读取 orders.txt");
        return;
    }

    log(QString("批量订单: 成功 %1 单 → result.txt").arg(n));
    QMessageBox::information(this, "批量处理完成",
        QString("处理 %1 笔订单完毕。\n详细结果见 result.txt。").arg(n));
    lblStatus->setText(QString("批量订单: %1 笔已处理").arg(n));
}

// =====================================================================
// 槽函数：添加网点
// =====================================================================
void MainWindow::doAddNode() {
    QByteArray na = editName->text().trimmed().toUtf8();
    if (na.isEmpty()) return;            // 空名称不处理

    bool ox, oy;
    double x = editX->text().toDouble(&ox);   // toDouble: QString→double
    double y = editY->text().toDouble(&oy);   // ox=true 表示转换成功
    if (!ox || !oy) {
        logErr("坐标须为数字");
        return;
    }

    if (!mgr->addNode(na.constData(), x, y)) {
        logErr("网点已存在: " + editName->text());
        return;
    }

    log("添加: " + editName->text());
    refreshComboBoxes();                 // 下拉框同步更新
    editName->clear();
    editX->clear();
    editY->clear();
    graph->update();                     // 刷新绘图

    int n = mgr->getGraph()->getNodeCount();
    lblStatus->setText(
        QString("节点: %1  |  新增: %2").arg(n).arg(QString::fromUtf8(na)));
}

// =====================================================================
// 槽函数：删除网点
// =====================================================================
void MainWindow::doDelNode() {
    QByteArray na = editName->text().trimmed().toUtf8();
    if (na.isEmpty()) return;

    if (!mgr->removeNode(na.constData())) {
        logErr("网点不存在: " + editName->text());
        return;
    }

    log("已删除: " + editName->text());
    graph->clearPath();                  // 清除高亮（被删节点可能在路径上）
    refreshComboBoxes();
    editName->clear();
    graph->update();

    int n = mgr->getGraph()->getNodeCount();
    lblStatus->setText(QString("节点: %1  |  已删除").arg(n));
}
