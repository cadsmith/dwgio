add_executable(dwgio_bare bare.cpp)

target_link_libraries(dwgio_bare PRIVATE dwgio::dwgio)

target_compile_features(dwgio_bare PRIVATE cxx_std_23)
