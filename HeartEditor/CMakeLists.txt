project(Editor)

# Main includes
include_directories("src")

# Dependency includes
include_directories("../Heart/src")
include_directories("../Heart")
include_directories("../Heart/lib")
include_directories("../Heart/lib/GLFW/include")
include_directories("../Heart/lib/glm")
include_directories("../Heart/lib/imgui")
include_directories("../Heart/lib/spdlog/include")
include_directories("../Heart/lib/optick/src")
include_directories("../Heart/lib/entt/src")

# Copy editor resources
add_custom_target(CopyEditorResources ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/resources/editor
  DEPENDS Engine
)

# Build main
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")
add_executable(Editor "${SOURCES}")

target_link_libraries(Editor Engine)

target_compile_features(Editor PUBLIC cxx_std_17)