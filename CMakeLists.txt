cmake_minimum_required(VERSION 3.21)
project(R15SmartCluster VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.5)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(r15-cluster MACOSX_BUNDLE app/main.cpp app/BikeState.h app/DisplaySettings.h app/RideComputer.h)
qt_add_qml_module(r15-cluster
    URI R15Cluster
    VERSION 1.0
    QML_FILES ui/Main.qml
)
target_link_libraries(r15-cluster PRIVATE Qt6::Quick)
function(bundle_cluster_fonts target)
    qt_add_resources(${target} cluster_fonts
        PREFIX "/fonts"
        BASE "assets/fonts/bricolage"
        FILES
            assets/fonts/bricolage/BricolageGrotesque-Regular.ttf
            assets/fonts/bricolage/BricolageGrotesque-SemiBold.ttf
            assets/fonts/bricolage/BricolageGrotesque-Bold.ttf
            assets/fonts/bricolage/OFL.txt
    )
endfunction()
bundle_cluster_fonts(r15-cluster)
set_target_properties(r15-cluster PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "R15 Smart Cluster"
    MACOSX_BUNDLE_GUI_IDENTIFIER "dev.r15smartcluster.desktop"
)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(state-check tests/state-check.cpp app/BikeState.h)
    target_include_directories(state-check PRIVATE app)
    target_link_libraries(state-check PRIVATE Qt6::Core)
    add_test(NAME simulator-state COMMAND state-check)
    qt_add_executable(ui-check tests/ui-check.cpp app/BikeState.h app/DisplaySettings.h app/RideComputer.h)
    target_include_directories(ui-check PRIVATE app)
    target_compile_definitions(ui-check PRIVATE PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(ui-check PRIVATE Qt6::Quick Qt6::Test)
    bundle_cluster_fonts(ui-check)
    add_test(NAME cockpit-ui COMMAND ui-check)
    set_tests_properties(cockpit-ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endif()
