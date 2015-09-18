/* parser.h
 *
 * The parser for the regex engine. Takes a regular expression
 * and returns a list of tokens to be used by the parser to build
 * the pattern matching object.
 */

#ifndef __regex_parser
#define __regex_parser

#include "tokens.h"
#include "obhash.h"


/** parse_regex
  *
  * Take a regular expression string and return a pointer to a list
  * of tokens. The input string can't be null, but it can be the empty
  * string. Any names of named groups are stored as keys in the
  * hashtable, with the value being a pointer to an int which tells
  * us the group number that the name represents.
  */
tlist_t* parse_regex(char*, obhash_t**);

/** parse_class
  *
  * Parse the part of regular expression between two brackets ([...]),
  * which has different syntax rules than the regular expression as
  * a whole. Return the set of characters that belong to the class.
  * The variable begin points at the first bracket and the
  * variable end points at the closing bracket. If the tree is to be
  * negated, the value of *negate is tree to true. An empty character
  * tree is an error.
  *
  *    Supports nested classes with - as the difference operator
  * and && as the intersection operator. Does the operation between
  * the tree that has been compiled so far with the tree derived from
  * the nested class. For example, [a-m&&[l-z]0-9] is equivalent to
  * [lm0-9].
  *
  *    A note about range syntax (as in 0-9): This syntax also works
  * with escape characters; for example, [\x3A-\x40] is equivalent
  * to [:;<=>?@].
  */
class_t* parse_class(char*);

#endif
