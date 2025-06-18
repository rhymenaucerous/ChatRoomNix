#CMakesLists.txtx file for subdirectory src. Files in src directory are added to the library of files that higher directory uses.
add_library(
    src
    cr_main.c
    cr_shared.c
    cr_listener.c
    cr_msg.c
    cr_rooms.c
    cr_users.c
    cr_chats.c
    cr_session_manager.c
    )

set_target_properties(src PROPERTIES LINKER_LANGUAGE C)