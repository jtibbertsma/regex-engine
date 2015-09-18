/* atom.c
 *
 * Each atom represents a portion of a regular expression, and
 * tries to match against a string.
 */

#include <assert.h>
#include <stdlib.h>

#include "atom.h"
#include "util.h"
#include "u8_translate.h"


/* enum info
 *
 * Keep track of various information about an atom. 
 */
typedef enum {

   // atom types
   Uninitialized   = 1 << 31,
   Class           = 1 << 30,
   String          = 1 << 29,
   Group           = 1 << 28,
   Atomic          = 1 << 27,
   Backreference   = 1 << 26,
   Subroutine      = 1 << 25,
   LookAhead       = 1 << 24,
   WordAnchor      = 1 << 23,
   EdgeAnchor      = 1 << 22,
   
   // atom attributes
   Invert          = 2,
   Greedy          = 1
} atom_info;

/* atom_info macros
 *
 * Used for accessing atom_info from an int. The 24 bits on the left
 * of an int are used to tell what type an atom is; only one of these
 * bits should be set at a time. The 8 bits on the right are booleans
 * used to encode attributes.
 */

/** SetType
  *
  * Change the type label to a new atom type.
  */
#define SetType(INFO, TYPE) \
   INFO &= 0xFF; \
   INFO |= TYPE

/** GetType
  *
  * Used for using a switch statement with the atom type.
  */
#define GetType(INFO) (atom_info) (INFO & ~0xFF)

/** TestOpt
  *
  * Check whether an option is set.
  */
#define TestOpt(INFO, OPT) (INFO & OPT)

/** SetOpt
  *
  * Set an option.
  */
#define SetOpt(INFO, OPT, BOOL) \
   INFO = BOOL ? INFO | OPT : INFO & ~OPT

/* atom
 *
 * Declaration of the atom structure.
 */
struct _reg_atom {
   int index;
   int info;
   union {
      class_t* class;
      char*    string;
      core_t*  group;
      int      index;
   } data;
   urange32_t  range;
};

char* trash;   // nothing

/****************************single matches**************************/

/** match_string
  *
  * Match the string against string in atom->data.string.
  */
static char* match_string(atom_t* atom, char* str) {
   char* match = atom->data.string;
   while (*match != '\0') {
      if (*str++ != *match++)
         return NULL;
   }
   return str;
}

/** match_reference
  *
  * Search for a backreference using the captured groups array.
  */
static char* match_reference(atom_t* atom, char* str, range_t* gr) {
   char* begin = range_group(gr, atom->data.index)->begin;
   char* end   = range_group(gr, atom->data.index)->end;
   if (!begin)
      return NULL;
   while (begin != end) {
      if (*str == '\0')
         return NULL;
      if (*begin++ != *str++)
         return NULL;
   }
   return str;
}

#define TRUe  TestOpt(atom->info, Invert) ? NULL : str
#define FALSe TestOpt(atom->info, Invert) ? str  : NULL

/** match_class
  *
  * Do a single match for the tree case, which just involves testing
  * a single character against a class.
  */
static char* match_class(atom_t* atom, char* str) {
   u8cdpnt_t* cp = u8_decode(str);
   str = u8_end(cp);
   bool isel = class_search(atom->data.class, u8_deref(cp));
   free(cp);
   if (isel)
      return TRUe;
   return FALSe;
}

/** match_lookahead
  *
  * Match against group, but consume none of str.
  */
static char* match_lookahead(atom_t* atom, char* str,
                                          range_t* gr, char* head) {
   gr = core_match(atom->data.group, str,
                            NULL, gr, NULL, 0, &trash, head);
   if (gr) {
      return TRUe;
   }
   return FALSe;
}

/** match_wordanchor
  *
  * Match the empty string at the border of a word character and a non
  * word character.
  */
static char* match_wordanchor(atom_t* atom, char* str, char* head) {
   bool curr_is_word = class_search(word_characters, *str);
   bool prev_is_word = class_search(word_characters, *(str-1));
   bool curr_is_head =  str == head;
   bool curr_is_end  = *str == '\0';
   if (curr_is_head && curr_is_end)
      return FALSe;
   if (curr_is_head) {
      if (curr_is_word)
         return TRUe;
      return FALSe;
   }
   if (curr_is_end) {
      if (prev_is_word)
         return TRUe;
      return FALSe;
   }
   if (curr_is_word)
      return prev_is_word ? FALSe : TRUe;
   else
      return prev_is_word ? TRUe : FALSe;
}

/** match_edgeanchor
  *
  * If invert is true, match the beginning of the input string. If
  * invert is false, match the end of the input string.
  */
