/* u8_translate.h
 *
 * Translate a string of characters to unicode codepoint, and
 * vice-versa.
 */

#ifndef __regex_u8_translate
#define __regex_u8_translate

/* Codepoint to indicate that we have attempted to decode a malformed
 * unicode code sequence.
 */
#define ErrorPoint 0xFFFFFFFF

/* u8cdpnt_t
 *
 * Data structure representing a single unicode codepoint.
 * Can be used to write data to a character array. A cdpnt
 * is said to be bound to a string if it has pointers into
 * that string. An unbound cdpnt will cause the bigin and end
 * functions to return NULL.
 */
typedef struct _u8cdpnt u8cdpnt_t;

/** decode
  *
  * Given a pointer to the first byte of a unicode code sequence,
  * decode the sequence and construct a cdpnt struct. The cdpnt which
  * is returned is bound to the given string.
  */
u8cdpnt_t* u8_decode(char*);

/** encode
  *
  * Given a char* and a cdpnt, write the appropriate code units
  * into the character array, starting at the given position.
  * The given cdpnt will be bound to the new string. The caller is
  * responsible for making sure there is room in the array for
  * the data to be written. The number of bytes in a cdpnt
  * can be determined with u8_bytelen. The return value is a pointer
  * to one after the final byte of the new code sequence.
  */
char* u8_encode(u8cdpnt_t*, char*);

/** new
  *
  * Create a new unbound cdpnt with given value. It is the caller's
  * responsibility to ensure that the given codepoint is valid;
  * however, this does assert that the given codepoint is less than
  * or equal to 10FFFF.
  */
u8cdpnt_t* u8_new(uint32_t);

/** bytelen
  *
  * Get the length of the unicode code sequence for the given
  * codepoint.
  */
int u8_bytelen(u8cdpnt_t*);

/** deref
  *
  * Get the codepoint.
  */
uint32_t u8_deref(u8cdpnt_t*);

/** begin
  *
  * Get a pointer to the first byte in the sequence. Returns null if
  * the cdpnt is unbound.
  */
char* u8_begin(u8cdpnt_t*);

/** end
  *
  * Get a pointer to one after the last byte in the sequence, which is
  * the first byte in the next sequence.
  */
char* u8_end(u8cdpnt_t*);

/** duplicate
  *
  * Get a copy of the pointer's domain in the form of a null terminated
  * character array. This copy must be freed. This can be used for
  * printing a single unicode character.
  */
char* u8_duplicate(u8cdpnt_t*);

#endif
