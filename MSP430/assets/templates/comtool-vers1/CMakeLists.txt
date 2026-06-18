cmake_minimum_required(VERSION 3.16)

project(comtool LANGUAGES CXX)

set(CMAKE_AUTOMOC_MOC_OPTIONS "-Muri=Qt6Compat")

# ---- Qt 自动处理 ----
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---- Qt 模块 ----
# Qt6 优先，如果是 Qt5 也能自动降级
find_package(Qt6 COMPONENTS Core Gui Widgets Network PrintSupport SerialPort Core5Compat QUIET)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
if (NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network PrintSupport SerialPort)
endif()

# ---- 源文件 ----
set(SOURCES
    main.cpp

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
    #lib
    lib/qcustomplot/qcustomplot.cpp
    lib/qcustomplot/qcustomplot.h
    # qrc
    qrc/main.qrc
)

# ---- 可执行文件 ----
add_executable(${PROJECT_NAME} ${SOURCES}
    global.h
    head.h
    databuffer.h databuffer.cpp
    serialworker.h serialworker.cpp
    filewriter.h filewriter.cpp
    filereader.h filereader.cpp
    filebuffer.h filebuffer.cpp
    logger.h logger.cpp
    serialpainter.h serialpainter.cpp
    filepainter.h filepainter.cpp

)


target_link_libraries(comtool PRIVATE Qt6::Widgets)
target_link_libraries(comtool PRIVATE Qt6::Core)
target_link_libraries(comtool PRIVATE Qt6::Core)
target_link_libraries(comtool PRIVATE Qt6::Core)
# ---- 头文件路径 ----
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/form
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/qcustomplot
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
        Qt6::PrintSupport
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::SerialPort
        Qt5::PrintSupport
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
