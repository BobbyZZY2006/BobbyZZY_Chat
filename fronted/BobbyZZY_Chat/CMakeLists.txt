cmake_minimum_required(VERSION 3.16)

project(BobbyZZY_Chat VERSION 0.1 LANGUAGES CXX)

# 自动处理 Qt 的 moc / ui / qrc
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 编译器标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# clangd 补全支持
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt 安装路径（macOS）
set(CMAKE_PREFIX_PATH "/Users/bobbyzzy/Qt/6.9.3/macos")

# 查找 Qt 模块
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

# 源文件列表
set(PROJECT_SOURCES
    main.cpp
    global.cpp
    logindialog.cpp
    registerdialog.cpp
    ResetDialog.cpp
    mainwindow.cpp
    ChatDialog.cpp
    CustomizeEdit.cpp
    TcpMgr.cpp
    TimerBtn.cpp
    HttpMgr.cpp
    UserMgr.cpp

    global.h
    logindialog.h
    registerdialog.h
    ResetDialog.h
    mainwindow.h
    ChatDialog.h
    CustomizeEdit.h
    TcpMgr.h
    TimerBtn.h
    HttpMgr.h
    UserMgr.h
    Singleton.h

    logindialog.ui
    registerdialog.ui
    ResetDialog.ui
    mainwindow.ui
    ChatDialog.ui

    res.qrc
    style/stylesheet.qss
)

# 创建可执行文件
qt_add_executable(BobbyZZY_Chat
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# 链接 Qt 库
target_link_libraries(BobbyZZY_Chat PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# macOS bundle 设置
set_target_properties(BobbyZZY_Chat PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 安装规则（可选）
include(GNUInstallDirs)
install(TARGETS BobbyZZY_Chat
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Qt6 finalize
qt_finalize_executable(BobbyZZY_Chat)

# 包含当前目录头文件
target_include_directories(BobbyZZY_Chat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 拷贝 config.ini
add_custom_command(TARGET BobbyZZY_Chat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/config.ini
        $<TARGET_FILE_DIR:BobbyZZY_Chat>/config.ini
)
