cmake_minimum_required(VERSION 3.10)
project(DataAnalysisTool VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 仅对UI模块使用QT
set(CMAKE_PREFIX_PATH "D:/QT/5.12.9/mingw73_64/lib/cmake")
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 包含目录 - 纯C++模块
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdparty/qcustomplot/include
    ${CMAKE_SOURCE_DIR}/thirdparty/json/include
)

# 纯C++源文件（不包含UI相关）
file(GLOB_RECURSE CORE_SOURCES 
    "src/data/*.cpp"
    "src/plugins/*.cpp" 
    "src/utils/*.cpp"
)

# UI相关源文件
file(GLOB_RECURSE UI_SOURCES 
    "src/core/*.cpp"
    "src/visualization/*.cpp"
)

file(GLOB_RECURSE HEADERS "include/*.h")

# 添加第三方库
add_subdirectory(thirdparty)

# 先创建纯C++核心库（不依赖QT）
add_library(CoreLibrary STATIC ${CORE_SOURCES})

# 设置纯C++库的属性
target_include_directories(CoreLibrary PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(CoreLibrary PUBLIC CORE_LIBRARY)

# 生成MOC文件（仅UI相关）
qt5_wrap_cpp(MOC_SOURCES 
    include/core/MainWindow.h
    include/visualization/PlotWidget.h
)

# 创建可执行文件（链接核心库和UI）
add_executable(${PROJECT_NAME} 
    main.cpp
    ${UI_SOURCES}
    ${MOC_SOURCES}
    ${HEADERS}
)

# 链接库
target_link_libraries(${PROJECT_NAME} 
    CoreLibrary
    qcustomplot
    Qt5::Core 
    Qt5::Widgets
)

# 为纯C++模块移除QT依赖
target_compile_definitions(CoreLibrary PUBLIC -DNO_QT_DEPENDENCIES)