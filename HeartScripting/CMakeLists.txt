project(CoreScripts)

# Copy source & built files
if(${HEART_BUILD_RUNTIME})
  add_custom_target(PublishScripts ALL
    DEPENDS Engine
    VERBATIM
    COMMAND "unset" "TARGETNAME" # Fixes issue with xcode generator
    COMMAND
      "$ENV{DOTNET_SDK}/dotnet"
      "publish"
      "${CMAKE_CURRENT_SOURCE_DIR}/Heart.NET.Sdk/Heart.NET.Sdk.csproj"
      "-r" "osx-arm64"
      "-c" "Release"
      "--self-contained"
  )

  add_custom_target(CopyScriptingResources ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/bin/osx-arm64/publish ${OUTPUT_DIRECTORY}/scripting
    DEPENDS PublishScripts
  )
else()
  add_custom_target(BuildScripts ALL
    DEPENDS Engine
    VERBATIM
    COMMAND "unset" "TARGETNAME"
    COMMAND
      "$ENV{DOTNET_SDK}/dotnet"
      "build"
      "${CMAKE_CURRENT_SOURCE_DIR}/Heart.NET.Sdk/Heart.NET.Sdk.csproj"
      "-c" "${CMAKE_BUILD_TYPE}"
  )

  add_custom_target(CopyScriptingResources ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/bin ${OUTPUT_DIRECTORY}/scripting
    DEPENDS BuildScripts
  )

  add_custom_target(
    CopyRequiredDlls ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${DOTNET_HOSTFXR_PATH} ${OUTPUT_DIRECTORY}/scripting
    DEPENDS CopyScriptingResources
  )
endif()