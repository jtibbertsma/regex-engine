/* parser.c
 *
 * Create a list of tokens from a string to compile a regex pattern.
 * Check for every possible syntax error.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
//#include <math.h>

#include "shre_errno.h"
#include "parser.h"
#include "util.h"


/************subroutines that act on the list of tokens**************/

/** part_of_string
  *
  * Check if the current node is a literal that should be part of a
  * string case.
  */
static inline bool part_of_string(tnode_t* node) {
   return node && tnode_token(node)->flag == LITERAL &&
           !(tnode_next(node) != NULL && 
               tnode_token(tnode_next(node))->flag == RANGE);
}

/** need_denullify
  *
  * Check for a character class that matches the zero byte.
  */
static inline bool need_denullify(tnode_t* node) {
   return (tnode_token(node)->flag == CLASS &&
           class_search(tnode_token(node)->data.class, '\0'))
       || (tnode_token(node)->flag == NCLASS &&
           !class_search(tnode_token(node)->data.class, '\0'));
}

/** string_list_search
  *
  * Return a pointer to a node in the list that contains a token
  * whose data is equal to the string.
  */
static tnode_t* string_list_search(tlist_t* list, char* str) {
   for (tnode_t* node = tlist_front(list);
                            node; node = tnode_next(node)) {
      if (strcmp(tnode_token(node)->data.string, str) == 0)
         return node;
   }
   return NULL;
}

/** convert_possessive
  *
  * Convert a possessive quantifier to an atomic group. So
  * (...)*+ becomes (?>(...)*).
  */
static void convert_possessive(tlist_t* list, tnode_t* node) {
   token_t* token = tnode_token(node);
   token->flag = ATOMIC;
   tnode_t* trash = NULL;
   tnode_t* range = tnode_prev(node);
   tnode_t* piece = tnode_prev(range);
   token->data.group = tlist_slice(list, piece, range, &trash);
}

/** stringify
  *
  * Create a new string node from a series of literal tokens.
  */
static tnode_t* stringify(tlist_t* list, tnode_t* node) {
   token_t token;
   token.flag = STRING; token.ngr = -1;
   tnode_t* end = node;
   tnode_t* insert = NULL;
   
   // slice out list of applicable literals
   while (part_of_string(tnode_next(end))) {
      end = tnode_next(end);
   }
   tlist_t* literals = tlist_slice(list, node, end, &insert);
   
   // figure out size of new string and allocate
   int size = 1;
   for (tnode_t* n = tlist_front(literals); n; n = tnode_next(n))
   	size += u8_bytelen(tlist_token(literals)->data.literal);
   token.data.string = calloc(sizeof(char), size);
   assert(token.data.string);
   
   // encode data to string
   for (char* u = token.data.string; !tlist_empty(literals); ) {
   	u8cdpnt_t* cp = tlist_token(literals)->data.literal;
      u = u8_encode(cp, u);
      free(cp);
      tlist_pop_front(literals);
   }
   tlist_free(literals);
   return tlist_insert(list, insert, token);
}

/** denullify
  *
  * Convert '[\0...]' to '(?:[...]|$)'.
  */
static void denullify(tnode_t* node) {
   token_t token;
   token.flag = tnode_token(node)->flag;
   token.ngr = -1;
   token.data.class = tnode_token(node)->data.class;
   if (token.flag == CLASS)
      class_delete_codepoint(token.data.class, 0);
   else
      class_insert_codepoint(token.data.class, 0);
   tnode_token(node)->flag = GROUP;
   tnode_token(node)->data.group = tlist_new();
   tlist_push_back(tnode_token(node)->data.group, token);
   token.data.class = NULL;
   token.flag = ALTERNATOR;
   tlist_push_back(tnode_token(node)->data.group, token);
   token.flag = EDGEANCH;
   tlist_push_back(tnode_token(node)->data.group, token);
}

/** convert_literal
  *
  * Convert a literal case into a class case.
  */
static void convert_literal(tnode_t* node) {
	tnode_token(node)->flag = CLASS;
	u8cdpnt_t* literal = tnode_token(node)->data.literal;
	tnode_token(node)->data.class = class_new();
	class_insert_codepoint(tnode_token(node)->data.class, u8_deref(literal));
	free(literal);
}

