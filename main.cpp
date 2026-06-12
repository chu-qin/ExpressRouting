// ============================================================================
// main.cpp —— 程序入口
// ============================================================================

#include "mainwindow.h"
#include <QApplication>
#include <iostream>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    std::cout << "========================================\n";
    std::cout << "  快递网点配送路径规划系统\n";
    std::cout << "  数据结构与算法 综合实验\n";
    std::cout << "========================================\n\n";

    MainWindow w;
    w.show();
    return app.exec();
}
