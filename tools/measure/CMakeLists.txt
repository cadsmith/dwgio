add_executable(dwgio_measure measure.cpp)

target_link_libraries(dwgio_measure PRIVATE dwgio::dwgio)

target_compile_features(dwgio_measure PRIVATE cxx_std_23)
