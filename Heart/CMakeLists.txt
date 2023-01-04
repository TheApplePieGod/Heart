project(Engine)

if(APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
endif()

# Legacy include imgui so imguizmo can build properly
include_directories("lib/imgui")

# Build dependencies
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
add_subdirectory("lib/GLFW")
add_subdirectory("lib/glm")
add_subdirectory("lib/imgui")
add_subdirectory("lib/spdlog")
add_subdirectory("lib/Glad")
add_subdirectory("lib/entt")
add_subdirectory("lib/imguizmo")
add_subdirectory("lib/json")
set(FLOURISH_GLFW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/GLFW/include")
set(FLOURISH_IMGUI_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui")
add_subdirectory("lib/Flourish")
set(BUILD_CPU_DEMOS OFF)
set(BUILD_OPENGL3_DEMOS OFF)
set(BUILD_BULLET2_DEMOS OFF)
set(BUILD_EXTRAS OFF)
set(BUILD_UNIT_TESTS OFF)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
add_subdirectory("lib/bullet3")
add_subdirectory("lib/FreeType")
add_library(Freetype::Freetype ALIAS freetype-interface) # Add alias so msdf can find
set (MSDF_ATLAS_GEN_BUILD_STANDALONE OFF)
add_subdirectory("lib/msdf")

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
  add_subdirectory("lib/optick")
endif()

# Include other dependency sources that need to be built
set(DEPENDENCY_SOURCES
  "lib/imgui/backends/imgui_impl_glfw.cpp"
  "lib/imgui/backends/imgui_impl_vulkan.cpp"
  "lib/imgui/backends/imgui_impl_opengl3.cpp"
  "lib/stb_image/stb_image.cpp"
  "lib/tinyfd/tinyfiledialogs.cpp"
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
if(APPLE)
  file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h" "src/*.mm" "src/hepch.h")
else()
  file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h" "src/hepch.h")
endif()
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
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/bullet3/src>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/msdf>"
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
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/lib/bullet3/src>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/lib/msdf>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Flourish/Flourish/src>"
  "$<INSTALL_INTERFACE:${DOTNET_RUNTIME_DIR}>"
)

# Link dependencies
target_link_libraries(Engine
  PUBLIC
  glfw glm imgui spdlog glad imguizmo nlohmann_json FlourishCore
  BulletDynamics BulletCollision LinearMath
  freetype msdf-atlas-gen
)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
  target_link_libraries(Engine
    PUBLIC
    OptickCore
  )
endif()

# PCH
target_precompile_headers(
  Engine
  PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/hepch.h>"
)

# Shared dist library
if (HEART_BUILD_RUNTIME)
  add_library(EngineRuntime STATIC "${SOURCES}" "${DEPENDENCY_SOURCES}")
  target_compile_definitions(EngineRuntime PUBLIC HE_DIST)
  target_compile_features(EngineRuntime PUBLIC cxx_std_17)
  target_include_directories(EngineRuntime
    PUBLIC
    "$<TARGET_PROPERTY:Engine,INTERFACE_INCLUDE_DIRECTORIES>"
  )
  target_link_libraries(EngineRuntime
    PUBLIC
    "$<TARGET_PROPERTY:Engine,LINK_LIBRARIES>"
  )
  target_precompile_headers(EngineRuntime REUSE_FROM Engine)
endif()

# Installation
include(CMakePackageConfigHelpers)

set(JSON_Install ON) # Make sure the json lib installs the proper files
install(
  TARGETS
    Engine glm imgui spdlog glad imguizmo nlohmann_json glfw FlourishCore
    BulletDynamics BulletCollision LinearMath
    freetype msdf-atlas-gen msdfgen-all msdfgen-core msdfgen-ext
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