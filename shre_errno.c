/* shre_errno.c
 *
 * Contains shre_strerror and shre_errno.
 */

#include "shre_errno.h"

shre_erflag shre_er = NERROR;

char* shre_strerror(shre_erflag flag) {
   switch (flag) {
case BOGESC: return "bogus escape (end of line)";
case HEXESC: return "invalid hexadecimal escape";
case EMPCLA: return "empty character tree";
case BADRAN: return "bad character range";
case BADINT: return "the integer is too large to parse";
case BADQAN: return "bad quanitifier {a,b}; a > b";
case UNBBRA: return "expected ']' before end of regular expression";
case UNBPAR: return "unbalanced parentheses";
case QUEPAR: return "invalid syntax following '?' in parentheses";
case NAMEXT: return "group name already exists";
case GRPDIG: return "group name must not begin with digit";
case NOTREP: return "nothing to repeat";
case BADREF: return "reference or subroutine call to invalid group"   ;
case NERROR: return "no error";
   }
   return "";
}
