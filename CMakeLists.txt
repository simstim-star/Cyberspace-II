cmake_minimum_required (VERSION 3.8)

project(__cyberspaceII LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_SOURCES "src/core/camera.cpp" "src/core/memory.cpp" "src/core/timer.cpp" "src/core/engine.cpp" "src/core/log.cpp" 
"src/error/error.cpp" "src/core/grid.cpp")
set(RENDERER_SOURCES "src/renderer/renderer.cpp" "src/renderer/shader.cpp" "src/renderer/light.cpp" "src/renderer/texture.cpp"
"src/renderer/billboard.cpp")
set(WIN32_SOURCES "src/win32/win_main.cpp" "src/win32/win_path.cpp" "src/win32/file_dialog.cpp")
set(UI_SOURCES)
set(GLTF_SOURCES "src/assets/gltf.cpp")
set(SOURCE_FILES ${CORE_SOURCES} ${WIN32_SOURCES} ${RENDERER_SOURCES} ${UI_SOURCES} ${GLTF_SOURCES})

set(CORE_HEADERS "src/core/camera.h" "src/core/memory.h" "src/core/timer.h" "src/core/engine.h" "src/core/log.h" 
"src/core/scene.h" "src/error/error.h" "src/core/grid.h")
set(RENDERER_HEADERS "src/renderer/renderer.h" "src/renderer/render_types.h" "src/renderer/shader.h" "src/renderer/light.h" 
"src/renderer/texture.h" "src/renderer/billboard.h")
set(WIN32_HEADERS "src/win32/win_path.h" "src/win32/str_helper.h" "src/win32/file_dialog.h")
set(UI_HEADERS)
set(DX_HELPERS "src/dx_helpers/barrier_helpers.h" "src/dx_helpers/desc_helpers.h")
set(GLTF_HEADERS "src/assets/gltf.h")
set(HEADER_FILES ${CORE_HEADERS} ${WIN32_HEADERS} ${RENDERER_HEADERS} ${UI_HEADERS} ${DX_HELPERS} ${DX_HELPERS} ${GLTF_HEADERS})

set(DEPS_FILES "deps/b64.h" "deps/cgltf.h" "deps/nuklear.h" "deps/stb_ds.h" "deps/stb_image.h" "deps/stb_image_resize2.h")

set(SHADER_FILES "src/shaders/sendai/gltf.hlsl" "src/shaders/sendai/pbr.hlsl" "src/shaders/sendai/billboard.hlsl" 
"src/shaders/sendai/grid.hlsl" "src/shaders/sendai/shader_defs.h")

set_source_files_properties(${SHADER_FILES} PROPERTIES LANGUAGE HLSL)

source_group("Core" FILES ${CORE_SOURCES} ${CORE_HEADERS})
source_group("Renderer" FILES ${RENDERER_SOURCES} ${RENDERER_HEADERS})
source_group("Win32" FILES ${WIN32_SOURCES} ${WIN32_HEADERS})
source_group("UI" FILES ${UI_SOURCES} ${UI_HEADERS})
source_group("GLTF" FILES ${GLTF_SOURCES} ${GLTF_HEADERS})
source_group("DX Helpers" FILES ${DX_HELPERS})
source_group("Shaders" FILES ${SHADER_FILES})
source_group("Libs" FILES ${DEPS_FILES})

set(ALL_PROJECT_FILES ${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})

# You need to add all files on the executable for them to be visible on 
# the source_group in Visual Studio
# https://stackoverflow.com/a/31653555/14815076
add_executable(${PROJECT_NAME} WIN32 ${ALL_PROJECT_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE "src/core/pch.h")
target_compile_options(${PROJECT_NAME} PRIVATE /W4)
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")

find_package(XMathC REQUIRED PATHS "C:/xmathc")
target_link_libraries(${PROJECT_NAME} PUBLIC d3d12.lib dxguid.lib dxgi.lib D3DCompiler.lib XMathC)
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