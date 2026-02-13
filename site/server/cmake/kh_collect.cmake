# cmake/kh_collect.cmake
# The ONLY place that uses file(GLOB...) / file(GLOB_RECURSE...)

function(kh_collect_module_sources out_srcs out_hdrs)
    set(_srcs "")
    set(_hdrs "")

    foreach(_mod IN LISTS ARGN)
        # Standard layout: <module>/src and <module>/inc
        if(EXISTS "${_mod}/src")
            file(GLOB_RECURSE _mod_srcs
                "${_mod}/src/*.c"
                "${_mod}/src/*.cc"
                "${_mod}/src/*.cpp"
                "${_mod}/src/*.cxx"
            )
            list(APPEND _srcs ${_mod_srcs})

            # Optional: show headers found in src in IDEs
            file(GLOB_RECURSE _mod_src_hdrs
                "${_mod}/src/*.h"
                "${_mod}/src/*.hh"
                "${_mod}/src/*.hpp"
                "${_mod}/src/*.hxx"
            )
            list(APPEND _hdrs ${_mod_src_hdrs})
        endif()

        if(EXISTS "${_mod}/inc")
            file(GLOB_RECURSE _mod_hdrs
                "${_mod}/inc/*.h"
                "${_mod}/inc/*.hh"
                "${_mod}/inc/*.hpp"
                "${_mod}/inc/*.hxx"
            )
            list(APPEND _hdrs ${_mod_hdrs})
        endif()

        # Headers-only modules (e.g., interface/): headers at module root
        get_filename_component(_name "${_mod}" NAME)
        if(EXISTS "${_mod}" AND NOT EXISTS "${_mod}/src" AND NOT EXISTS "${_mod}/inc")
            file(GLOB_RECURSE _root_hdrs
                "${_mod}/*.h"
                "${_mod}/*.hh"
                "${_mod}/*.hpp"
                "${_mod}/*.hxx"
            )
            list(APPEND _hdrs ${_root_hdrs})
        endif()

        # Parser specs: include them for IDE visibility (not compiled directly)
        if(_name STREQUAL "parser")
            file(GLOB _parser_specs
                "${_mod}/*.y"
                "${_mod}/*.yy"
                "${_mod}/*.ypp"
                "${_mod}/*.l"
                "${_mod}/*.lex"
            )
            list(APPEND _hdrs ${_parser_specs})
        endif()
    endforeach()

    if(_srcs)
        list(REMOVE_DUPLICATES _srcs)
    endif()
    if(_hdrs)
        list(REMOVE_DUPLICATES _hdrs)
    endif()

    set(${out_srcs} "${_srcs}" PARENT_SCOPE)
    set(${out_hdrs} "${_hdrs}" PARENT_SCOPE)
endfunction()
