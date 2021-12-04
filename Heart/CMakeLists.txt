project(Engine)

# Legacy include imgui so imguizmo can build properly
include_directories("lib/imgui")

# Build dependencies
set(GLFW_BUILD_DOCS OFF)
add_subdirectory("lib/GLFW")
add_subdirectory("lib/glm")
add_subdirectory("lib/imgui")
add_subdirectory("lib/spdlog")
add_subdirectory("lib/Glad")
add_subdirectory("lib/optick")
add_subdirectory("lib/entt")
add_subdirectory("lib/imguizmo")
add_subdirectory("lib/json")

# Include other dependency sources that need to be built
set(DEPENDENCY_SOURCES
  "lib/imgui/backends/imgui_impl_glfw.cpp"
  "lib/imgui/backends/imgui_impl_vulkan.cpp"
  "lib/imgui/backends/imgui_impl_opengl3.cpp"
  "lib/stb_image/stb_image.cpp"
)

# Copy engine resources
add_custom_target(
  CopyEngineResources ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/resources
  DEPENDS Engine
)

include(GNUInstallDirs)

# Build main
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h" "src/hepch.h")
#add_executable(Engine "${SOURCES}" "${DEPENDENCY_SOURCES}" "src/Heart/Core/EntryPoint.cpp")
add_library(Engine "${SOURCES}" "${DEPENDENCY_SOURCES}")
add_library(Heart::Engine ALIAS Engine)

target_compile_features(Engine PUBLIC cxx_std_17)

# Includes
target_include_directories(
  Engine
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/GLFW/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/glm>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/spdlog/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/Glad/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/optick/src>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/entt/src>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/imguizmo>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/json/include>"
  "$<BUILD_INTERFACE:$ENV{VULKAN_SDK}/include>"

  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/GLFW/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/glm>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/imgui>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/spdlog/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Glad/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/optick/src>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/entt/src>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/imguizmo>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/json/include>"
  "$<INSTALL_INTERFACE:$ENV{VULKAN_SDK}/include>"
)

# Locate vulkansdk libraries
if (WIN32)
  find_library(vulkan
    NAMES "vulkan-1"
    PATHS "$ENV{VULKAN_SDK}/lib"
  )
  find_library(shaderc
    NAMES "shaderc_shared"
    PATHS "$ENV{VULKAN_SDK}/lib"
  )
  find_library(spirvglsl
    NAMES "spirv-cross-glsl"
    PATHS "$ENV{VULKAN_SDK}/lib"
  )
else()
  find_library(vulkan
    NAMES "libvulkan.so"
    PATHS "$ENV{VULKAN_SDK}/lib"
  )
  find_library(shaderc
    NAMES "libshaderc_shared.so"
    PATHS "$ENV{VULKAN_SDK}/lib"
  )
  find_library(spirvglsl
    NAMES "libspirv-cross-glsl.a"
    PATHS "$ENV{VULKAN_SDK}/lib"
  )
endif()

if (WIN32)
  if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    find_library(spirvcored
      NAMES "spirv-cross-cored"
      PATHS "$ENV{VULKAN_SDK}/lib"
    )
    target_link_libraries(Engine ${spirvcored})
  else()
    find_library(spirvcore
      NAMES "spirv-cross-core"
      PATHS "$ENV{VULKAN_SDK}/lib"
    )
    target_link_libraries(Engine ${spirvcore})
  endif()
else()
  find_library(spirvcore
    NAMES "libspirv-cross-core.a"
    PATHS "$ENV{VULKAN_SDK}/lib"
  )
  target_link_libraries(Engine ${spirvcore})
endif()

# Link dependencies
target_link_libraries(
  Engine
  glfw glm imgui spdlog glad OptickCore imguizmo nlohmann_json ${vulkan} ${shaderc} ${spirvglsl}
)

# PCH
target_precompile_headers(
  Engine
  PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/hepch.h>"
)

# Installation
include(CMakePackageConfigHelpers)

set(JSON_Install ON) # Make sure the json lib installs the proper files
install(
  TARGETS Engine glm imgui spdlog glad imguizmo nlohmann_json glfw OptickCore
  EXPORT EngineTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  DIRECTORY src/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
    PATTERN "*.h"
)
install(
  DIRECTORY lib/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.inl"
)
install(
  EXPORT EngineTargets
  FILE EngineTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Heart
  NAMESPACE Heart::
)
configure_package_config_file( 
  "HeartConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/HeartConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Heart
  PATH_VARS
  CMAKE_INSTALL_LIBDIR
)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/HeartConfigVersion.cmake
  VERSION 0.1.0
  COMPATIBILITY SameMajorVersion
)
install(
  FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/HeartConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/HeartConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Heart
)