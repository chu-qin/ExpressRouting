cmake_minimum_required(VERSION 3.16)

project(ExpressRouting VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
    Graph.cpp
    Graph.h
    Dijkstra.cpp
    Dijkstra.h
    OrderManager.cpp
    OrderManager.h
    Containers.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ExpressRouting
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(ExpressRouting ${PROJECT_SOURCES})
endif()

target_include_directories(ExpressRouting PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ExpressRouting PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# 将 data/ 目录复制到编译输出目录
add_custom_command(TARGET ExpressRouting POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:ExpressRouting>/data
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ExpressRouting)
endif()
