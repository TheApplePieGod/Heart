project(CoreScripts LANGUAGES CSharp)

INCLUDE(CSharpUtilities)

set(CMAKE_DOTNET_TARGET_FRAMEWORK_VERSION "v4.8")

# Build main
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cs")
add_library(CoreScripts SHARED "${SOURCES}")

# target_compile_options(CoreScripts PRIVATE "-debug:embedded")

target_link_libraries(CoreScripts CommonLib)

set_target_properties(
    CoreScripts PROPERTIES
    VS_GLOBAL_ROOTNAMESPACE "Heart"
)