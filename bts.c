/* bts.c
 *
 * Implementation of backtrack stack.
 */

#include <assert.h>
#include <stdlib.h>

#include "bts.h"

typedef struct _bnode bnode_t;

struct _bnode {
   state_t state;
   bnode_t* next;
};

struct _bts {
   bnode_t* top;
};

#define Assign(STATE) \
   STATE.index     = ind; \
   STATE.str       = str; \
   STATE.matches   = mat; \
   STATE.recursive = rec; \
   STATE.inner     = inn; \
   STATE.nbr       = nbr; \
   STATE.nest      = NULL


/************************public functions****************************/

void bts_push(bts_t* obj, int ind, char* str, uint32_t mat,
                                    bool rec, bts_t* inn, int nbr) {
   assert(obj);
   assert(str);
   if (!obj->top) {
      obj->top = malloc(sizeof(bnode_t));
      assert(obj->top);
      obj->top->next = NULL;
      Assign(obj->top->state);
   } else {
      bnode_t* new_node = malloc(sizeof(bnode_t));
      assert(new_node);
      new_node->next = obj->top;
      Assign(new_node->state);
      obj->top = new_node;
   }
}

state_t* bts_top(bts_t* obj) {
   assert(obj);
   assert(obj->top);
   return &(obj->top->state);
}

void bts_set_top(bts_t* obj, int index, uint32_t matches,
                                                range_t* nest) {
   assert(obj);
   assert(obj->top);
   obj->top->state.index = index;
   obj->top->state.matches = matches;
   obj->top->state.nest = nest;
}

void bts_pop(bts_t* obj) {
   assert(obj);
   assert(obj->top);
   bnode_t* old_top = obj->top;
   obj->top = obj->top->next;
   free(old_top);
}

bool bts_empty(bts_t* obj) {
   assert(obj);
   return !obj->top;
}

bts_t* bts_new() {
   bts_t* obj = malloc(sizeof(bts_t));
   assert(obj);
   obj->top = NULL;
   return obj;
}

void bts_free(bts_t* obj) {
   if (obj) {
      while (obj->top) {
         if (obj->top->state.recursive) {
            bts_free(obj->top->state.inner);
            if (obj->top->state.nest)
               range_free(obj->top->state.nest);
         }
         bts_pop(obj);
      }
      free(obj);
   }
}

/********************************************************************/
