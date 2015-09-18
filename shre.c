/* shre.c
 *
 * Implementation of the regex interface.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "class.h"
#include "core.h"
#include "parser.h"
#include "factory.h"
#include "tokens.h"
#include "range.h"
#include "util.h"
#include "obhash.h"
#include "shre.h"

/* pattern
 *
 * Declaration of the pattern object, which is more or less
 * a wrapper for a core object.
 */
struct _pattern {
   core_t* core;
   obhash_t* names;    // named groups
   char* regex;        // the string passed into compile
};

/* match
 *
 * Declaration of match object. Has an array of group structs
 * which represent group captures.
 */
struct _match {
   obhash_t* names;
   range_t* groups;
   uint32_t offset;
};

/* scanner
 *
 * Declaration of scanner struct. obj keeps track of the search position
 * in the string.
 */
struct _scanner {
   pattern_t* pattern;
   char* start;
   char* curr;
};

// static functions
//   The functions 'match_new' and 'free_pattern' shouldn't be called
//   by the user.

/** match_new
  *
  * Make a new match object from the return value of core_match.
  */
static match_t* match_new(range_t*, obhash_t*, int);

/** free_pattern
  *
  * Deallocate the memory used by a pattern object. Don't free
  * the regex string, since that will be freed in the hash table.
  */
static void free_pattern(pattern_t* pattern) {
   obhash_free(pattern->names);
   core_free(pattern->core);
   free(pattern);
}


/*************************global variables***************************/

obhash_t* ptable = NULL;    // global pattern hash table

class_t* word_characters = NULL;    // values of word characters

char* trash;

/**************************regex engine functions********************/

void start_regex_engine() {
   assert(!ptable);
   ptable = obhash_new( (void (*)(void*)) &free_pattern);
   word_characters = parse_class("[\\w]");
}

bool engine_is_initialized() {
   return ptable;
}

int num_patterns() {
   assert(ptable);
   return obhash_size(ptable);
}

void clear_cache() {
   assert(ptable);
   obhash_clear(ptable);
}

void cleanup_regex_engine() {
   assert(ptable);
   obhash_free(ptable);
   ptable = NULL;
   class_free(word_characters);
   word_characters = NULL;
}

/*****************************regex operations***********************/

pattern_t* shre_compile(char* regex) {
   assert(ptable);
   assert(regex);
   
   // look for pattern in hashtable
   pattern_t* pattern = (pattern_t*) obhash_find(ptable, regex);
   if (pattern)
      return pattern;

   // build a new pattern
   obhash_t* names = NULL;
   tlist_t* tokens = parse_regex(regex, &names);
   if (!tokens)
      return NULL;   // bad regular expression; shre_er is tree
   pattern = malloc(sizeof(pattern_t));
   assert(pattern);
   pattern->regex = strdup(regex);
   pattern->names = names;
   pattern->core = build_core(tokens);
   obhash_add(ptable, pattern->regex, pattern); // add new pattern
   return pattern;
}

const char* shre_expression(pattern_t* obj) {
   assert(ptable);
   assert(obj);
   return obj->regex;
}

match_t* shre_search(pattern_t* pattern, char* str) {
   assert(ptable);
   assert(pattern);
   assert(str);
   char* start = str;
   for (;; ++str) {
      range_t* groups = core_match(pattern->core, str,
                              NULL, NULL, NULL, 0, &trash, start);
      if (groups)
         return match_new(groups, pattern->names,
                          range_group(groups, 0)->begin - start);
      if (*str == '\0')
         break;
   }
   return NULL;
}

match_t* shre_entire(pattern_t* pattern, char* str) {
   assert(ptable);
   assert(pattern);
   assert(str);
   range_t* groups =
          core_match(pattern->core, str, NULL, NULL,
                                  NULL, 0, &trash, str);
   if (!groups)
      return NULL;
   if (*(range_group(groups, 0)->end) == '\0') {
      return match_new(groups, pattern->names,
                          range_group(groups, 0)->begin - str);
   }
   return NULL;
}

