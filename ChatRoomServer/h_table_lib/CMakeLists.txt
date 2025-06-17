#CMakesLists.txtx file for subdirectory src. Files in src directory are added to the library of files that higher directory uses.
add_library(
    h_table_lib
    h_table.c
    h_table.h
    )

set_target_properties(h_table_lib PROPERTIES LINKER_LANGUAGE C)