project(Tests)

# Build dependencies
add_subdirectory("lib/doctest")

# Build main
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")
add_executable(Tests "${SOURCES}")

target_compile_features(Tests PUBLIC cxx_std_17)

# Include directories
target_include_directories(
  Tests
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/doctest>"
)

target_link_libraries(Tests Engine)

# PCH
target_precompile_headers(Tests REUSE_FROM Engine)