/** weedeat
  *
  * Do the following things to the token list:
  *
  * 1) We allow the user to match the null terminating byte as a
  *    literal or in a character class. We treat this as an anchor
  *    to the end of a string. Convert to anchor case.
  *
  * 2) Combine each series of non-repeating literal tokens into a
  *    string token.
  *
  * 3) Convert a possessive token into a non-capturing atomic group
  *    token.
  *
  * 4) Convert non-string literal cases to class cases.
  */
static tlist_t* weedeat(tlist_t* list) {
   for (tnode_t* node = tlist_front(list);
                            node; node = tnode_next(node)) {
      if (need_denullify(node)) {
         denullify(node);
      } else if (part_of_string(node)) {
         node = stringify(list, node);
      } else if (tnode_token(node)->flag == POSSESSIVE) {
         convert_possessive(list, node);
      } else if (tnode_token(node)->flag == LITERAL) {
         convert_literal(node);
      }
   }
   return list;
}
   
/** badref_check
  *
  * Check the list of tokens to see if there are any backreference
  * that refer to non-existent groups. Convert named-backreferences
  * to regular backreferences using the name list.
  */
static bool badref_check(tlist_t* list, int max, tlist_t* names) {
   for (tnode_t* node = tlist_front(list);
                      node; node = tnode_next(node)) {
      token_t* token = tnode_token(node);
      switch (token->flag) {
         case GROUP: case LOOKAHEAD: case NLOOKAHEAD: case ATOMIC:
      // case LOOKBEHIND: case NLOOKBEHIND:
            if (badref_check(token->data.group, max, names))
               return true;
            break;
         case NAME: {
            tnode_t* name_node 
                     = string_list_search(names, token->data.string);
            if (!name_node)
               return true;
            token->flag = token->ngr == 0 ? SUBROUTINE : REFERENCE;
            token->ngr = tnode_token(name_node)->ngr;
            free(token->data.string);
            token->data.string = NULL;
            break;
         }
         default:
            if (tnode_token(node)->ngr >= max)
               return true;
      }
   }
   return false;
}

/***********************various parsing subroutines******************/

/** ErrorCheck
  *
  * Check a condition and class shre_er if the condition is true.
  */
#define ErrorCheck(CONDITION, ERRNO) \
   if (CONDITION) { \
      shre_er = ERRNO; \
      goto Error; \
   }

/** is_escaped
  *
  * Returns true if the character pointed to by str is escaped by a
  * backslash, keeping in mind that two backslashes in a row combine
  * to become a literal backslash.
  *
static bool is_escaped(char* str) {
   int count = 0;
   while (*(--str) == '\\')
      ++count;
   return count % 2 == 1;
}

 ** find_closing
  *
  * Find the balanced closing character in str, where o is the
  * opening character, and c is the closing character.  If the
  * brackets\ parentheses are unbalanced, returns NULL or a
  * pointer to the last c if there is one.
  *
  * We also need to make sure that the closing character isn't inside
  * of brackets '[' ']', since characters in brackets are treated
  * as literals.
  *
  * Examples: find_closing(str, '[', ']') where
  * str == "[123[567\[0]2]4]" will return a pointer to index 13,
  * find_closing(str, '\0', ']') on the same str will return
  * a pointer to index 11, and find_closing(str, '(', ')') will
  * return NULL. If str is "[[[[]", find_closing(str, '[', ']')
  * returns a pointer to index 4. If str is "([)])",
  * find_closing(str, '(', ')') will return index 4.
  */
static char* find_closing(char* str, char o, char c) {
   char* prev = NULL;
   char* bracket_check = NULL;
   for (++str; *str != '\0'; ) {
      if (*str == c) {
         return str;
      } else if (*str == o) {
         prev = find_closing(str, o, c);
         if (!prev)
            return NULL;
         str = prev + 1;
      } else if (*str == '[') {
         bracket_check = find_closing(str, '[', ']');
         if (bracket_check)
            str = bracket_check + 1;
         else
            ++str;
      } else if (*str == '\\') {
         if (*(str+1) != '\0')
            ++str;
         ++str;
      } else {
         ++str;
      }
   }
   return prev;
}

/** hex_value
  *
  * Gives the integer value of a single hex character.
  */
static inline char hex_value(char symbol) {
   return (symbol >= '0' && symbol <= '9') ? symbol - '0' :
          (symbol >= 'a' && symbol <= 'f') ? symbol - 'a' + 10 :
                                             symbol - 'A' + 10;
}

