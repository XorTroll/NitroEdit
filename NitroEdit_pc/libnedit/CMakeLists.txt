cmake_minimum_required(VERSION 3.14)

project(libnedit LANGUAGES CXX)

# Project to build libnedit as a shared library, to be used dynamically by the base PC program and the modules

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBNEDIT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../libnedit)

include_directories(include ${LIBNEDIT_ROOT}/include)

set(SOURCES
    ${LIBNEDIT_ROOT}/source/ntr/fmt/nfs/nfs_NitroFs.cpp

    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_BMG.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_NARC.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_NCGR.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_NCLR.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_NSCR.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_ROM.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_SBNK.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_SDAT.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_SSEQ.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_STRM.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_SWAR.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fmt/fmt_Utility.cpp

    ${LIBNEDIT_ROOT}/source/ntr/fs/fs_Base.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fs/fs_BinaryFile.cpp
    ${LIBNEDIT_ROOT}/source/ntr/fs/fs_Stdio.cpp

    ${LIBNEDIT_ROOT}/source/ntr/gfx/gfx_Conversion.cpp

    ${LIBNEDIT_ROOT}/source/ntr/util/util_Compression.cpp
    ${LIBNEDIT_ROOT}/source/ntr/util/util_String.cpp
)

add_library(libnedit SHARED
    ${SOURCES}
)

set_target_properties(libnedit PROPERTIES PREFIX "")

add_custom_command(TARGET libnedit POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:libnedit>
                    $ENV{NEDIT_OUTPUT_PATH}
                    COMMENT "Copying shared library to output directory...")
