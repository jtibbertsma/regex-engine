/* clist.c
 *
 * Implementation for the atom list.
 */

#include <stdlib.h>
#include <assert.h>

#include "atom.h"
#include "clist.h"

struct _clist {
   int index;
   atom_t* pointer;
   clist_t* next;
};

clist_t* clist_next(clist_t* prev) {
   assert(prev);
   return prev->next;
}

atom_t* clist_pointer(clist_t* curr) {
   assert(curr);
   return curr->pointer;
}

int clist_index(clist_t* curr) {
   assert(curr);
   return curr->index;
}

clist_t* clist_new(clist_t* prev, atom_t* c, int i) {
   if (prev && !prev->pointer) {
      prev->pointer = c;
      prev->index = i;
      return prev;
   }
   clist_t* newone = malloc(sizeof(clist_t));
   assert(newone);
   newone->next = NULL;
   if (!prev) {
      newone->pointer = NULL;
      newone->index = -1;
   } else {
      newone->pointer = c;
      newone->index = i;
   }
   return newone;
}

void clist_free(clist_t* node) {
   while (node) {
      clist_t* next = node->next;
      free(node);
      node = next;
   }
}
