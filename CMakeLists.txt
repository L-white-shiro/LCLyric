cmake_minimum_required(VERSION 3.16)

project(untitled1 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(untitled1
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET untitled1 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(untitled1 SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(untitled1
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(untitled1 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.untitled1)
endif()
set_target_properties(untitled1 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS untitled1
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(untitled1)
endif()

# ---- 构建后自动部署运行依赖（调试 CRT + Qt DLL），避免运行/双击时报缺 DLL ----
# ------------------------------------------------------------------
# 构建后自动部署运行依赖（MSVC Debug 运行所需）
#   由本机环境自动注入，可安全删除
# ------------------------------------------------------------------
if(MSVC)
    set(_DEPLOY_CRT_DIR "D:/Software/CS/Microsoft Visual Studio/18/Community/VC/Redist/MSVC/14.51.36231/debug_nonredist/x64/Microsoft.VC145.DebugCRT")
    if(EXISTS "${_DEPLOY_CRT_DIR}")
        file(GLOB _DEPLOY_CRT_DLLS "${_DEPLOY_CRT_DIR}/*.dll")
        foreach(_dll ${_DEPLOY_CRT_DLLS})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_dll}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
        endforeach()
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/ucrt/ucrtbased.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endif()
    # 目标实际依赖的 Qt DLL（需要 CMake >= 3.21）
    # 注意：生成表达式展开后是分号列表，直接放在 COMMAND 里会被整体加引号而失败，
    #       因此先生成一个脚本，用 -P 执行
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
        file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qt_deploy_$<CONFIG>.cmake" CONTENT "set(_dlls \"$<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>\")\nif(_dlls)\n    file(COPY \${_dlls} DESTINATION \"$<TARGET_FILE_DIR:${PROJECT_NAME}>\")\nendif()")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -P
                    "${CMAKE_CURRENT_BINARY_DIR}/qt_deploy_$<CONFIG>.cmake")
    endif()
    # GUI 程序需要的平台插件
    if(EXISTS "D:/Software/CS/QT/6.12.0/msvc2022_64/plugins/platforms/qwindowsd.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "D:/Software/CS/QT/6.12.0/msvc2022_64/plugins/platforms/qwindowsd.dll"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/qwindowsd.dll")
    endif()
endif()





