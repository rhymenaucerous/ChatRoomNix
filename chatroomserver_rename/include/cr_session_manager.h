#ifndef CR_SESSION
#define CR_SESSION

#include "cr_shared.h"
#include "cr_msg.h"
#include "cr_users.h"
#include "cr_rooms.h"
#include "cr_chats.h"

#define NO_MATCH 5

/**
 * @brief Maintains session with client. Listens for client packets and
 * responds according to messaging protocols after conducting necessary
 * actions.
 * 
 * @param p_cr_package pointer to package with client file descriptor,
 * users_t struct, and rooms_t struct.
 * @return int SUCCESS (0) or FAILURE (1).
 */
int
cr_sm_session_manager (cr_package_t * p_cr_package);

#endif //CR_SESSION

//End of cr_session_manager.h file