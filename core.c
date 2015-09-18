/* core.c
 *
 * Implementation of the core regex matching object.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "bts.h"
#include "range.h"
#include "util.h"

// If this number isn't a power of two, then important
//   things will break.
#define DEFCAP 8

/* branch
 *
 * Represents a linear sequence of atoms.
 */
struct _branch {
   atom_t** atoms;      // sequence of atoms
   branch_t* next;      // pointer to next branch
   int load;            // next position to add a atom
};

/* core
 *
 * Declaration of core object.
 */
struct _reg_core {
   branch_t* start;     // first branch
   int index;           // group number; -1 if non-capturing
};


/************************main matching logic*************************/

/** branch_match
  *
  * Do matches until there are no more search positions left on the
  * stack, or until a match is found. We can tell that a match
  * has been found when an index has pushed onto the stack which
  * is equal to obj->load.
  */
static char* branch_match(branch_t* obj, bts_t* stack,
                                   range_t* gr, char* head) {
   while (!bts_empty(stack)) {
      state_t* top = bts_top(stack);
      if (top->index == obj->load) {
         char* str = top->str;
         bts_pop(stack);
         return str;
      }
      atom_match(obj->atoms[top->index], stack, gr, head);
   }
   return NULL;
}

range_t* core_match(core_t* obj,   char* str,
                    bts_t* outer,  range_t* groups, 
                    bts_t* stack,  int nbr,
                    char** back,   char* head) {
   assert(obj);
   assert(str && head && back);
   char* end;  // temporarily stores the end of the group capture
   bool top_level = obj->index == 0 && !groups;
   if (top_level)
      groups = range_new(core_groups(obj));
   assert(groups);
   group_t* captcha = range_group(groups, obj->index);
   if (obj->index >= 0) {     // null out capture for current group
      captcha->begin = NULL;
      captcha->end   = NULL;
   }
   branch_t* curr = obj->start;
   for (int i = 0; i < nbr; ++i) {
      assert(curr->next);
      curr = curr->next;
   }
   if (!stack) {
      stack = bts_new();
      bts_push(stack, 0, str, 0, false, NULL, 0);
   }
   assert(!bts_empty(stack));
   for (;;) {   
      end = branch_match(curr, stack, groups, head);
      if (end) {
         goto FoundMatch;
      }
      if (curr->next) {
         ++nbr;
         curr = curr->next;
         bts_push(stack, 0, str, 0, false, NULL, 0);
      } else {
         bts_free(stack);
         break;
      }
   }
   if (top_level)
      range_free(groups);
   return NULL;

FoundMatch:
   if (obj->index >= 0) {     // record group capture
      captcha->begin = str;
      captcha->end   = end;
   }
   if (outer) {    // save nested backtrack positions?
      if (!bts_empty(stack)) {
         bts_push(outer, 0, str, 0, true, stack, nbr);
      } else if (curr->next) {
         bts_push(stack, 0, str, 0, false, NULL, 0);
         bts_push(outer, 0, str, 0, true, stack, nbr + 1);
      } else {
         bts_free(stack);
         bts_push(outer, 0, str, 0, true, NULL, 0);
      }
   } else {
      bts_free(stack);
   }
   *back = end;
   return groups;
}

/************************branch operations***************************/

/** ensure_capacity
  *
  * Make sure the atoms array is large enough to hold another new
  * atom.
  */
static void branch_ensure_capacity(branch_t* obj) {
   if (obj->load >= DEFCAP && ispow2(obj->load)) {
      int newcap = obj->load * 2;
      atom_t** newarray = calloc(newcap, sizeof(atom_t*));
      assert(newarray);
      memcpy(newarray, obj->atoms, obj->load*sizeof(atom_t*));
      free(obj->atoms);
      obj->atoms = newarray;
   }
}

atom_t* branch_add_atom(branch_t* obj) {
   assert(obj);
   branch_ensure_capacity(obj);
   atom_t* new_atom = atom_new(obj->load);
   obj->atoms[obj->load++] = new_atom;
   return new_atom;
}

/** new
  *
  * Create a new branch.
  */
static branch_t* branch_new() {
   branch_t* obj = malloc(sizeof(branch_t));
   assert(obj);
   obj->load = 0;
   obj->next = NULL;
   obj->atoms = calloc(DEFCAP, sizeof(atom_t*));
   assert(obj->atoms);
   return obj;
}

/** free
  *
  * Free the memory of a branch, including each atom inside of
  * the branch.
  */
static void branch_free(branch_t* obj) {
   if (obj) {
      for (int i = 0; i < obj->load; ++i) {
         atom_free(obj->atoms[i]);
      }
      free(obj->atoms);
      free(obj);
   }
}

/**************************core operations***************************/

int core_index(core_t* obj) {
   assert(obj);
   return obj->index;
}

int core_groups(core_t* obj) {
   return _core_groups(obj) + 1;
}

int _core_groups(core_t* obj) {
   assert(obj);
   int high = obj->index, next;
   branch_t* curr = obj->start;
   while (curr) {
      for (int i = 0; i < curr->load; ++i) {
         if (atom_has_group(curr->atoms[i])) {
            next = atom_highest_index(curr->atoms[i]);
            if (next > high)
               high = next;
         }
      }
      curr = curr->next;
   }
   return high;
}

core_t* core_find_core(core_t* obj, int index) {
   assert(obj && index >= 0);
   if (index == obj->index)
      return obj;
   branch_t* curr = obj->start;
   while (curr) {
      for (int i = 0; i < curr->load; ++i) {
         if (atom_has_group(curr->atoms[i])) {
            core_t* inner = atom_find_core(curr->atoms[i], index);
            if (inner)
               return inner;
         }
      }
      curr = curr->next;
   }
   return NULL;
}

branch_t* core_add_branch(core_t* obj, branch_t* insert) {
   assert(obj);
   if (!obj->start) {
      obj->start = branch_new();
      return obj->start;
   }
   insert->next = branch_new();
   return insert->next;
}

core_t* core_new(int index) {
   core_t* obj = malloc(sizeof(core_t));
   assert(obj);
   obj->index = index;
   obj->start = NULL;
   return obj;
}

void core_free(core_t* obj) {
   if (obj) {
      branch_t* branch = obj->start;
      while (branch) {
         branch_t* next = branch->next;
         branch_free(branch);
         branch = next;
      }
      free(obj);
   }
}

/********************************************************************/
