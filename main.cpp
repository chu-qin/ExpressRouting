// ============================================================================
// main.cpp — 程序入口
// ============================================================================
// Qt 应用程序的标准启动流程：
//   1. 创建 QApplication 对象（管理事件循环、窗口系统）
//   2. 创建主窗口 MainWindow 实例
//   3. 调用 show() 显示窗口
//   4. 调用 a.exec() 进入事件循环（程序在此等待用户操作）
//
// QApplication 必须在使用任何 Qt GUI 组件之前创建。
// argc/argv 传递给 QApplication 以支持命令行参数（如 -style fusion）。
// ============================================================================

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);   // 创建 Qt 应用（解析命令行参数）
    MainWindow w;                 // 创建主窗口（构造函数中加载数据、构建 UI）
    w.show();                     // 显示窗口
    return a.exec();              // 进入事件循环，程序在此阻塞直到窗口关闭
}
