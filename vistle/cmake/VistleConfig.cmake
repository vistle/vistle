#Use this tho find vistle components
#e.g. with: find_package(Vistle REQUIRED COMPONENTS sensei sensei_vtk boost_mpi)
macro(vistle_find_component comp req)

    set(_VISTLE_REQUIRED)
    if(${req} AND Vistle_FIND_REQUIRED)
        set(_VISTLE_REQUIRED REQUIRED)
    endif()

    find_package(vistle_${comp} CONFIG ${_VISTLE_REQUIRED} HINTS ${CMAKE_CURRENT_LIST_DIR})

    set(__vistle_comp_found ${vistle_${comp}_FOUND})

    # FindPackageHandleStandardArgs expects <package>_<component>_FOUND
    set(Vistle_${comp}_FOUND ${__vistle_comp_found})

    # FindVistle sets Vistle_<COMPONENT>_FOUND
    string(TOUPPER ${comp} _VISTLE_COMP)
    set(Vistle_${_VISTLE_COMP}_FOUND ${__vistle_comp_found})

    unset(_VISTLE_REQUIRED)
    unset(__vistle_comp_found)
    unset(_VISTLE_COMP)

endmacro()

foreach(__vistle_comp IN LISTS Vistle_FIND_COMPONENTS)

    vistle_find_component(${__vistle_comp} ${Vistle_FIND_REQUIRED_${__vistle_comp}})

endforeach()
