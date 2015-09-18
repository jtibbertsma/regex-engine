/* shre.h
 *
 * Interface for the shre regular expressions engine.
 *
 * Supported syntax:
 *
 *    .     
 *    Match any character except for newline.
 *
 *    |
 *    Match multiple possible regular expressions. The regular
 *    expression 'a|b' matches either 'a' or 'b'.
 *
 *    ^
 *    Anchor to front of string.
 *
 *    $
 *    Anchor to back of string.
 *
 *    \
 *    Treat next character as a literal.
 *
 *    \0
 *    Match a null terminating character.
 *
 *    [...]
 *    Character classes. Match any character in between the two
 *    brackets, except for the characters that have special meaning
 *    in a character tree, which need to be escapes:
 *
 *       [...]
 *       Character classes can be nested. The meaning of a nested
 *       tree depends on what directly precedes it. If there is
 *       nothing special before the nested tree, then is treated
 *       as though there were no brackets; in other words, the result
 *       is the union between the inner tree and the outer class.
 *         If a bracket is found that doesn't match another bracket,
 *       then it is treated as a literal.
 *    
 *       -
 *       Character range. For example, [a-z] matches any single lower
 *       case letter. The first character being greater than the
 *       second is an error.
 *         If the '-' occurs at the beginning or end of the character
 *       tree, it is treated as a literal.
 *         If the '-' occurs directly before a nested character tree,
 *       then it denotes set difference; the resulting tree is the
 *       set difference between the outer tree and the inner class.
 *
 *       &&
 *       If two ampersands precede a nested character tree, then they
 *       denote tree intersection between the inner and outer character
 *       classes.
 *
 *       \
 *       Treat the following character as a literal. Hexadecimal
 *       escapes also work with ranges, i.e. '[\x23-\x43]'.
 *
 *       ^
 *       At the beginning of a character tree, denotes that the tree
 *       is an inverted tree, which matches characters that aren't in
 *       the class.
 *
 *       ]
 *       In most regex engines, ']' is treated as a literal if it is
 *       the first character in the class. In this engine it must be
 *       escaped.
 *
 *    \sSdDwWhH
 *    Perl shorthand classes:
 *       '\d' --> '[0-9]'
 *       '\D' --> '[^0-9]'
 *       '\w' --> '[a-zA-Z0-9_]'
 *       '\W' --> '[^a-zA-Z0-9_]'
 *       '\s' --> '[ \t\r\n\f]'
 *       '\S' --> '[^ \t\r\n\f]'
 *       '\h' --> '[a-fA-F0-9]'
 *       '\H' --> '[^a-fA-F0-9]'
 *
 *    *
 *    Match zero or more of the preceding token. If the preceding token
 *    cannot be repeated, an error occurs.
 *
 *    +
 *    Matches one or more of the preceding token. If it follows a
 *    quantifier token, (such as '*', '+', etc.), then it denotes that
 *    quantifier is to be possessive, meaning that it will match as
 *    much as possible without saving backtracking information.
 *
 *    {n,m}
 *    Matches the preceding token between n and m times. To match
 *    unlimited times, use {n,}.
 *
 *    ?
 *    If this follows a normal token, it is equivalent to {0,1}. If
 *    it follows a quantifier token, it denotes that that quantifier is
 *    lazy.
 *
 *    (...)
 *    Match the regular expression within the parenthesis. The portion
 *    of the string matched by the group is remembered by the regular
 *    expression engine.
 *
 *    (?<name>...)
 *    Named capturing group.
 *
 *    (?:...)
 *    Non-capturing group; same as a regular group, but the portion of
 *    the string matched by ... is not remembered.
 *
 *    (?=...)
 *    Lookahead assertion; matches the empty string if ... matches the
 *    next portion of the input string.
 *
 *    (?!...)
 *    Negative lookahead assertion; matches the empty string if ...
 *    does not match the next portion of the input string.
 *
 *    (?>...)
 *    Atomic group. Same as a normal group, but the engine never
 *    backtracks into the group. Doesn't capture matches. If you want
 *    to capture the match from an atomic group, wrap it in a normal
 *    group.
 *
 *    (?<=...)
 *    Positive lookbehind assertion
 *
 *    (?<!...)
 *    Negative loolbehind asertion.
 *
 *    \number
 *    Backreference to group with index 'number'. This syntax only
 *    works with single-digit groups. For multi-digit groups use the
 *    \g syntax. If a backreference refers to a group that doesn't
 *    exist, compilation fails. However, a backreference can refer
 *    to a group that comes later in the regular expression, since
 *    it's possible that the group could have been captured in a 
 *    previous recursion level. If a backreference refers to a group
 *    that hasn't been captured yet, it won't match anything.
 *
 *    \g<number>, \g'number'
 *    An alternate backreference syntax, can be used with multiple
 *    digits. The syntax '\g<0>', a backreference to the overall match,
 *    will be accepted by the parser, but will never match anything,
 *    since the group capture for the overall match isn't recorded
 *    until the end of the match.
 *
 *    \g<name>, \g'name'
 *    Backreference to named group.
 *
 *    \k
 *    Exactly the same as \g.
 *
 *    (?number)
 *    Match the regular expression in group 'number'. This is
 *    different from a back-reference in that it performs the match
 *    again. This can be used within the group 'number' to do
 *    recursive matches. Any group captures within the subroutine
 *    call temporarily overwrite current group captures, but the
 *    group captures revert to the way they were before the subroutine
 *    call. Unlike the PCRE engine, subroutine calls are not atomic;
 *    they can be backtracked into. To make a subroutine call atomic,
 *    you can wrap it in an atomic group, like (?>(?0)).
 *
 *    (?&name)
 *    Subroutine call to a named group.
 *
 *    (?R)
 *    Recursion. Same as (?0).
 *
 *    \Q...\E, \Q...
 *    Match the literal character string ... .
 *
 *    \xdd
 *    Where each d is a digit, match a literal character denoted by
 *    the hexadecimal number.
 *
 *    \0dd
 *    Octal escape.
 *
 * To use the regex engine:
 *
 *   First, call 'start_regex_engine' to initialize necessary data
 *   structures.
 *
 *   Use 'shre_compile' to get a pointer to a compiled pattern.
 *
 *   Use the compiled pattern to perform a single search with
 *   'shre_search' or 'shre_match'.
 *
 *   Use 'scan_new' to get a pointer to a scanner object, which
 *   allows you to use 'scan_next' to get every match in the input
 *   string one-by-one.
 *
 *   If a match is found, you will get a pointer to a match object.
 *   Use 'match_get' to get a copy of the matching portion of the
 *   input string. Use 'match_offset' to get the distance from the
 *   beginning of the input string to the first character of the match.
 *   Use 'match_free' to free the match object once you're done with
 *   it. You can free a scanner with a normal call to 'free'.
 *
 *   Remember to free string copies that you get from 'match_get',
 *   'match_group', 'match_named_group', and 'shre_replace'.
 */

