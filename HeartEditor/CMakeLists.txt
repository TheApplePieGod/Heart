project(Editor)

# Copy editor resources
add_custom_target(CopyEditorResources ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/resources/editor
  DEPENDS Engine
)

# Build main
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")
add_executable(Editor "${SOURCES}")

target_compile_features(Editor PUBLIC cxx_std_17)

# Include directories
target_include_directories(
  Editor
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
)

target_link_libraries(Editor Engine)