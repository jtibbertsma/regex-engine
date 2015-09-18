/* class.h
 *
 * A tree contains a set of codepoints. It is implemented as a
 * binary search tree with each node containing a disjoint urange32
 * struct.
 */

#ifndef __regex_class
#define __regex_class

#include <stdbool.h>
#include "hooks.h"
#include "util.h"

typedef struct _class class_t;

/** insert_codepoint
  *
  * Add an integer to the class. If the integer is already in the
  * tree, then nothing is changed.
  */
void class_insert_codepoint(class_t*, uint32_t);

/** insert_range
  *
  * Add a range to class.
  */
void class_insert_range(class_t*, urange32_t);

/** remove
  *
  * Removes a given integer from the class. If the integer is not
  * in the tree, then nothing is changed.
  */
void class_delete_codepoint(class_t*, uint32_t);

/** remove
  *
  * Removes a given range from the class.
  */
void class_delete_range(class_t*, urange32_t);

/** search
  *
  * Checks if a given codepoint is in the class. Guaranteed to run in
  * O(lg(n)) time for a class consisting of n ranges.
  */
bool class_search(class_t*, uint32_t);

/** union
  *
  * Find the union of two classes. For this function and the following
  * two, the left argument becomes the result of the operation.
  */
void class_union(class_t*, const class_t*);

/** intersection
  *
  * Takes the intersection of two classes.
  */
void class_intersection(class_t*, const class_t*);

/** difference
  *
  * Find the difference of the right argument from the left
  * argument.
  */
void class_difference(class_t*, const class_t*);

/** empty
  *
  * Checks if a tree is empty.
  */
bool class_empty(class_t*);

/** cardinality
  *
  * Get the number of codepoints that are in the character class.
  */
int class_cardinality(class_t*);

/** size
  *
  * Get the number of disjoint ranges in the character class.
  */
int class_size(class_t*);

/** new
  *
  * Creates an empty class.
  */
class_t* class_new();

/** free
  *
  * Frees a tree's memory.
  */
void class_free(class_t*);

/** hook
  *
  * Debugger hook; print out all ranges in visiting order.
  */
#ifdef CLASS_HOOK
void class_hook(class_t*);
#endif /* ifdef CLASS_HOOK */

#endif
