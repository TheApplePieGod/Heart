set(DOXYGEN_SOURCE1 "../Heart/src")
set(DOXYGEN_SOURCE2 "../HeartEditor/src")
set(DOXYGEN_HOMEPAGE "../README.md")
set(DOXYGEN_DOCS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})

configure_file(doxy.in doxy.cfg @ONLY)

add_custom_target(
    HeartDocs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxy.cfg
    WORKING_DIRECTORY ".."
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)