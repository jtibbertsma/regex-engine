/* util.h
 *
 * Some useful functions and structs.
 */

#ifndef __regex_util
#define __regex_util

#include <stdbool.h>
#include <stdint.h>

/* The following structs hold a range of 16-bit ints and 32-bit ints
 * respectively. These are used to keep track of unicode tables and
 * the 32-bit range is used in the implementation of class_t.
 */
typedef struct {
   uint16_t lo;
   uint16_t hi;
} urange16_t;

typedef struct {
   uint32_t lo;
   uint32_t hi;
} urange32_t;

/** substring
  *
  * Given two pointers to different positions in a string, allocate
  * a new character array which contains the string between the two
  * input pointers.
  */
char* substring(char* begin, char* end);

/** safe_strcat
  *
  * Like normal strcat, except this ensures that the dst array is
  * large enough by checking that strlen(dst) + strlen(src) < *size,
  * and reallocating if necessary, storing the new size in *size.
  */
char* safe_strcat(char* dst, char* src, int* size);

/** count_ones
  *
  * Count the number of ones in the binary representation of an
  * integer.
  */
int count_ones(int);

/** ispow2
  *
  * Returns true if the given int is a power of two.
  */
bool ispow2(int);

/** strhash
  *
  * Hash function for a string.
  */
uint32_t strhash(char*);

#endif
