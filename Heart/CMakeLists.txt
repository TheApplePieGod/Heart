project(Engine)

if(APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
endif()

# Legacy include imgui so imguizmo can build properly
include_directories("lib/imgui")

# Build dependencies
if(NOT ANDROID AND ${CMAKE_BUILD_TYPE} MATCHES Debug)
  # Temp disable on android due to TLS issue
  # https://github.com/wolfpld/tracy/issues/779
  set(TRACY_ENABLE ON)
  set(TRACY_ON_DEMAND ON)
  set(FLOURISH_TRACY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/tracy/public")
  if(DEFINED ENV{NSIGHT_AFTERMATH_SDK})
    set(FLOURISH_ENABLE_AFTERMATH ON)
  endif()
  add_subdirectory("lib/tracy")
  add_compile_definitions(HE_ENABLE_TRACY)
else()
  set(TRACY_ENABLE OFF)
endif()
if(NOT ANDROID)
  set(GLFW_BUILD_DOCS OFF)
  set(GLFW_BUILD_TESTS OFF)
  set(GLFW_BUILD_EXAMPLES OFF)
    set(FLOURISH_GLFW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/GLFW/include")
  add_subdirectory("lib/GLFW")
endif()
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)
add_subdirectory("lib/glm")
add_subdirectory("lib/imgui")
add_subdirectory("lib/spdlog")
add_subdirectory("lib/Glad")
add_subdirectory("lib/entt")
add_subdirectory("lib/imguizmo")
add_subdirectory("lib/json")
set(FLOURISH_IMGUI_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui")
add_subdirectory("lib/Flourish")
set(BUILD_CPU_DEMOS OFF)
set(BUILD_OPENGL3_DEMOS OFF)
set(BUILD_BULLET2_DEMOS OFF)
set(BUILD_EXTRAS OFF)
set(BUILD_UNIT_TESTS OFF)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
add_subdirectory("lib/bullet3")
set(FT_DISABLE_ZLIB ON)
set(FT_DISABLE_BZIP2 ON)
set(FT_DISABLE_PNG ON)
set(FT_DISABLE_HARFBUZZ ON)
set(FT_DISABLE_BROTLI ON)
add_subdirectory("lib/FreeType")
add_library(Freetype::Freetype ALIAS freetype-interface) # Add alias so msdf can find
set(MSDF_ATLAS_CORE_ONLY ON)
set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_VCPKG OFF)
set(MSDF_ATLAS_USE_CPP11 ON)
set(MSDF_ATLAS_USE_SKIA OFF)
set(MSDF_ATLAS_DYNAMIC_RUNTIME ON)
set(MSDFGEN_DISABLE_PNG ON)
add_subdirectory("lib/msdf")
set(TinyTIFF_BUILD_SHARED_LIBS OFF)
set(TinyTIFF_BUILD_STATIC_LIBS ON)
set(TinyTIFF_BUILD_TESTS OFF)
add_subdirectory("lib/tinytiff")
if (NOT ANDROID)
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory("lib/efsw")
endif()

# Include other dependency sources that need to be built
set(DEPENDENCY_SOURCES
  "lib/imgui/backends/imgui_impl_vulkan.cpp"
  "lib/stb_image/stb_image.cpp"
  "lib/tinyfd/tinyfiledialogs.cpp"
)
if(ANDROID)
  set(DEPENDENCY_SOURCES
    ${DEPENDENCY_SOURCES}
    "lib/imgui/backends/imgui_impl_android.cpp"
  )
else()
  set(DEPENDENCY_SOURCES
    ${DEPENDENCY_SOURCES}
    "lib/imgui/backends/imgui_impl_glfw.cpp"
  )
endif()

# Copy engine resources
add_custom_target(
  CopyEngineResources ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIRECTORY}/resources
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/resources ${OUTPUT_DIRECTORY}/resources/engine
  DEPENDS Engine
)

# Build main
if(APPLE)
  file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h" "src/*.mm" "src/hepch.h")
else()
  file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h" "src/hepch.h")
endif()
add_library(Engine STATIC "${SOURCES}" "${DEPENDENCY_SOURCES}")
add_library(Heart::Engine ALIAS Engine)

target_compile_features(Engine PUBLIC cxx_std_17)

# Includes
target_include_directories(Engine PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/GLFW/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/glm>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/spdlog/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/Glad/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/entt/src>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/imguizmo>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/json/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/utf-cpp/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/bullet3/src>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/msdf>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/tinytiff/src>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/efsw/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/Flourish/Flourish/src>"
  "$<BUILD_INTERFACE:${DOTNET_RUNTIME_DIR}>"
)
if(ANDROID)
  target_include_directories(Engine PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_ANDROID_NDK}/sources/android>"
  )
endif()

# Link dependencies
set(LINK_LIBS
  glm imgui spdlog glad imguizmo nlohmann_json FlourishCore
  BulletDynamics BulletCollision LinearMath
  freetype msdf-atlas-gen TinyTIFF
)
target_link_libraries(Engine PUBLIC ${LINK_LIBS})
if(TRACY_ENABLE)
  target_link_libraries(Engine PUBLIC TracyClient)
endif()
if(ANDROID)
  target_link_libraries(Engine PUBLIC android)
else()
  target_link_libraries(Engine PUBLIC glfw efsw-static)
endif()

# PCH
target_precompile_headers(
  Engine
  PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/hepch.h>"
)
