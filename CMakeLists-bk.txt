cmake_minimum_required(VERSION 3.10)
project(vive_robot_calibration)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 所需
set(CMAKE_PREFIX_PATH "D:/QT/5.12.9/mingw73_64/lib/cmake")
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 头文件路径
include_directories(
    # ${PROJECT_SOURCE_DIR}/include
    # ${PROJECT_SOURCE_DIR}/src/configfile
    ${PROJECT_SOURCE_DIR}/src/algorithm
    # ${PROJECT_SOURCE_DIR}/src/utils
    # ${PROJECT_SOURCE_DIR}/src/communication
    # ${PROJECT_SOURCE_DIR}/lib/jsoncpp/include
)

# 添加子模块（编译为静态库）
add_subdirectory(src/algorithm)
# add_subdirectory(src/communication)
# add_subdirectory(src/utils)
# add_subdirectory(src/configfile)
add_subdirectory(src/ui)


# 添加主程序
add_executable(data_parsing_tool src/main.cpp)

# 链接各模块静态库
target_link_libraries(data_parsing_tool
    PRIVATE
        algorithm
        # communication
        # htc_vive
        # utils
        # configfile
        ui
)