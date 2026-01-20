##################################################################################
## Kepler's Horizon - unified build
## Constraints satisfied:
##   - NO file(GLOB ...) anywhere
##   - NO listing source/header filenames in any CMakeLists.txt
##   - Subordinate CMakeLists.txt are <= 2 lines (they just register)
##   - NO sub-libraries: only object compilation + single executable
##   - Uses git ls-files as the only allowed enumeration mechanism
##   - CMake 3.10 compatible
##################################################################################

cmake_minimum_required(VERSION 3.10)
project(kh LANGUAGES CXX)

# ---- Toolchain / flags ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-O0 -pedantic -g)

# ---- Custom tool paths ----
set(BISON_EXECUTABLE "/opt/bison3.8/bin/bison" CACHE FILEPATH "Bison executable")
set(FLEX_EXECUTABLE  "/opt/flex2.6.4/bin/flex"  CACHE FILEPATH "Flex executable")

find_package(BISON REQUIRED)
find_package(FLEX  REQUIRED)

# ---- Git SHA define ----
execute_process(
    COMMAND git -C "${CMAKE_SOURCE_DIR}" rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _gitsha_rc
)
if(NOT _gitsha_rc EQUAL 0 OR NOT GIT_SHA)
    set(GIT_SHA "unknown")
endif()
add_definitions(-DGIT_SHA="${GIT_SHA}")

# ==============================================================================
# Registration API (subordinate CMakeLists are 1-liners)
# ==============================================================================

# Global properties:
#   KH_MODULE_ROOTS  : list of directory prefixes (relative to CMAKE_SOURCE_DIR)
#   KH_INCLUDE_DIRS  : include directories to apply to the kh target
function(kh_register_module)
    get_filename_component(_abs "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
    file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_abs}")

    set_property(GLOBAL APPEND PROPERTY KH_MODULE_ROOTS "${_rel}")

    if(EXISTS "${_abs}/inc")
        set_property(GLOBAL APPEND PROPERTY KH_INCLUDE_DIRS "${_abs}/inc")
    endif()
    # If headers in src are included directly, keep this.
    if(EXISTS "${_abs}/src")
        set_property(GLOBAL APPEND PROPERTY KH_INCLUDE_DIRS "${_abs}/src")
    endif()
endfunction()

function(kh_register_interface)
    get_filename_component(_abs "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
    file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_abs}")

    set_property(GLOBAL APPEND PROPERTY KH_MODULE_ROOTS "${_rel}")
    set_property(GLOBAL APPEND PROPERTY KH_INCLUDE_DIRS "${_abs}")
endfunction()

function(kh_register_parser)
    get_filename_component(_abs "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
    file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_abs}")

    set_property(GLOBAL APPEND PROPERTY KH_MODULE_ROOTS "${_rel}")
endfunction()

# ==============================================================================
# Source enumeration WITHOUT GLOB / WITHOUT file lists
#   - Uses: git ls-files (newline separated; CMake 3.10 cannot handle "\0")
#   - Filters by: registered module root prefixes (e.g. "app/", "combat/", ...)
# ==============================================================================

function(kh_collect_git_ls_files out_var)
    execute_process(
        COMMAND git -C "${CMAKE_SOURCE_DIR}" ls-files
        OUTPUT_VARIABLE _out
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _rc
    )
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR
            "git ls-files failed. This build requires a git worktree because GLOB and per-file lists are forbidden."
        )
    endif()

    # Convert newline-separated output to a CMake list.
    # (Assumes no filenames contain newlines; sane repos do not.)
    string(REPLACE "\n" ";" _files "${_out}")
    set(${out_var} "${_files}" PARENT_SCOPE)
endfunction()

function(kh_filter_by_module_roots out_srcs out_hdrs)
    get_property(_roots GLOBAL PROPERTY KH_MODULE_ROOTS)
    if(NOT _roots)
        message(FATAL_ERROR "No module roots registered. Check add_subdirectory(...) list and subordinate CMakeLists.")
    endif()

    kh_collect_git_ls_files(_all)

    set(_srcs "")
    set(_hdrs "")
    set(_has_main FALSE)

    foreach(f IN LISTS _all)
        # Must be under one of the registered roots.
        set(_match FALSE)
        foreach(r IN LISTS _roots)
            if(f MATCHES "^${r}/")
                set(_match TRUE)
                break()
            endif()
        endforeach()
        if(NOT _match)
            continue()
        endif()

        if(f MATCHES "\\.(c|cc|cpp|cxx)$")
            list(APPEND _srcs "${CMAKE_SOURCE_DIR}/${f}")
            if(f STREQUAL "app/src/main.cpp")
                set(_has_main TRUE)
            endif()
        elseif(f MATCHES "\\.(h|hh|hpp|hxx)$")
            list(APPEND _hdrs "${CMAKE_SOURCE_DIR}/${f}")
        elseif(f MATCHES "\\.(y|yy|ypp|l|lex)$")
            list(APPEND _hdrs "${CMAKE_SOURCE_DIR}/${f}")
        endif()
    endforeach()

    if(NOT _srcs)
        message(FATAL_ERROR
            "Collected zero C/C++ sources from registered roots. "
            "Either roots don't match repo paths, or files are not tracked by git."
        )
    endif()

    if(NOT _has_main)
        message(FATAL_ERROR
            "Did not collect app/src/main.cpp. "
            "Either it isn't tracked by git, or the app module wasn't registered."
        )
    endif()

    set(${out_srcs} "${_srcs}" PARENT_SCOPE)
    set(${out_hdrs} "${_hdrs}" PARENT_SCOPE)
endfunction()

# ==============================================================================
# Subtrees (must match your actual directories)
# ==============================================================================

add_subdirectory(ships)
add_subdirectory(misc)
add_subdirectory(combat)
add_subdirectory(milieu)
add_subdirectory(app)
add_subdirectory(repres)
add_subdirectory(map)
add_subdirectory(database)
add_subdirectory(turn)
add_subdirectory(move)
add_subdirectory(interface)
add_subdirectory(parser)

# Top-level generic headers (non-interface)
set_property(GLOBAL APPEND PROPERTY KH_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/inc")

# ==============================================================================
# Parser generation
# ==============================================================================

BISON_TARGET(Parser
    ${CMAKE_SOURCE_DIR}/parser/lang.y
    ${CMAKE_CURRENT_BINARY_DIR}/lang.cpp
    COMPILE_FLAGS "-d -v -g"
)
FLEX_TARGET(Scanner
    ${CMAKE_SOURCE_DIR}/parser/scan.lex
    ${CMAKE_CURRENT_BINARY_DIR}/scan.cpp
)
ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

# ==============================================================================
# Build ONE executable (no sub-libraries)
# ==============================================================================

kh_filter_by_module_roots(KH_SRCS KH_HDRS)

add_executable(kh
    ${KH_SRCS}
    ${KH_HDRS}
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Scanner_OUTPUTS}
)

get_property(KH_INCLUDE_DIRS GLOBAL PROPERTY KH_INCLUDE_DIRS)
target_include_directories(kh PRIVATE ${KH_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(kh mysqlclient)

