#include <stdio.h>
#include <stdlib.h>

#include "dank.h"

int main() {
   start_regex_engine();
   
   char* name1 = "Blah";
   char* name2 = "Walafey";
   char regex[256];
   sprintf(regex, "(?<%s>\\b\\w+\\b) (?<%s>123)", name1, name2);
   match_t* match = dank_search(dank_compile(regex), "this 123");
   if (match) {
      char* one = match_named_group(match, name1);
      char* two = match_named_group(match, name2);
      printf("Expected output:\n\nthis\n123\n\n");
      printf("Real output:\n\n%s\n%s\n", one, two);
      free(one);
      free(two);
   } else {
      printf("Fail\n");
   }
   
   cleanup_regex_engine();
   return 0;
}
