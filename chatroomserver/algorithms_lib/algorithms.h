#ifndef ALGO_LIB
#define ALGO_LIB

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#ifndef SHARED_MACROS
#define SHARED_MACROS

#define FREE(a) \
    free(a); \
    (a) = NULL

//macros enabling clear returns from all functions.
#define SUCCESS 0
#define FAILURE 1
#define FAILURE_NEGATIVE -1

//NOTE: 512, 1024, 4096 are common buffer size chunks due to them being powers
//of 2, efficiently using memory. 1024 is a nice middle ground for potential
//packet sizes.
#define BUFF_SIZE 1024

#define CONTINUE 1
#define STOP 0

//filenames should not exceed 50 characters - this will enable some paths.
#define FILE_NAME_MAX_LEN 50

//Maximums for strings: IPv4 or IPv6 compatable.
#define HOST_MAX_STRING 40 //Max IP length is (IPv6) 40 -> 
                           //7 colons + 32 hexadecimal digits + terminator.

#define PORT_MAX_STRING 6 //Only numeric services allowed - max length of
                          //65535 is 5 + terminator.

//IP length + Port length - 1 (one less terminator) + 2 (: designator in code).
#define ADDR_MAX_STRING (HOST_MAX_STRING + PORT_MAX_STRING + 1)

#endif //SHARED_MACROS

#define ROUND_COUNT 5
#define FIRST_VAL 1
#define SECOND_VAL 2

/**
 * @brief Simple function to calculate the higher of two values.
 * 
 * @param value_1 first value.
 * @param value_2 second value.
 * @return int with higher value. if they are equal, the first value will
 * return.
 */
int bigger_value (int value_1, int value_2);

/**
 * @brief Helper function to is_prime. Calculates (base^exponent) % modulus.
 * Decreases the computation time by at least O(e) from the conventional 
 * method. Limited by uint_16 size.
 * Pseudocode pulled from: https://en.wikipedia.org/wiki/Modular_
 * exponentiation#:~:text=Modular%20exponentiation%20is%20the%20remainder
 * ,c%20%3D%20be%20mod%20m.
 * 
 * @param base 
 * @param exponent 
 * @param modulus 
 * @return uint16_t resul of formula.
 */
uint16_t
modular_pow (uint16_t base, uint16_t exponent, uint16_t modulus);

/**
 * @brief Helper function to next_prime. Determines whether the supplied value
 * is prime. Uses the Miller-Rabin test. Round count macro determines accuracy
 * of testing, default used is five.
 * Reference: https://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test
 * 
 * @param value value to be tested for primeness.
 * @return true is returned if value is prime.
 * @return false is returned if value isn't prime.
 */
bool
is_prime(uint16_t value);

/**
 * @brief Helper function to h_table_re_hash. Iterates through odd numbers to
 * determine the next prime number in the integer series.
 * 
 * @param value is input integer.
 * @return uint16_t next prime number in integer series.
 */
uint16_t
next_prime(uint16_t value);

/**
 * @brief Per the rand() man page, if randomness and portability are both
 * important, random() should be used to instead. Generates a 32 bit number.
 * new_session_ID sets first 32 bits and then second 32 bits of 64 bit session
 * ID.
 * 
 * @return uint64_t 
 */
uint64_t
new_session_ID ();

/**
 * @brief Psuedo-randomly decides between two options.
 * 
 * @return int FIRST_VAL or SECOND_VAL returned (1 or 2 respectively).
 */
int
just_pick ();

/**
 * @brief Returns a psuedo random int between 0 and 100.
 * 
 * @return uint8_t psuedo random int between 0 and 100.
 */
uint8_t
random_between_0_100 ();

#endif //ALGO_LIB

//End of algorithms.h
