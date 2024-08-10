cmake_minimum_required (VERSION 3.30)
project(bookkeeper-client-jni CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra")

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif(CMAKE_BUILD_TYPE MATCHES DEBUG)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

include(FetchContent)
FetchContent_Declare(
    jni_bind
    GIT_REPOSITORY https://github.com/google/jni-bind.git
    GIT_TAG Release-1.1.0-beta
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)
FetchContent_MakeAvailable(jni_bind)

find_package(JNI)
if(NOT JNI_FOUND)
    message(SEND_ERROR "Could not find JNI")
endif(NOT JNI_FOUND)

file(GLOB SRC_FILES ./src/*.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE ./include ${JNI_INCLUDE_DIRS} ${jni_bind_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${JNI_LIBRARIES})

