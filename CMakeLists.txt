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

# ------------------------------------------------------------------------------
# Find Qt6 Components
# ------------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Sql
)

# ------------------------------------------------------------------------------
# Source Files
# ------------------------------------------------------------------------------
set(SOURCES
    # Core
    src/main.cpp
    src/core/database.cpp
    src/core/itemimporter.cpp
    src/core/vehicleimporter.cpp
    src/core/operationsmanager.cpp

    # UI - Main
    src/ui/mainwindow.cpp

    # UI - Tabs
    src/ui/dashboardwidget.cpp
    src/ui/financewidget.cpp
    src/ui/datahubwidget.cpp
    src/ui/auditorwidget.cpp

    # UI - Data Hub Subtabs
    src/ui/vehiclespecstab.cpp

    # UI - Operations Tab
    src/ui/operationswidget.cpp
    src/ui/equipmentoperationstab.cpp
    src/ui/operationssettingstab.cpp
    src/ui/fuellogwidget.cpp
    src/ui/materialmovementtab.cpp

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
    src/core/operationsmanager.h
    src/core/unitconverter.h

    # UI - Main
    src/ui/mainwindow.h

    # UI - Tabs
    src/ui/dashboardwidget.h
    src/ui/financewidget.h
    src/ui/datahubwidget.h
    src/ui/auditorwidget.h

    # UI - Data Hub Subtabs
    src/ui/vehiclespecstab.h

    # UI - Operations Tab
    src/ui/operationswidget.h
    src/ui/equipmentoperationstab.h
    src/ui/operationssettingstab.h
    src/ui/fuellogwidget.h
    src/ui/materialmovementtab.h

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

# ------------------------------------------------------------------------------
# Create Executable
# ------------------------------------------------------------------------------
qt_add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UIS}
    ${RESOURCES}
)

# ------------------------------------------------------------------------------
# Target Configuration
# ------------------------------------------------------------------------------
# Tell AUTOUIC where to find .ui files
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ui"
)

# Include directories for #include statements
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ui
)

# Link Qt6 libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
)

# ------------------------------------------------------------------------------
# Copy Data Files to Build Directory
# ------------------------------------------------------------------------------
# This copies the data folder (containing vehicles.json, etc.) to the build
# directory so the application can find them at runtime.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/data"
    COMMENT "Copying data folder to build directory..."
)

# ------------------------------------------------------------------------------
# Installation (Optional - for future use)
# ------------------------------------------------------------------------------
# install(TARGETS ${PROJECT_NAME}
#     BUNDLE DESTINATION .
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# )
# install(DIRECTORY data/ DESTINATION data)
