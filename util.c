/* util.c
 *
 * Various functions.
 */

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "util.h"

char* substring(char* begin, char* end) {
   assert(begin && end);
   int size = end - begin + 1;
   assert(size > 0);
   char* substr = calloc(size, sizeof(char));
   assert(substr);
   char* ret = substr;
   while (begin != end)
      *substr++ = *begin++;
   return ret;
}

char* safe_strcat(char* dst, char* src, int* size) {
   assert(dst && src && size);
   int len = strlen(dst) + strlen(src) + 1;
   if (len > *size) {
      do *size *= 2; while (len > *size);
      char *newdst = malloc(*size);
      assert(newdst);
      strcpy(newdst, dst);
      free(dst);
      dst = newdst;
   }
   return strcat(dst, src);
}

int count_ones(int u) {
   /* This implementation comes from stack overflow;
    * I do not claim to be clever enough to come up with this.
    * Runs in constant time with constant complexity.
    */
   uint32_t uCount = u
                       - ((u >> 1) & 033333333333)
                       - ((u >> 2) & 011111111111);
   return ((uCount + (uCount >> 3)) & 030707070707) % 63;
}

bool ispow2(int u) {
   /* A given integer is a power of two if and only if it has
    * exactly one '1' in its binary representation.
    */
   return count_ones(u) == 1;
} 

uint32_t strhash(char* string) {
   assert(string);
   uint32_t hashcode = 0;
   for (; *string != '\0'; ++string)
      hashcode = hashcode * 31 + *string;
   return hashcode;
}
