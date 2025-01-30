# ImGui
cpmaddpackage(
        NAME
        imgui
        VERSION
        1.91.7
        GITHUB_REPOSITORY
        ocornut/imgui
        DOWNLOAD_ONLY
        TRUE)

# CMakeLists.txt from https://gist.githubusercontent.com/rokups/f771217b2d530d170db5cb1e08e9a8f4
file(
        DOWNLOAD
        "https://gist.githubusercontent.com/rokups/f771217b2d530d170db5cb1e08e9a8f4/raw/4c2c14374ab878ca2f45daabfed4c156468e4e27/CMakeLists.txt"
        "${imgui_SOURCE_DIR}/CMakeLists.txt"
        EXPECTED_HASH
        SHA256=fd62f69364ce13a4f7633a9b50ae6672c466bcc44be60c69c45c0c6e225bb086)

# Options
set(IMGUI_EXAMPLES FALSE)
set(IMGUI_DEMO TRUE)
set(IMGUI_ENABLE_STDLIB_SUPPORT TRUE)

# FreeType (https://github.com/cpm-cmake/CPM.cmake/wiki/More-Snippets#freetype)
find_package(freetype CONFIG REQUIRED)

set(FREETYPE_FOUND TRUE)
set(FREETYPE_INCLUDE_DIRS "")
set(FREETYPE_LIBRARIES Freetype::Freetype)

# Add subdirectory
add_subdirectory(${imgui_SOURCE_DIR} EXCLUDE_FROM_ALL SYSTEM)

target_include_directories(imgui INTERFACE "${imgui_SOURCE_DIR}")

# Fixing linking options
target_sources(
        imgui
        PUBLIC ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
