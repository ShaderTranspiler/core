cmake_minimum_required(VERSION 3.14)
project(ShaderTranspilerCore)
option(NO_CATCH "Skip downloading and building Catch2 and the unit tests (testing won't be available)" OFF)

# make sure julia executable is available
find_program(JULIA_EXEC julia)
if (NOT JULIA_EXEC)
    message(FATAL_ERROR "Couldn't locate Julia executable (needed for locating libjulia). Please ensure Julia is installed and added to PATH.")
endif()

# find and include Julia lib and headers
execute_process(
    COMMAND ${JULIA_EXEC} -e "print(normpath(joinpath(Sys.BINDIR, \"..\", \"include\", \"julia\")))"
    OUTPUT_VARIABLE JULIA_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${JULIA_EXEC} -e "using Libdl; print(Libdl.dlpath(\"libjulia\"))"
    OUTPUT_VARIABLE JULIA_LIB
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# on windows we can't link the dll itself
# on linux and mac, the file pointed to by libjulia should work
if (WIN32)
    get_filename_component(JULIA_BINDIR ${JULIA_LIB} DIRECTORY)
    set(JULIA_LIBDIR "${JULIA_BINDIR}/../lib")
    get_filename_component(JULIA_LIBDIR ${JULIA_LIBDIR} ABSOLUTE)

    set(JULIA_LIB "${JULIA_LIBDIR}/libjulia.dll.a")
    if (NOT EXISTS "${JULIA_LIB}")
        set(JULIA_LIB "${JULIA_LIBDIR}/julia.lib")
    endif()

    if (NOT EXISTS "${JULIA_LIB}")
        message(FATAL_ERROR "Couldn't locate Julia library (libjulia.dll.a or julia.lib)")
    endif()
endif()

message(STATUS "Found Julia include dir: ${JULIA_INCLUDE_DIR}")
message(STATUS "Found Julia library: ${JULIA_LIB}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LIB_OUTPUT_DIR ${CMAKE_BINARY_DIR}/lib)

file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS "src/*.cpp" "include/*.h" "sandbox/*.cpp" "sandbox/*.h" "tests/*.cpp" "tests/*.h")

# ccache

find_program(CCACHE_EXEC ccache)
if (CCACHE_EXEC)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXEC}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_EXEC}")

    message(STATUS "ccache found and enabled for compiler caching")
else()
    message(WARNING "ccache not found, compiler caching won't be possible")
endif()

# clang-format

find_program(CLANG_FORMAT_EXEC NAMES clang-format)
if (CLANG_FORMAT_EXEC)
    # style checking
    add_custom_target(
        check-format
        COMMAND ${CLANG_FORMAT_EXEC}
                -style=file
                -n
                --Werror
                ${ALL_SOURCES}
        COMMENT "Checking C++ code style..."
    )

    # automatic style enforcement
    add_custom_target(
        fix-format
        COMMAND ${CLANG_FORMAT_EXEC}
                -style=file
                -i
                ${ALL_SOURCES}
        COMMENT "Fixing C++ code style..."
    )

    message(STATUS "clang-format found, check-format and fix-format targets enabled")
else()
    message(WARNING "clang-format could not be located, code style checking won't be possible.")
endif()

# clang-tidy

find_program(CLANG_TIDY_EXEC clang-tidy)
if (CLANG_TIDY_EXEC)
    set(CLANG_TIDY_CMD "${CLANG_TIDY_EXEC}" "-extra-arg=-Wno-unknown-warning-option")    
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_CMD})
    
    message(STATUS "clang-tidy found and enabled")
else()
    message(WARNING "clang-tidy could not be located, static analysis won't be possible.")
endif()

add_library(CommonConfig INTERFACE)

# extra safety options
if (MSVC)
    target_compile_options(CommonConfig INTERFACE /W4)
