/* factory.c
 *
 * Create a regex core object from the tokens of tokens from the
 * parser.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include "factory.h"
#include "clist.h"

static core_t* _build_core(tlist_t* tokens, int index, clist_t* subs) {
   assert(tokens);
   assert(!tlist_empty(tokens));
   core_t* core = core_new(index);
   branch_t* branch = core_add_branch(core, NULL);
   atom_t* curr = NULL;

   do {
      token_t* token = tlist_token(tokens);
      switch (token->flag) {

         /* Empty regular expression matches any string. Do nothing.
          */
         case EMPTY:
            break;

         /* Alternator character. This creates a new branch within
          * the core object.
          */
         case ALTERNATOR:
            branch = core_add_branch(core, branch);
            break;

         /* String case. Give the string to the new atom.
          */
         case STRING:
            curr = branch_add_atom(branch);
            atom_set_string(curr, token->data.string);
            break;

         /* Character tree case. Same as literal case except that
          * the tree is already made by the lexer.
          */
         case CLASS:
            curr = branch_add_atom(branch);
            atom_set_class(curr, token->data.class);
            atom_set_invert(curr, false);
            break;
         case NCLASS:
            curr = branch_add_atom(branch);
            atom_set_class(curr, token->data.class);
            atom_set_invert(curr, true);
            break;

         /* Repetition cases. Sets a range in the current atom.
          */
         case RANGE:
            assert(curr);
            atom_set_range(curr, token->data.range.begin,
                                 token->data.range.end);
            break;

         /* Group case. Create a new core object and put it inside
          * the new atom.
          */
         case GROUP:
            curr = branch_add_atom(branch);
            atom_set_core(curr,
                  _build_core(token->data.group, token->ngr, subs), 0);
            break;
         case ATOMIC:
            curr = branch_add_atom(branch);
            atom_set_core(curr,
                  _build_core(token->data.group, token->ngr, subs), 1);
            break;

         /* Backreference case. Set the backreference group number
          * inside the new atom.
          */
         case REFERENCE:
            curr = branch_add_atom(branch);
            atom_set_reference(curr, token->ngr);
            break;
         
         /* Lookahead case. Set the group in the new atom and tree
          * invert to be true in the negated case.
          */
         case LOOKAHEAD:
            curr = branch_add_atom(branch);
            atom_set_core(curr, _build_core(token->data.group,
                                   token->ngr, subs), 2);
            atom_set_invert(curr, false);
            break;
         case NLOOKAHEAD:
            curr = branch_add_atom(branch);
            atom_set_core(curr, _build_core(token->data.group,
                                   token->ngr, subs), 2);
            atom_set_invert(curr, true);
            break;
            
         /* Subroutine call. We need to save the pointer to the new
          * atom, along with the index of the core that atom will
          * need to point, so that we can connect the atoms to their
          * cores after the cores are built.
          */
         case SUBROUTINE:
            curr = branch_add_atom(branch);
            subs = clist_new(subs, curr, token->ngr);
            break;
            
         /* Match the empty string the at one of the following
          * locations: between a word character and a non-word
          * character, between a word character and a null terminating
          * character, and at the beginning of the string if the first
          * character is a word character.
          */
         case WORDANCH:
         	curr = branch_add_atom(branch);
         	atom_set_anchor(curr, 1);
         	break;
         case NWORDANCH:
         	curr = branch_add_atom(branch);
         	atom_set_anchor(curr, 1);
         	atom_set_invert(curr, true);
         	break;
         	
         /* Match the empty string at the beginning of a string.
          */
         case STANCH:
         	curr = branch_add_atom(branch);
         	atom_set_anchor(curr, 2);
         	atom_set_invert(curr, true);
         	break;
         
         /* Match the empty string at the end of a string.
          */
         case EDGEANCH:
         	curr = branch_add_atom(branch);
         	atom_set_anchor(curr, 2);
         	break;

         /* Make the current atom lazy instead of greedy.
          */
         case LAZY:
            assert(curr);
            atom_set_greedy(curr, false);
            break;
            
         /* Should have been converted already.
          */
         case POSSESSIVE: case LITERAL: case NAME:
         	assert(false);
      }
      tlist_pop_front(tokens);
   } while (!tlist_empty(tokens));

   tlist_free(tokens);
   return core;
}

core_t* build_core(tlist_t* tokens) {
   clist_t* subs = clist_new(NULL, NULL, 0);
   clist_t* cl   = subs;
   core_t* core  = _build_core(tokens, 0, subs);
   for (atom_t* curr = clist_pointer(cl); curr; ) {
      atom_set_core(curr, core_find_core(core, clist_index(cl)), 4);
      cl = clist_next(cl);
      curr = !cl ? NULL : clist_pointer(cl);
   }
   clist_free(subs);
   return core;
}

/********************************************************************/
