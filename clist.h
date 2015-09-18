/* clist.h
 *
 * We allow subroutine tokens to come earlier in a regular expression
 * than the group that they're referring to. So in build_core, we need
 * to keep track of the subroutine atoms until after all cores have
 * been built. Hence this file, which declares a list of atom
 * pointers.
 */

#ifndef __regex_clist
#define __regex_clist

#include "atom.h"

/* clist
 *
 * Holds a atom_t pointer and the int which represents the index
 * of the core needed for the subroutine call.
 */
typedef struct _clist clist_t;

/** clist_next
  *
  * Gives the next node.
  */
clist_t* clist_next(clist_t*);

/** clist_pointer
  *
  * Gives the pointer to the atom.
  */
atom_t* clist_pointer(clist_t*);

/** clist_index
  *
  * Gives the index of the core.
  */
int clist_index(clist_t*);

/** clist_new
  *
  * Create a new node.
  */
clist_t* clist_new(clist_t*, atom_t*, int);

/** clist_free
  *
  * Delete all nodes.
  */
void clist_free(clist_t*);

#endif
 