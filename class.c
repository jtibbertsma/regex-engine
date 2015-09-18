/* class.c
 *
 * Implementation of character classes. Character classes are
 * implemented with binary search trees of disjoint ranges.
 * The tree is balanced so that searches are fast. Insertions,
 * deletions, and set operations are implemented with a scheme
 * where we transform the tree into a linked-list (called a vine),
 * and then transform the vine into a balanced binary search tree,
 * which uses a simple algorithm. The vine_to_tree tree_to_vine
 * idea is from a paper called "Tree Rebalancing in Optimal
 * Time and Space", but I don't use their algorithm.
 */

#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include "class.h"

#define EmptyVal 0xFFFFFFFF

/* Set lo to this value in the root of an empty tree. This should be
 * value greater than 0x10FFFF, the highest unicode codepoint.
 */
#define EmptyTree(Tree) (Tree->range.lo == EmptyVal)

/* class_t
 *
 * Declaration of tree struct, which consists of a urange32, and
 * pointers to children.
 */
struct _class {
   urange32_t range;
   class_t*   lchild;
   class_t*   rchild;
};

/********************************misc********************************/

/** construct
  *
  * Construct a new node with given range and set children to NULL.
  */
static class_t* class_construct(urange32_t range) {
   class_t* tree = malloc(sizeof(class_t));
   assert(tree);
   tree->range = range;
   tree->lchild = NULL;
   tree->rchild = NULL;
   return tree;
}

/** tree_height
  *
  * Gives the height of a tree.
  */
static int tree_height(class_t* tree) {
   if (!tree)
      return 0;
   return (int) fmax(tree_height(tree->lchild),
                     tree_height(tree->rchild)) + 1;
}

/** balance_factor
  *
  * Calculate the balance factor of a tree, which is the height of the
  * left subtree minus the height of the right subtree.
  */
static inline int balance_factor(class_t* tree) {
   return tree_height(tree->lchild) - tree_height(tree->rchild);
}

/*******************************balancing****************************/

/** swap_ranges
  *
  * Swap the range fields of two classes. By using swaps, we can be
  * sure that the pointer returned by class_new always points at the
  * root of the tree. The downside is that we have to do more
  * assignments to do a tree rotation.
  */
static inline void swap_ranges(class_t* left, class_t* right) {
   urange32_t swap = left->range;
   left->range = right->range;
   right->range = swap;
}

/** rotate_right
  *
  * Do a rotate left operation.
  */
static void rotate_right(class_t* parent) {
   assert(parent->lchild);
   class_t* child   = parent->lchild;
   class_t* chinewr = parent->rchild;
   class_t* chinewl = child->rchild;
   parent->lchild   = child->lchild;
   parent->rchild   = child;
   child->rchild    = chinewr;
   child->lchild    = chinewl;
   swap_ranges(parent, child);
}

/** rotate_left
  *
  * Do a rotate right operation.
  */
static void rotate_left(class_t* parent) {
   assert(parent->rchild);
   class_t* child   = parent->rchild;
   class_t* chinewl = parent->lchild;
   class_t* chinewr = child->lchild;
   parent->rchild   = child->rchild;
   parent->lchild   = child;
   child->lchild    = chinewl;
   child->rchild    = chinewr;
   swap_ranges(parent, child);
}

/** move_min_to_root
  *
  * Using tree rotations, put the minimum node at the root, using
  * the property of binary trees that tells us we can get to the
  * min node by following the leftmost path from the root.
  */
static void move_min_to_root(class_t* root) {
   if (!root->lchild)
      return;
   move_min_to_root(root->lchild);
   rotate_right(root);
}

/** vine_to_tree
  *
  * Converts a vine to a perfectly balanced binary tree. A vine is
  * is bst which is an ordered linked list, with the smallest value
  * at the root, and the links are the right children, or oppositely,
  * with the largest value at the root and left children as links.
  * A vine whose root is the minimum value is called increasing, and
  * vice-versa. Perfectly balanced means that the balance_factor for
  * the tree is in [-1, 1]. Calling this function on a non-vine tree
  * will probably result in a failed assertion in one of the rotation
  * functions.
  */