/** parse_int
  *
  * Returns the integer value between the pointers begin and end.
  * Begin points to one before the first digit and end points to
  * one after the last digit. The base variable should be 8, 10,
  * or 16.
  */
static int parse_int(char* begin, char* end, int base) {
   int integer = 0, place_value = 1;
   while (--end != begin) {
   	integer += hex_value(*end) * place_value;
      place_value *= base;
   }
   return integer;
}

/** num_digits
  *
  * Get the number of digits in an integer.
  *
static inline int num_digits(int num) {
   return num == 0 ? 1 : (int) floor(log10(num)) + 1;
}

 ** is_number
  *
  * Return 1 if the input char pointers delimit an integer, with
  * the first pointer one before the first digit, and the second
  * pointer one after the last digit. If not, return 0. If the
  * pointers denote an int, but it has more than nine digits,
  * return -1.
  */
static int is_number(char* str, char* end) {
   int digit_count = 0;
   for (++str; str != end; ++str) {
      ++digit_count;
      if (!isdigit(*str) && !isxdigit(*str))
         return 0;
   }
   return digit_count <= 9 ? 1 : -1;
}

/** isodigit
  *
  * A version of isdigit for octal numerals.
  */
static inline bool isodigit(int digit) {
	return digit >= '0' && digit <= '7';
}

/** isoctal
  *
  * Test for an octal escape.
  */
static inline bool isoctal(char* r) {
	return isodigit(*r) && isodigit(*(r+1)) && isodigit(*(r+2));
}

/** parse_escape
  *
  * If the given character represents an escape, return true and
  * class the int argument to the value of the escape. If the character
  * doesn't represent an escape, then return false. Returns true in
  * the case of invalid hexadecimal or octal escape; calling function
  * has to make sure shre_er == NERROR.
  */
static bool parse_escape(char** str, int* value) {
	char* regex = *str;

	// check for octal escape
	if (isoctal(regex)) {
		*str = regex-- + 3;
		*value = parse_int(regex, *str, 8);
		return true;
	}

   switch (*regex) {
      case '0': *value = 0x00; break;  // null character
      case 'a': *value = 0x07; break;  // bell character
      case 'b': *value = 0x08; break;  // backspace
      case 't': *value = 0x09; break;  // horizontal tab
      case 'n': *value = 0x0A; break;  // newline
      case 'v': *value = 0x0B; break;  // vertical tab
      case 'f': *value = 0x0C; break;  // formfeed
      case 'r': *value = 0x0D; break;  // carriage return

      // hexadecimal escape
      case 'x':
         *str = regex + 3;
         if (!(is_number(regex, *str)))
            shre_er = HEXESC;
         *value = parse_int(regex, *str, 16);
         return true;

      default:
         return false;
   }
   ++(*str);
   return true;
}

/** is_shorthand
  *
  * Does a character following a backslash represent a shorthand
  * character class? See function shorthand for the meaning of each
  * class in terms of normal class syntax.
  */
static inline bool is_shorthand(char symbol) {
    return symbol == 'd' ||
           symbol == 'D' ||
           symbol == 'w' ||
           symbol == 'W' ||
           symbol == 's' ||
           symbol == 'S' ||
           symbol == 'h' ||
           symbol == 'H';
}

/** IsIntersectionOperator
  *
  * Necessary conditions for char* regex to be pointing at the
  * first character of an intersection operator in parse_class,
  * assuming that begin and end point at the beginning and end
  * of the character class, respectively.
  */
#define IsIntersectionOperator() \
 (    *(regex+1) == '&' \
   && regex-1 != begin \
   && (  (*(regex+2) == '[' \
          && find_closing(regex+2, '[', ']') != end) \
       || (*(regex+2) == '\\' && is_shorthand(*(regex+3)))   ))

/** IsDifferenceOperator
  *
  * Necessary conditions for char* regex to be pointing at the
  * first character of a difference operator in parse_class.
  */
#define IsDifferenceOperator() \
   ((*(regex+1) == '[' && find_closing(regex+1, '[', ']') != end) \
    || (*(regex+1) == '\\' && is_shorthand(*(regex+2))))

