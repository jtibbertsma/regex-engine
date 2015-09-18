/* tokens.c
 *
 * tlist operations.
 */

#include <stdlib.h>
#include <assert.h>

#include "tokens.h"

/* tlist
 *
 * Definition of the token list.
 */
struct _tlist {
   tnode_t* front;
   tnode_t* back;
};

/* tnode
 *
 * Definition of a node in the token list.
 */
struct _tnode {
   token_t token;
   tnode_t* next;
   tnode_t* prev;
};

//
// node access functions
//

token_t* tnode_token(tnode_t* node) {
   assert(node);
   return &(node->token);
}

tnode_t* tnode_next(tnode_t* node) {
   assert(node);
   return node->next;
}

tnode_t* tnode_prev(tnode_t* node) {
   assert(node);
   return node->prev;
}

/**************************list operations***************************/

token_t* tlist_token(tlist_t* list) {
   assert(list);
   assert(list->front);
   return &(list->front->token);
}

tnode_t* tlist_front(tlist_t* list) {
   assert(list);
   return list->front;
}

tnode_t* tlist_back(tlist_t* list) {
   assert(list);
   return list->back;
}

void tlist_push_back(tlist_t* list, token_t token) {
   assert(list);
   tnode_t* new_node = malloc(sizeof(tnode_t));
   assert(new_node);
   new_node->next = NULL;
   new_node->token = token;
   if (!list->front) {
      new_node->prev = NULL;
      list->front = list->back = new_node;
   } else {
      list->back->next = new_node;
      new_node->prev = list->back;
      list->back = new_node;
   }
}

void tlist_push_front(tlist_t* list, token_t token) {
   assert(list);
   tnode_t* new_node = malloc(sizeof(tnode_t));
   assert(new_node);
   new_node->prev = NULL;
   new_node->token = token;
   if (!list->front) {
      new_node->next = NULL;
      list->front = list->back = new_node;
   } else {
      list->front->prev = new_node;
      new_node->next = list->front;
      list->front = new_node;
   }
}

tnode_t* tlist_insert(tlist_t* list, tnode_t* insert, token_t token) {
   assert(list);
   if (!insert) {
      tlist_push_front(list, token);
      return list->front;
   }
   if (insert == list->back) {
      tlist_push_back(list, token);
      return list->back;
   }
   tnode_t* new_node = malloc(sizeof(tnode_t));
   assert(new_node);
   new_node->token = token;
   new_node->next = insert->next;
   new_node->prev = insert;
   insert->next->prev = new_node;
   insert->next = new_node;
   return new_node;
}

void tlist_pop_front(tlist_t* list) {
   assert(list->front);
   tnode_t* old = list->front;
   list->front = list->front->next;
   if (list->front)
      list->front->prev = NULL;
   else
      list->back = NULL;
   free(old);
}

bool tlist_empty(tlist_t* list) {
   assert(list);
   return !list->front;
}

int tlist_size(tlist_t* list) {
   assert(list);
   int size = 0;
   for (tnode_t* t = list->front; t; t = t->next) {
      ++size;
   }
   return size;
}

tlist_t* tlist_slice(tlist_t* list, tnode_t* start,
                                  tnode_t* end, tnode_t** insert) {
   assert(list && start && end && insert);
   tlist_t* new = tlist_new();

   /* special case; slice whole list */
   if (start == list->front && end == list->back) {
      new->front = list->front; new->back = list->back;
      list->front = list->back = NULL;
      *insert = NULL;
      return new;
   }

   /* normal cases */
   tnode_t* prev = start->prev;
   tnode_t* next = end->next;
   start->prev = NULL;
   end->next = NULL;
   new->front = start;
   new->back = end;
   if (prev) {
      prev->next = next;
      *insert = prev;
   } else {
      list->front = next;
      *insert = NULL;
   }
   if (next) {
      next->prev = prev;
   } else {
      list->back = prev;
   }
   return new;
}

tlist_t* tlist_new() {
   tlist_t* list = malloc(sizeof(tlist_t));
   assert(list);
   list->front = list->back = NULL;
   return list;
}
   

void tlist_free(tlist_t* list) {
   assert(list);
   while (list->front) {
      tlist_pop_front(list);
   }
   free(list);
}

