project(CoreScripts)

# Copy source & built files
add_custom_target(CopyScriptingResources ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/bin
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/bin ${OUTPUT_DIRECTORY}/scripting
  DEPENDS Engine
)