bool quick_search(char* regex, char* str) {
   assert(regex);
   assert(str);
   pattern_t* pattern = shre_compile(regex);
   if (!pattern)
      return false;
   char* start = str;
   while (*str != '\0') {
      range_t* groups =
           core_match(pattern->core, ++str, NULL, NULL, 
                                  NULL, 0, &trash, start);
      if (groups) {
         range_free(groups);
         return true;
      }
   }
   return false;
}

bool quick_entire(char* regex, char* str) {
   assert(regex);
   assert(str);
   pattern_t* pattern = shre_compile(regex);
   if (!pattern)
      return false;
   range_t* groups =
             core_match(pattern->core, str, NULL, NULL,
                                       NULL, 0, &trash, str);
   if (!groups)
      return false;
   if (*(range_group(groups, 0)->end) == '\0') {
      range_free(groups);
      return true;
   }
   range_free(groups);
   return false;
}

/*****************************match operations************************/

char* match_get(match_t* match) {
   assert(match);
   group_t* main = range_group(match->groups, 0);
   return substring(main->begin, main->end);
}

int match_num_groups(match_t* match) {
   assert(match);
   return range_size(match->groups);;
}

uint32_t match_offset(match_t* match) {
   assert(match);
   return match->offset;
}

char* match_group(match_t* match, int gr) {
   assert(match);
   group_t* captcha = range_group(match->groups, gr);
   if (!captcha || !captcha->begin)
      return NULL;
   return substring(captcha->begin, captcha->end);
}

char* match_named_group(match_t* match, char* grname) {
   assert(match);
   int* gr = NULL;
   if (match->names)
      gr = (int*) obhash_find(match->names, grname);
   if (!gr)
      return NULL;
   return match_group(match, *gr);
}

match_t* match_new(range_t* groups, obhash_t* names, int offset) {
   match_t* match = malloc(sizeof(match_t));
   assert(match);
   match->offset = offset;
   match->names = names;
   match->groups = groups;
   return match;
}

void match_free(match_t* match) {
   if (match) {;
      range_free(match->groups);
      free(match);
   }
}


/*************************scanner operations*************************/

scanner_t* scan_new(pattern_t* pattern, char* input) {
   assert(pattern);
   assert(input);
   scanner_t* scanner = malloc(sizeof(scanner_t));
   assert(scanner);
   scanner->pattern = pattern;
   scanner->start = scanner->curr = input;
   return scanner;
}

match_t* scan_next(scanner_t* sc) {
   assert(ptable);
   assert(sc);
   for (;; ++sc->curr) {
      char* pos = sc->curr;
      range_t* groups = core_match(sc->pattern->core, sc->curr,
                           NULL, NULL, NULL, 0, &sc->curr, sc->start);
      if (groups) {
         if (pos == sc->curr)
            scan_increment(sc);
         return match_new(groups, sc->pattern->names,
                          range_group(groups, 0)->begin - sc->start);
      }
      if (*sc->curr == '\0')
         break;
   }
   return NULL;
}

match_t* scan_try(scanner_t* sc) {
   assert(ptable);
   assert(sc);
   range_t* groups = core_match(sc->pattern->core, sc->curr, NULL,
                                     NULL, NULL, 0, &trash, sc->start);
   if (groups)
      return match_new(groups, sc->pattern->names,
                       range_group(groups, 0)->begin - sc->start);
   return NULL;
}

void scan_seek(scanner_t* sc, uint32_t seek) {
   assert(sc);
   uint32_t len = strlen(sc->start);
   sc->curr = sc->start + (seek >= len ? len : seek);
}

uint32_t scan_tell(scanner_t* sc) {
   assert(sc);
   return sc->curr - sc->start;
}

void scan_increment(scanner_t* sc) {
   assert(sc);
   if (*sc->curr != '\0')
      ++sc->curr;
}

/********************************************************************/
