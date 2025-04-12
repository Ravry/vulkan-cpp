cmake_minimum_required(VERSION 3.10)

project(vulkan_cpp_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw")
set(UTILS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/utils")

find_package(Vulkan REQUIRED)

if (WIN32)
add_custom_target(build_shaders ALL
    COMMAND "${PROJECT_SOURCE_DIR}/shaders/compile.bat"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/shaders"
)
endif(WIN32)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
add_dependencies(${PROJECT_NAME} build_shaders)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src/headers)
target_include_directories(${PROJECT_NAME} PUBLIC ${UTILS_DIR})

target_include_directories(${PROJECT_NAME} PUBLIC ${GLFW_DIR}/include)
target_link_directories(${PROJECT_NAME} PUBLIC ${GLFW_DIR}/lib-mingw-w64)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw3)

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES})