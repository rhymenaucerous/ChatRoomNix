add_library(
    include
    cr_main.h
    cr_shared.h
    cr_listener.h
    cr_msg.h
    cr_rooms.h
    cr_users.h
    cr_chats.h
    cr_session_manager.h
    )

set_target_properties(include PROPERTIES LINKER_LANGUAGE C)

#End of CMakeLists.txt file for include directory
