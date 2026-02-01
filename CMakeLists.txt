cmake_minimum_required(VERSION 3.20)
project(Arma3PAAConverter VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    message(STATUS "Building for Windows")

    # Set vcpkg toolchain if available
    if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
    endif()

    # Find packages via vcpkg
    find_package(Boost REQUIRED)
    find_package(unofficial-libsquish CONFIG REQUIRED)
    find_package(PNG REQUIRED)
    find_package(imgui CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glad CONFIG REQUIRED)
    find_package(Stb REQUIRED)
    find_path(PORTABLE_FILE_DIALOGS_INCLUDE_DIRS "portable-file-dialogs.h")

    set(SQUISH_TARGET unofficial::libsquish::squish)
    set(GLAD_TARGET glad::glad)
    set(IMGUI_TARGET imgui::imgui)
    set(STB_INCLUDE ${Stb_INCLUDE_DIR})
    set(PFD_INCLUDE ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS})
    set(USE_VCPKG TRUE)

else()
    message(STATUS "Building for Linux")

    # Find packages - Linux system packages
    find_package(Boost REQUIRED)
    find_package(PNG REQUIRED)
    find_package(glfw3 REQUIRED)

    # libsquish - no cmake config on Arch, link directly
    find_path(SQUISH_INCLUDE_DIR squish.h)
    find_library(SQUISH_LIBRARY squish)
    if(NOT SQUISH_INCLUDE_DIR OR NOT SQUISH_LIBRARY)
        message(FATAL_ERROR "libsquish not found. Install with: pacman -S libsquish")
    endif()

    # stb - header-only (Arch puts them in stb/ subdirectory)
    find_path(STB_INCLUDE_DIR stb_image.h PATHS /usr/include/stb)
    if(NOT STB_INCLUDE_DIR)
        message(FATAL_ERROR "stb not found. Install with: pacman -S stb")
    endif()

    set(SQUISH_TARGET ${SQUISH_LIBRARY})
    set(STB_INCLUDE ${STB_INCLUDE_DIR})
    set(PFD_INCLUDE "${CMAKE_SOURCE_DIR}/include")
    set(USE_VCPKG FALSE)

    # For Linux GUI, use bundled imgui/glad from arma-workbench or local third_party
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/imgui/imgui.h")
        set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")
        set(GLAD_DIR "${CMAKE_SOURCE_DIR}/third_party/glad")
    elseif(EXISTS "$ENV{HOME}/Documents/arma-workbench/third_party/imgui/imgui.h")
        set(IMGUI_DIR "$ENV{HOME}/Documents/arma-workbench/third_party/imgui")
        set(GLAD_DIR "$ENV{HOME}/Documents/arma-workbench/third_party/glad")
    else()
        message(WARNING "imgui/glad not found - GUI will not be built")
        set(BUILD_GUI FALSE)
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
)

if(NOT USE_VCPKG)
    include_directories(${SQUISH_INCLUDE_DIR} ${STB_INCLUDE})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/paa.cpp
    src/image_loader.cpp
)

set(HEADERS
    include/paa.h
    include/image_loader.h
    include/utils.h
)

#=============================================================================
# CLI Executable
#=============================================================================
add_executable(arma3-paa-cli ${SOURCES} ${HEADERS})

target_link_libraries(arma3-paa-cli PRIVATE
    ${SQUISH_TARGET}
    PNG::PNG
    Boost::boost
)

if(USE_VCPKG)
    target_include_directories(arma3-paa-cli PRIVATE ${STB_INCLUDE})
endif()

#=============================================================================
# GUI Application
#=============================================================================
if(NOT DEFINED BUILD_GUI)
    set(BUILD_GUI TRUE)
endif()

if(BUILD_GUI)
    if(USE_VCPKG)
        # Windows: vcpkg provides everything
        set(GUI_SOURCES
            src/gui_main.cpp
            src/paa.cpp
            src/image_loader.cpp
        )

        add_executable(arma3-paa-gui ${GUI_SOURCES})

        target_link_libraries(arma3-paa-gui PRIVATE
            ${SQUISH_TARGET}
            PNG::PNG
            Boost::boost
            ${IMGUI_TARGET}
            glfw
            ${GLAD_TARGET}
        )

        target_include_directories(arma3-paa-gui PRIVATE
            ${STB_INCLUDE}
            ${PFD_INCLUDE}
        )
    else()
        # Linux: bundle imgui/glad sources
        set(GUI_SOURCES
            src/gui_main.cpp
            src/paa.cpp
            src/image_loader.cpp
            # ImGui core
            ${IMGUI_DIR}/imgui.cpp
            ${IMGUI_DIR}/imgui_draw.cpp
            ${IMGUI_DIR}/imgui_tables.cpp
            ${IMGUI_DIR}/imgui_widgets.cpp
            # ImGui backends
            ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
            ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
            # GLAD
            ${GLAD_DIR}/src/glad.c
        )

        add_executable(arma3-paa-gui ${GUI_SOURCES})

        target_include_directories(arma3-paa-gui PRIVATE
            ${IMGUI_DIR}
            ${IMGUI_DIR}/backends
            ${GLAD_DIR}/include
            ${PFD_INCLUDE}
            ${STB_INCLUDE}
        )

        target_link_libraries(arma3-paa-gui PRIVATE
            ${SQUISH_TARGET}
            PNG::PNG
            Boost::boost
            glfw
            ${CMAKE_DL_LIBS}
        )
    endif()

    install(TARGETS arma3-paa-gui DESTINATION bin)
endif()

# Installation
install(TARGETS arma3-paa-cli DESTINATION bin)

message(STATUS "Configuration:")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  SQUISH: ${SQUISH_TARGET}")
message(STATUS "  STB: ${STB_INCLUDE}")
message(STATUS "  Build GUI: ${BUILD_GUI}")
