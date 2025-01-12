project(CoreScripts)

if (APPLE)
  set(PREBUILD_COMMANDS "unset" "TARGETNAME" "&&")
endif()

add_custom_target(BuildScripts ALL
  DEPENDS Engine
  VERBATIM
  COMMAND
    ${PREBUILD_COMMANDS}
    "dotnet"
    "build"
    "${CMAKE_CURRENT_SOURCE_DIR}/Heart.NET.Sdk/Heart.NET.Sdk.csproj"
    "-c" "${CMAKE_BUILD_TYPE}"
)

add_custom_target(CopyScriptingResources ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/bin ${OUTPUT_DIRECTORY}/scripting
  COMMAND ${CMAKE_COMMAND} -E copy ${DOTNET_HOSTFXR_PATH} ${OUTPUT_DIRECTORY}/scripting
  DEPENDS BuildScripts
)
