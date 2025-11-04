cmake_minimum_required(VERSION 3.16)

project(comtool LANGUAGES CXX)

# ---- Qt 自动处理 ----
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---- Qt 模块 ----
# Qt6 优先，如果是 Qt5 也能自动降级
find_package(Qt6 COMPONENTS Core Gui Widgets Network SerialPort Core5Compat QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network SerialPort)
endif()

# ---- 源文件 ----
set(SOURCES
    main.cpp
    head.h
    # api
    api/appconfig.cpp
    api/appconfig.h
    api/appdata.cpp
    api/appdata.h
    api/qthelper.cpp
    api/qthelper.h
    api/qthelperdata.cpp
    api/qthelperdata.h
    # form
    form/frmcomtool.cpp
    form/frmcomtool.h
    form/frmcomtool.ui
    # qrc
    qrc/main.qrc
)

# ---- 可执行文件 ----
add_executable(${PROJECT_NAME} ${SOURCES})

# ---- 头文件路径 ----
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/form
)

# ---- 链接 Qt 库 ----
if (Qt6_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::SerialPort
        Qt6::Core5Compat
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::SerialPort
    )
endif()

# ---- 输出路径 ----
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../bin"
)

# ---- 资源文件（main.rc） ----
if (WIN32)
    set(APP_RC "${CMAKE_SOURCE_DIR}/qrc/main.rc")
    if (EXISTS ${APP_RC})
        target_sources(${PROJECT_NAME} PRIVATE ${APP_RC})
    endif()
endif()

# ---- 禁用编译警告（等价于 CONFIG += warn_off）----
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /w)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -w)
endif()

# ---- 复制运行时所需文件 ----
# 可选：拷贝 file/ 目录 和 qrc/qm 目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/file
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/file
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/qrc/qm
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/qrc/qm
)
