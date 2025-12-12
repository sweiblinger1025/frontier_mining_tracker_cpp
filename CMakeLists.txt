# ==============================================================================
# FrontierSuite - Frontier Mining Tracker (C++/Qt6)
# ==============================================================================
# A comprehensive desktop application for tracking mining operations in
# "Out of Ore" with financial ledgering, skill progression, and dynamic
# storytelling capabilities.
#
# Build Instructions:
#   mkdir build && cd build
#   cmake ..
#   cmake --build .
#
# Requirements:
#   - CMake 3.16+
#   - Qt6 (Widgets, Sql)
#   - C++17 compiler
# ==============================================================================
cmake_minimum_required(VERSION 3.16)

# ------------------------------------------------------------------------------
# Project Definition
# ------------------------------------------------------------------------------
project(frontier_mining_tracker_cpp
    VERSION 0.1.0
    DESCRIPTION "Mining operations tracker for Out of Ore"
    LANGUAGES CXX
)

# ------------------------------------------------------------------------------
# C++ Standard Configuration
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Use -std=c++17 not -std=gnu++17

# ------------------------------------------------------------------------------
# Qt6 Auto-Processing
# ------------------------------------------------------------------------------
# AUTOMOC: Automatically process Q_OBJECT macros (signals/slots)
# AUTORCC: Automatically compile .qrc resource files
# AUTOUIC: Automatically compile .ui designer files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Core)

# ------------------------------------------------------------------------------
# Source Files
# ------------------------------------------------------------------------------
set(SOURCES
    # Core
    src/main.cpp
    src/core/database.cpp
    src/core/itemimporter.cpp
    src/core/vehicleimporter.cpp
    src/core/recipeimporter.cpp
    src/core/locationimporter.cpp

    # Core - Operations
    src/core/operationsmanager.cpp

    # UI - Main
    src/ui/mainwindow.cpp

    # UI - Tabs
    src/ui/dashboardwidget.cpp
    src/ui/financewidget.cpp
    src/ui/datahubwidget.cpp
    src/ui/auditorwidget.cpp

    # UI - Operations Tab
    src/ui/operationswidget.cpp
    src/ui/equipmentoperationstab.cpp
    src/ui/operationssettingstab.cpp
    src/ui/fuellogwidget.cpp
    src/ui/materialmovementtab.cpp
    src/ui/inventorytab.cpp
    src/ui/costanalysistab.cpp
    src/ui/productiontab.cpp
    src/ui/productioncalculatortab.cpp
    src/ui/productionlogtab.cpp

    # UI - Data Hub Subtabs
    src/ui/vehiclespecstab.cpp
    src/ui/recipestab.cpp
    src/ui/locationstab.cpp

    # UI - Dialogs
    src/ui/additemdialog.cpp
    src/ui/addtransactiondialog.cpp
)

set(HEADERS
    # Core
    src/core/types.h
    src/core/database.h
    src/core/itemimporter.h
    src/core/vehicleimporter.h
    src/core/recipeimporter.h
    src/core/locationimporter.h

    # Core - Operations
    src/core/operationsmanager.h
    src/core/unitconverter.h

    # UI - Main
    src/ui/mainwindow.h

    # UI - Tabs
    src/ui/dashboardwidget.h
    src/ui/financewidget.h
    src/ui/datahubwidget.h
    src/ui/auditorwidget.h

    # UI - Operations Tab
    src/ui/operationswidget.h
    src/ui/equipmentoperationstab.h
    src/ui/operationssettingstab.h
    src/ui/fuellogwidget.h
    src/ui/materialmovementtab.h
    src/ui/inventorytab.h
    src/ui/costanalysistab.h
    src/ui/productiontab.h
    src/ui/productioncalculatortab.h
    src/ui/productionlogtab.h

    # UI - Data Hub Subtabs
    src/ui/vehiclespecstab.h
    src/ui/recipestab.h
    src/ui/locationstab.h

    # UI - Dialogs
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
    src/ui/shiftlogtab.h src/ui/shiftlogtab.cpp
    src/ui/cycletimetab.h src/ui/cycletimetab.cpp



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
    Qt6::Core
)
