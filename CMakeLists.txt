cmake_minimum_required(VERSION 3.11)
project(shift_k)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(CRITERION REQUIRED criterion)

include_directories(include)

add_executable(shift_k_prog src/shift_k.c src/main.c)
add_executable(test)

target_compile_options(test PRIVATE ${CRITERION_CFLAGS})
target_link_libraries(test PRIVATE ${CRITERION_LIBRARIES})
