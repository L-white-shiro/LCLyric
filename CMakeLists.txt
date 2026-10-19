cmake_minimum_required(VERSION 3.16)

project(untitled1 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
        main.cpp



)

add_executable(smtc_probe smtc_probe.cpp)
target_compile_features(smtc_probe PRIVATE cxx_std_20)
target_link_libraries(smtc_probe PRIVATE windowsapp)
target_compile_options(smtc_probe PRIVATE /utf-8)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(untitled1
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        main.h
        Lyriclabel.h
    )
else()
    if(ANDROID)
        add_library(untitled1 SHARED ${PROJECT_SOURCES})
    else()
        add_executable(untitled1 ${PROJECT_SOURCES})
    endif()
endif()

target_link_libraries(untitled1 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    windowsapp
)

set_target_properties(untitled1 PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_compile_features(untitled1 PRIVATE cxx_std_20)
target_compile_options(untitled1 PRIVATE /utf-8)

include(GNUInstallDirs)
install(TARGETS untitled1
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(untitled1)
endif()