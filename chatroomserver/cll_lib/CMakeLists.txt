#CMakesLists.txtx file for subdirectory src. Files in src directory are added to the library of files that higher directory uses.
add_library(
    cll_lib
    cll.c
    cll.h
    )

set_target_properties(cll_lib PROPERTIES LINKER_LANGUAGE C)