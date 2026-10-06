cmake_minimum_required(VERSION 3.25)

project(
    retro_battle
    VERSION 0.1.0
    LANGUAGES CXX
)

include(CTest)

find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

function(retro_enable_warnings target)
    if(MSVC)
        target_compile_options(
            ${target}
            PRIVATE
                /W4
                /permissive-
        )
    else()
        target_compile_options(
            ${target}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
        )
    endif()
endfunction()

add_library(
    retro_shared
    STATIC
        src/shared/version.cpp
)

target_include_directories(
    retro_shared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shared/include
)

target_compile_features(
    retro_shared
    PUBLIC
        cxx_std_20
)

retro_enable_warnings(retro_shared)

add_executable(
    retro_client
    src/client/main.cpp
)

target_link_libraries(
    retro_client
    PRIVATE
        retro_shared
        raylib
        glfw
)

retro_enable_warnings(retro_client)

add_executable(
    retro_server
    src/server/main.cpp
)

target_link_libraries(
    retro_server
    PRIVATE
        retro_shared
)

retro_enable_warnings(retro_server)

if(BUILD_TESTING)
    add_executable(
        retro_tests
        tests/version_test.cpp
    )

    target_link_libraries(
        retro_tests
        PRIVATE
            retro_shared
    )

    retro_enable_warnings(retro_tests)

    add_test(
        NAME retro_tests
        COMMAND retro_tests
    )
endif()