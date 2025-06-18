#include "../include/cr_main.h"

/**
 * @brief Set the config members given a buffer with the variable inside.
 * Helper function to config_file_open. Is called at iterations of fgets
 * loop to set different members.
 * 
 * @param p_config_info pointer to the config info structure.
 * @param p_buffer pointer to buffer data.
 * @param target_index the index of the fgets loop which determines which
 * member is being set.
 * @return int SUCCESS or FAILURE (0 or 1 respectively).
 */
static int
set_config_members (config_info_t * p_config_info, char * p_buffer,
                                            uint8_t * target_index)
{
    if ((NULL == p_config_info) || (NULL == p_buffer) || (NULL == target_index))
    {
        fprintf(stderr, "set_config_members: input NULL\n");
        return FAILURE;
    }
    
    char * p_string_holder;

    long value_holder;
    
    switch (*target_index)
    {
        case 0:
            //MAX: 65535
            strncpy(p_config_info->p_host, p_buffer, HOST_MAX_STRING);
            int len = strlen(p_config_info->p_host);

            //NOTE: Removing that pesky newline character.
            if (((p_config_info->p_host)[len - 1]) == '\n')
            {
                (p_config_info->p_host)[len - 1] = '\0';
            }

            break;
        case 1:
            //MAX: 65535
            value_holder = strtol(p_buffer, &p_string_holder, BASE10);

            if (FAILURE == port_range_check(&value_holder))
            {
                fprintf(stderr, "set_config_members: server listening port "
                                         "value (out of range 1-65535).\n");
                return FAILURE;
            }

            sprintf(p_config_info->p_port, "%ld", value_holder);

            break;
        case 2:
            //MAX: 10 mins or 600 seconds
            value_holder = strtol(p_buffer, &p_string_holder, BASE10);

            if ((MIN_TOTAL_ROOMS > value_holder) ||
                (MAX_TOTAL_ROOMS < value_holder))
            {
                fprintf(stderr, "set_config_members: max rooms out of "
                                                    "range (1-20).\n");
                return FAILURE;
            }

            p_config_info->max_rooms = value_holder;

            break;
        case 3:
            //max allowed: 100 clients
            value_holder = strtol(p_buffer, &p_string_holder, BASE10);

            if ((MIN_TOTAL_CLIENTS > value_holder) ||
                (MAX_TOTAL_CLIENTS < value_holder))
            {
                fprintf(stderr, "set_config_members: max clients out of "
                                                      "range (2-50).\n");
                return FAILURE;
            }

            p_config_info->max_client = value_holder;

            break;
    }

    if (errno == ERANGE)
    {
        perror("set_config_members: strtol conversion\n");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Opens the configuration file and sets the configuration info
 * structure with the user-supplied values.
 * 
 * @param p_config_info pointer to the config info structure.
 * @return int SUCCESS or FAILURE (0 or 1 respectively).
 */
static int
config_file_open (config_info_t * p_config_info)
{
    if ((NULL == p_config_info))
    {
        fprintf(stderr, "config file open: input NULL\n");
        return FAILURE;
    }
    
    FILE * file_pointer;

    file_pointer = fopen(CONFIG_FILENAME, "r");

    if (NULL == file_pointer)
    {
        perror("config_file_open: fopen");
        return FAILURE;
    }

    //NOTE: Array is hard set due to fighter file requirements.
    uint8_t target_lines[4] = {2, 5, 8, 11};
    int current_line = 1;

    char p_buffer[BUFF_SIZE];

    //WARNING: The counter checks for all four target lines, altering target
    //lines must be done in conjuction with altering input file standards.
    for (uint8_t target_counter = 0; target_counter < 4 ; target_counter++)
    {
        while (current_line != (target_lines[target_counter] + 1))
        {
            fgets(p_buffer, sizeof(p_buffer), file_pointer);
            current_line++;
        }

        if (FAILURE == set_config_members(p_config_info, p_buffer,
                                                &target_counter))
        {
            fprintf(stderr, "config_file_open: set_config_members failure.\n");
            if (FAILURE_NEGATIVE == fclose(file_pointer))
            {
                perror("config_file_open: fclose");
                return FAILURE;
            }
            return FAILURE;
        }
    }

    if (FAILURE_NEGATIVE == fclose(file_pointer))
    {
        perror("config_file_open: fclose");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Driver code for the chat room server. Handles commandline
 * arguments and input files.
 * 
 * @return int SUCCESS or FAILURE (0 or 1 respectively).
 */
int
main ()
{
    //NOTE: To ensure text is properly read from the users.txt and other files,
    //the encoding is manually set to UTF-8 (whether it was already that
    //that encoding or not), so that the program can be portable.
    setlocale(LC_ALL, "en_US.UTF-8");
    
    config_info_t * p_config_info = calloc(1, sizeof(config_info_t));

    if(NULL == p_config_info)
    {
        fprintf(stderr, "main: p_config_info calloc\n");
        return FAILURE;
    }

    if (FAILURE == config_file_open(p_config_info))
    {
        fprintf(stderr, "main: config_file_open()\n");
        FREE(p_config_info);
        return FAILURE;
    }

    if (FAILURE == cr_listener(p_config_info))
    {
        fprintf(stderr, "main: cr_listener()\n");
        FREE(p_config_info);
        return FAILURE;
    }

    FREE(p_config_info);
    return SUCCESS;
}

//End of chat_room_main.c file
