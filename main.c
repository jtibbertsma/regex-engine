/* regex tester program
 *
 * Use the command "NEW" to enter a new regular expression.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "shre.h"
#include "shre_errno.h"

#define BUFFER 12000

char buf[BUFFER];
pattern_t* shre;
match_t* match;

void trim(char* str) {
   if (*str != '\0') {
      while (*(++str) != '\0');
      if (*(str-1) == '\n') {
         *(str-1) = '\0';
      }
   }
}

void do_test() {
Start:
   if (isatty(fileno(stdin)))
      printf("Enter a regular expression:\n\n");
   if (fgets(buf, BUFFER, stdin) == NULL)
      return;
   trim(buf);
   if ((shre = shre_compile(buf)) == NULL) {
      printf("error: %s\n", shre_strerror(shre_er));
      goto Start;
   }
   if (isatty(fileno(stdin)))
      printf("\nEnter text to test the regular expression:\n\n");
   while (fgets(buf, BUFFER, stdin) != NULL) {
      trim(buf);
      if (strcmp(buf, "NEW") == 0) {
         goto Start;
      }
      printf("Pattern:  '%s'\n", shre_expression(shre));
      printf("String:   '%s'\n", buf);
      printf("Match:    ");
      if ((match = shre_search(shre, buf)) == NULL) {
         printf(" None\n\n");
      } else {
         printf("'%s'\n", match_get(match));
         for (int i = 1; i < match_num_groups(match); ++i) {
            printf("Group %2d: ", i);
            char* str = match_group(match, i);
            if (!str) {
               printf(" NULL\n");
            } else {
               printf("'%s'\n", str);
               free(str);
            }
         }
         printf("\n");
         match_free(match);
      }
   }
}

int main() {
   start_regex_engine();
   do_test();
   cleanup_regex_engine();
   return 0;
}