/** NestedClass
  *
  * Combine the nested class with the class compiled so far.
  * intersection and difference are bools which denote whether
  * the nested class was preceded by a intersection or difference
  * operator. The values of the negate and nest_negate variables
  * must also be taken into account.
  */
#define NestedClass() \
   assert(!(intersection && difference)); \
   if (intersection || difference) { \
      if (intersection == nest_negate) { \
         class_difference(class, nest_class); \
      } else { \
         class_intersection(class, nest_class); \
      } \
      intersection = difference = false; \
   } else { \
      if (nest_negate) { \
         class_intersection(class, nest_class); \
         *negate = !*negate; \
      } else { \
         class_union(class, nest_class); \
      } \
   } \
   class_free(nest_class)

/** parse_shorthand
  *
  * If the given character represents a shorthand character class,
  * then return true and class the given class variable to point at
  * a new class representing the class. Otherwise return false. This
  * works very similarly to the function 'parse_escape' defined above.
  */
static class_t* _parse_class(char*, char*, bool*);
static bool parse_shorthand(char lit, bool* negate, class_t** class) {
   char* regex;
   switch (lit) {
      case 'd': regex = "[0-9]";         break;
      case 'D': regex = "[^0-9]";        break;
      case 'w': regex = "[a-zA-Z0-9_]";  break;
      case 'W': regex = "[^a-zA-Z0-9_]"; break;
      case 's': regex = "[ \t\r\n\f]";   break;
      case 'S': regex = "[^ \t\r\n\f]";  break;
      case 'h': regex = "[a-fA-F0-9]";   break;
      case 'H': regex = "[^a-fA-F0-9]";  break;
      default: return false;
   }
   *class = _parse_class(regex, regex + strlen(regex) - 1, negate);
   assert(*class);
   return true;
}

/** GetBound
  *
  * Helper macro for 'parse_range'.
  */
#define GetBound(NUM, BEGIN, END) \
   int u = is_number(BEGIN, END); \
   if (u == 1) { \
      NUM = parse_int(BEGIN, END, 10); \
   } else if (u == -1) { \
      shre_er = BADINT; \
      return true; \
   } else { \
      return false; \
   }

/** parse_range
  *
  * This function is used if *str == '{'. If this character
  * is at the beginning of a range case, class the two integers
  * to values the range denotes and return true. Otherwise, return
  * false.
  */
static bool parse_range(char** regex, int* a, int* b) {
   assert(**regex == '{');
   char* end = strchr(*regex, '}');
   char* comma = strchr(*regex, ',');
   if (!end || *regex + 1 == comma)
      return false;
   if (comma && comma < end) {
      GetBound(*a, *regex, comma)//;
      *regex = comma;
      if (comma + 1 == end) {
         *b = -1;
         *regex = end + 1;
         return true;
      }
   }
   GetBound(*b, *regex, end)//;
   if (!comma)
      *a = *b;
   *regex = end + 1;
   return true;
}

/** lazy_applicable
  *
  * Check if would be relevant to change the default greediness
  * for the previous token.
  */
static inline bool lazy_applicable(tflag prev_flag) {
   return prev_flag == RANGE;
}

/** range_applicable
  *
  * Check if the previous token is compatible with a repetition
  * case.
  */
static inline bool range_applicable(tflag prev_flag) {
   return prev_flag == LITERAL    || prev_flag == CLASS  || 
          prev_flag == NCLASS     || prev_flag == GROUP  ||
          prev_flag == REFERENCE  || prev_flag == ATOMIC ||
          prev_flag == SUBROUTINE;
}

/** SetRange
  *
  * Set the range in a range token.
  */
#define SetRange(A,B) \
   token.flag = RANGE; \
   token.data.range.begin = A; \
   token.data.range.end = B

/*****************************parse_class****************************/

