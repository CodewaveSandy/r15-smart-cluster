cmake_minimum_required(VERSION 3.21)
project(R15SmartCluster VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.5)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8)
    qt_policy(SET QTP0004 NEW)
endif()

find_package(Threads REQUIRED)
qt_add_library(cluster_core STATIC
    domain/BikeState.h
    domain/BikeState.cpp
    domain/BikeSnapshot.h
    domain/RideComputer.h
    domain/RideComputer.cpp
    data/IBikeDataSource.h
    data/simulator/SimulatorBikeDataSource.h
    data/simulator/SimulatorBikeDataSource.cpp
    settings/DisplaySettings.h
    settings/DisplaySettings.cpp
    diagnostics/DiagnosticLog.h
    diagnostics/DiagnosticLog.cpp
    application/AppDiagnostics.h
    application/AppDiagnostics.cpp
)
target_include_directories(cluster_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cluster_core PUBLIC Qt6::Core Threads::Threads)
qt_add_executable(r15-cluster MACOSX_BUNDLE app/main.cpp app/Fonts.h)
qt_add_qml_module(r15-cluster
    URI R15Cluster
    VERSION 1.0
    QML_FILES
        ui/Main.qml
        ui/ErrorScreen.qml
        ui/components/ClusterHeader.qml
        ui/components/ClusterIcon.qml
        ui/components/ClusterText.qml
        ui/components/FuelGauge.qml
        ui/components/Tachometer.qml
        ui/components/TellTale.qml
        ui/input/DesktopInput.qml
        ui/screens/ClockScreen.qml
        ui/screens/DashboardScreen.qml
        ui/screens/HelpScreen.qml
        ui/screens/SettingsScreen.qml
        ui/screens/VehicleInfoScreen.qml
        ui/theme/Theme.qml
)
target_link_libraries(r15-cluster PRIVATE cluster_core Qt6::Quick)
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
    qt_add_executable(state-check tests/state-check.cpp)
    target_link_libraries(state-check PRIVATE cluster_core)
    add_test(NAME simulator-state COMMAND state-check)
    qt_add_executable(boundary-check tests/boundary-check.cpp)
    target_link_libraries(boundary-check PRIVATE cluster_core Qt6::Test)
    add_test(NAME failure-boundaries COMMAND boundary-check)
    qt_add_executable(ui-check tests/ui-check.cpp)
    target_compile_definitions(ui-check PRIVATE PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(ui-check PRIVATE cluster_core Qt6::Quick Qt6::Test)
    bundle_cluster_fonts(ui-check)
    add_test(NAME cockpit-ui COMMAND ui-check)
    set_tests_properties(cockpit-ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
    add_test(NAME startup-smoke COMMAND r15-cluster --smoke-test --log-directory "${CMAKE_CURRENT_BINARY_DIR}/testlogs/startup")
    add_test(NAME startup-fallback COMMAND r15-cluster --smoke-test --test-startup-failure --log-directory "${CMAKE_CURRENT_BINARY_DIR}/testlogs/fallback")
    set_tests_properties(startup-fallback PROPERTIES WILL_FAIL TRUE)
    set_tests_properties(simulator-state failure-boundaries startup-smoke startup-fallback PROPERTIES TIMEOUT 15)
    set_tests_properties(cockpit-ui PROPERTIES TIMEOUT 45)
    set_tests_properties(startup-smoke startup-fallback PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endif()