static char* match_edgeanchor(atom_t* atom, char* str, char* head) {
   if (TestOpt(atom->info, Invert))
      return str == head ? str : NULL;
   return *str == '\0' ? str : NULL;
}

/** match_subroutine
  *
  * Do a group match, keep track of backtracking tokens, but don't
  * overwrite group capture information.
  */
static char* match_subroutine(atom_t* atom,   uint32_t matches,
                              char* str,      range_t* gr,
                              bts_t* stack,   bts_t* inner,
                              int nbr,        char* head,
                                              range_t* nest) {
   char* end = NULL;
   if (!nest)
      nest = range_copy(gr);
   range_t* temp = nest;
   nest = core_match(atom->data.group, str,
                    stack, nest, inner, nbr, &end, head);
   if (!nest) {
      range_free(temp);
      return NULL;
   }
   if (!bts_top(stack)->inner) {
      bts_pop(stack);
      range_free(nest);
   } else {
      bts_set_top(stack, atom->index, matches, nest);
   }
   return end;
}

/** match_atomic
  *
  * Do a group match, but without keeping track of group backtracking
  * information.
  */
static char* match_atomic(atom_t* atom, char* str,
                                          range_t* gr, char* head) {
   char* end;
   gr = core_match(atom->data.group, str,
                              NULL, gr, NULL, 0, &end, head);
   if (!gr)
      return NULL;
   return end;
}

/** match_group
  *
  * Do a single match for the group case, which involves calling
  * core_match on a nested core.
  */
static char* match_group(atom_t* atom,    uint32_t matches,
                         char* str,      range_t* gr,
                         bts_t* stack,   bts_t* inner,
                         int nbr,        char* head) {
   char* end = NULL;
   gr = core_match(atom->data.group, str,
                    stack, gr, inner, nbr, &end, head);
   if (!gr)
      return NULL;
   if (!bts_top(stack)->inner) {
      bts_pop(stack);
   } else {
      bts_set_top(stack, atom->index, matches, NULL);
   }
   return end;
}

/**************************repetition logic**************************/

/** DoMatch
  *
  * Call the appropriate matching function, classting str to the return
  * value.
  */
#define DoMatch(FLAG) \
   switch (FLAG) { \
      case Class: \
         str = match_class(atom, str); \
         break; \
      case Backreference: \
         str = match_reference(atom, str, gr); \
         break; \
      case Group: \
         str = match_group(atom, matches, str, gr, \
                                    stack, inner, nbr, head); \
         break; \
      case Subroutine: \
         str = match_subroutine(atom, matches, str, gr, \
                              stack, inner, nbr, head, nest); \
         break; \
      case Atomic: \
         str = match_atomic(atom, str, gr, head); \
         break; \
      default: \
         assert(false); \
   }

/** greedy_match
  *
  * Do as many matches as possible.
  */
static void greedy_match(atom_t* atom, bts_t* stack,
                                            range_t* gr, char* head) {
   state_t*   top        = bts_top(stack);
   char*      str        = top->str;
   uint32_t   matches    = top->matches;
   bool       recursive  = top->recursive;
   bts_t*     inner      = top->inner;
   int        nbr        = top->nbr;
   range_t*   nest       = top->nest;
   bts_pop(stack);
   for (;; ++matches) {
      if (matches >= atom->range.lo && matches <= atom->range.hi) {
         bts_push(stack, atom->index + 1, str, 0, false, NULL, 0);
      }
      if (matches >= atom->range.hi || *str == '\0')
         break;
      DoMatch(GetType(atom->info));
      if (!str)
         break;
      if (recursive) {
         recursive = false;
         inner = NULL;
         nbr = 0;
      }
   }
}

/** lazy_match
  *
  * Do as few matches as possible.
  */
static void lazy_match(atom_t* atom, bts_t* stack,
                                          range_t* gr, char* head) {
   state_t*   top        = bts_top(stack);
   char*      str        = top->str;
   uint32_t   matches    = top->matches;
   bool       recursive  = top->recursive;
   bts_t*     inner      = top->inner;
   int        nbr        = top->nbr;
   range_t*   nest       = top->nest;
   bts_pop(stack);
   for (;; ++matches) {
      if (matches > atom->range.hi || *str == '\0')
         break;
      char* temp = str;
      if (matches != atom->range.hi)
         DoMatch(GetType(atom->info));
      if (str && matches >= atom->range.lo && matches < atom->range.hi) {
         bts_push(stack, atom->index, str, matches + 1, false, NULL, 0);
      }
      if (matches >= atom->range.lo && matches <= atom->range.hi) {
         bts_push(stack, atom->index + 1, temp, 0, false, NULL, 0);
         break;
      }
      if (!str) {
         break;
      }
      if (recursive) {
         recursive = false;
         inner = NULL;
         nbr = 0;
      }
   }
}

