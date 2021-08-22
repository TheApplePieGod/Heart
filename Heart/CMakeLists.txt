project(Engine)

# Main includes
include_directories("src")
include_directories(".")

# Dependency includes
include_directories("lib")
include_directories("lib/GLFW/include")
include_directories("lib/glm")
include_directories("lib/imgui")
include_directories("lib/spdlog/include")
include_directories("$ENV{VULKAN_SDK}/Include")

# Build dependencies
add_subdirectory("lib/GLFW")
add_subdirectory("lib/glm")
add_subdirectory("lib/imgui")
add_subdirectory("lib/spdlog")

# Include other dependency sources that need to be built
set(DEPENDENCY_SOURCES
    "lib/imgui/backends/imgui_impl_glfw.cpp"
    "lib/imgui/backends/imgui_impl_vulkan.cpp"
    "lib/stb_image/stb_image.cpp"
)

# Build main
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h" "htpch.h")
#add_executable(Engine "${SOURCES}" "${DEPENDENCY_SOURCES}" "src/Heart/Core/EntryPoint.cpp")
add_library(Engine "${SOURCES}" "${DEPENDENCY_SOURCES}")

# Locate vulkansdk libraries
find_library(vulkan
  NAMES "vulkan-1"
  PATHS "$ENV{VULKAN_SDK}/Lib"
)
find_library(shaderc
  NAMES "shaderc_shared"
  PATHS "$ENV{VULKAN_SDK}/Lib"
)
find_library(spirvcore
  NAMES "spirv-cross-core"
  PATHS "$ENV{VULKAN_SDK}/Lib"
)
find_library(spirvhlsl
  NAMES "spirv-cross-hlsl"
  PATHS "$ENV{VULKAN_SDK}/Lib"
)
find_library(spirvglsl
  NAMES "spirv-cross-glsl"
  PATHS "$ENV{VULKAN_SDK}/Lib"
)

# Link dependencies
target_link_libraries(Engine glfw)
target_link_libraries(Engine glm)
target_link_libraries(Engine imgui)
target_link_libraries(Engine spdlog)
target_link_libraries(Engine ${vulkan})
target_link_libraries(Engine ${shaderc})
target_link_libraries(Engine ${spirvcore})
target_link_libraries(Engine ${spirvhlsl})
target_link_libraries(Engine ${spirvglsl})

target_precompile_headers(Engine
    PUBLIC "htpch.h"
)

target_compile_features(Engine PUBLIC cxx_std_17)