project(Editor)

# Copy editor resources
add_custom_target(CopyEditorResources ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIRECTORY}/resources
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/resources ${OUTPUT_DIRECTORY}/resources/editor
  DEPENDS Engine
)

add_custom_target(CopyEditorTemplates ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/templates ${OUTPUT_DIRECTORY}/templates
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

set_target_properties(Editor PROPERTIES WIN32_EXECUTABLE true)

if (APPLE)
  set_target_properties(Editor PROPERTIES
    XCODE_GENERATE_SCHEME TRUE
    XCODE_SCHEME_ENVIRONMENT PATH=$ENV{PATH}:$ENV{DOTNET_SDK}
    XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE Metal
  )
endif()

# PCH
target_precompile_headers(Editor REUSE_FROM Engine)