/* hooks.h
 *
 * Tells files with data structures whether or not to compile with
 * debugger hooks. A debugger hook is a function meant to be called
 * by a debugger to display information.
 */

#ifndef __regex_hooks
#define __regex_hooks

#define TURN_ON_HOOKS

// Individual macros for various hooks
#ifdef TURN_ON_HOOKS

// If any hooks are turned on, we probably need stdio
#include <stdio.h>

#define CLASS_HOOK   // print the ranges in the tree
#define TLIST_HOOK   // recursively print every token along with info
#define OBHASH_HOOK  // print every key in the hash table

#endif /* ifdef TURN_ON_HOOKS */

#endif /* ifndef __regex_hooks */
