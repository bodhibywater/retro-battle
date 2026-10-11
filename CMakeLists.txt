cmake_minimum_required(VERSION 3.25)

project(
    retro_battle
    VERSION 0.1.0
    LANGUAGES CXX
)

include(CTest)

find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(boost_asio CONFIG REQUIRED)

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
        src/shared/world/tile_map.cpp
        src/shared/world/movement.cpp
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
        Boost::asio
)

retro_enable_warnings(retro_server)

if(BUILD_TESTING)
    find_package(Catch2 3 CONFIG REQUIRED)

    add_executable(
        retro_tests
        tests/version_test.cpp
        tests/world_test.cpp
    )

    target_link_libraries(
        retro_tests
        PRIVATE
            retro_shared
            Catch2::Catch2WithMain
    )

    retro_enable_warnings(retro_tests)

    include(Catch)
    catch_discover_tests(retro_tests)
endif()
