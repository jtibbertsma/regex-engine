#include <stdio.h>
#include <stdlib.h>

#include "dank.h"

int main() {
   start_regex_engine();
   
   match_t* match =
        dank_search(dank_compile("(?<name>123)\\g<name>"), "123123");
   char* group = match_named_group(match, "name");
   printf("Should print '123':\n\n%s\n", group);
   
   free(group);
   match_free(match);
   cleanup_regex_engine();
   return 0;
}
