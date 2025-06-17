#ifndef CR_LISTENER
#define CR_LISTENER

#include "cr_shared.h"
#include "cr_users.h"
#include "cr_session_manager.h"

#define CLEAN 0
#define DONT_CLEAN 1

/**
 * @brief Creates shared structures and thread pool for the server to being listening.
 * Starts listening funciton.
 * 
 * @param p_config_info pointer to configuration information from main server.
 * @return int SUCCESS (0) or FAILURE (1).
 */
int
cr_listener(config_info_t * p_config_info);

#endif //CR_LISTENER

//End of cr_msg.h file
