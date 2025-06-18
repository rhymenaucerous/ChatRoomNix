#include "algorithms.h"

/**
 * @brief Simple function to calculate the higher of two values.
 * 
 * @param value_1 first value.
 * @param value_2 second value.
 * @return int with higher value. if they are equal, the first value will
 * return.
 */
int
bigger_value (int value_1, int value_2)
{
    if (value_2 > value_1)
    {
        return value_2;
    }
    
    return value_1;
}

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
modular_pow (uint16_t base, uint16_t exponent, uint16_t modulus)
{
    uint16_t result = 1;
    base = base % modulus;

    while (exponent > 0)
    {
        if (exponent % 2 == 1)
        {
            result = (result * base) % modulus;
        }

        base = (base * base) % modulus;

        exponent = exponent >> 1;
    }
    
    return result;
}


//Note: the alternative to this method would be maintaining a table of primes
//up to 65535, according to the sieve of Eratosthenes . This would probably be
//more efficient, given the size limitations on the hash table. This
//implementation would be more simple, though, so miller-rabin test is 
//implemented for learning purposes.

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
is_prime (uint16_t value)
{
    if (value <= 1)
    {
        return false;
    }
    if (value <= 3)
    {
        return true;
    }

    //factoring out powers of two
    uint16_t odd_int = value - 1;
    uint16_t power_of_two = 0;

    while (odd_int % 2 == 0)
    {
        power_of_two += 1;
        odd_int = odd_int/2;
    }

    //Seeding inside the loop would generate the same number each time due
    //to the computational speed.
    srand(time(NULL));

    uint16_t max_number = value -2;
    uint16_t minimum_number = 2;

    for (uint8_t counter1 = 0; counter1 < ROUND_COUNT; counter1++)
    {
        //Getting the number within range will decrease the randomness
        //slightly our base for prime calculations will be determined.
        uint16_t prime_base = rand() % (max_number + 1 - minimum_number)
                                                       + minimum_number;

        uint16_t calculation_1 = modular_pow(prime_base,odd_int,value);
        uint16_t calculation_2 = 0;

        //test to see if strong base, will skip to next loop if not
        if ((1 == calculation_1) || ((value - 1) == calculation_1))
        {
            continue;
        }

        for (uint16_t counter2 = 0; counter2 < power_of_two; counter2++)
        {
            calculation_2 = modular_pow(calculation_1,2,value);

            if ((1 == calculation_2) && (1 != calculation_1) &&
                                (calculation_1 != (value - 1)))
            {
                return false;
            }

            calculation_1 = calculation_2;
        }

        if (1 != calculation_2)
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Helper function to h_table_re_hash. Iterates through odd numbers to
 * determine the next prime number in the integer series.
 * 
 * @param value is input integer.
 * @return uint16_t next prime number in integer series.
 */
uint16_t
next_prime (uint16_t value)
{
    if (value <= 1)
    {
        return 2;
    }
    
    if (value == 2)
    {
        return 3;
    }

    //The search will start at the next odd number
    if (value % 2 == 0)
    {
        value += 1;
    }
    else
    {
        value += 2;
    }

    uint16_t double_value_holder = value * 2;

    //Bertrand's postulate: for any int  > 3 there exists at least one prime
    //number, p, such that n < p < 2n.
    for (; value < double_value_holder; value += 2)
    {
        if (is_prime(value))
        {
            return value;
        }
    }

    return FAILURE;
}

/**
 * @brief Per the rand() man page, if randomness and portability are both
 * important, random() should be used to instead. Generates a 32 bit number.
 * new_session_ID sets first 32 bits and then second 32 bits of 64 bit session
 * ID.
 * 
 * @return uint64_t 
 */
uint64_t
new_session_ID (int randomness)
{
    uint64_t session_ID;
    long ID_bit_holder;
    long ID_bit_holder_2;

    memset(&session_ID,0,sizeof(uint64_t));

    srandom(randomness);
    ID_bit_holder = random();

    srandom(randomness);
    ID_bit_holder_2 = random();

    session_ID = (ID_bit_holder << 32) | ID_bit_holder_2;
    
    return session_ID;
}

/**
 * @brief Psuedo-randomly decides between two options.
 * 
 * @return int FIRST_VAL or SECOND_VAL returned (1 or 2 respectively).
 */
int
just_pick ()
{
    srandom(time(NULL));

    long random_num = random();

    long max_random = 2147483647; //value is 2^31 - 1

    if (random_num > max_random/2) //About 50% chance!
    {
        return FIRST_VAL;
    }
    else
    {
        return SECOND_VAL;
    }
}

/**
 * @brief Returns a psuedo random int between 0 and 100.
 * 
 * @return uint8_t psuedo random int between 0 and 100.
 */
uint8_t
random_between_0_100 ()
{
    srand(time(NULL));

    return rand() % 101;
}

//End of algorithms.c
