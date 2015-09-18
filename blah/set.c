/* set.c
 *
 * The implementation of set. The set is implemented as a bit-array.
 * It is best used to keep track of small integers,  (<= 127)
 * in order to be efficient in terms of memory and speed.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "set.h"
#include "util.h"


/* set
 *
 * Declaration of the set.
 */
struct _set {
   unsigned int* bits;
   int capacity;
};

#define DEFCAP 4  // default capacity of internal array

// number of bits in an integer
#define I (sizeof(int) * 8)

// check or alter the value of a single bit in the array
#define SetBit(A,k)     ( A[k/I] |=  (1 << (k%I)) )
#define ClearBit(A,k)   ( A[k/I] &= ~(1 << (k%I)) )
#define TestBit(A,k)    ( A[k/I] &   (1 << (k%I)) )

// gcc version
#ifdef __GNUC__
#define VERSION  ( __GNUC__ * 10000 \
                 + __GNUC_MINOR__ * 100 \
                 + __GNUC_PATCHLEVEL__)
#endif

// If possible, use a builtin ffs function.
#if defined (__GNUC__) && VERSION >= 30202
#define FFS(ARG) __builtin_ffs(ARG)
#else
#define FFS(ARG)     naive_ffs(ARG)
#define NAIVE
#endif

/** set_ensure_capacity
  *
  * Make sure that the internal array of the set is big enough
  * to hold the given integer, and if not, expand the array
  * and change the capacity field.
  */
static void set_ensure_capacity(set_t* this, unsigned int val) {
   if (val >= this->capacity * I) {
      int newcap = this->capacity;
      do newcap *= 2; while (val >= newcap * I);
      unsigned int* newarray = calloc(newcap, sizeof(int));
      assert(newarray);
      memcpy(newarray, this->bits, this->capacity * sizeof(int));
      free(this->bits);
      this->bits = newarray;
      this->capacity = newcap;
   }
}

/** naive_ffs
  *
  * Return the position of the first one in the binary representation
  * of an unsigned integer plus one. So if the input has binary
  * representation 00000000000000000000000000001000, the this function
  * returns 4. Uses a naive algorithm that individually tests each bit.
  */
#ifdef NAIVE
static int naive_ffs(unsigned int u) {
   for (unsigned int i = 0; i < I; ++i) {
      if (u & (1 << i))
         return i + 1;
   }
   return 0;
}
#endif

/*************************public functions***************************/

void set_add(set_t* this, unsigned int val) {
   assert(this);
   set_ensure_capacity(this, val);
   SetBit(this->bits, val);
}

void set_remove(set_t* this, unsigned int val) {
   assert(this);
   set_ensure_capacity(this, val);
   ClearBit(this->bits, val);
}

bool set_element(set_t* this, unsigned int val) {
   assert(this);
   set_ensure_capacity(this, val);
   return TestBit(this->bits, val);
}

   ////////////////////////////////////////////////

#define DO_OP(LOOP_BODY) \
   assert(left && right); \
   set_ensure_capacity(left, right->capacity * I); \
   set_ensure_capacity(right, left->capacity * I); \
   set_t* newset = malloc(sizeof(set_t)); \
   assert(newset); \
   newset->capacity = left->capacity; \
   newset->bits = calloc(newset->capacity, sizeof(int)); \
   assert(newset->bits); \
   for (int i = 0; i < newset->capacity; ++i) { \
      LOOP_BODY; \
   } \
   return newset;

#define DO_XOP(LOOP_BODY) \
   assert(left && right); \
   set_ensure_capacity(left, right->capacity * I); \
   set_ensure_capacity(right, left->capacity * I); \
   for (int i = 0; i < left->capacity; ++i) { \
      LOOP_BODY; \
   }

   ///////////////////////////////////////////////

bool set_subset(set_t* left, set_t* right) {
   DO_XOP(if ((right->bits[i] & left->bits[i]) != left->bits[i])
             return false)
   return true;
}

set_t* set_union(set_t* left, set_t* right) {
   DO_OP(newset->bits[i] = left->bits[i] | right->bits[i])
}

void set_xunion(set_t* left, set_t* right) {
   DO_XOP(left->bits[i] |= right->bits[i])
}

set_t* set_intersection(set_t* left, set_t* right) {
   DO_OP(newset->bits[i] = left->bits[i] & right->bits[i])
}

void set_xintersection(set_t* left, set_t* right) {
   DO_XOP(left->bits[i] &= right->bits[i])
}

set_t* set_difference(set_t* left, set_t* right) {
   DO_OP(newset->bits[i] = left->bits[i] & ~right->bits[i])
}

void set_xdifference(set_t* left, set_t* right) {
   DO_XOP(left->bits[i] &= ~right->bits[i])
}

set_t* set_symmetric(set_t* left, set_t* right) {
   DO_OP(newset->bits[i] = left->bits[i] ^ right->bits[i])
}

void set_xsymmetric(set_t* left, set_t* right) {
   DO_XOP(left->bits[i] ^= right->bits[i])
}

bool set_empty(set_t* this) {
   assert(this);
   for (int i = 0; i < this->capacity; ++i) {
      if (this->bits[i] != 0)
         return false;
   }
   return true;
}

int set_cardinality(set_t* this) {
   assert(this);
   int card = 0;
   for (int i = 0; i < this->capacity; ++i) {
      card += count_ones(this->bits[i]);
   }
   return card;
}

bool set_equality(set_t* left, set_t* right) {
   DO_XOP(if (left->bits[i] != right->bits[i])
             return false)
   return true;
}

int set_pop(set_t* this) {
   assert(this);
   for (int i = 0; i < this->capacity; ++i) {
      if (this->bits[i] != 0) {
         int k = i*I + (FFS(this->bits[i]) - 1);
         ClearBit(this->bits, k);
         return k;
      }
   }
   return -1;
}

set_t* set_new() {
   set_t* newset = malloc(sizeof(set_t));
   assert(newset);
   newset->capacity = DEFCAP;
   newset->bits = calloc(DEFCAP, sizeof(int));
   assert(newset->bits);
   return newset;
}

set_t* set_copy(set_t* that) {
   set_t* newset = malloc(sizeof(set_t));
   assert(newset);
   newset->capacity = that->capacity;
   newset->bits = calloc(that->capacity, sizeof(int));
   assert(newset->bits);
   memcpy(newset->bits, that->bits, that->capacity * sizeof(int));
   return newset;
}

void set_free(set_t* this) {
   if (this) {
      free(this->bits);
      free(this);
   }
}

/******************************set_hook******************************/

#ifdef SET_HOOK
void set_hook(set_t* set) {
   for (unsigned long i = 0; i < set->capacity * I - 1; ++i) {
      printf("U+%lX\t%s\n", i, TestBit(set->bits, i) ? "In" : "Out");
   }
}
#endif

/********************************************************************/
