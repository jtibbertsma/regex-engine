/* obhash.h
 *
 * Hashtable using strings as keys. Each node holds a pointer to
 * something.
 */

#ifndef __regex_hash
#define __regex_hash

#include "hooks.h"

/* obhash
 *
 * This is a hash table data structure whose purpose is to hold
 * a pointer to some object, which is indexed by a null-terminated
 * string.
 */
typedef struct _obhash obhash_t;

/** find
  *
  * Given a key string, return a pointer to the matching object
  * or null if the key isn't found.
  */
void* obhash_find(obhash_t*, char*);

/** add
  *
  * Add a new object to the hashtable. When the hashtable is freed,
  * it automatically frees all of the keys, so you may want to strdup
  * the string you're using. If a key which is in the table
  * is used to add a new object to the table, then the old object is
  * freed (if the pointer to the freeing function isn't null).
  */
void obhash_add(obhash_t*, char*, void*);

/** size
  *
  * Gives the number of objects held in the table.
  */
int obhash_size(obhash_t*);

/** clear
  *
  * Delete all nodes in the hashtable without freeing the hashtable
  * itself.
  */
void obhash_clear(obhash_t*);

/** free
  *
  * Free the hash table, each object kept track of by the table, and
  * each key.
  */
void obhash_free(obhash_t*);

/** new
  *
  * Create a new hashtable. The argument is a pointer to a function
  * that can be used to free the object. If you don't want the table
  * to free the objects, use NULL as the argument.
  */
obhash_t* obhash_new(void (*)(void*));

/** hook
  *
  */
#ifdef OBHASH_HOOK
void obhash_hook(obhash_t*);
#endif /* ifdef OBHASH_HOOK */

#endif
