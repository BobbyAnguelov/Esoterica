cmake_minimum_required(VERSION 3.22)

project(ufbx)
add_executable(ufbx ../test_build.cpp)

set_property(TARGET ufbx PROPERTY COMPILE_WARNING_AS_ERROR ON)
if(MSVC)
  target_compile_options(ufbx PRIVATE /W4 /WX)
endif()