static void vine_to_tree(class_t* vine) {
   if (!vine)
      return;

   // Do rotations until the balance_factor is in [-1, 1]. Then
   //   both children are vines, and we can continue recursively.
   int bf = balance_factor(vine);
   void (*rotate)(class_t*);
   if (bf < -1) {
      bf *= -1;
      rotate = &rotate_left;
   } else if (bf > 1) {
      rotate = &rotate_right;
   }
   for (; bf > 1; bf -= 2)
      rotate(vine);

   vine_to_tree(vine->lchild);
   vine_to_tree(vine->rchild);
}

/** tree_to_vine
  *
  * Converts an arbitrary tree to an increasing vine.
  */
static void tree_to_vine(class_t* tree) {
   while (tree) {
      move_min_to_root(tree);
      tree = tree->rchild;
   }
}

/** balance
  *
  * Pefectly balance an arbitrary binary tree.
  *
static void balance(class_t* tree) {
   tree_to_vine(tree);
   vine_to_tree(tree);
}

************************inserting and deleting***********************/

/** one_away_ranges
  *
  * Checks an increasing vine for ranges whose endpoints differ by one,
  * and combine them.
  */
static inline void one_away_ranges(class_t* vine) {
   for (; vine->rchild; vine = vine->rchild) {
      if (vine->range.hi + 1 == vine->rchild->range.lo) {
         class_t* child = vine->rchild;
         vine->range.hi = child->range.hi;
         vine->rchild = child->rchild;
         free(child);
      }
   }
}

/** case_t
  *
  * A set of cases returned by find_case_set_ptrs, used by both
  * vine_insert and vine_delete.
  */
typedef enum {
   OVERLAP_ALL,       // input range overlaps all ranges.
   OVERLAP_MULTIPLE,  // input range overlaps multiple existing ranges
   OVERLAP_ONE,       // input range overlaps one range
   DISJOINT,          // input range is disjoint with tree ranges
   LESS_THAN_MIN      // input range is less than current min
} case_t;

/** find_case_set_ptrs
  *
  * Return a case_t (see above) that tells the insert or delete
  * function what to do. Set the following class_t pointers:
  *
  * ln --> Left node in the range. The fist node such that range.lo
  *        is less than ln->range.hi, or NULL if range.lo is greater
  *        than any codepoint in the class.
  *
  * rn --> Right node in the range. This is the first node such that
  *        range.hi is greater than rn->range.lo, or the root if there
  *        is no such node. In some cases, rn is above ln.
  *
  * lpar-> Parent node of ln.
  *
  * rpar-> Parent node of rn.
  */
case_t find_case_set_ptrs(class_t* vine, urange32_t range,
                                      class_t** ln, class_t** rn,
                                      class_t** lpar, class_t** rpar) {
   class_t* temp = NULL;
   for (*ln = vine; *ln && range.lo >= (*ln)->range.hi;
                                      *ln = (*ln)->rchild) {
      *lpar = *ln;
   }
   for (*rn = *lpar ? *lpar : vine;
        *rn && range.hi >= (*rn)->range.lo; *rn = (*rn)->rchild) {
      temp = *rpar;
      *rpar = *rn;
   }
   if (!*rpar && range.hi < (*rn)->range.lo)
      return LESS_THAN_MIN;
   *rn = *rpar ? *rpar : vine;
   if (*rpar)
      *rpar = temp;
   if (*rn == *lpar)
      return DISJOINT;
   if (*rn == *ln)
      return OVERLAP_ONE;
   if (!(*rn)->rchild && !*lpar)
      return OVERLAP_ALL;
   return OVERLAP_MULTIPLE;
}

/** vine_insert
  *
  * Insert a range into an increasing vine. Ensure that the resulting
  * vine consists of disjoint ranges. We assume that the vine is
  * non-empty.
  */
static void vine_insert(class_t* vine, urange32_t range) {
   class_t* ln, * rn, * lpar = NULL, * rpar = NULL;
   switch (find_case_set_ptrs(vine, range, &ln, &rn, &lpar, &rpar)) {

      case OVERLAP_ALL:
         vine->range.lo = (int) fmin(ln->range.lo, range.lo);
         vine->range.hi = (int) fmax(rn->range.hi, range.hi);
         class_free(vine->rchild);
         vine->rchild = NULL;
         break;

      case OVERLAP_MULTIPLE: {
         ln->range.lo = (int) fmin(ln->range.lo, range.lo);
         ln->range.hi = (int) fmax(rn->range.hi, range.hi);
         class_t* kill = ln->rchild;
         ln->rchild = rn->rchild;
         rn->rchild = NULL;
         class_free(kill);
         break;
      }

      case OVERLAP_ONE:
         ln->range.lo = (int) fmin(ln->range.lo, range.lo);
         ln->range.hi = (int) fmax(rn->range.hi, range.hi);
         break;

      case DISJOINT: {
         class_t* end = rn->rchild;
         rn->rchild = class_construct(range);
         rn->rchild->rchild = end;
         break;
      }

      case LESS_THAN_MIN:
         vine->lchild = class_construct(range);
         rotate_right(vine);
         break;
   }
   one_away_ranges(vine);
}

