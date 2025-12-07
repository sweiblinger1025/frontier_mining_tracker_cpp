cmake_minimum_required(VERSION 3.16)
project(frontier_mining_tracker_cpp LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt6 REQUIRED COMPONENTS Core)

set(SOURCES
    src/main.cpp
    src/ui/mainwindow.cpp
    src/ui/dashboardwidget.cpp
    src/ui/financewidget.cpp
    src/ui/operationswidget.cpp
    src/ui/datahubwidget.cpp
    src/ui/auditorwidget.cpp
    src/core/database.cpp
    src/ui/additemdialog.cpp
    src/ui/addtransactiondialog.cpp

)

set(HEADERS
    src/ui/mainwindow.h
    src/ui/dashboardwidget.h
    src/ui/financewidget.h
    src/ui/operationswidget.h
    src/ui/datahubwidget.h
    src/ui/auditorwidget.h
    src/core/types.h
    src/core/database.h
    src/ui/additemdialog.h
    src/ui/addtransactiondialog.h
)

set(UIS
    ui/mainwindow.ui
)
set(RESOURCES
    resources/icons.qrc
)

qt_add_executable(frontier_mining_tracker_cpp
    ${SOURCES}
    ${HEADERS}
    ${UIS}
    ${RESOURCES}




)

set_target_properties(frontier_mining_tracker_cpp PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ui"
)

target_include_directories(frontier_mining_tracker_cpp PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ui
)

target_link_libraries(frontier_mining_tracker_cpp PRIVATE
    Qt6::Widgets
    Qt6::Sql
)
target_link_libraries(frontier_mining_tracker_cpp PRIVATE Qt6::Core)
