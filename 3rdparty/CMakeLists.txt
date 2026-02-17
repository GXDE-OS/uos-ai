#tar -czvf qmarkdowntextedit.tar.gz ./qmarkdowntextedit
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/qmarkdowntextedit")
    message("find qmarkdowntextedit")
else()
    message("decompression qmarkdowntextedit")
    execute_process(
        COMMAND tar -xzvf qmarkdowntextedit.tar.gz
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE QMARKDOWN_UNZIP_RESULT
    )

    if (NOT QMARKDOWN_UNZIP_RESULT EQUAL "0")
        message(FATAL_ERROR "tar qmarkdowntextedit.tar.gz failed: ${QMARKDOWN_UNZIP_RESULT}")
    endif()
endif()

add_subdirectory(qmarkdowntextedit)
