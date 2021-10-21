if(NOT ${CMAKE_VERSION} VERSION_LESS 3.13)
    # add_link_options requires at least cmake 3.13
    if(NOT VISTLE_LINKER)
        set(VISTLE_LINKER
            "Default"
            CACHE STRING "Configure linker" FORCE)
    endif()
    # Set the possible values for cmake-gui
    set_property(CACHE VISTLE_LINKER PROPERTY STRINGS "Default" "bfd" "gold" "lld")
    if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
        if(VISTLE_LINKER AND NOT VISTLE_LINKER STREQUAL "Default")
            add_link_options(-fuse-ld=${VISTLE_LINKER})
        endif()
        add_link_options(-flto)
    endif()
endif()
