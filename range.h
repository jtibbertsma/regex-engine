/* range.h
 *
 * Struct for temporarily storing group captures.
 */

#ifndef __regex_range
#define __regex_range

/* group
 *
 * A simple struct which represents a substring of a string
 * by keeping a pointer to the beginning of the string and
 * a pointer to one after the end of the string.
 */
typedef struct {
   char* begin;
   char* end;
} group_t;

/* range
 *
 * A wrapper struct that holds an array of group_t, and the
 * size of the array.
 */
typedef struct _range range_t;

/** group
  *
  * Get the group with the given index, or return null if the
  * group doesn't exist.
  */
group_t* range_group(range_t*, int);

/** size
  *
  * Gives the size of the internal array, which is the number of
  * groups in the regular expression including the overall match.
  */
int range_size(range_t*);

/** copy
  *
  * Copy the range struct.
  */
range_t* range_copy(range_t*);

/** new
  *
  * Create a new range; you need to give the constructor the size
  * of the array.
  */
range_t* range_new(int);

/** free
  *
  * Free the range's memory.
  */
void range_free(range_t*);

#endif
