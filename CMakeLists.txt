cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(A3Texture VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    # windows.h defines min and max as macros, which breaks std::min and
    # std::max wherever it is pulled in.
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(MSVC)
    # Sources are UTF-8; without this MSVC reads them as the system code page.
    add_compile_options(/utf-8)
endif()

find_package(unofficial-libsquish CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_path(PORTABLE_FILE_DIALOGS_INCLUDE_DIRS "portable-file-dialogs.h")
find_path(LZO_INCLUDE_DIR "lzo/lzo1x.h" REQUIRED)
find_library(LZO_LIBRARY NAMES lzo2 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/paa.cpp
    src/image_loader.cpp
    src/channel_packer.cpp
    src/texture_role.cpp
    src/job_runner.cpp
    src/p3d_reader.cpp
    src/rvmat_writer.cpp
)

set(HEADERS
    include/paa.h
    include/image_loader.h
    include/channel_packer.h
    include/texture_role.h
    include/job_runner.h
    include/p3d_reader.h
    include/rvmat_writer.h
    include/rvmat_shaders.h
    include/viewer_3d.h
    include/model_renderer.h
    include/paa_texture.h
    include/rvmat_parser.h
    include/binary_reader.h
    include/utils.h
)

#=============================================================================
# CLI Executable
#=============================================================================
add_executable(a3texture-cli ${SOURCES} ${HEADERS})

target_link_libraries(a3texture-cli PRIVATE
    unofficial::libsquish::squish
    PNG::PNG
    TIFF::TIFF
    nlohmann_json::nlohmann_json
    ${LZO_LIBRARY}
)

target_include_directories(a3texture-cli PRIVATE ${Stb_INCLUDE_DIR} ${LZO_INCLUDE_DIR})

#=============================================================================
# GUI Application
#=============================================================================
set(GUI_SOURCES
    src/gui_main.cpp
    src/paa.cpp
    src/image_loader.cpp
    src/channel_packer.cpp
    src/texture_role.cpp
    src/job_runner.cpp
    src/p3d_reader.cpp
    src/rvmat_writer.cpp
    src/viewer_3d.cpp
    src/model_renderer.cpp
    src/paa_texture.cpp
    src/rvmat_parser.cpp
)

add_executable(a3texture-gui ${GUI_SOURCES})

target_link_libraries(a3texture-gui PRIVATE
    unofficial::libsquish::squish
    PNG::PNG
    TIFF::TIFF
    ${LZO_LIBRARY}
    imgui::imgui
    glfw
    glad::glad
)

target_include_directories(a3texture-gui PRIVATE
    ${Stb_INCLUDE_DIR}
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    ${LZO_INCLUDE_DIR}
)

# Installation
install(TARGETS a3texture-cli a3texture-gui DESTINATION bin)

message(STATUS "Configuration:")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
