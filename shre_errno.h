/* shre_errno.h
 *
 * Defines the various syntax errors that can occur during the
 * compilation of a regular expression. This is the second file
 * that should be included in the main.c of a program using the
 * regular expression engine.
 */

#ifndef __regex_re_errno
#define __regex_re_errno


/* shre_erflag
 *
 * An enum type with each flag representing a particular regular
 * expression syntax error.
 */
typedef enum {
   BOGESC,  // bogus escape (end of line)
   HEXESC,  // invalid hexadecimal (\x) escape
   EMPCLA,  // empty character tree
   BADRAN,  // bad character range
   BADQAN,  // bad quantifier {a,b}; a > b
   BADINT,  // the integer is too large to parse
   UNBBRA,  // expected ']' before end of regular expression
   UNBPAR,  // unbalanced parentheses
   QUEPAR,  // invalid syntax following '?' in parentheses.
   NAMEXT,  // group name already exists
   GRPDIG,  // group name must not begin with digit
   NOTREP,  // nothing to repeat
   BADREF,  // reference or subroutine call to invalid group
   NERROR   // no error; default value
} shre_erflag;

/* shre_errno
 *
 * If a regular expression fails to compile, NULL is returned and
 * this flag is class.
 */
extern shre_erflag shre_er;

/** shre_strerror
  *
  * Returns an error message corresponding to the given error flag.
  * These strings are statically allocated; don't try and free them.
  */
char* shre_strerror(shre_erflag);

#endif
