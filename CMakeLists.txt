cmake_minimum_required (VERSION 3.8)

project(__cyberspaceII LANGUAGES CXX)
include(FetchContent)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_SOURCES "src/core/camera.cpp" "src/core/timer.cpp" "src/core/engine.cpp" "src/core/log.cpp" 
"src/error/error.cpp" "src/core/grid.cpp" "src/core/memory.cpp")
set(RENDERER_SOURCES "src/renderer/renderer.cpp" "src/renderer/shader.cpp" "src/renderer/light.cpp" "src/renderer/texture.cpp"
"src/renderer/billboard.cpp")
set(WIN32_SOURCES "src/win32/win_main.cpp" "src/win32/win_path.cpp" "src/win32/file_dialog.cpp")
set(UI_SOURCES "src/ui/ui.cpp")
set(ASSET_LOADER_SOURCES "src/assets/asset_loader.cpp")
set(SOURCE_FILES ${CORE_SOURCES} ${WIN32_SOURCES} ${RENDERER_SOURCES} ${UI_SOURCES} ${ASSET_LOADER_SOURCES})

set(CORE_HEADERS "src/core/camera.h" "src/core/timer.h" "src/core/engine.h" "src/core/log.h" 
"src/core/scene.h" "src/error/error.h" "src/core/grid.h" "src/core/memory.h" "src/core/upload_buffer.h")
set(RENDERER_HEADERS "src/renderer/renderer.h" "src/renderer/render_types.h" "src/renderer/shader.h" "src/renderer/light.h" 
"src/renderer/texture.h" "src/renderer/billboard.h")
set(WIN32_HEADERS "src/win32/win_path.h" "src/win32/file_dialog.h")
set(ASSET_LOADER_HEADERS "src/assets/asset_loader.h")
set(UI_HEADERS "src/ui/ui.h")
set(HEADER_FILES ${CORE_HEADERS} ${WIN32_HEADERS} ${RENDERER_HEADERS} ${UI_HEADERS} ${DX_HELPERS} ${DX_HELPERS} ${ASSET_LOADER_HEADERS})

set(SHADER_FILES "src/shaders/sendai/pbr.hlsl" "src/shaders/sendai/billboard.hlsl" 
"src/shaders/sendai/grid.hlsl" "src/shaders/sendai/shader_defs.h")

set_source_files_properties(${SHADER_FILES} PROPERTIES LANGUAGE HLSL)

source_group("Core" FILES ${CORE_SOURCES} ${CORE_HEADERS})
source_group("Renderer" FILES ${RENDERER_SOURCES} ${RENDERER_HEADERS})
source_group("Win32" FILES ${WIN32_SOURCES} ${WIN32_HEADERS})
source_group("UI" FILES ${UI_SOURCES} ${UI_HEADERS})
source_group("Asset Loader" FILES ${ASSET_LOADER_SOURCES} ${ASSET_LOADER_HEADERS})
source_group("DX Helpers" FILES ${DX_HELPERS})
source_group("Shaders" FILES ${SHADER_FILES})

set(ALL_PROJECT_FILES ${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})

FetchContent_Declare(
    directx_headers
    GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
    GIT_TAG        13a4c364681301385204238a5dd46b65f27ab665
)
FetchContent_MakeAvailable(directx_headers)
message(STATUS "DirectX Headers Source Dir: ${directx_headers_SOURCE_DIR}")

FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG de771018665d324238915da99564164af90617fd
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    directxtex
    GIT_REPOSITORY https://github.com/microsoft/DirectXTex.git
    GIT_TAG        main
)

FetchContent_MakeAvailable(directxtex)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
)

# You need to add all files on the executable for them to be visible on 
# the source_group in Visual Studio
# https://stackoverflow.com/a/31653555/14815076
add_executable(${PROJECT_NAME} WIN32 ${ALL_PROJECT_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE "src/core/pch.h")
target_compile_options(${PROJECT_NAME} PRIVATE /W4)
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")

target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR} 
    ${imgui_SOURCE_DIR}/backends
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${directx_headers_SOURCE_DIR}/include/directx
    ${directx_headers_SOURCE_DIR}/include
    ${assimp_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PUBLIC d3d12.lib dxguid.lib dxgi.lib D3DCompiler.lib DirectX-Headers assimp::assimp imgui DirectXTex)

target_link_options(${PROJECT_NAME} PRIVATE /INCREMENTAL:NO)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/sendai $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/sendai
    COMMENT "Copying triangle shaders" VERBATIM
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying assets" VERBATIM
)