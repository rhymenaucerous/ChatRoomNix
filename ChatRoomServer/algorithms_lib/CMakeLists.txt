#CMakesLists.txtx file for subdirectory src. Files in src directory are added to the library of files that higher directory uses.
add_library(
    algorithms_lib
    algorithms.c
    algorithms.h
    )

set_target_properties(algorithms_lib PROPERTIES LINKER_LANGUAGE C)