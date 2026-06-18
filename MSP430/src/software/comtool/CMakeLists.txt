cmake_minimum_required(VERSION 3.16)
project(comtool LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC_MOC_OPTIONS "-Muri=Qt6Compat")

# 查找所有需要的 Qt6 组件（一次性查找，避免重复）
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    PrintSupport
    SerialPort
    Core5Compat
)

# ==== 源文件、头文件、资源文件 ====
set(SOURCES
    main.cpp

    # api
    api/appconfig.cpp
    api/appdata.cpp
    api/qthelper.cpp
    api/qthelperdata.cpp

    # form
    form/frmcomtool.cpp

    # lib
    lib/qcustomplot/qcustomplot.cpp

    # logger
    logger.cpp

    # util
    util/frameparser.cpp
    util/framedispatcher.cpp
    util/framefactory.cpp

    # core
    core/commander.cpp
    core/datamanager.cpp
    core/databuffer.cpp

    # transport
    transport/serialtransport.cpp
    transport/filetransport.cpp

    # view
    view/datavisualizer.cpp

    # storage
    storage/csvwriter.cpp
    storage/csvreader.cpp
    storage/filemanager.cpp
)

set(HEADERS
    # api
    api/appconfig.h
    api/appdata.h
    api/qthelper.h
    api/qthelperdata.h

    # form
    form/frmcomtool.h
    form/frmcomtool.ui          # .ui 文件也放在这里便于 IDE 显示

    # lib
    lib/qcustomplot/qcustomplot.h

    # global
    global.h
    head.h

    # logger
    logger.h

    # entity
    entity/baseframe.h
    entity/frametype.h
    entity/datavoltageframe.h
    entity/datatempframe.h
    entity/databothframe.h
    entity/ansframe.h
    entity/errortype.h

    # util
    util/frameparser.h
    util/framedispatcher.h
    util/framefactory.h

    # core
    core/commander.h
    core/datamanager.h
    core/databuffer.h

    # transport
    transport/itransport.h
    transport/serialtransport.h
    transport/filetransport.h

    # view
    view/chartconfig.h
    view/datavisualizer.h

    # storage
    storage/csvwriter.h
    storage/csvreader.h
    storage/filemanager.h
    storage/istorage.h
)

set(RESOURCES
    qrc/main.qrc
)

# ==== 创建可执行文件 ====
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
    component/clickablebutton.h component/clickablebutton.cpp
)

# ==== 链接 Qt 库（一次性链接所有需要的模块）====
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::PrintSupport
    Qt6::SerialPort
    Qt6::Core5Compat
)

# ==== 头文件包含路径 ====
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/form
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/qcustomplot
    ${CMAKE_CURRENT_SOURCE_DIR}/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)

# ==== 输出路径 ====
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../comtool-2.4.1"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../lib"
)

# ==== Windows 资源文件（图标等）====
if(WIN32)
    set(APP_RC "${CMAKE_SOURCE_DIR}/qrc/main.rc")
    if(EXISTS ${APP_RC})
        target_sources(${PROJECT_NAME} PRIVATE ${APP_RC})
    endif()
endif()

# ==== 禁用所有警告（根据需要可调整）====
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /w)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -w)
endif()

# ==== 构建后复制翻译文件（qm）====
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/qrc/qm"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/qrc/qm"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/qrc/qm"
    COMMENT "Copying translation files (qm) to output directory..."
)
