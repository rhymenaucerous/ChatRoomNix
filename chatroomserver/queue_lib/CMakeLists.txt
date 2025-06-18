#CMakesLists.txtx file for subdirectory src. Files in src directory are added to the library of files that higher directory uses.
add_library(
    queue_lib
    queue.c
    queue.h
    )

set_target_properties(queue_lib PROPERTIES LINKER_LANGUAGE C)