/** vine_delete
  *
  * Delete a range from a vine, setting the value of the root's range
  * to the empty tree value if necessary. Make sure not to delete the
  * root; swap value with another node if necessary.
  */
static void vine_delete(class_t* vine, urange32_t range) {
   class_t* ln, * rn, * lpar = NULL, * rpar = NULL;
   switch (find_case_set_ptrs(vine, range, &ln, &rn, &lpar, &rpar)) {

      case LESS_THAN_MIN: case DISJOINT:
         break;

      case OVERLAP_ALL:
         if (range.lo <= ln->range.lo && range.hi >= rn->range.hi) {
            vine->range.lo = EmptyVal;
            class_free(vine->rchild);
            vine->rchild = NULL;
            break;
         }
      case OVERLAP_MULTIPLE:
         if (range.hi <= rn->range.hi) {
            rn->range.lo = range.hi;
            rn = rpar;
         }
         if (range.lo <= ln->range.lo) {
            if (!lpar) {
               swap_ranges(ln, rn->rchild);
               class_t* kill = ln->rchild;
               class_t* temp = rn->rchild;
               ln->rchild = rn->rchild->rchild;
               temp->rchild = NULL;
               class_free(kill);
               break;
            }
            ln = lpar;
         } else {
            ln->range.hi = range.lo;
         }
         if (ln != rn) {
            class_t* kill = ln->rchild;
            ln->rchild = rn->rchild;
            rn->rchild = NULL;
            class_free(kill);
         }
         break;

      case OVERLAP_ONE:
         if (range.lo <= ln->range.lo && range.hi >= ln->range.hi) {
            if (!lpar) {
               if (!ln->rchild) {
                  vine->range.lo = EmptyVal;
                  break;
               }
               swap_ranges(ln, ln->rchild);
            } else {
               ln = lpar;
            }
            class_t* kill = ln->rchild;
            ln->rchild = ln->rchild->rchild;
            kill->rchild = NULL;
            free(kill);
            break;
         }
         if (range.lo > ln->range.lo && range.hi < ln->range.hi) {
            urange32_t new_range = { range.hi + 1, ln->range.hi };
            ln->range.hi = range.lo - 1;
            class_t* end = ln->rchild;
            ln->rchild = class_construct(new_range);
            ln->rchild->rchild = end;
            break;
         }
         if (range.lo <= ln->range.lo) {
            ln->range.lo = range.hi;
         } else {
            ln->range.hi = range.lo;
         }
         break;
   }
}

//
// public insertion/ deletion interface
//

void class_insert_codepoint(class_t* tree, uint32_t cp) {
   assert(tree);
   urange32_t range = { cp, cp };
   class_insert_range(tree, range);
}

void class_insert_range(class_t* tree, urange32_t range) {
   assert(tree);
   assert(range.lo <= range.hi);

   // empty tree case
   if (EmptyTree(tree)) {
      tree->range = range;
      return;
   }
   
   // convert the tree to a vine and do a vine insert
   tree_to_vine(tree);
   vine_insert(tree, range);
   if (tree->rchild)
      vine_to_tree(tree);
}

void class_delete_codepoint(class_t* tree, uint32_t cp) {
   assert(tree);
   urange32_t range = { cp, cp };
   class_delete_range(tree, range);
}

void class_delete_range(class_t* tree, urange32_t range) {
   assert(tree);
   assert(range.lo <= range.hi);

   // empty tree case
   if (EmptyTree(tree)) {
      return;
   }
   
   // convert the tree to a vine and do a vine delete
   tree_to_vine(tree);
   vine_delete(tree, range);
   if (!EmptyTree(tree) && tree->rchild)
      vine_to_tree(tree);
}

/*****************************searching******************************/

