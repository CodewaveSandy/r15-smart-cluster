cmake_minimum_required(VERSION 3.21)
project(R15SmartCluster VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.5)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(r15-cluster MACOSX_BUNDLE app/main.cpp app/BikeState.h)
qt_add_qml_module(r15-cluster
    URI R15Cluster
    VERSION 1.0
    QML_FILES ui/Main.qml
)
target_link_libraries(r15-cluster PRIVATE Qt6::Quick)
set_target_properties(r15-cluster PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "R15 Smart Cluster"
    MACOSX_BUNDLE_GUI_IDENTIFIER "dev.r15smartcluster.desktop"
)

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(state-check tests/state-check.cpp app/BikeState.h)
    target_include_directories(state-check PRIVATE app)
    target_link_libraries(state-check PRIVATE Qt6::Core)
    add_test(NAME simulator-state COMMAND state-check)
endif()
