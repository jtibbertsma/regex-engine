/* tokens.h
 *
 * Defines the token struct, token-list node struct, the operations
 * for the token list, and the token list itself.
 */

#ifndef __regex_tokens
#define __regex_tokens

#include <stdbool.h>
#include "class.h"
#include "u8_translate.h"
#include "hooks.h"


/* enum tflag
 *
 * A token contains a flag of this type. Tells us what the purpose of
 * the token is and what information it contains.
 */
typedef enum {
   LITERAL,     // single literal character
   STRING,      // match a series of literals
   NAME,        // stores the name of a named group
   ALTERNATOR,  // alternator; x|y where x and y are patterns, matches a
                //   string that matches either x or y.
   CLASS,       // match any single character in a class.
   NCLASS,      // negated tree; match any character not in a class.
   GROUP,       // nested regular expression within parens.
   ATOMIC,      // group that the engine doesn't backtrack into
   RANGE,       // repeat for a tree amount of times.
   LAZY,        // force the previous quantifier to match as little as
                //   possible
   POSSESSIVE,  // force the previous quantifier to match as much as
                //   possible without backtracking.
   REFERENCE,   // match the same thing that the backreferenced group
                //   matched previously.
   LOOKAHEAD,   // positive lookahead assertion
   NLOOKAHEAD,  // negative lookahead assertion
//   LOOKBEHIND,  // positive lookbehind assertion
//   NLOOKBEHIND, // negative lookbehind assertion
   WORDANCH,    // match the empty string on the border of a word
                //   character
   NWORDANCH,   // match anywhere except a word boundary.
   STANCH,      // match the beginning of a string
   EDGEANCH,    // match the end of a string
   SUBROUTINE,  // possibly recursive call to a group
   EMPTY        // regular expression is empty; matches everything
} tflag;

typedef struct _tlist tlist_t;


/* token
 *
 * Contains information to build part of the pattern matching
 * structure.
 */
typedef struct {
   union {
      u8cdpnt_t* literal;
      struct {
         int begin;
         int end;
      } range;
      char* string;
      class_t* class;
      tlist_t* group;
   } data;
	int ngr;
   tflag flag;
} token_t;

typedef struct _tnode tnode_t;

/** tnode_token
  *
  * Gives a pointer to the token in the node.
  */
token_t* tnode_token(tnode_t*);

/** tnode_prev
  *
  * Gives a pointer to the previous node.
  */
tnode_t* tnode_prev(tnode_t*);

/** tnode_next
  *
  * Gives a pointer to the next node.
  */
tnode_t* tnode_next(tnode_t*);

//
// list operations
//

/** token
  *
  * Get a pointer to the token in the front node of the list.
  */
token_t* tlist_token(tlist_t*);

/** front
  *
  * Get a pointer to the front node of the list.
  */
tnode_t* tlist_front(tlist_t*);

/** back
  *
  * Get a pointer to the last node in the list.
  */
tnode_t* tlist_back(tlist_t*);

/** push
  *
  * Push a new token onto the list at the end.
  */
void tlist_push_back(tlist_t*, token_t);

/** push
  *
  * Push a new token onto the list at the beginning.
  */
void tlist_push_front(tlist_t*, token_t);

/** list_insert
  *
  * Create a new list node. The new node is the node to be inserted
  * after; it should be an search of the list being passed into
  * the function. If the node is null, the node will be the new front
  * of the list.
  */
tnode_t* tlist_insert(tlist_t*, tnode_t*, token_t);

/** pop
  *
  * Pops the token at the front of the list. Data structures
  * that a token keeps track of must be freed explicitly.
  */
void tlist_pop_front(tlist_t*);

/** empty
  *
  * Checks if the list is empty.
  */
bool tlist_empty(tlist_t*);

/** size
  *
  * Gives the number of nodes in the list.
  */
int tlist_size(tlist_t*);

/** slice
  *
  * Slice a new list from a given list. The second and third
  * arguments are the first and last node of the new list.
  * The fourth argument is tree to the node which comes before
  * the list which was sliced out. Returns the new list.
  */
tlist_t* tlist_slice(tlist_t*, tnode_t*, tnode_t*, tnode_t**);

/** new
  *
  * Create a new list.
  */
tlist_t* tlist_new();

/** free
  *
  * Free the list, but don't deallocate data held by tokens.
  */
void tlist_free(tlist_t*); 

/** obliterate
  *
  * Frees the list, and also deallocate data held by tokens.
  */
void tlist_obliterate(tlist_t*);

/** hook
  *
  */
#ifdef TLIST_HOOK
void tlist_hook(tlist_t*);
#endif /* ifdef TLIST_HOOK */

#endif
