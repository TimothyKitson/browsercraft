cmake_minimum_required(VERSION 3.16)
project(VoxelEngine CXX)

# Reconstructed header: the recovered CMakeLists.txt kept only everything from
# target_link_libraries(glm) onward, so the project setup below was rebuilt from
# the source tree and the flags the surviving tail depends on.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

file(GLOB_RECURSE VOXEL_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(VoxelEngine ${VOXEL_SOURCES})
target_include_directories(VoxelEngine PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/third_party")

if(NOT EMSCRIPTEN)
    find_package(SDL2 REQUIRED)
endif()

target_link_libraries(VoxelEngine PRIVATE glm::glm)

if(EMSCRIPTEN)
    # Which assets get baked into the .data file. tools/build-web.ps1 points
    # this at a staging folder holding the lower-resolution texture set, so
    # the page download stays small.
    set(WEB_ASSET_DIR "${CMAKE_SOURCE_DIR}/assets" CACHE PATH "Assets to preload into the web build")

    set_target_properties(VoxelEngine PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")

    # main() catches std::exception, and the shipped build imports __cxa_throw
    # and the invoke_* thunks, so exceptions must stay enabled.
    target_compile_options(VoxelEngine PRIVATE -sUSE_SDL=2 -fexceptions -O2)
    target_link_options(VoxelEngine PRIVATE
        -sUSE_SDL=2
        -fexceptions
        -sMIN_WEBGL_VERSION=2          # WebGL2 == OpenGL ES 3.0
        -sMAX_WEBGL_VERSION=2
        -sFULL_ES3
        -sALLOW_MEMORY_GROWTH=1        # the world allocates chunks as you explore
        -sINITIAL_MEMORY=268435456     # 256 MB up front so early streaming doesn't thrash
        -sSTACK_SIZE=4MB
        -sEXIT_RUNTIME=0
        -sENVIRONMENT=web
        -O2
        --preload-file "${WEB_ASSET_DIR}@assets"
        --shell-file "${CMAKE_SOURCE_DIR}/web/shell.html"
    )
    # Rebuild the page whenever the shell changes.
    set_property(TARGET VoxelEngine APPEND PROPERTY LINK_DEPENDS "${CMAKE_SOURCE_DIR}/web/shell.html")
else()
    target_link_libraries(VoxelEngine PRIVATE SDL2::SDL2 SDL2::SDL2main)

    if(WIN32)
        target_link_libraries(VoxelEngine PRIVATE opengl32)
    elseif(APPLE)
        target_link_libraries(VoxelEngine PRIVATE "-framework OpenGL")
    else()
        target_link_libraries(VoxelEngine PRIVATE GL)
    endif()

    # Copy assets (shaders, textures) next to the built executable so the
    # relative paths the engine uses at runtime ("assets/...") resolve correctly.
    add_custom_command(TARGET VoxelEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets
                $<TARGET_FILE_DIR:VoxelEngine>/assets
    )

    # On Windows the SDL2 DLL needs to sit next to the exe.
    if(WIN32)
        add_custom_command(TARGET VoxelEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:SDL2::SDL2>
                    $<TARGET_FILE_DIR:VoxelEngine>
        )
    endif()
endif()