void tlist_obliterate(tlist_t* list) {
   assert(list);
   while (list->front) {
      token_t token = list->front->token;
      switch (token.flag) {
         case CLASS: case NCLASS:
            class_free(token.data.class);
            break;
         case GROUP: case ATOMIC: case LOOKAHEAD: case NLOOKAHEAD:
            tlist_obliterate(token.data.group);
            break;
         case LITERAL:
            free(token.data.literal);
            break;
         case STRING: case NAME:
            free(token.data.string);
            break;
         default:
            break;
      }
      tlist_pop_front(list);
   }
   free(list);
}

/****************************tlist_hook******************************/

#ifdef TLIST_HOOK

static uint32_t lowest(class_t* tree) {
   uint32_t j = 0;
   if (class_empty(tree))
      return 0xFFFFFFFF;
   while (++j) {
      if (class_search(tree, j))
         return j;
   }
   return 0xFFFFFFFF;
}

static char* token_info(token_t* token) {
   static char buff[1 << 12];
   switch (token->flag) {
      case LITERAL: {
         char* str = u8_duplicate(token->data.literal);
         sprintf(buff, "Literal: U+%X; %s",
                       u8_deref(token->data.literal), str);
         free(str);
         return buff;
      }

      case STRING: 
         sprintf(buff, "String: \"%s\"", token->data.string);
         return buff;

      case NAME: 
         sprintf(buff, "Name: \"%s\"", token->data.string);
         return buff;

      case ALTERNATOR: 
         sprintf(buff, "Alternator");
         return buff;

      case CLASS: 
         sprintf(buff, "Class: Lowest code point: U+%4X",
                                 lowest(token->data.class));
         return buff;

      case NCLASS: 
         sprintf(buff, "Negated Class: Lowest code point: U+%4X",
                                 lowest(token->data.class));
         return buff;

      case GROUP: 
         sprintf(buff, "Group");
         return buff;

      case ATOMIC: 
         sprintf(buff, "Atomic Group");
         return buff;

      case RANGE: 
         sprintf(buff, "Range: %d to %d", token->data.range.begin,
            token->data.range.end == -1 ? 0x7FFFFFFF
                                        : token->data.range.end);
         return buff;

      case LAZY: 
         sprintf(buff, "Lazy Quantifier");
         return buff;

      case POSSESSIVE: 
         sprintf(buff, "Possessive Quantifier");
         return buff;

      case REFERENCE: 
         sprintf(buff, "Backreference: Reference to group %d",
                                                   token->ngr);
         return buff;

      case LOOKAHEAD: 
         sprintf(buff, "Lookahead Assertion");
         return buff;

      case NLOOKAHEAD: 
         sprintf(buff, "Negative Lookahead Assertion");
         return buff;
/*
      case LOOKBEHIND: 
         sprintf(buff, "Lookbehind Assertion");
         return buff;

      case NLOOKBEHIND: 
         sprintf(buff, "Negative Lookbehind Assertion");
         return buff;
*/
      case WORDANCH: 
         sprintf(buff, "Word Barrier Anchor");
         return buff;

      case NWORDANCH: 
         sprintf(buff, "Negated Word Barrier Anchor");
         return buff;

      case STANCH: 
         sprintf(buff, "Start Anchor");
         return buff;

      case EDGEANCH: 
         sprintf(buff, "End Anchor");
         return buff;

      case SUBROUTINE: 
         sprintf(buff, "Subroutine Call: Call to group %d",
                                                   token->ngr);
         return buff;

      case EMPTY: 
         sprintf(buff, "Empty Regex");
         return buff;
   }
   return "";
}

#define HasList(FLAG) (\
   FLAG == GROUP       || \
   FLAG == ATOMIC      || \
   FLAG == LOOKAHEAD   || \
   FLAG == NLOOKAHEAD     \
)

static void hook_recurse(tlist_t* list, int level) {
   printf("\n!!!!!!!!!!!!!!!!!!!!!!!!! Level %d\n\n", level);
   tnode_t* curr = list->front;
   for (int i = 0; curr; curr = curr->next) {
      token_t token = curr->token;
      printf("%2d\t%s\n", ++i, token_info(&token));
      if (HasList(token.flag)) {
         hook_recurse(token.data.group, level + 1);
         printf("\n!!!!!!!!!!!!!!!!! Back To Level %d\n\n", level);
      }
   }
}

void tlist_hook(tlist_t* list) {
   hook_recurse(list, 0);
}

#endif  /* ifdef TLIST_HOOK */

/********************************************************************/