else()
    target_compile_options(CommonConfig INTERFACE
        -Wall -Wextra -Wpedantic -Wconversion
    )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # enable asan (and force disable incompatible options on MSVC)
    if (MSVC)
        target_compile_options(CommonConfig INTERFACE /fsanitize=address)

        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        
        string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string(REPLACE "/ZI" "/Zi" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

        target_link_options(CommonConfig INTERFACE "/INCREMENTAL:NO")
    elseif(UNIX)
        target_compile_options(CommonConfig INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer
        )

        target_link_options(CommonConfig INTERFACE -fsanitize=address,undefined)
    endif()
endif()

set(LIB_SOURCES ${ALL_SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX "sandbox/.*")
list(FILTER LIB_SOURCES EXCLUDE REGEX "tests/.*")

add_library(ShaderTranspilerCore SHARED ${LIB_SOURCES})
target_link_libraries(ShaderTranspilerCore PRIVATE CommonConfig)

target_include_directories(ShaderTranspilerCore PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(ShaderTranspilerCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(ShaderTranspilerCore PRIVATE ${JULIA_INCLUDE_DIR})

target_link_libraries(ShaderTranspilerCore PRIVATE ${JULIA_LIB})

target_compile_definitions(ShaderTranspilerCore PRIVATE JULIA_ENABLE_THREADING=1)

set_target_properties(ShaderTranspilerCore PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY                ${LIB_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY                ${LIB_OUTPUT_DIR}
    
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          ${LIB_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        ${LIB_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${LIB_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     ${LIB_OUTPUT_DIR}

    ARCHIVE_OUTPUT_DIRECTORY_DEBUG          ${LIB_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE        ${LIB_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${LIB_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL     ${LIB_OUTPUT_DIR}
)

# Catch2

if (NOT NO_CATCH)
    message(STATUS "tests are enabled, Catch2 will be downloaded and built, tests target will be enabled")
    message(STATUS "use -DNO_CATCH=ON to skip test-related builds")

    set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/cmake_deps_${CMAKE_SYSTEM_NAME}")

    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.12.0
    )

    FetchContent_MakeAvailable(Catch2)

    if (TARGET Catch2::Catch2WithMain)
        add_executable(tests tests/run.cpp)
        target_link_libraries(tests PRIVATE ShaderTranspilerCore Catch2::Catch2WithMain CommonConfig)
        target_include_directories(tests PRIVATE include)

        # copy lib for test runner
        add_custom_command(TARGET tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:ShaderTranspilerCore>
            $<TARGET_FILE_DIR:tests>
        )

        # CTest
        include(CTest)
        enable_testing()
        add_test(NAME UnitTests COMMAND tests)

        message(STATUS "catch2 found, tests target enabled for unit testing")
    else()
        message(WARNING "catch2 could not be located, unit testing won't be possible")
    endif()
else()
    message(STATUS "Skipping Catch2 build and tests target (NO_CATCH=ON)")
endif()

# Doxygen (+ Graphviz)
find_package(Doxygen COMPONENTS dot)
if (Doxygen_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")

    set(DOXYGEN_GENERATE_HTML YES)

    set(DOXYGEN_GENERATE_LATEX YES)
    set(DOXYGEN_USE_PDFLATEX YES)
    set(DOXYGEN_PDF_HYPERLINKS YES)

    set(DOXYGEN_RECURSIVE YES)
    
    # Graphviz
    set(DOXYGEN_HAVE_DOT YES)
    set(DOXYGEN_DOT_IMAGE_FORMAT svg)
    set(DOXYGEN_DOT_TRANSPARENT YES)
    set(DOXYGEN_CALL_GRAPH YES)
    set(DOXYGEN_CALLER_GRAPH YES)
    set(DOXYGEN_DIRECTORY_GRAPH YES)

    set(DOXYGEN_UML_LOOK YES)
    set(DOXYGEN_TEMPLATE_RELATIONS YES)
    set(DOXYGEN_INCLUDE_GRAPH YES)
    set(DOXYGEN_INTERACTIVE_SVG YES)

    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_EXTRACT_STATIC YES)

    doxygen_add_docs(
        docs
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        COMMENT "Generating HTML and LaTeX docs with doxygen..."
    )

    message(STATUS "doxygen and graphviz found, docs target enabled")

    find_program(PDFLATEX_COMPILER pdflatex)

    if (PDFLATEX_COMPILER)
        if (WIN32)
            set(MAKE_CMD "make.bat")
        else()
            set(MAKE_CMD "make")
        endif()

        add_custom_command(TARGET docs POST_BUILD
            COMMAND ${MAKE_CMD}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs/latex"
            COMMENT "Compiling LaTeX docs to PDF..."
            VERBATIM
        )
        message(STATUS "pdflatex found, automatic LaTeX -> PDF docs generation enabled for docs target")
    else()
        message(WARNING "pdflatex could not be located, PDF generation from LaTeX docs won't be possible.")
    endif()
else()
    message(WARNING "doxygen or graphviz could not be located, docs generation won't be possible.")
endif()

# sandbox targets
# for trying out and playing around with the library
# cpp files prefixed with run_ are treated as separate targets
# other cpp files can be used for includes and such

file(GLOB SANDBOX_SOURCES "sandbox/run_*.cpp")

foreach (SBOX_FILE ${SANDBOX_SOURCES})
    get_filename_component(FNAME ${SBOX_FILE} NAME_WE)

    add_executable(${FNAME} ${SBOX_FILE})

    target_link_libraries(${FNAME} PRIVATE ShaderTranspilerCore CommonConfig)

    target_include_directories(${FNAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sandbox)
    target_include_directories(${FNAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    # copy lib next to sandbox executable
    add_custom_command(TARGET ${FNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ShaderTranspilerCore>
        $<TARGET_FILE_DIR:${FNAME}>
    )
endforeach()
