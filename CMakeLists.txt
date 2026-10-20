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
find_package(nlohmann_json CONFIG REQUIRED)

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
        src/shared/world/sample_map.cpp
        src/shared/world/player_state.cpp
        src/shared/net/protocol.cpp
        src/shared/net/framing.cpp
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

target_link_libraries(
    retro_shared
    PUBLIC nlohmann_json::nlohmann_json
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

# Always stage artwork when building retro_client, even when building that
# target directly. Ninja presets put the executable in CMAKE_CURRENT_BINARY_DIR.
add_custom_target(
    copy_overworld_assets
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/public/overworld"
        "${CMAKE_CURRENT_BINARY_DIR}/assets/overworld"
    COMMENT "Copying overworld assets"
    VERBATIM
)
add_dependencies(retro_client copy_overworld_assets)

add_executable(
    retro_server
    src/server/main.cpp
    src/server/server.cpp
    src/server/world_state.cpp
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
        tests/town_test.cpp
        tests/player_state_test.cpp
        tests/protocol_test.cpp
        tests/server_world_test.cpp
        src/server/world_state.cpp
    )

    target_link_libraries(
        retro_tests
        PRIVATE
            retro_shared
            Catch2::Catch2WithMain
    )

    target_include_directories(
        retro_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/server
    )

    retro_enable_warnings(retro_tests)

    include(Catch)
    catch_discover_tests(retro_tests)

    # Headless TCP integration test.
    find_package(Python3 COMPONENTS Interpreter QUIET)

    if(Python3_Interpreter_FOUND)
        add_test(
            NAME server_integration
            COMMAND
                "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/server_integration.py"
                "$<TARGET_FILE:retro_server>"
        )

        set_tests_properties(
            server_integration
            PROPERTIES TIMEOUT 20
        )
    endif()
endif()
