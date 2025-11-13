# 构建QCustomPlot（需要QT）
set(QCUSTOMPLOT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/qcustomplot/src/qcustomplot.cpp
)

set(QCUSTOMPLOT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/qcustomplot/include/qcustomplot.h
)

add_library(qcustomplot STATIC ${QCUSTOMPLOT_SOURCES} ${QCUSTOMPLOT_HEADERS})

target_include_directories(qcustomplot PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/qcustomplot/include
)

# QCustomPlot需要链接QT
set(CMAKE_PREFIX_PATH "D:/QT/5.12.9/mingw73_64/lib/cmake")
target_link_libraries(qcustomplot Qt5::Core Qt5::Widgets)

# JSON库（纯C++）
add_library(jsoncpp INTERFACE)
target_include_directories(jsoncpp INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/json/include
)