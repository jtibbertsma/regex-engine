/* u8_translate.c
 *
 * Lowlevel operations for transforming UTF-8 encoded byte arrays
 * to and from codepoints.
 */

#include <stdlib.h>
#include <assert.h>

#include "u8_translate.h"
#include "util.h"

/* Declaration of cdpnt struct. It keeps track of two char pointers
 * which delimit a cdpnt's domain, a pointer to the next
 * codepoint, and an int which is the struct's unicode code point.
 */
struct _u8cdpnt {
   char*         begin;
   char*         end;
   uint32_t  codepoint;
};

/***************************static functions*************************/

/** IsCont
  *
  * Check if a byte is of the form 10xxxxxx.
  */
#define IsCont(BYTE) ((BYTE & 0xC0) == 0x80)

/** ByteLen
  *
  * Gives the length of a unicode code sequence given a codepoint.
  */
#define ByteLen(CP) \
   (CP < 0x0080 ? 1 : CP < 0x0F00 ? 2 : CP < 0xFFFF ? 3 : 4)

/** decoding functions
  *
  * Translate a sequence of bytes encoded in UTF-8 into an unsigned
  * integer.
  */

static inline uint32_t u8decode2bytes(char* u) {
   int b1 = *u     & 0x1F;
   int b2 = *(u+1) & 0x3F;
   return (b1 << 6) | b2;
}

static inline uint32_t u8decode3bytes(char* u) {
   int b1 = *u     & 0x0F;
   int b2 = *(u+1) & 0x3F;
   int b3 = *(u+2) & 0x3F;
   return (b1 << 12) | (b2 << 6) | b3;
}

static inline uint32_t u8decode4bytes(char* u) {
   int b1 = *u     & 0x08;
   int b2 = *(u+1) & 0x3F;
   int b3 = *(u+2) & 0x3F;
   int b4 = *(u+3) & 0x3F;
   return (b1 << 18) | (b2 << 12) | (b3 << 6) | b4;
}

/** _decode
  *
  * Given a char*, decode a codepoint and tree the end pointer
  * to point to one after the end of the decoded sequence.
  */
static uint32_t _decode(char* begin, char** end) {
   switch (*begin & 0xF0) {

      // ascii case
      case 0x00: case 0x10: case 0x20: case 0x30:
      case 0x40: case 0x50: case 0x60: case 0x70:
         *end = begin + 1;
         return *begin;

      // sequence beginning with continuing byte
      case 0x80: case 0x90: case 0xA0: case 0xB0:
         *end = begin + 1;
         break;

      // multibyte cases
      case 0xC0: case 0xD0:   // 110xxxxx
         *end = begin + 2;
         if (!IsCont(*(begin+1)))
            break;
         return u8decode2bytes(begin);

      case 0xE0:              // 1110xxxx
         *end = begin + 3;
         if (!(IsCont(*(begin+1)) && IsCont(*(begin+2))))
            break;
         return u8decode3bytes(begin);

      case 0xF0:              // 11110xxx
         *end = begin + 4;
         if (!(IsCont(*(begin+1)) && IsCont(*(begin+2))
                                  && IsCont(*(begin+3))))
            break;
         return u8decode4bytes(begin);
   }
   return ErrorPoint;
}

/** encoding functions
  *
  * Write bytes to a character array given a codepoint.
  */
  
static inline void u8encode2bytes(uint32_t cp, char* write) {
   *write++ = 0xC0 | (0x1F & (cp >> 6));
   *write++ = 0x80 | (0x3F & cp);
}

static inline void u8encode3bytes(uint32_t cp, char* write) {
   *write++ = 0xE0 | (0xEF & (cp >> 12));
   *write++ = 0x80 | (0x3F & (cp >> 6));
   *write++ = 0x80 | (0x3F & cp);
}

static inline void u8encode4bytes(uint32_t cp, char* write) {
   *write++ = 0xF0 | (0xF7 & (cp >> 18));
   *write++ = 0x80 | (0x3F & (cp >> 12));
   *write++ = 0x80 | (0x3F & (cp >> 6));
   *write++ = 0x80 | (0x3F & cp);
}

/** _encode
  *
  * Write a codepoint to a string, setting the end pointer to one after
  * the end of the code sequence.
  */
static char* _encode(uint32_t codepoint, char* write) {
   int len = ByteLen(codepoint);
   switch (len) {
      case 1:
         *write = codepoint;
         break;
      case 2:
         u8encode2bytes(codepoint, write);
         break;
      case 3:
         u8encode3bytes(codepoint, write);
         break;
      case 4:
         u8encode4bytes(codepoint, write);
         break;
   }
   return write + len;
}

/** construct
  *
  * Static constructor for a cdpnt.
  */
static u8cdpnt_t* u8_construct(uint32_t codepoint,
                                            char* begin, char* end) {
   u8cdpnt_t* new = malloc(sizeof(u8cdpnt_t));
   assert(new);
   new->codepoint = codepoint;
   new->begin     = begin;
   new->end       = end;
   return new;
}

/***************************public functions*************************/

u8cdpnt_t* u8_decode(char* string) {
   assert(string);
   char* end;
   uint32_t codepoint = _decode(string, &end);
   return u8_construct(codepoint, string, end);
}

char* u8_encode(u8cdpnt_t* code, char* write) {
   assert(code && write);
   if (code->begin) {
      int len = ByteLen(code->codepoint);
      for (int i = 0; i < len; ++i) {
         write[i] = code->begin[i];
      }
      code->begin = write;
      code->end   = write + len;
   } else {
      code->begin = write;
      code->end   = _encode(code->codepoint, write);
   }
   return code->end;
}

u8cdpnt_t* u8_new(uint32_t codepoint) {
   assert(codepoint <= 0x10FFFF);
   return u8_construct(codepoint, NULL, NULL);
}

int u8_bytelen(u8cdpnt_t* code) {
   assert(code);
   return ByteLen(code->codepoint);
}

uint32_t u8_deref(u8cdpnt_t* code) {
   assert(code);
   return code->codepoint;
}

char* u8_begin(u8cdpnt_t* code) {
   assert(code);
   return code->begin;
}

char* u8_end(u8cdpnt_t* code) {
   assert(code);
   return code->end;
}

char* u8_duplicate(u8cdpnt_t* code) {
   assert(code);
   if (code->begin)
      return substring(code->begin, code->end);
   char* dup = calloc(sizeof(char), ByteLen(code->codepoint) + 1);
   (void) _encode(code->codepoint, dup);
   return dup;
}

/********************************************************************/
