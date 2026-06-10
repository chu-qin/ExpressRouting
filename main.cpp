// ============================================================================
// main.cpp — 程序入口
// ============================================================================
// Qt 应用程序启动流程：
//   1. QApplication 初始化（解析命令行参数、管理事件循环）
//   2. 切换到可执行文件目录（确保 data/ 相对路径正确）
//   3. 创建主窗口（构造函数中加载数据、构建 UI）
//   4. 调用 show() 显示窗口
//   5. 进入事件循环 a.exec()，程序在此阻塞直到窗口关闭
// ============================================================================

#include <QApplication>
#include <QDir>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // 切换到 .exe 所在目录（Qt Creator 中运行时工作目录可能是 build/）
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    MainWindow win;
    win.show();
    return app.exec();
}
