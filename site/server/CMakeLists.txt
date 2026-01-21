cmake_minimum_required(VERSION 3.10)
project(kh LANGUAGES CXX)

# ---- Toolchain / flags ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-O0 -pedantic -g)

# ---- Custom tool paths (preserved from your prior working file) ----
set(BISON_EXECUTABLE "/opt/bison3.8/bin/bison" CACHE FILEPATH "Bison executable")
set(FLEX_EXECUTABLE  "/opt/flex2.6.4/bin/flex"  CACHE FILEPATH "Flex executable")

find_package(BISON REQUIRED)
find_package(FLEX  REQUIRED)

# ---- Helper that is allowed to use GLOB ----
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(kh_collect)

# =============================================================================
# Registration API (subordinate CMakeLists are 1-liners)
# =============================================================================

# Global properties:
#   KH_MODULE_DIRS   : absolute module directories (one per add_subdirectory)
#   KH_INCLUDE_DIRS  : include directories to apply to the kh target
function(kh_register_module_cpp)
    get_filename_component(_abs "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
    set_property(GLOBAL APPEND PROPERTY KH_MODULE_DIRS "${_abs}")

    if(EXISTS "${_abs}/inc")
        set_property(GLOBAL APPEND PROPERTY KH_INCLUDE_DIRS "${_abs}/inc")
    endif()
    if(EXISTS "${_abs}/src")
        set_property(GLOBAL APPEND PROPERTY KH_INCLUDE_DIRS "${_abs}/src")
    endif()
endfunction()

# Headers live directly in the module directory (no src/ or inc/)
function(kh_register_headers_only)
    get_filename_component(_abs "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
    set_property(GLOBAL APPEND PROPERTY KH_MODULE_DIRS "${_abs}")
    set_property(GLOBAL APPEND PROPERTY KH_INCLUDE_DIRS "${_abs}")
endfunction()

# Parser dir only contains lex/yacc specs; no include dirs implied here
function(kh_register_parser)
    get_filename_component(_abs "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
    set_property(GLOBAL APPEND PROPERTY KH_MODULE_DIRS "${_abs}")
endfunction()

# =============================================================================
# Subtrees (match your directory structure)
# =============================================================================
add_subdirectory(app)
add_subdirectory(combat)
add_subdirectory(database)
add_subdirectory(interface)
add_subdirectory(map)
add_subdirectory(milieu)
add_subdirectory(misc)
add_subdirectory(move)
add_subdirectory(parser)
add_subdirectory(repres)
add_subdirectory(ships)
add_subdirectory(turn)

# Top-level generic headers (non-interface)
if(EXISTS "${CMAKE_SOURCE_DIR}/inc")
    set_property(GLOBAL APPEND PROPERTY KH_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/inc")
endif()

# =============================================================================
# Collect sources/headers (GLOB is confined to cmake/kh_collect.cmake)
# =============================================================================
get_property(_mods GLOBAL PROPERTY KH_MODULE_DIRS)
if(NOT _mods)
    message(FATAL_ERROR "No modules registered. Check subordinate CMakeLists.txt files.")
endif()

kh_collect_module_sources(KH_SRCS KH_HDRS ${_mods})

# =============================================================================
# Parser generation (preserved: outputs, flags, dependency)
# =============================================================================
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

# =============================================================================
# Build ONE executable (no sub-libraries)
# =============================================================================
add_executable(kh
    ${KH_SRCS}
    ${KH_HDRS}
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Scanner_OUTPUTS}
)

get_property(KH_INCLUDE_DIRS GLOBAL PROPERTY KH_INCLUDE_DIRS)
target_include_directories(kh PRIVATE
    ${KH_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}  # for bison-generated headers
)

# Preserved from your prior working file
target_link_libraries(kh PRIVATE mysqlclient)
