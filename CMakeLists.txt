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

    add_custom_command(TARGET StudentDataExtractorGui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/output/libOpenXLSX.dll"
                "$<TARGET_FILE_DIR:StudentDataExtractorGui>"
        COMMENT "Copying OpenXLSX runtime DLL next to StudentDataExtractorGui"
    )

    if(MINGW)
        get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
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

if(WIN32)
    install(TARGETS StudentDataExtractorGui
        RUNTIME DESTINATION .
        COMPONENT Application
    )

    install(FILES
        "${CMAKE_BINARY_DIR}/output/libOpenXLSX.dll"
        "${MINGW_BIN_DIR}/libstdc++-6.dll"
        "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
        DESTINATION .
        COMPONENT Application
    )

    set(CPACK_PACKAGE_NAME "StudentDataExtractor")
    set(CPACK_PACKAGE_VENDOR "Student Data Extractor")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Student Data Extractor desktop application")
    set(CPACK_PACKAGE_VERSION "1.0.0")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Student Data Extractor")
    set(CPACK_PACKAGE_FILE_NAME "StudentDataExtractor-Setup")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    set(CPACK_NSIS_DISPLAY_NAME "Student Data Extractor")
    set(CPACK_NSIS_PACKAGE_NAME "Student Data Extractor")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_MENU_LINKS
        "StudentDataExtractorGui.exe" "Student Data Extractor"
    )
    set(CPACK_CREATE_DESKTOP_LINKS StudentDataExtractorGui)
    set(CPACK_COMPONENTS_ALL Application)
    set(CPACK_GENERATOR "NSIS")
    include(CPack)
endif()

target_include_directories(StudentDataExtractor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(workbook_inspector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(OpenXLSX CONFIG QUIET)
if(OpenXLSX_FOUND)
    target_link_libraries(StudentDataExtractor PRIVATE OpenXLSX::OpenXLSX)
    if(WIN32)
        target_link_libraries(StudentDataExtractorGui PRIVATE OpenXLSX::OpenXLSX)
    endif()
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
    if(WIN32)
        target_link_libraries(StudentDataExtractorGui PRIVATE OpenXLSX::OpenXLSX)
    endif()
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
