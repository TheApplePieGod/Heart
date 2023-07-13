project(Runtime)

# Build main
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")

if(APPLE)
  add_executable(Runtime MACOSX_BUNDLE "${SOURCES}")
else()
  add_executable(Runtime "${SOURCES}")
endif()

target_compile_features(Runtime PUBLIC cxx_std_17)

# Include directories
target_include_directories(
  Runtime
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
)

target_link_libraries(Runtime EngineRuntime)

if (WIN32)
  set_target_properties(Runtime PROPERTIES WIN32_EXECUTABLE true)
elseif(APPLE)
  set_target_properties(Runtime PROPERTIES
    BUNDLE True
    MACOSX_BUNDLE_GUI_IDENTIFIER heart.runtime
    MACOSX_BUNDLE_BUNDLE_NAME Runtime
    MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
    XCODE_GENERATE_SCHEME TRUE
    XCODE_SCHEME_ENVIRONMENT PATH=$ENV{PATH}:$ENV{DOTNET_SDK}
    XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE Metal
  )
endif()

# PCH
target_precompile_headers(Runtime REUSE_FROM EngineRuntime)
