/* bts.h
 *
 * Bts originally stood for backtrack-stack, since I used it only
 * for backtracking, but now it's used to hold information used by the
 * whole pattern matching process. During the matching process, the
 * next move is determined by checking the top of the stack.
 */

#ifndef __regex_bts
#define __regex_bts

#include <stdbool.h>
#include "range.h"


typedef struct _bts bts_t;

/* state
 *
 * Holds the necessary information to match a string against an
 * atom, or to backtrack into a nested group.
 */
typedef struct {
   int index;      // the index of atom to search
   uint32_t matches;  // starting value of the match counter
   char* str;      // starting position in the input string
   bts_t* inner;   // stack for search of inner core
   range_t* nest;  // inner group captures for subroutines
   int nbr;        // branch number to start with
   bool recursive; // used for various purposes
} state_t;

/** push
  *
  * Push data onto the top node of the stack. The arguments are
  * every piece of information listed above.
  */
void bts_push(bts_t*, int, char*, uint32_t, bool, bts_t*, int);

/** top
  *
  * Get a pointer to the top state on the stack.
  */
state_t* bts_top(bts_t*);

/** class_top_index
  *
  * This is needed when coming out of a recursive core search. A nested
  * core doesn't know the index of the atom that holds it or the
  * value of the match counter, so this data can be added to the top
  * of the stack using this function.
  */
void bts_set_top(bts_t*, int, uint32_t, range_t*);

/** pop
  *
  * Pop the stack. This doesn't free any of the objects pointed to
  * by pointers on the top node of the stack.
  */
void bts_pop(bts_t*);

/** empty
  *
  * Returns true if the stack is empty.
  */
bool bts_empty(bts_t*);

/** new
  *
  * Return a pointer to a new stack.
  */
bts_t* bts_new();

/** free
  *
  * Free all unpopped nodes in the stack and the stack itself.
  */
void bts_free(bts_t*);

#endif
