//NOTE: Given the interconnected nature of the program (shared struct 
//utilization), and the fact that most events are triggered by network
//activity, this program will only test some modules that are utilized
//during the run of the program.

#include "include/cr_shared.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#define H_TABLE_KEY_LENGTH 5

cll_t * p_test_cll = NULL;
h_table_t * p_test_h_table_1 = NULL;
h_table_t * p_test_h_table_2 = NULL;
char data[6] = {0,1,2,3,4,5};

/**
 * @brief Implementation of jenkins hash. Ensures a positive number is passed.
 * 
 * @param p_key pointer to the key being hashed.
 * @return int hash value.
 */
static int64_t
h_table_jenkins_hash (const void * p_key)
{
    if (NULL == p_key)
    {
        fprintf(stderr,"h_table_jenkins_hash: input NULL\n");
        return FAILURE_NEGATIVE;
    }
    
    const char * p_data = (const char *) p_key;

    int hash = 0;

    for (int counter = 0; counter < H_TABLE_KEY_LENGTH; counter++)
    {
        hash += p_data[counter];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return abs(hash);
}

/**
 * @brief Supports cuint suite.
 * 
 * @return int 0.
 */
static int
init_suite1 ()
{
    return 0;
}

/**
 * @brief Supports cuint suite.
 * 
 * @return int 0.
 */
static int
clean_suite1 ()
{
    return 0;
}

/**
 * @brief test cll_init.
 * 
 */
static void
test_cll_init ()
{
    p_test_cll = cll_init();
    CU_ASSERT(NULL != p_test_cll);
}

/**
 * @brief tests cll_size.
 * 
 */
static void
test_cll_size ()
{
    CU_ASSERT(0 == cll_size(p_test_cll));
}

/**
 * @brief tests cll_insert_element_begin.
 * 
 */
static void
test_cll_insert_begin ()
{
    CU_ASSERT(SUCCESS == cll_insert_element_begin(p_test_cll, (data + 1)));
    CU_ASSERT(SUCCESS == cll_insert_element_begin(p_test_cll, data));
    //cll -> 0,1

    CU_ASSERT((data + 1) == cll_return_element(p_test_cll, 1));
    CU_ASSERT((data) == cll_return_element(p_test_cll, 0));
}

/**
 * @brief tests cll_insert_element_end.
 * 
 */
static void
test_cll_insert_end ()
{
    CU_ASSERT(SUCCESS == cll_insert_element_end(p_test_cll, (data + 4)));
    CU_ASSERT(SUCCESS == cll_insert_element_end(p_test_cll, (data + 5)));
    //cll -> 0,1,4,5

    CU_ASSERT((data + 4) == cll_return_element(p_test_cll, 2));
    CU_ASSERT((data + 5) == cll_return_element(p_test_cll, 3));
}

/**
 * @brief tests cll_insert_element.
 * 
 */
static void
test_cll_insert ()
{
    CU_ASSERT(SUCCESS == cll_insert_element(p_test_cll, (data + 3), 2));
    CU_ASSERT(SUCCESS == cll_insert_element(p_test_cll, (data + 2), 3));
    //cll -> 0,1,3,2,4,5

    CU_ASSERT((data + 3) == cll_return_element(p_test_cll, 2));
    CU_ASSERT((data + 2) == cll_return_element(p_test_cll, 3));
}

/**
 * @brief tests cll_sort.
 * 
 */
static void
test_cll_sort ()
{
    CU_ASSERT(SUCCESS == cll_sort(p_test_cll));
    //cll -> 0,1,2,3,4,5

    CU_ASSERT((data) == cll_return_element(p_test_cll, 0));
    CU_ASSERT((data + 5) == cll_return_element(p_test_cll, 5));
}

/**
 * @brief tests cll_remove_element_begin.
 * 
 */
static void
test_cll_remove_begin ()
{
    CU_ASSERT(SUCCESS == cll_remove_element_begin(p_test_cll, NULL));
    //cll -> 1,2,3,4,5

    CU_ASSERT(5 == cll_size(p_test_cll));
    CU_ASSERT((data + 1) == cll_return_element(p_test_cll, 0));
}

/**
 * @brief tests cll_remove_element_end.
 * 
 */
static void
test_cll_remove_end ()
{
    CU_ASSERT(SUCCESS == cll_remove_element_end(p_test_cll, NULL));
    //cll -> 1,2,3,4

    CU_ASSERT(4 == cll_size(p_test_cll));
    CU_ASSERT((data + 4) == cll_return_element(p_test_cll, 3));
}

/**
 * @brief tests cll_remove_element.
 * 
 */
static void
test_cll_remove_element ()
{
    CU_ASSERT(SUCCESS == cll_remove_element(p_test_cll, 1, NULL));
    //cll -> 1,3,4

    CU_ASSERT(3 == cll_size(p_test_cll));
    CU_ASSERT((data + 3) == cll_return_element(p_test_cll, 1));
}

/**
 * @brief tests cll_return_element.
 * 
 */
static void
test_cll_return_element ()
{
    CU_ASSERT((data + 1) == cll_return_element(p_test_cll, 0));
    CU_ASSERT((data + 3) == cll_return_element(p_test_cll, 1));
    CU_ASSERT((data + 4) == cll_return_element(p_test_cll, 2));
}

/**
 * @brief tests cll_destroy.
 * 
 */
static void
test_cll_destroy ()
{
    CU_ASSERT(SUCCESS == cll_destroy(&p_test_cll, NULL));
}

/**
 * @brief test h_table_init.
 * 
 */
static void
test_h_table_init ()
{
    //NOTE: Linking in CMake is acting weird and adding next_prime in this
    //c file resolves the issue.
    p_test_h_table_1 = h_table_init(next_prime(3), NULL);
    p_test_h_table_2 = h_table_init(3, &h_table_jenkins_hash);

    CU_ASSERT(NULL != p_test_h_table_1);
    CU_ASSERT(NULL != p_test_h_table_2);
}

/**
 * @brief tests h_table_new_entry.
 * 
 */
static void
test_h_table_new_entry ()
{
    CU_ASSERT(SUCCESS == h_table_new_entry(p_test_h_table_1, data, "0"));
    CU_ASSERT(FAILURE == h_table_new_entry(p_test_h_table_1, data, "0"));
    CU_ASSERT(SUCCESS == h_table_new_entry(p_test_h_table_1, (data + 1), "1"));

    CU_ASSERT(SUCCESS == h_table_new_entry(p_test_h_table_2, data, "0"));
    CU_ASSERT(FAILURE == h_table_new_entry(p_test_h_table_2, data, "0"));
    CU_ASSERT(SUCCESS == h_table_new_entry(p_test_h_table_2, (data + 1), "1"));
}

/**
 * @brief tests h_table_return_entry.
 * 
 */
static void
test_h_table_return_entry ()
{
    CU_ASSERT(data == h_table_return_entry(p_test_h_table_1, "0"));
    CU_ASSERT(data == h_table_return_entry(p_test_h_table_2, "0"));
}

/**
 * @brief tests h_table_destroy_entry.
 * 
 */
static void
test_h_table_destroy_entry ()
{
    CU_ASSERT(data == h_table_destroy_entry(p_test_h_table_1, "0"));
    CU_ASSERT(NULL == h_table_return_entry(p_test_h_table_1, "0"));

    CU_ASSERT(data == h_table_destroy_entry(p_test_h_table_2, "0"));
    CU_ASSERT(NULL == h_table_return_entry(p_test_h_table_2, "0"));
}

/**
 * @brief tests h_table_destroy.
 * 
 */
static void
test_h_table_destroy ()
{
    CU_ASSERT(SUCCESS == h_table_destroy(p_test_h_table_1, NULL));
    CU_ASSERT(SUCCESS == h_table_destroy(p_test_h_table_2, NULL));
}


int main ()
{
    CU_TestInfo suite1_tests[] = 
    {
        {"Testing test_cll_init():", test_cll_init},

        {"Testing cll_size():", test_cll_size},

        {"Testing cll_insert_begin():", test_cll_insert_begin},

        {"Testing cll_insert_end():", test_cll_insert_end},

        {"Testing cll_insert():", test_cll_insert},

        {"Testing cll_sort():", test_cll_sort},
        
        {"Testing test_cll_remove_begin():", test_cll_remove_begin},

        {"Testing cll_remove_end():", test_cll_remove_end},
        
        {"Testing cll_remove_element():", test_cll_remove_element},

        {"Testing cll_return_element():", test_cll_return_element},

        {"Testing cll_destroy():", test_cll_destroy},

        {"Testing h_table_init():", test_h_table_init},

        {"Testing h_table_new_entry():", test_h_table_new_entry},

        {"Testing h_table_return_entry():", test_h_table_return_entry},

        {"Testing h_table_destroy_entry():", test_h_table_destroy_entry},

        {"Testing h_table_destroy():", test_h_table_destroy},
        
        CU_TEST_INFO_NULL
    
    };

    CU_SuiteInfo suites[] = 
    {
        {"Suite-1:", init_suite1, clean_suite1, .pTests = suite1_tests},
        CU_SUITE_INFO_NULL
    };

    if (0 != CU_initialize_registry())
    {
        return CU_get_error();
    }

    if (0 != CU_register_suites(suites))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_show_failures(CU_get_failure_list());
    int num_failed = CU_get_number_of_failures();
    CU_cleanup_registry();
    printf("\n");
    return num_failed;
}

//End of cr_tester.c file
