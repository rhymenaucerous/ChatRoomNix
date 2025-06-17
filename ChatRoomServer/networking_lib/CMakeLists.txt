#CMakesLists.txtx file for subdirectory src. Files in src directory are added to the library of files that higher directory uses.
add_library(
    networking_lib
    networking.h
    networking.c
    )

set_target_properties(networking_lib PROPERTIES LINKER_LANGUAGE C)