static class_t* _parse_class(char* begin, char* end, bool* negate) {
   char*     regex = begin;
   class_t*  class = class_new();
   bool      nest_negate;
   class_t*  nest_class;
   char*     nest_end;
   bool      intersection = false;
   bool      difference = false;
   int       prev_escape = -1;
   int       range_low, range_high;

   // check for negation
   *negate = false;
   if (*(++regex) == '^') {
      ++begin;
      ++regex;
      *negate = true;
   }
   ErrorCheck(regex == end, EMPCLA);

   do {
      switch (*regex) {

         // && operator
         case '&':
            prev_escape = -1;
            if (IsIntersectionOperator()) {
               regex += 2;
               intersection = true;
            } else {    // treat as literal
               class_insert_codepoint(class, *regex++);
            }
            break;

         // character range or difference operator
         case '-':
            if (regex - 1 == begin              //
                       || regex + 1 == end) {   // treat as literal
               class_insert_codepoint(class, *regex++);
               prev_escape = -1;
               break;
            } else if (IsDifferenceOperator()) {
               ++regex;
               difference = true;
               prev_escape = -1;
               break;
            }
			{
            // character range
            if (prev_escape >= 0) {
               range_low = prev_escape;
               prev_escape = -1;
            } else {
               range_low = *(regex-1);
            }
            if (*(regex+1) == '\\') {
            	regex += 2;
               if (parse_escape(&regex, &prev_escape)) {
                  if (shre_er != HEXESC)
                     goto Error;
                  range_high = prev_escape;
               } else {
                  range_high = *regex;
                  ++regex;
               }
            } else {
               range_high = *(regex+1);
               regex += 2;
            }
            ErrorCheck(range_low > range_high, BADRAN);
            urange32_t range = { range_low, range_high };
            class_insert_range(class, range);
            break;
			}

         // nested character class
         case '[':
            prev_escape = -1;
            if (regex - 1 == begin) {  // '[' directly at beginning
               class_insert_codepoint(class, *regex++);
               break;
            }
            nest_end = find_closing(regex, '[', ']');
            if (nest_end == end) {     // treat as literal
               class_insert_codepoint(class, *regex++);
               break;
            }
            nest_class = _parse_class(regex, nest_end, &nest_negate);
            if (!nest_class)
               goto Error;
            NestedClass();
            regex = nest_end + 1;
            break;

         // backslash
         case '\\':
            ++regex;

            // check for escapes
            if (parse_escape(&regex, &prev_escape)) {
               if (shre_er != NERROR)
                  goto Error;
               class_insert_codepoint(class, prev_escape);
               break;
            }

            // check for shorthand character classes
            if (parse_shorthand(*regex, &nest_negate, &nest_class)) {
               NestedClass();
               ++regex;
               prev_escape = -1;
               break;
            }

         // default; treat character as literal.
         default:
            prev_escape = -1;
            class_insert_codepoint(class, *regex++);
      }
   } while (regex != end);
   return class;

Error:
   class_free(class);
   return NULL;
}

class_t* parse_class(char* regex) {
   assert(regex);
   char *end = regex + strlen(regex) - 1;
   assert(*regex == '[' && *end == ']');
   bool negate;
   return _parse_class(regex, end, &negate);
}

/*****************************parse_regex*********************************/

