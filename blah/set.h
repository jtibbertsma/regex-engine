/* set.h
 *
 * Set of non-negative integers, which defines common mathematical
 * set operations such as union and intersection, and checks for
 * element membership in constant time.
 */

#ifndef __regex_set
#define __regex_set

#include <stdbool.h>
#include "hooks.h"

typedef struct _set set_t;

/** add
  *
  * Add an integer to the set. If the integer is already in the set,
  * then nothing is changed.
  */
void set_add(set_t*, unsigned int);

/** remove
  *
  * Removes a given integer from the set. If the integer is not in the
  * set, then nothing is changed.
  */
void set_remove(set_t*, unsigned int);

/** element
  *
  * Checks if a given integer is in the set.
  */
bool set_element(set_t*, unsigned int);

/** subset
  *
  * Checks if the right argument is a subset of the left argument.
  */
bool set_subset(set_t*, set_t*);

/** union
  *
  * Takes two sets and creates a new set which represents
  * their union. The x version is void; instead of creating
  * a new set, the argument on the left becomes the union.
  */
set_t* set_union(set_t*, set_t*);
void set_xunion(set_t*, set_t*);

/** intersection
  *
  * Takes two sets and returns their intersection.
  */
set_t* set_intersection(set_t*, set_t*);
void set_xintersection(set_t*, set_t*);

/** difference
  *
  * Returns the difference of the right argument from the left
  * argument.
  */
set_t* set_difference(set_t*, set_t*);
void set_xdifference(set_t*, set_t*);

/** symmetric difference
  *
  * Returns the set which contains the numbers that are in exactly
  * one of the argument sets.
  */
set_t* set_symmetric(set_t*, set_t*);
void set_xsymmetric(set_t*, set_t*);

/** empty
  *
  * Checks if a set is empty.
  */
bool set_empty(set_t*);

/** cardinality
  *
  * Gives the cardinality of the set.
  */
int set_cardinality(set_t*);

/** equality
  *
  * Checks if two set are equal.
  */
bool set_equality(set_t*, set_t*);

/** pop
  *
  * Return the smallest integer stored in the set and pop it.
  * If the set is empty, returns -1.
  */
int set_pop(set_t*);

/** new
  *
  * Creates an empty set.
  */
set_t* set_new();

/** copy
  *
  * Copy constructor.
  */
set_t* set_copy(set_t*);

/** free
  *
  * Frees a set's memory.
  */
void set_free(set_t*);

/** hook
  *
  */
#ifdef SET_HOOK
void set_hook(set_t*);
#endif /* ifdef SET_HOOK */

#endif