bool class_search(class_t* tree, uint32_t cp) {
   while (tree) {
      if (cp < tree->range.lo)
         tree = tree->lchild;
      else if (cp > tree->range.hi)
         tree = tree->rchild;
      else
         return true;
   }
   return false;
}

/***************************set operations***************************/

/** union_recurse
  *
  * Do a vine_insert on the left argument using the range from the
  * right argument, and then continue on to the children.
  */
static void union_recurse(class_t* left, const class_t* right) {
   if (!right)
      return;
   vine_insert(left, right->range);
   union_recurse(left, right->lchild);
   union_recurse(left, right->rchild);
}

void class_union(class_t* left, const class_t* right) {
   assert(left && right && left != right);
   if (EmptyTree(right))
      return;
   tree_to_vine(left);
   if (EmptyTree(left)) {
      left->range = right->range;
      union_recurse(left, right->lchild);
      union_recurse(left, right->rchild);
   } else {
      union_recurse(left, right);
   }
   vine_to_tree(left);
}

/** difference_recurse
  *
  * Same idea as union, but use delete instead of insert.
  */
static void difference_recurse(class_t* left, const class_t* right) {
   if (!right)
      return;
   vine_delete(left, right->range);
   difference_recurse(left, right->lchild);
   difference_recurse(left, right->rchild);
}

void class_difference(class_t* left, const class_t* right) {
   assert(left && right && left != right);
   if (EmptyTree(right))
      return;
   tree_to_vine(left);
   if (EmptyTree(left)) {
      left->range = right->range;
      difference_recurse(left, right->lchild);
      difference_recurse(left, right->rchild);
   } else {
      difference_recurse(left, right);
   }
   vine_to_tree(left);
}

void class_intersection(class_t* left, const class_t* right) {
   assert(left && right && left != right);
}

/**************************various public****************************/

bool class_empty(class_t* tree) {
   assert(tree);
   return EmptyTree(tree);
}

int class_cardinality(class_t* tree) {
   if (!tree || EmptyTree(tree))
      return 0;
   return (tree->range.hi - tree->range.lo + 1)
        + class_cardinality(tree->lchild)
        + class_cardinality(tree->rchild);
}

int class_size(class_t* tree) {
   if (!tree || EmptyTree(tree))
      return 0;
   return 1 + class_size(tree->lchild) + class_size(tree->rchild);
}

class_t* class_new() {
   urange32_t range = { EmptyVal, 0 };
   return class_construct(range);
}

void class_free(class_t* tree) {
   if (tree) {
      class_free(tree->lchild);
      class_free(tree->rchild);
      free(tree);
   }
}

/****************************class hook******************************/

#ifdef CLASS_HOOK

#include <string.h>  /* memset */

/* Length of path string in class_hook. If we're testing large inputs,
 * we may need to make this bigger.
 */
#define PATHLEN 1024

/* What column to have first character of the range string in
 * range structure.
 */
#define BRACKET_START 30

static char* range_string(urange32_t range) {
   static char rstr[64];
   //sprintf(rstr, "{ U+%X, U+%X }", range.lo, range.hi);
   sprintf(rstr, "{ %d, %d }", range.lo, range.hi);
   return rstr;
}

static int pathform(int printed, int len) {
   int bracket_start = BRACKET_START;
   return (int) fmax(0, bracket_start - printed + len);
}

static void hook_structure(class_t* tree, char* path, int level) {
   if (!tree)
      return;
   char* rstr = range_string(tree->range);
   int printed = printf("%3d   %s ", balance_factor(tree), path);
   printf("%*s\n", pathform(printed, strlen(rstr)), rstr);
   path[level] = 'l';
   hook_structure(tree->lchild, path, level + 1);
   path[level] = 'r';
   hook_structure(tree->rchild, path, level + 1);
   path[level] = '\0';
}

static void hook_inorder(class_t* tree) {
   if (!tree)
      return;
   hook_inorder(tree->lchild);
   printf("%s\n", range_string(tree->range));
   hook_inorder(tree->rchild);
}

void class_hook(class_t* tree) {
   printf("Number of Ranges:\n%d\n", class_size(tree));
   if (EmptyTree(tree))
      return;
   printf("\nIn order:\n\n");
   hook_inorder(tree);
   printf("\nStructure:\n\n");
   char path[PATHLEN];
   memset(path, '\0', PATHLEN);
   hook_structure(tree, path, 0);
   printf("\n");
}

#endif /* ifdef CLASS_HOOK */

/********************************************************************/