static tlist_t* _parse_regex(char* regex, char* end,
                                     int* next_group, tlist_t* name_list) {
   tlist_t*    list = tlist_new();

   // empty regex case
   if (regex == end) {
      token_t token;
      token.ngr = -1;
      token.flag = EMPTY;
      tlist_push_back(list, token);
      return list;
   }
  
   char*       back;
   char*       bracket;
   bool        negate;
   class_t*    class;
   tflag       prev_flag;
   int         a, b;
   token_t     name_token; name_token.flag = NAME;
   bool        top_level = *end == '\0';
   
   // prevents the first iteration of the loop from trying to use
   // prev_flag
   ErrorCheck(*regex == '*' || *regex == '?' || *regex == '+'
              || (*regex == '{' && parse_range(&regex, &a, &b)), NOTREP);

   // all other cases
   do {
      token_t token;
      token.ngr = -1;
      switch (*regex) {

         // alternator character
         case '|':
            ++regex;
            token.flag = ALTERNATOR;
            break;

         // character class
         case '[':
            back = find_closing(regex, '[', ']');
            ErrorCheck(!back, UNBBRA);
            class = _parse_class(regex, back, &negate);
            if (!class)
               goto Error;
            token.data.class = class;
            token.flag = negate ? NCLASS : CLASS;
            regex = back + 1;
            break;

         // match any character except newline
         case '.':
            ++regex;
            token.flag = NCLASS;
            token.data.class = class_new();
            class_insert_codepoint(token.data.class, '\0');
            class_insert_codepoint(token.data.class, '\r');
            class_insert_codepoint(token.data.class, '\n');
            class_insert_codepoint(token.data.class, '\f');
            class_insert_codepoint(token.data.class, '\v');
            break;
            
         // match the empty string at the front of a string
         case '^':
            ++regex;
            token.flag = STANCH;
            break;
         
         // match the empty string at the end of a string
         case '$':
            ++regex;
            token.flag = EDGEANCH;
            break;

         // closing parens before opening parens
         case ')':
            shre_er = UNBPAR;
            goto Error;

         // parentheses
         case '(':
{  bool capture = false;
   back = find_closing(regex, '(', ')');
   ErrorCheck(!back || back == end, UNBPAR);
   if (*(regex+1) == '?') {   // (?...) syntax
      ++regex;
   
      // subroutine call
      if (is_number(regex, back) == 1) {
         token.flag = SUBROUTINE;
         token.ngr = parse_int(regex, back, 10);
         regex = back + 1;
         break;
      }
   
      // any other case
      switch (*(++regex)) {
      
         // lookahead assertions
         case '=':
            token.flag = LOOKAHEAD;
            break;
         case '!':
            token.flag = NLOOKAHEAD;
            break;
         
         // support for python style named groups
         case 'P':
         	++regex;
         	ErrorCheck(!(*regex == '<' || *regex == '\''), QUEPAR);

         // lookbehind assertions
         case '<':
            /*
            if (*(regex-1) != P && *regex == '<') {
					if (*(regex+1) == '=') {
						token.flag = LOOKBEHIND;
						++regex;
						break;
					} else if (*(regex+1) == '!') {
						token.flag = NLOOKBEHIND;
						++regex;
						break;
					}
				}
            */
         // named group
         case '\'':
            ErrorCheck(isdigit(*(++regex)), GRPDIG);
            capture = true;
            bracket = strchr(regex, *(regex-1) == '<' ? '>' : '\'');
            ErrorCheck(!bracket || bracket >= back, QUEPAR);
            name_token.data.string = substring(regex, bracket);
            name_token.ngr = *next_group;
            ErrorCheck(string_list_search(name_list,
                              name_token.data.string) != NULL, NAMEXT);
            tlist_push_back(name_list, name_token);
            token.flag = GROUP;
            regex = bracket;
            break;
      
         // subroutine call to named group
         case '&':
            token.flag = NAME;
            token.data.string = substring(++regex, back);
            token.ngr = 0;
            goto SkipParse;
         
         // regular expression recursion
         case 'R':
            ErrorCheck(regex + 1 != back, QUEPAR);
            token.flag = SUBROUTINE;
            token.ngr = 0;
            goto SkipParse;

         // atomic group
         case '>':
            token.flag = ATOMIC;
            break;
   
         // nocapture group
         case ':':
            token.flag = GROUP;
            break;
      
         // syntax error
         default:
            shre_er = QUEPAR;
            goto Error;
      }
   } else {
      capture = true;
      token.flag = GROUP;
   }
   if (capture)
      token.ngr = (*next_group)++;
   token.data.group = _parse_regex(regex + 1,
                                back, next_group, name_list);
   if (!token.data.group)
      goto Error;
SkipParse:
   regex = back + 1;
   break;
}

         // either change greediness on previous token or denote a range
         //   from 0 matches to 1 match.
         case '?':
            ++regex;
            if (range_applicable(prev_flag)) {
               SetRange(0, 1);
               break;
            } else if (lazy_applicable(prev_flag)) {
               token.flag = LAZY;
               break;
            } else {
               shre_er = NOTREP;
               goto Error;
            }

         // denote a range of 1 match to infinite matches or possessive
         //   quantifier
         case '+':
            ++regex;
            if (range_applicable(prev_flag)) {
               SetRange(1, -1);
               break;
            } else if (lazy_applicable(prev_flag)) {
               token.flag = POSSESSIVE;
               break;
            } else {
               shre_er = NOTREP;
               goto Error;
            }

         // denote a range of 0 matches to infinite matches
         case '*':
            ++regex;
            ErrorCheck(!range_applicable(prev_flag), NOTREP);
            SetRange(0, -1);
            break;

         // could denote a quantifier range; if not, treat as literal
         case '{':
            if (!parse_range(&regex, &a, &b)) {
               token.flag = LITERAL;
               token.data.literal = u8_decode(regex++);
               break;
            }
            ErrorCheck(!range_applicable(prev_flag), NOTREP);
            if (shre_er == BADINT)
               goto Error;
            ErrorCheck(b >= 0 && a > b, BADQAN);
            SetRange(a, b);
            break;

         // escapes/ back references
         case '\\':
            ++regex;
            ErrorCheck(*regex == '\0', BOGESC);
            
            // word anchor
            if (*regex == 'b') {
               ++regex;
               token.flag = WORDANCH;
               break;
            }
            if (*regex == 'B') {
               ++regex;
               token.flag = NWORDANCH;
               break;
            }
            
            // anchor to end of string
            if (*regex == '0') {
               ++regex;
               token.flag = EDGEANCH;
               break;
            }

            // regular escape characters, octal escapes,
            // hexadecimal escapes
            if (parse_escape(&regex, &a)) {
               if (shre_er != NERROR)
                  goto Error;
               token.flag = LITERAL;
               token.data.literal = u8_new(a);
               break;
            }
            
            // not a newline
            if (*regex == 'N') {
               ++regex;
               token.flag = NCLASS;
               token.data.class = class_new();
               class_insert_codepoint(token.data.class, '\0');
               class_insert_codepoint(token.data.class, '\r');
					class_insert_codepoint(token.data.class, '\n');
					class_insert_codepoint(token.data.class, '\f');
					class_insert_codepoint(token.data.class, '\v');
               break;
            }

            // shorthand character class
            if (parse_shorthand(*regex, &negate, &class)) {
               ++regex;
               token.flag = negate ? NCLASS : CLASS;
               token.data.class = class;
               break;
            }

            // backreference
            if (isdigit(*regex)) {
               token.ngr = *regex - '0';
               token.flag = REFERENCE;
               regex++;
               break;
            }
            
            // another backreference syntax
            if (*regex == 'g' || *regex == 'k') {
               ++regex;
               back = NULL;
               if (*regex == '\'') {
                  back = strchr(regex+1, '\'');
               } else if (*regex == '<') {
                  back = strchr(regex+1, '>');
               }
               if (!back) {
                  token.flag = LITERAL;
                  token.data.literal = u8_new(*(regex-1));
                  break;
               }
               int test = is_number(regex, back);
               if (test == 0) {
                  token.flag = NAME;
                  token.data.string = substring(regex + 1, back);
                  regex = back + 1;
               } else if (test == 1) {
                  token.ngr = parse_int(regex, back, 10);
                  token.flag = REFERENCE;
                  regex = back + 1;
               } else if (test == -1) {
                  shre_er = BADINT;
                  goto Error;
               }
               break;
            }

            // \Q...\E syntax
            if (*regex == 'Q') {
            	++regex;
            	if (*regex == '\\' && *(regex+1) == 'E') {
            		regex += 2;
            		continue;
            	}
               token.flag = STRING;
               char* begin = regex;
               while (*regex && *regex != '\\' && *(regex+1) != 'E')
               	++regex;
               token.data.string = substring(begin, regex);
               if (*regex) regex += 2;
               break;
            }
            
            // fall through to literal case

         // treat the character as a literal
         default:
            token.flag = LITERAL;
            token.data.literal = u8_decode(regex);
            regex = u8_end(token.data.literal);
      }
      tlist_push_back(list, token);
      prev_flag = token.flag;
   } while (regex != end);
   if (top_level)
      ErrorCheck(badref_check(list, *next_group, name_list), BADREF);
   assert(shre_er == NERROR);
   return weedeat(list);

Error:
   tlist_obliterate(list);
   if (top_level)
      tlist_obliterate(name_list);
   return NULL;
}

tlist_t* parse_regex(char* regex, obhash_t** group_names) {
   assert(regex);
   shre_er = NERROR;
   tlist_t* name_list = tlist_new();
   int num_groups = 1;
   tlist_t* tokens = _parse_regex(regex, regex + strlen(regex),
                                              &num_groups, name_list);
   if (!tokens)
      return NULL;
   if (!tlist_empty(name_list)) {
   	*group_names = obhash_new(&free);
		do {
			token_t* s = tlist_token(name_list);
			int* w = malloc(sizeof(int));
			assert(w);
			*w = s->ngr;
			obhash_add(*group_names, s->data.string, w);
			tlist_pop_front(name_list);
		} while (!tlist_empty(name_list));
	}
   tlist_free(name_list);
   return tokens;
}

/********************************************************************/
