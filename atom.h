/* atom.h
 *
 * One atom matches one token zero or more times. A single atom
 * can match against a tree, or it could hold a nested core to
 * search a group.
 */

#ifndef __regex_atom
#define __regex_atom

typedef struct _reg_atom atom_t;

#include <stdbool.h>
#include "class.h"
#include "bts.h"
#include "core.h"

// maximum number of repetitions
#define MAXREPS 1000000000

// used by the matching logic to implement word anchors
extern class_t* word_characters;


/** match
  *
  * Do a match for a single atom of the regular expression, possibly
  * including repetitions.
  */
void atom_match(atom_t*, bts_t*, range_t*, char*);

/** give_set
  *
  * "Gives" a tree to the atom in the sense that the atom now owns
  * the tree and is responsible for freeing it. A tree in a atom is
  * used to denote a tree of characters that are accepted by the atom,
  * so that a atom can test whether it accepts a particular character
  * in constant time.
  */
void atom_set_class(atom_t*, class_t*);

/** give_string
  *
  * Give a string to a atom creating a atom that matches a string
  * of literals once.
  */
void atom_set_string(atom_t*, char*);

/** give_core
  *
  * Give atom possession of a nested core. This construct is used to
  * match groups within a regular expression. The int sets the atom's
  * internal flag:
  * 0 -> group, 1 -> atomic, 2 -> lookahead, 3 -> lookbehind,
  * 4 -> subroutine.
  */
void atom_set_core(atom_t*, core_t*, int);

/** set_anchor
  *
  * Set the atom to be anchor. The int argument tells the engine
  * what sort of anchor to create; 1 is a word anchor, and 2 is
  * a beginning or end anchor.
  */
void atom_set_anchor(atom_t*, int);

/** set_invert
  *
  * Normally a atom accepts characters that are in its tree; this
  * function can be used to change a atoms behavior to accepting
  * any character NOT in its class. This same field is also used in
  * the lookahead case to tell whether it's a normal or
  * a negative lookahead.
  */
void atom_set_invert(atom_t*, bool);

/** set_range
  *
  * Set a atom to accept repetitions; for example, the regex construct
  * a{2,4} accepts 'aa', 'aaa', and 'aaaa'. If the third argument
  * is a -1, the atom will not have a max amount of repetitions.
  */
void atom_set_range(atom_t*, int, int);

/** set_reference
  *
  * Set a atom to search for a match to a previously captured group;
  * i.e. '(suki)\1' matches 'sukisuki'.
  */
void atom_set_reference(atom_t*, int);

/** set_greedy
  *
  * Set the greediness attribute of the atom.
  */
void atom_set_greedy(atom_t*, bool);

/** has_group
  *
  * Returns true if the atom contains a group which keeps track
  * of group captures.
  */
bool atom_has_group(atom_t*);

/** highest_index
  *
  * Call _core_groups function on nested core.
  */
int atom_highest_index(atom_t*);

/** find_core
  *
  * Helper function for core_find_core.
  */
core_t* atom_find_core(atom_t*, int);

/** new
  *
  * Create a new atom.
  */
atom_t* atom_new(int);

/** free
  *
  * Deallocate the atom and anything it points to.
  */
void atom_free(atom_t*);

#endif
