cmake_minimum_required(VERSION 3.13)
project(indi_altaz_arduino CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(INDI REQUIRED libindi)

add_executable(indi_altaz_arduino indi_altaz_arduino.cpp)
target_include_directories(indi_altaz_arduino PRIVATE ${INDI_INCLUDE_DIRS})
target_compile_options(indi_altaz_arduino PRIVATE ${INDI_CFLAGS_OTHER})
target_link_directories(indi_altaz_arduino PRIVATE ${INDI_LIBRARY_DIRS})
target_link_libraries(indi_altaz_arduino PRIVATE indidriver nova)

install(TARGETS indi_altaz_arduino DESTINATION bin)