#ifndef __regex_interface
#define __regex_interface

#include <stdbool.h>


typedef struct _pattern pattern_t;
typedef struct _match match_t;
typedef struct _scanner scanner_t;

//
// regex engine functions
//

/** start_regex_engine
  *
  * Initializes the regex module. This function must be called before
  * any regex operation is used. Failure to call this function
  * will cause an assertion failure.
  */
void start_regex_engine();

/** engine_is_initialized
  *
  * Return true if the engine is initialized. If false, then
  * start_regex_engine() must be called before any regex operations
  * are used.
  */
bool engine_is_initialized();

/** num_patterns
  *
  * Get the number of pattern objects currently held in the pattern
  * cache.
  */
int num_patterns();

/** clear_cache
  *
  * Clear regex memory cache. This deallocates all patterns held in
  * the pattern cache. Any pointer to a pattern is now invalid.
  */
void clear_cache();

/** cleanup_regex_engine
  *
  * Free all the memory used by the regex engine.
  */
void cleanup_regex_engine();

//
// regex operations
//

/** compile
  *
  * Takes a regular expression in the form of a
  * char array and builds a pattern object that finds strings
  * that match the regular expression. If the regular expression
  * syntax is bad, this function will return NULL and shre_errno
  * will be class.
  */
pattern_t* shre_compile(char*);

