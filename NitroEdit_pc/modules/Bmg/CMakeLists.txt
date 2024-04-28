cmake_minimum_required(VERSION 3.14)

project(Bmg VERSION $ENV{NEDIT_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Xml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Xml)
find_library(LIBNEDIT_LIB NAMES nedit libnedit HINTS ../../libnedit/build)

set(LIBNEDIT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../libnedit)
set(MODULES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_compile_definitions(NEDIT_MAJOR=$ENV{NEDIT_MAJOR})
add_compile_definitions(NEDIT_MINOR=$ENV{NEDIT_MINOR})
add_compile_definitions(NEDIT_MICRO=$ENV{NEDIT_MICRO})
add_compile_definitions(NEDIT_BUGFIX=$ENV{NEDIT_BUGFIX})
add_compile_definitions(NEDIT_VERSION="$ENV{NEDIT_VERSION}")

include_directories(include ${MODULES_ROOT}/include ${LIBNEDIT_ROOT}/include)

set(SOURCES
    source/base_Include.cpp
    source/mod.cpp
    source/ui/ui_BmgSubWindow.cpp
    source/ui/ui_BmgMessageSubWindow.cpp
)

set(QOBJ_SOURCES
    include/ui/ui_BmgSubWindow.hpp source/ui/ui_BmgSubWindow.ui
    include/ui/ui_BmgMessageSubWindow.hpp source/ui/ui_BmgMessageSubWindow.ui
)

add_library(Bmg SHARED
    ${SOURCES}
    ${QOBJ_SOURCES}
)

target_link_libraries(Bmg PRIVATE ${LIBNEDIT_LIB})
target_link_libraries(Bmg PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

add_custom_command(TARGET Bmg POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:Bmg>
                    $ENV{NEDIT_OUTPUT_MODULES_PATH}
                    COMMENT "Copying NitroEdit shared library module to output directory...")
