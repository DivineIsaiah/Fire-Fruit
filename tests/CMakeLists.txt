enable_testing()

add_executable(student_tests
    test_main.cpp
    ../src/filesystem/FileSearcher.cpp
    ../src/utils/StringUtils.cpp
    ../src/extraction/ColumnMatcher.cpp
    ../src/extraction/StudentExtractor.cpp
)

target_include_directories(student_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(student_tests PRIVATE OpenXLSX::OpenXLSX)

if(WIN32)
    add_custom_command(TARGET student_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/output/libOpenXLSX.dll"
                "$<TARGET_FILE_DIR:student_tests>"
        COMMENT "Copying OpenXLSX runtime DLL next to student_tests"
    )

    add_custom_command(TARGET student_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "C:/mingw64/bin/libstdc++-6.dll"
                "$<TARGET_FILE_DIR:student_tests>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "C:/mingw64/bin/libgcc_s_seh-1.dll"
                "$<TARGET_FILE_DIR:student_tests>"
        COMMENT "Copying MinGW runtime DLLs next to student_tests"
    )
endif()

add_test(NAME student_tests COMMAND student_tests)