/** expression
  *
  * Returns the original regular expression that was used to compile
  * the given pattern object. Do not free the return value of this
  * function.
  */
const char* shre_expression(pattern_t*);

/** search
  *
  * Returns a pointer to a match object. If no match is found, the
  * return value is NULL. Any returned match values, unlike patterns,
  * must be freed explicitly with the match_free function.
  */
match_t* shre_search(pattern_t*, char*);

/** match
  *
  * Return a match object only if the entire input string matches
  * the pattern..
  */
match_t* shre_entire(pattern_t*, char*);

/** quick_search/ quick_match
  *
  * Same as shre_search and shre_match, except that these versions take
  * a string instead of a pattern object, and their return value
  * is a simple bool rather than a match object. Since patterns
  * are saved in a cache, you can use these functions with the same
  * pattern over and over again without the engine having to compile
  * the pattern every time. If a bad regular expression is entered,
  * false is returned and shre_errno is class. You can check that there
  * are no errors by checking shre_errno == NERROR (no error) after
  * using.
  */
bool quick_search(char*, char*);
bool quick_entire(char*, char*);

/** replace
  *
  * Swaps out all leftmost non-overlapping occurances of pattern
  * in the first string with the second string. The string passed
  * into the function is not affected; a new string is allocated
  * and returned. If the pattern never matches the string, then
  * a copy of the original string is returned.
  *
  * In the replacement string, you can use normal \g or \k
  * syntax to swap in group captures, and \g<0>, \g'0',
  * \k<0>, or \k'0' to swap in the overall match.
  */
char* shre_replace(pattern_t*, char*, char*);

//
// match operations
//

/** match_get
  *
  * Get the matching string from the match object. Freeing the return
  * value of this function would be bad.
  */
char* match_get(match_t*);

/** num_groups
  *
  * Returns the number of groups kept track of by the match, including
  * the zero'th group, which is the match itself. Sending an int
  * outside the range of [0, num_groups) into match_group will cause
  * match_group to return NULL.
  */
int match_num_groups(match_t*);

/** offset
  *
  * Gives the offset from the beginning of the input string to the
  * beginning of the match.
  */
uint32_t match_offset(match_t*);

/** group
  *
  * Returns a string matched by a given group in a match. If the
  * group doesn't exist, or wasn't captured in the match, return
  * NULL.
  */
char* match_group(match_t*, int);

/** named_group
  *
  * Same as group, but refer to the group using the name you specified
  * in the regular expression. Return NULL if the named group does
  * not exist, or failed to be captured.
  */
char* match_named_group(match_t*, char*);

/** match_free
  *
  * Deallocate memory used by the match object.
  */
void match_free(match_t*);

//
// scanner
//

/** scan_new
  *
  * Make a new scanner.
  */
scanner_t* scan_new(pattern_t*, char*);

/** scan_next
  *
  * Get the next match. If the match is zero length, then the scanner
  * is incremented by one.
  */
match_t* scan_next(scanner_t*);

/** scan_try
  *
  * Try to match the current position; return NULL if no match, and
  * if there is a match, then return the match without changing
  * the position of the scanner read head.
  */
match_t* scan_try(scanner_t*);

/** scan_seek
  *
  * Set a new current position in the string to search. The argument
  * is the offset in bytes from the beginning of the input string to
  * the desired position. This function doesn't let you tree the search
  * position past the null terminating character.
  */
void scan_seek(scanner_t*, uint32_t);

/** scan_tell
  *
  * Get the offset from the beginning of the input string to the
  * current search position.
  */
uint32_t scan_tell(scanner_t*);

/** scan_increment
  *
  * Increment the reading head on the scanner.
  */
void scan_increment(scanner_t*);

#endif
