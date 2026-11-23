include(CMakeFindDependencyMacro)
find_dependency(Threads)

include("${CMAKE_CURRENT_LIST_DIR}/dwgioTargets.cmake")

foreach(config IN ITEMS MINSIZEREL RELWITHDEBINFO)
  if (NOT DEFINED CMAKE_MAP_IMPORTED_CONFIG_${config})
    set_property(TARGET dwgio::dwgio
      PROPERTY MAP_IMPORTED_CONFIG_${config} Release Debug
    )
  endif()
endforeach()
