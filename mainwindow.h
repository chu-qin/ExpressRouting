#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// ============================================================================
// Qt 头文件 — 每个的作用见右侧注释
// ============================================================================
#include <QMainWindow>       // 主窗口基类（setCentralWidget / addToolBar / statusBar）
#include <QToolBar>          // 顶部工具栏（放快捷按钮）
#include <QStatusBar>        // 底部状态栏（显示节点数、边数、操作结果）
#include <QSplitter>         // 可拖动的分割条（左侧面板 | 右侧绘图区 可调大小）
#include <QComboBox>         // 下拉选择框（选起点/终点，避免用户手输错字）
#include <QPushButton>       // 按钮（计算路径、批次排班、添加/删除网点等）
#include <QTextEdit>         // 多行文本显示框（日志输出，支持 HTML 彩色文字）
#include <QLineEdit>         // 单行输入框（网点名称、X 坐标、Y 坐标输入）
#include <QTableWidget>      // 表格控件（显示路径详情、批次结果）
#include <QVBoxLayout>       // 纵向布局器（控件从上到下排列）
#include <QHBoxLayout>       // 横向布局器（控件从左到右排列）
#include <QFormLayout>       // 表单布局器（"起点: [下拉框]" 这种对齐格式）
#include <QLabel>            // 标签文字（"起点:" "日志:" 等静态文字）
#include <QGroupBox>         // 带标题的分组框（"路径查询""网点管理" 等分组）
#include <QMessageBox>       // 弹窗对话框（警告、错误、信息提示）
#include <QPainter>          // 画笔引擎（drawLine / drawEllipse / drawText）
#include <QPaintEvent>       // 绘图事件（paintEvent 函数参数类型）
#include <QHeaderView>       // 表格表头控制（最后一列自动拉伸）
#include <QScrollArea>       // 滚动区域（控制面板内容过多时可滚动）
#include <QFont>             // 字体（设置绘图时节点标签的字体）
#include <QColor>            // 颜色（节点蓝、高亮红、边灰等）
#include <QPen>              // 画笔属性（线宽、颜色）
#include <QBrush>            // 画刷属性（填充色、渐变）
#include <QLinearGradient>   // 线性渐变（节点从浅蓝到深蓝的渐变效果）
#include <cmath>             // C 数学库（atan2 / cos / sin → 画箭头用）

#include "ExpressManager.h"  // 业务管理层（我们的后端逻辑）

// ============================================================================
// GraphWidget — 图绘制控件
// ============================================================================
// 这是一个自定义 QWidget，重写了 paintEvent 来实现所有绘图：
//   - 灰色背景网格
//   - 灰色有向边 + 箭头 + 边权数字（如 "5.5"）
//   - 蓝色渐变实心圆 + 城市名标签
//   - 红色粗线高亮最短路径 + 节点变红
//
// 为什么单独做成一个控件？
//   放在 QSplitter 右侧，有自己独立的坐标系（0,0）= 控件左上角，
//   无需像画在整个 MainWindow 上那样手动偏移面板宽度。
// ============================================================================
class GraphWidget : public QWidget {
    Q_OBJECT    // Qt 元对象宏（启用信号槽、MOC 预处理）

public:
    ExpressManager* mgr;          // 指向业务管理层的指针（获取图数据）
    MyVector<int>   highlight;    // 当前需要高亮的路径（节点索引序列）

    explicit GraphWidget(QWidget* parent = nullptr)
        : QWidget(parent), mgr(nullptr) {
        setMinimumSize(500, 400);
        // 尺寸策略：水平+垂直都扩展，填满分配给它的所有空间
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    // 设置高亮路径并刷新
    void showPath(const MyVector<int>& path) {
        highlight = path;
        update();    // 触发 Qt 重绘 → 调用 paintEvent
    }

    // 清除高亮路径并刷新
    void clearPath() {
        highlight.clear();
        update();
    }

protected:
    // 绘图事件（Qt 在需要重绘时自动调用）
    void paintEvent(QPaintEvent*) override;
};

// ============================================================================
// MainWindow — 主窗口
// ============================================================================
// 界面结构：
//  ┌─────────────────────────────────────────────────────┐
//  │ 工具栏: [最短路径] [批次排班] [批量订单]            │
//  ├───────────────┬─────────────────────────────────────┤
//  │ 控制面板(可调) │                                     │
//  │  路径查询      │        GraphWidget 绘图区            │
//  │  网点管理      │                                     │
//  │  运行日志      │                                     │
//  │  结果详情      │                                     │
//  ├───────────────┴─────────────────────────────────────┤
//  │ 状态栏: 节点:30 | 边:90 | 就绪                       │
//  └─────────────────────────────────────────────────────┘
// ============================================================================
class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    ExpressManager* mgr;      // 业务逻辑层指针
    GraphWidget*    graph;    // 绘图控件
    QSplitter*      splitter; // 分割器（左侧面板 | 右侧绘图区）

    // ---- 控件成员 ----
    QComboBox*   cmbFrom;   // 起点下拉框
    QComboBox*   cmbTo;     // 终点下拉框
    QPushButton* btnPath;   // 工具栏 → 最短路径
    QPushButton* btnTopo;   // 工具栏 → 批次排班
    QPushButton* btnOrders; // 工具栏 → 批量订单
    QLineEdit*   editName;  // 网点管理 → 名称输入
    QLineEdit*   editX;     // 网点管理 → X 坐标
    QLineEdit*   editY;     // 网点管理 → Y 坐标
    QPushButton* btnAdd;    // 网点管理 → 添加按钮
    QPushButton* btnDel;    // 网点管理 → 删除按钮
    QTextEdit*   txtLog;    // 日志输出框
    QTableWidget* tblRes;   // 结果详情表
    QLabel*      lblStatus; // 状态栏文字标签

    // ---- 构建界面函数 ----
    void buildToolBar();       // 创建顶部工具栏
    void buildPanel();         // 创建左侧控制面板
    void buildStatusBar();     // 创建底部状态栏
    void loadData();           // 加载 nodes.txt / edges.txt
    void refreshComboBoxes();  // 刷新起点/终点下拉框列表

    // ---- 日志辅助函数 ----
    void log(const QString& s);    // 普通日志（灰色文字）
    void logErr(const QString& s); // 错误日志（红色文字）

    // ---- 槽函数（按钮点击响应） ----
private slots:
    void doFindPath();   // 计算最短路径
    void doTopo();       // 批次排班检测
    void doOrders();     // 批量处理订单
    void doAddNode();    // 添加网点
    void doDelNode();    // 删除网点

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
};

#endif
