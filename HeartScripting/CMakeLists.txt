project(CoreScripts)

if (APPLE)
  set(PREBUILD_COMMANDS "unset" "TARGETNAME" "&&")
endif()

add_custom_target(BuildScripts ALL
  DEPENDS Engine
  VERBATIM
  COMMAND
    ${PREBUILD_COMMANDS}
    "$ENV{DOTNET_SDK}/dotnet"
    "build"
    "${CMAKE_CURRENT_SOURCE_DIR}/Heart.NET.Sdk/Heart.NET.Sdk.csproj"
    "-c" "${CMAKE_BUILD_TYPE}"
)

# Copy source & built files
if(${HEART_BUILD_RUNTIME})
  add_custom_target(PublishScripts ALL
    DEPENDS BuildScripts
    VERBATIM
    COMMAND
      ${PREBUILD_COMMANDS}
      "$ENV{DOTNET_SDK}/dotnet"
      "publish"
      "${CMAKE_CURRENT_SOURCE_DIR}/Heart.NET.Sdk/Heart.NET.Sdk.csproj"
      "-r" "${DOTNET_RUNTIME_STR}"
      "-c" "Release"
      "--self-contained"
  )

  add_custom_target(CopyPublishResources ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/bin/${DOTNET_RUNTIME_STR}/publish ${OUTPUT_DIRECTORY}/dotnet
    DEPENDS PublishScripts
  )
endif()

add_custom_target(CopyScriptingResources ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/bin ${OUTPUT_DIRECTORY}/scripting
  COMMAND ${CMAKE_COMMAND} -E copy ${DOTNET_HOSTFXR_PATH} ${OUTPUT_DIRECTORY}/scripting
  DEPENDS BuildScripts
)