cmake_minimum_required(VERSION 3.22)

project(Runtime)

# TODO: this is super scuffed. Need a better way to compile runtime
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")

set(HEART_BUILD_RUNTIME ON)
add_subdirectory(.. build)

# Build main
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")

if(APPLE)
  add_executable(Runtime MACOSX_BUNDLE "${SOURCES}")
elseif(ANDROID)
  add_library(Runtime SHARED "${SOURCES}")
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

target_link_libraries(Runtime Engine)

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
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"
    BUILD_RPATH "@executable_path/../Frameworks"
  )
elseif(ANDROID)
  find_library(
    log-lib
    log
  )

  add_library(
    android-app-glue
    STATIC
    ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
  )

  # Link onCreate function for android to call
  set_target_properties(Runtime PROPERTIES
    APPEND_STRING
    PROPERTY LINK_FLAGS " -u ANativeActivity_onCreate"
  )

  target_link_libraries(Runtime android-app-glue ${log-lib})
endif()

# PCH
target_precompile_headers(Runtime REUSE_FROM Engine)
