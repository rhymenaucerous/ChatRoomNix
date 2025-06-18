#CMakesLists.txtx file for subdirectory src. Files in src directory are added to the library of files that higher directory uses.
add_library(
    t_pool_lib
    t_pool.c
    t_pool.h
    )

set_target_properties(t_pool_lib PROPERTIES LINKER_LANGUAGE C)