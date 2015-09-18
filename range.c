/* range.c
 *
 * Let's implementing the range struct.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "range.h"

/* range
 *
 * Definition of range struct. Consists of an array and an int which
 * represents the size of the array. Eventually maybe add pointers to
 * more arrays which could represent recursive group captures.
 */
struct _range {
   group_t* groups;
   int size;
};

group_t* range_group(range_t* ra, int index) {
   assert(ra);
   if (index < 0 || index >= ra->size)
      return NULL;
   return ra->groups + index;
}

int range_size(range_t* ra) {
   assert(ra);
   return ra->size;
}

range_t* range_copy(range_t* that) {
   assert(that);
   range_t* ra = range_new(that->size);
   memcpy(ra->groups, that->groups, that->size * sizeof(group_t));
   return ra;
}

range_t* range_new(int size) {
   assert(size >= 1);
   range_t* new = malloc(sizeof(range_t));
   assert(new);
   new->groups = calloc(size, sizeof(group_t));
   assert(new->groups);
   new->size = size;
   return new;
}

void range_free(range_t* range) {
   if (range) {
      free(range->groups);
      free(range);
   }
}

/********************************************************************/
