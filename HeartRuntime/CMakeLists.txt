project(Runtime)

# Build main
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")
add_executable(Runtime "${SOURCES}")

target_compile_features(Runtime PUBLIC cxx_std_17)

# Include directories
target_include_directories(
  Runtime
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
)

target_link_libraries(Runtime Engine)

set_target_properties(Runtime PROPERTIES WIN32_EXECUTABLE true)

# PCH
target_precompile_headers(Runtime REUSE_FROM Engine)