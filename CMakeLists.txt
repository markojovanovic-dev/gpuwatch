cmake_minimum_required(VERSION 3.20)
project(gpuwatch VERSION 1.1.3 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
set(GPUWATCH_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}/gpuwatch")

include(FetchContent)

FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG        v5.0.0
)
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

find_path(NVML_INCLUDE_DIR
    NAMES nvml.h
    PATHS
        /usr/local/cuda/include
        /usr/include
        /opt/cuda/include
        $ENV{CUDA_HOME}/include
        $ENV{CUDA_PATH}/include
)

find_library(NVML_LIBRARY
    NAMES nvidia-ml
    PATHS
        /usr/lib/x86_64-linux-gnu
        /usr/lib64
        /usr/local/cuda/lib64
        /usr/local/cuda/lib64/stubs
        $ENV{CUDA_HOME}/lib64
        $ENV{CUDA_HOME}/lib64/stubs
)

if(NOT NVML_INCLUDE_DIR)
    message(FATAL_ERROR
        "NVML header (nvml.h) not found. Install the CUDA toolkit:\n"
        "  sudo apt install nvidia-cuda-toolkit\n"
        "Or set CUDA_HOME to your CUDA installation directory."
    )
endif()

if(NOT NVML_LIBRARY)
    message(FATAL_ERROR
        "NVML library (libnvidia-ml.so) not found. Install NVIDIA drivers:\n"
        "  sudo apt install nvidia-driver-xxx\n"
        "Or use the stub library from the CUDA toolkit."
    )
endif()

message(STATUS "NVML include: ${NVML_INCLUDE_DIR}")
message(STATUS "NVML library: ${NVML_LIBRARY}")

add_executable(gpuwatch
    src/main.cpp
    src/nvml_monitor.cpp
    src/gpu_database.cpp
    src/tui.cpp
)

target_compile_definitions(gpuwatch PRIVATE
    GPUWATCH_VERSION="${PROJECT_VERSION}"
    GPUWATCH_DATADIR="${GPUWATCH_DATADIR}"
)

target_include_directories(gpuwatch PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${NVML_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(gpuwatch PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
    ${NVML_LIBRARY}
    ${SQLITE3_LIBRARIES}
    pthread
)

install(TARGETS gpuwatch DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/gpu_specs.db DESTINATION ${GPUWATCH_DATADIR})

add_custom_command(TARGET gpuwatch POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/data/gpu_specs.db
        $<TARGET_FILE_DIR:gpuwatch>/gpu_specs.db
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "gpuwatch")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Terminal-based NVIDIA GPU monitoring tool")
set(CPACK_PACKAGE_CONTACT "markojovanovic-dev")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsqlite3-0, nvidia-driver-535 | nvidia-driver-545 | nvidia-driver-550 | nvidia-driver-560 | nvidia-driver-570 | nvidia-driver-580 | nvidia-driver-590")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
include(CPack)
