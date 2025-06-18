#CMake Lists File
cmake_minimum_required(VERSION 3.22.1)

#The version of C that is being compiled will be decided by the compiler, gcc. 
project(
    MyCLibrary
    LANGUAGES "C"
)

#The following command will force cmake to use the c99 standard for compilation.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --std=gnu99 -g -Werror -Wall -Wpedantic -Wunused-parameter")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm -pthread -lcunit -lssl -lcrypto")

if (DEBUG EQUAL "1")
    message("DEBUGGING VERSION")
    add_compile_definitions(DEBUG)
else()
    message("RELEASE VERSION")
endif()

find_package(OpenSSL REQUIRED)

#Relevant subdirectories for all executables
add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(networking_lib)
add_subdirectory(algorithms_lib)
add_subdirectory(t_pool_lib)
add_subdirectory(queue_lib)
add_subdirectory(cll_lib)
add_subdirectory(h_table_lib)

#Chat Room Server Executable
add_executable(chat_room src/cr_main.c)

target_link_libraries(
    chat_room
    PUBLIC src
    PUBLIC include
    PUBLIC networking_lib
    PUBLIC algorithms_lib
    PUBLIC t_pool_lib
    PUBLIC queue_lib
    PUBLIC cll_lib
    PUBLIC h_table_lib
    PUBLIC ssl
    PUBLIC crypto
)

#Chat Room Server Tester Executable
add_executable(chat_room_unit_tester cr_tester.c)

target_link_libraries(
    chat_room_unit_tester
    PUBLIC -lcunit
    PUBLIC src
    PUBLIC include
    PUBLIC networking_lib
    PUBLIC algorithms_lib
    PUBLIC t_pool_lib
    PUBLIC queue_lib
    PUBLIC cll_lib
    PUBLIC h_table_lib
    PUBLIC ssl
    PUBLIC crypto
)

#End of CMakelists.txt file