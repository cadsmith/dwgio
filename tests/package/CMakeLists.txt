cmake_minimum_required(VERSION 3.28)
project(dwgio_package LANGUAGES CXX)

find_package(dwgio CONFIG REQUIRED)

add_executable(dwgio_package main.cpp)

target_link_libraries(dwgio_package PRIVATE dwgio::dwgio)

target_compile_features(dwgio_package PRIVATE cxx_std_23)