/************************main matching logic*************************/

/** NoRangeCase
  *
  * Do a non-range case match.
  */
#define NoRangeCase(OPERATION) \
   bts_pop(stack); \
   str = OPERATION; \
   if (str) \
      bts_push(stack, atom->index + 1, str, 0, false, NULL, 0); \
   return

void atom_match(atom_t* atom, bts_t* stack,
                                      range_t* gr, char* head) {
   assert(atom);
   state_t* top = bts_top(stack);
   char* str = top->str;
   assert(GetType(atom->info) != Uninitialized);
   
   // cases that can't involve ranges
   switch (GetType(atom->info)) {
      case String:
         NoRangeCase(match_string(atom, str));
      case LookAhead:
         NoRangeCase(match_lookahead(atom, str, gr, head));
      case WordAnchor:
         NoRangeCase(match_wordanchor(atom, str, head));
      case EdgeAnchor:
         NoRangeCase(match_edgeanchor(atom, str, head));
      default:
         break;
   }
      
   // other cases
   if (TestOpt(atom->info, Greedy))
      greedy_match(atom, stack, gr, head);
   else
      lazy_match(atom, stack, gr, head);
}

/**************************atom operations**************************/

void atom_set_class(atom_t* atom, class_t* that) {
   assert(atom);
   assert(that);
   assert(GetType(atom->info) == Uninitialized);
   SetType(atom->info, Class);
   atom->data.class = that;
}

void atom_set_string(atom_t* atom, char* that) {
   assert(atom);
   assert(that);
   assert(GetType(atom->info) == Uninitialized);
   SetType(atom->info, String);
   atom->data.string = that;
}

void atom_set_core(atom_t* atom, core_t* that, int flag) {
   assert(atom);
   assert(that);
   assert(GetType(atom->info) == Uninitialized);
   switch (flag) {
      case 0: SetType(atom->info, Group);      break;
      case 1: SetType(atom->info, Atomic);     break;
      case 2: SetType(atom->info, LookAhead);  break;
 //   case 3: SetType(atom->info, LookBehind); break;
      case 4: SetType(atom->info, Subroutine); break;
      default:                                 break;
   }
   atom->data.group = that;
}

void atom_set_anchor(atom_t* atom, int flag) {
   assert(atom);
   switch (flag) {
      case 1:
         SetType(atom->info, WordAnchor);
         break;
      case 2:
         SetType(atom->info, EdgeAnchor);
         break;
      default:
         assert(false);
   }
}

void atom_set_invert(atom_t* atom, bool val) {
   assert(atom);
   SetOpt(atom->info, Invert, val);
}

void atom_set_range(atom_t* atom, int a, int b) {
   assert(atom);
   atom->range.lo = a;
   atom->range.hi = b == -1 ? MAXREPS : b;
}

void atom_set_reference(atom_t* atom, int ref) {
   assert(atom);
   assert(GetType(atom->info) == Uninitialized);
   SetType(atom->info, Backreference);
   atom->data.index = ref;
}

void atom_set_greedy(atom_t* atom, bool val) {
   assert(atom);
   SetOpt(atom->info, Greedy, val);
}

bool atom_has_group(atom_t* atom) {
   assert(atom);
   return GetType(atom->info) == Atomic
       || GetType(atom->info) == Group
       || GetType(atom->info) == LookAhead;
}

int atom_highest_index(atom_t* atom) {
   assert(atom_has_group(atom));
   return _core_groups(atom->data.group);
}

core_t* atom_find_core(atom_t* atom, int index) {
   assert(atom_has_group(atom));
   return core_find_core(atom->data.group, index);
}

atom_t* atom_new(int index) {
   atom_t* atom = malloc(sizeof(atom_t));
   assert(atom);
   atom->index = index;
   SetType(atom->info, Uninitialized);
   atom->range.lo = 1;
   atom->range.hi = 1;
   SetOpt(atom->info, Greedy, true);
   return atom;
}

void atom_free(atom_t* atom) {
   if (atom) {
      switch (GetType(atom->info)) {
         case Class:
            class_free(atom->data.class);
            break;
         case String:
            free(atom->data.string);
            break;
         case Group: case Atomic: case LookAhead: //case LookBehind:
            core_free(atom->data.group);
            break;
         default:
            break;
      }
      free(atom);
   }
}

/********************************************************************/
