/* obhash.c
 *
 * Implementation of generic string hashtable.
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "obhash.h"
#include "util.h"


#define TABLESIZE 7

typedef struct _hnode hnode_t;

struct _obhash {
   void (*obfree)(void*);
   hnode_t** chains;
   int size;
   int load;
};

/* hnode
 *
 * Node that holds an object, a key, and a pointer to another node.
 */
struct _hnode {
   hnode_t* next;
   char* key;
   void* obj;
};


/**************************static functions**************************/

/** free_chains
  *
  * Deallocates all of the nodes in array of nodes, including the
  * objects and keys, but doesn't free the array itself.
  */
static void free_chains(hnode_t** chains,
                               int size, void (*obfree)(void*)) {
   for (int i = 0; i < size; ++i) {
      hnode_t* node = chains[i];
      while (node) {
         hnode_t* next = node->next;
         if (obfree)
            obfree(node->obj);
         free(node->key);
         free(node);
         node = next;
      }
      chains[i] = NULL;
   }
}

/** hnode_new
  *
  * Make a new hnode and tree the next field to NULL. Returns the
  * new node.
  */
static hnode_t* hnode_new(char* key, void* obj) {
   hnode_t* newnode = malloc(sizeof(hnode_t));
   assert(newnode);
   newnode->key = key;
   newnode->obj = obj;
   newnode->next = NULL;
   return newnode;
}

/** get_node
  *
  * For a given string return NULL if the bucket is empty. If a node is
  * returned, then if the bool is set to true, it is the node
  * containing the key. If false, then the node is the last in the
  * chain, and doesn't contain the correct key.
  */
static hnode_t* get_node(hnode_t** chains, int index,
                                   char* key, bool* found) {
   hnode_t* node = chains[index];
   if (!node)
      return NULL;
   for (;; node = node->next) {
      if (strcmp(key, node->key) == 0) {
         *found = true;
         return node;
      }
      if (!node->next)
         break;
   }
   *found = false;
   return node;
}

/** add_to_chains
  *
  * Add an object to the hnode array of chains. Return true if a new
  * node is created.
  */
static bool add_to_chains(hnode_t** chains, int size,
                        char* key, void* obj, void (*obfree)(void*)) {
   bool found;
   int i = strhash(key) % size;
   hnode_t* node = get_node(chains, i, key, &found);
   if (!node) {
      chains[i] = hnode_new(key, obj);
      return true;
   } else if (found) {
      if (obfree)
         obfree(node->obj);
      node->obj = obj;
      return false;
   } else {
      node->next = hnode_new(key, obj);
      return true;
   }
}

/** expand_table
  *
  * Double the internal array of the hash table.
  */
static void expand_table(obhash_t* table) {
   int newsize = table->size * 2 + 1;
   hnode_t** newchains = calloc(newsize, sizeof(hnode_t*));
   assert(newchains);
   for (int i = 0; i < table->size; ++i) {
      hnode_t* node = table->chains[i];
      while (node) {
         hnode_t* next = node->next;
  (void) add_to_chains(newchains, newsize,
                              node->key, node->obj, table->obfree);
         free(node);
         node = next;
      }
   }
   free(table->chains);
   table->chains = newchains;
   table->size = newsize;
}

/**************************public functions**************************/

void* obhash_find(obhash_t* table, char* key) {
   assert(table && key);
   bool found = false;
   int index = strhash(key) % table->size;
   hnode_t* node = get_node(table->chains, index, key, &found);
   if (found)
      return node->obj;
   return NULL;
}

void obhash_add(obhash_t* table, char* key, void* obj) {
   assert(table && key);
   if (table->load * 2 > table->size)
      expand_table(table);
   if (add_to_chains(table->chains, table->size,
                                    key, obj, table->obfree))
      ++table->load;
}

int obhash_size(obhash_t* table) {
   assert(table);
   return table->load;
}

void obhash_clear(obhash_t* table) {
   assert(table);
   free_chains(table->chains, table->size, table->obfree);
   table->load = 0;
}

obhash_t* obhash_new(void (*obfree)(void*)) {
   obhash_t* table = malloc(sizeof(obhash_t));
   assert(table);
   table->obfree = obfree;
   table->size = TABLESIZE;
   table->load = 0;
   table->chains = calloc(table->size, sizeof(hnode_t*));
   assert(table->chains);
   return table;
}

void obhash_free(obhash_t* table) {
   if (table) {
      free_chains(table->chains, table->size, table->obfree);
      free(table->chains);
      free(table);
   }
}

/****************************obhash_hook*****************************/

#ifdef OBHASH_HOOK

void obhash_hook(obhash_t* table) {
   int count = 0;
   for (int i = 0; i < table->size; ++i) {
      hnode_t* curr = table->chains[i];
      while (curr) {
         printf("%6d\t%s\n", ++count, curr->key);
         curr = curr->next;
      }
   }
}

#endif /* ifdef OBHASH_HOOK */

/********************************************************************/

