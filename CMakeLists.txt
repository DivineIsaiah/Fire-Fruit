cmake_minimum_required(VERSION 3.16)

project(StudentDataExtractor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MINGW OR UNIX)
    add_compile_options(-include cstdint)
endif()

# ---------------------------------------------------------
# Main console application
# ---------------------------------------------------------

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

# ---------------------------------------------------------
# Workbook inspector
# ---------------------------------------------------------

add_executable(workbook_inspector
    src/debug/WorkbookInspector.cpp
)

# ---------------------------------------------------------
# Windows GUI application
# ---------------------------------------------------------

if(WIN32)

    add_executable(StudentDataExtractorGui WIN32
        src/gui/GuiMain.cpp
        src/app/Application.cpp
        src/filesystem/DirectoryBrowser.cpp
        src/filesystem/FileSearcher.cpp
        src/excel/ExcelWriter.cpp
        src/extraction/StudentExtractor.cpp
        src/extraction/ColumnMatcher.cpp
        src/utils/StringUtils.cpp
        src/utils/Logger.cpp
    )

    target_include_directories(StudentDataExtractorGui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(StudentDataExtractorGui PRIVATE
        comctl32
        shell32
    )

endif()

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------

target_include_directories(StudentDataExtractor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(workbook_inspector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------------------------------------
# OpenXLSX
# ---------------------------------------------------------

find_package(OpenXLSX CONFIG QUIET)

if(OpenXLSX_FOUND)

    target_link_libraries(StudentDataExtractor PRIVATE
        OpenXLSX::OpenXLSX
    )

    if(WIN32)
        target_link_libraries(StudentDataExtractorGui PRIVATE
            OpenXLSX::OpenXLSX
        )
    endif()

    target_link_libraries(workbook_inspector PRIVATE
        OpenXLSX::OpenXLSX
    )

else()

    message(STATUS "OpenXLSX not found via package config; trying FetchContent.")

    include(FetchContent)

    set(OPENXLSX_CREATE_DOCS OFF CACHE BOOL "" FORCE)
    set(OPENXLSX_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(OPENXLSX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(OPENXLSX_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        openxlsx
        GIT_REPOSITORY https://github.com/troldal/OpenXLSX.git
        GIT_TAG v0.3.1
    )

    FetchContent_MakeAvailable(openxlsx)

    target_link_libraries(StudentDataExtractor PRIVATE
        OpenXLSX::OpenXLSX
    )

    if(WIN32)
        target_link_libraries(StudentDataExtractorGui PRIVATE
            OpenXLSX::OpenXLSX
        )
    endif()

    target_link_libraries(workbook_inspector PRIVATE
        OpenXLSX::OpenXLSX
    )

endif()

# ---------------------------------------------------------
# Runtime DLLs for Windows
# ---------------------------------------------------------

if(WIN32)

    if(MINGW)

        get_filename_component(MINGW_BIN_DIR
            "${CMAKE_CXX_COMPILER}"
            DIRECTORY
        )

        add_custom_command(TARGET StudentDataExtractor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MINGW_BIN_DIR}/libstdc++-6.dll"
                    "$<TARGET_FILE_DIR:StudentDataExtractor>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
                    "$<TARGET_FILE_DIR:StudentDataExtractor>"
            COMMENT "Copying MinGW runtime DLLs next to StudentDataExtractor"
        )

        add_custom_command(TARGET StudentDataExtractorGui POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MINGW_BIN_DIR}/libstdc++-6.dll"
                    "$<TARGET_FILE_DIR:StudentDataExtractorGui>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
                    "$<TARGET_FILE_DIR:StudentDataExtractorGui>"
            COMMENT "Copying MinGW runtime DLLs next to StudentDataExtractorGui"
        )

    endif()

endif()

# ---------------------------------------------------------
# Tests
# ---------------------------------------------------------

enable_testing()

add_subdirectory(tests)

# ---------------------------------------------------------
# Installation
# ---------------------------------------------------------

if(WIN32)

    install(TARGETS StudentDataExtractorGui
        RUNTIME DESTINATION .
        COMPONENT Runtime
    )

    # OpenXLSX runtime DLL
    install(FILES
        "${CMAKE_BINARY_DIR}/output/libOpenXLSX.dll"
        DESTINATION .
        COMPONENT Runtime
    )

    if(MINGW)

        get_filename_component(MINGW_BIN_DIR
            "${CMAKE_CXX_COMPILER}"
            DIRECTORY
        )

        install(FILES
            "${MINGW_BIN_DIR}/libstdc++-6.dll"
            "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
            DESTINATION .
            COMPONENT Runtime
        )

    endif()

endif()
   

# ---------------------------------------------------------
# CPack / NSIS installer
# ---------------------------------------------------------

set(CPACK_GENERATOR "NSIS")

set(CPACK_PACKAGE_NAME "StudentDataExtractor")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Student Data Extractor")

set(CPACK_NSIS_DISPLAY_NAME "Student Data Extractor")
set(CPACK_NSIS_PACKAGE_NAME "Student Data Extractor")

set(CPACK_NSIS_CREATE_DESKTOP_LINK
    StudentDataExtractorGui.exe
)

# Only package our application runtime component.
set(CPACK_COMPONENTS_ALL Runtime)

include(CPack)
