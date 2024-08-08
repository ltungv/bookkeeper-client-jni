cmake_minimum_required (VERSION 3.30)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(bookkeeper-client-jni CXX)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra")

if(NOT JAVA_HOME)
    message(SEND_ERROR "JAVA_HOME is not found")
endif(NOT JAVA_HOME)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif(CMAKE_BUILD_TYPE MATCHES DEBUG)

set(LIBS jvm)

file(GLOB SRC_FILES ./src/*.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ./include ${JAVA_HOME}/include ${JAVA_HOME}/include/linux)
target_link_directories(${PROJECT_NAME} PRIVATE ${JAVA_HOME}/lib ${JAVA_HOME}/lib/server)
target_link_libraries(${PROJECT_NAME} ${LIBS})

file(GLOB TEST_FILES ./test/*.cpp)
foreach(file ${TEST_FILES})
    get_filename_component(name ${file} NAME_WE)
    message("Found test: ${name}")
    set(target "${PROJECT_NAME}_test_${name}")

    add_executable(${target} ./catch/main.cpp ${file})
    target_link_libraries(${target} ${PROJECT_NAME}_lib ${LIBS})
    target_include_directories(${target} PRIVATE ./catch ./include)
endforeach()



