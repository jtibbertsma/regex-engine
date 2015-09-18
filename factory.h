/* factory.h
 *
 * Builds the backtracking machine based on the data in the list
 * of tokens from the parser.
 */

#ifndef __regex_factory
#define __regex_factory

#include "tokens.h"
#include "core.h"

/** build_core
  *
  * Returns a core object built from a list of parser tokens.
  */
core_t* build_core(tlist_t*);

#endif
