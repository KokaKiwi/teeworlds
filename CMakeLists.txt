cmake_minimum_required(VERSION 2.8)

project(teeworlds)

set(OFLAGS "-ggdb3")
# set(OFLAGS "-march=native -msse -msse2 -msse3 -mmmx")

set(CMAKE_C_FLAGS "${OFLAGS} -Wall -fno-exceptions")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

find_library(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

find_library(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})
include_directories(/usr/include/SDL)

find_library(Threads REQUIRED)

find_library(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

find_library(OpenGL REQUIRED)

include_directories(src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/game)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/game/generated)

macro(compile_content target action output_name)
    set(${target}_HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/game/generated/${output_name}.h)
    set(${target}_SOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/game/generated/${output_name}.cpp)

    set(${target}_HEADER ${${target}_HEADER_FILE})
    set(${target}_SOURCE ${${target}_SOURCE_FILE})

    add_custom_command(
        OUTPUT ${${target}_SOURCE_FILE}
        COMMAND
            python ${CMAKE_CURRENT_SOURCE_DIR}/datasrc/compile.py
        ARGS
            ${action}_source > ${${target}_SOURCE_FILE}
        DEPENDS
            ${${target}_HEADER_FILE}
            ${CMAKE_CURRENT_SOURCE_DIR}/datasrc/content.py
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating ${${target}_SOURCE}"
        VERBATIM
    )
    add_custom_command(
        OUTPUT ${${target}_HEADER_FILE}
        COMMAND
            python ${CMAKE_CURRENT_SOURCE_DIR}/datasrc/compile.py
        ARGS
            ${action}_header > ${${target}_HEADER_FILE}
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/datasrc/content.py
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating ${${target}_HEADER}"
        VERBATIM
    )
endmacro(compile_content)

compile_content(network network protocol)
compile_content(client_content client_content client_data)
compile_content(server_content server_content server_data)

macro(chash output)
    set(FILES ${ARGN})
    add_custom_command(
        OUTPUT ${output}
        COMMAND
            python ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmd5.py
        ARGS
            ${FILES} > ${output}
        DEPENDS
            ${FILES}
        COMMENT
            "Generating ${output}"
        VERBATIM
    )
endmacro(chash)

chash(${CMAKE_CURRENT_BINARY_DIR}/game/generated/nethash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/shared/protocol.h
    ${network_HEADER}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game/tuning.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game/gamecore.cpp
    ${network_HEADER}
)

file(GLOB teeworlds_engine_SOURCES src/engine/shared/*.cpp src/base/*.c)

file(GLOB teeworlds_client_SOURCES src/engine/client/*.cpp)
file(GLOB teeworlds_server_SOURCES src/engine/server/*.cpp)
file(GLOB teeworlds_bot_SOURCES src/engine/bot/*.cpp)

file(GLOB teeworlds_versionserver_SOURCES src/versionsrv/*.cpp)
file(GLOB teeworlds_masterserver_SOURCES src/mastersrv/*.cpp)

file(GLOB teeworlds_game_shared_SOURCES src/game/*.cpp)
list(APPEND teeworlds_game_shared_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/game/generated/nethash.cpp)
list(APPEND teeworlds_game_shared_SOURCES ${network_SOURCE})

file(GLOB_RECURSE teeworlds_game_client_SOURCES src/game/client/*.cpp)
list(APPEND teeworlds_game_client_SOURCES ${client_content_SOURCE})

file(GLOB_RECURSE teeworlds_game_bot_SOURCES src/game/bot/*.cpp)
list(APPEND teeworlds_game_bot_SOURCES ${client_content_SOURCE})

file(GLOB_RECURSE teeworlds_game_server_SOURCES src/game/server/*.cpp)
list(APPEND teeworlds_game_server_SOURCES ${server_content_SOURCE})

file(GLOB teeworlds_game_editor_SOURCES src/game/editor/*.cpp)

file(GLOB teeworlds_tools_SOURCES src/tools/*.cpp src/tools/*.c)

file(GLOB wavpack_SOURCES src/engine/external/wavpack/*.c)
file(GLOB pnglite_SOURCES src/engine/external/pnglite/*.c)
file(GLOB jsonparser_SOURCES src/engine/external/json-parser/*.c)

add_executable(teeworlds
    ${teeworlds_game_shared_SOURCES}
    ${teeworlds_game_client_SOURCES}
    ${teeworlds_engine_SOURCES}
    ${teeworlds_client_SOURCES}
    ${teeworlds_game_editor_SOURCES}
    ${wavpack_SOURCES}
    ${pnglite_SOURCES}
    ${jsonparser_SOURCES}
)

add_executable(teeworlds_srv
    ${teeworlds_engine_SOURCES}
    ${teeworlds_server_SOURCES}
    ${teeworlds_game_shared_SOURCES}
    ${teeworlds_game_server_SOURCES}
)

add_executable(teeworlds_bot
    ${teeworlds_game_shared_SOURCES}
    ${teeworlds_game_bot_SOURCES}
    ${teeworlds_engine_SOURCES}
    ${teeworlds_bot_SOURCES}
)

add_executable(versionsrv
    ${teeworlds_versionserver_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/game/generated/nethash.cpp
    ${teeworlds_engine_SOURCES}
)

add_executable(mastersrv
    ${teeworlds_masterserver_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/game/generated/nethash.cpp
    ${teeworlds_engine_SOURCES}
)

target_link_libraries(teeworlds z)
target_link_libraries(teeworlds_srv z)
target_link_libraries(teeworlds_bot z)
target_link_libraries(versionsrv z)
target_link_libraries(mastersrv z)

target_link_libraries(teeworlds
    X11
    GL
    GLU
    SDL
)

target_link_libraries(teeworlds pthread)
target_link_libraries(teeworlds_srv pthread)
target_link_libraries(teeworlds_bot pthread)
target_link_libraries(versionsrv pthread)
target_link_libraries(mastersrv pthread)

target_link_libraries(teeworlds freetype)
target_link_libraries(teeworlds_srv freetype)
target_link_libraries(versionsrv freetype)
target_link_libraries(mastersrv freetype)
