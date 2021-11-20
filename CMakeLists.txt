cmake_minimum_required(VERSION 3.0.0)
project(opengl_basics VERSION 0.1.0)
set(CMAKE_BUILD_TYPE Release)

include(CTest)
enable_testing()

include_directories("C:/GameProjects/Libraries/glad/include")
include_directories("C:/GameProjects/Libraries/glfw/include")
include_directories("C:/GameProjects/Libraries/fmt/include")
include_directories("C:/GameProjects/opengl_basics/imgui")
include_directories("C:/GameProjects/Libraries/glm")
add_executable(opengl_basics engine.cpp)
target_link_libraries(opengl_basics PRIVATE "C:/GameProjects/Libraries/glfw/lib-vc2019/glfw3.lib")

STRING (REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
STRING (REGEX REPLACE "/W3" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /O2 /EHsc /arch:AVX")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /O2")
get_target_property(MAIN_CFLAGS opengl_basics COMPILE_OPTIONS)
# also see: COMPILE_DEFINITIONS INCLUDE_DIRECTORIES
message("-- Target compiler flags are: ${CMAKE_CXX_FLAGS}")

add_custom_command(TARGET opengl_basics POST_BUILD
COMMAND echo built with the flags: ${CMAKE_CXX_FLAGS})
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
