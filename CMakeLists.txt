cmake_minimum_required(VERSION 3.16)

project(StudentDataExtractor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MINGW OR UNIX)
    add_compile_options(-include cstdint)
endif()

add_executable(StudentDataExtractor
    src/main.cpp
    src/app/Application.cpp
    src/filesystem/DirectoryBrowser.cpp
    src/filesystem/FileSearcher.cpp
    src/excel/ExcelWriter.cpp
    src/extraction/StudentExtractor.cpp
    src/extraction/ColumnMatcher.cpp
    src/utils/StringUtils.cpp
    src/utils/Logger.cpp
)

add_executable(workbook_inspector
    src/debug/WorkbookInspector.cpp
)

target_include_directories(StudentDataExtractor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(workbook_inspector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(OpenXLSX CONFIG QUIET)
if(OpenXLSX_FOUND)
    target_link_libraries(StudentDataExtractor PRIVATE OpenXLSX::OpenXLSX)
    target_link_libraries(workbook_inspector PRIVATE OpenXLSX::OpenXLSX)
else()
    message(STATUS "OpenXLSX not found via package config; trying FetchContent.")
    include(FetchContent)
    FetchContent_Declare(
        openxlsx
        GIT_REPOSITORY https://github.com/troldal/OpenXLSX.git
        GIT_TAG v0.3.1
    )
    FetchContent_MakeAvailable(openxlsx)
    target_link_libraries(StudentDataExtractor PRIVATE OpenXLSX::OpenXLSX)
    target_link_libraries(workbook_inspector PRIVATE OpenXLSX::OpenXLSX)
endif()

if(WIN32)
    add_custom_command(TARGET StudentDataExtractor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/output/libOpenXLSX.dll"
                "$<TARGET_FILE_DIR:StudentDataExtractor>"
        COMMENT "Copying OpenXLSX runtime DLL next to StudentDataExtractor"
    )

    add_custom_command(TARGET StudentDataExtractor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "C:/mingw64/bin/libstdc++-6.dll"
                "$<TARGET_FILE_DIR:StudentDataExtractor>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "C:/mingw64/bin/libgcc_s_seh-1.dll"
                "$<TARGET_FILE_DIR:StudentDataExtractor>"
        COMMENT "Copying MinGW runtime DLLs next to StudentDataExtractor"
    )
endif()

enable_testing()
add_subdirectory(tests)
