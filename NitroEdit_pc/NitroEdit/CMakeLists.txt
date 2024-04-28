cmake_minimum_required(VERSION 3.5)

project(NitroEdit VERSION $ENV{NEDIT_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_library(LIBNEDIT_LIB NAMES nedit libnedit HINTS ../libnedit/build)

set(LIBNEDIT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../libnedit)
set(MODULES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../modules)

add_compile_definitions(NEDIT_MAJOR=$ENV{NEDIT_MAJOR})
add_compile_definitions(NEDIT_MINOR=$ENV{NEDIT_MINOR})
add_compile_definitions(NEDIT_MICRO=$ENV{NEDIT_MICRO})
add_compile_definitions(NEDIT_BUGFIX=$ENV{NEDIT_BUGFIX})
add_compile_definitions(NEDIT_MODULES_DIR="$ENV{NEDIT_MODULES_DIR}")
add_compile_definitions(NEDIT_VERSION="$ENV{NEDIT_VERSION}")

include_directories(include ${MODULES_ROOT}/include ${LIBNEDIT_ROOT}/include)

set(SOURCES
    source/main.cpp
    source/base_Include.cpp
    source/mod/mod_Loader.cpp
    source/ui/ui_MainWindow.cpp
)

set(QOBJ_SOURCES
    include/ui/ui_MainWindow.hpp source/ui/ui_MainWindow.ui
)

set(PROJECT_SOURCES
    ${SOURCES}
    ${QOBJ_SOURCES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(NitroEdit
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET NitroEdit APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(NitroEdit SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(NitroEdit
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(NitroEdit PRIVATE ${LIBNEDIT_LIB})
target_link_libraries(NitroEdit PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

add_custom_command(TARGET NitroEdit POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:NitroEdit>
                    $ENV{NEDIT_OUTPUT_PATH}
                    COMMENT "Copying executable to output directory...")

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.NitroEdit)
endif()
set_target_properties(NitroEdit PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS NitroEdit
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(NitroEdit)
endif()
