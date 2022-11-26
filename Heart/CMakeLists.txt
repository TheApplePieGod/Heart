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
add_subdirectory("lib/Flourish")

# Include other dependency sources that need to be built
set(DEPENDENCY_SOURCES
  "lib/imgui/backends/imgui_impl_glfw.cpp"
  "lib/imgui/backends/imgui_impl_vulkan.cpp"
  "lib/imgui/backends/imgui_impl_opengl3.cpp"
  "lib/stb_image/stb_image.cpp"
)

execute_process(
  COMMAND python "scripts/configure-hostfxr.py"
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE DOTNET_RUNTIME_DIR
  ERROR_VARIABLE DOTNET_HOSTFXR_DIR
)

# Copy engine resources
add_custom_target(
  CopyEngineResources ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIRECTORY}/resources
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/resources ${OUTPUT_DIRECTORY}/resources/engine
  DEPENDS Engine
)

include(GNUInstallDirs)

# Build main
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h" "src/hepch.h")
add_library(Engine STATIC "${SOURCES}" "${DEPENDENCY_SOURCES}")
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
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/utf-cpp/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/Flourish/Flourish/src>"
  "$<BUILD_INTERFACE:${DOTNET_RUNTIME_DIR}>"

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
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/utf-cpp/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Flourish/Flourish/src>"
  "$<INSTALL_INTERFACE:${DOTNET_RUNTIME_DIR}>"
)

# Link dependencies
target_link_libraries(
  Engine
  PUBLIC
  glfw glm imgui spdlog glad OptickCore imguizmo nlohmann_json FlourishCore
)

# Copy required dlls
add_custom_target(
  CopyRequiredDlls ALL
  COMMAND ${CMAKE_COMMAND} -E copy ${DOTNET_HOSTFXR_DIR}/hostfxr.dll ${OUTPUT_DIRECTORY}
  DEPENDS Engine
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
  TARGETS Engine glm imgui spdlog glad imguizmo nlohmann_json glfw OptickCore FlourishCore
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