/* core.h
 *
 * This is the core object of the regex engine that checks whether
 * or not a string matches a pattern. The core is arranged into
 * a series of atoms that match subpatterns.
 */

#ifndef __regex_core
#define __regex_core

typedef struct _reg_core core_t;
typedef struct _branch   branch_t;

#include "atom.h"

/** match
  *
  * Given a pointer to a string, return NULL if the string doesn't
  * match, or return a pointer to a range object which contains all
  * group capture information. To do: write a much more detailed
  * explanation of this function.
  */
range_t* core_match(core_t*, char*, bts_t*, range_t*,
                              bts_t*, int, char**, char*);

/** index
  *
  * Gives the core's groups number; the top level core held by the
  * pattern struct has index 0.
  */
int core_index(core_t*);

/** groups
  *
  * Get the size of the groups capture array.
  */
int core_groups(core_t*);
int _core_groups(core_t*);

//
// used by factory.c to build a core
//

/** find_core
  *
  * Return the core with the given index. Returns null if not
  * found.
  */
core_t* core_find_core(core_t*, int);

/** add_branch
  *
  * Create a new branch inside of the core. This allows the core
  * to represent '|' syntax where several different patterns can
  * match.
  */
branch_t* core_add_branch(core_t*, branch_t*);

/** add_atom
  *
  * Create a new atom on the current branch and return it.
  */
atom_t* branch_add_atom(branch_t*);

//
// allocation
//

/** new
  *
  * Create a core containing no atoms.
  */
core_t* core_new(int);

/** free
  *
  * Deallocate memory used by the core.
  */
void core_free(core_t*);

#endif
