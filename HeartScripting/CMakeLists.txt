project(ScriptingCore LANGUAGES CSharp)

INCLUDE(CSharpUtilities)

set(CMAKE_DOTNET_TARGET_FRAMEWORK_VERSION "v4.8")

# Build main
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cs")
add_library(ScriptingCore SHARED "${SOURCES}")

# target_compile_options(ScriptingCore PRIVATE "-debug:embedded")

target_link_libraries(ScriptingCore CommonLib)

set_target_properties(
    ScriptingCore PROPERTIES
    VS_GLOBAL_ROOTNAMESPACE "Heart"
)