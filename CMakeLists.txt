cmake_minimum_required(VERSION 3.20)
project(progetto_computer_grafica)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(GL_SILENCE_DEPRECATION)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

file(GLOB_RECURSE COMMON_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/*
)

file(GLOB SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*
)

add_executable(progetto_computer_grafica
    main_carousel.cpp
)

target_sources(progetto_computer_grafica PRIVATE
    ${COMMON_FILES}
    ${SHADER_FILES}
)

target_include_directories(progetto_computer_grafica PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/3dparty
)

set_property(TARGET progetto_computer_grafica PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

target_link_libraries(progetto_computer_grafica
    PRIVATE
        glfw
        glm::glm-header-only
        OpenGL::GL
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/common PREFIX "common" FILES ${COMMON_FILES})
source_group("shaders" FILES ${SHADER_FILES})