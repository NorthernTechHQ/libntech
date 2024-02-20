#ifndef __STRING_SEQUENCE_H__
#define __STRING_SEQUENCE_H__

#include <sequence.h> // Seq
#include <stdbool.h> // bool
#include <writer.h> // Writer

#define STR_LENGTH_PREFIX_LEN 10

/**
  @brief Create a new Sequence from splitting a string on a fixed delimiter
  @param [in] str String to split.
  @param [in] delimiter The delimiter, a fixed string.
  @return A pointer to the always created Sequence
  */
Seq *SeqStringFromString(const char *str, char delimiter);

/**
 * @brief Join elements in sequence into a string
 * @param[in] seq Sequence of strings to join
 * @param[in] sep Separator between elements (can be NULL)
 * @return The concatenation of the elements in sequence
 * @note Sequence must contain only NUL-terminated strings, otherwise behavior
 *       is undefined.
 */
char *StringJoin(const Seq *seq, const char *sep);

/**
 * @brief Split string into a sequence based on a set of characters.
 * @param[in] str String to split.
 * @param[in] charset Characters to split on.
 * @return Sequence of substrings split on characters.
 * @note If the empty string is passed as character set, then a sequence of one
 *       element containing the entire string is returned. This function is
 *       similar to SeqStringFromString(). However, it splits on multiple
 *       delimiters as opposed to one.
 */
Seq *StringSplit(const char *str, const char *charset);

/**
 @brief Determine if string sequence contains a string
 */
bool SeqStringContains(const Seq *seq, const char *str);

/**
 * @brief Return the total string length of a sequence of strings
 */
int SeqStringLength(Seq *seq);

/**
 * Serialize the string into a length-prefixed format that consists of:
 * 1. 10 bytes of length prefix, where index 9 must be a space
 * 2. The data, with no escaping / modifications
 * 3. A single newline (\n) for readability
 * It is assumed that the string only contains ascii printable
 * characters and is NUL-terminated.
 */
bool WriteLenPrefixedString(Writer *w, const char *string);

/**
 * @brief Read (deserialize) a length-prefixed string
 *
 * Expects the format described in WriteLenPrefixedString() above.
 *
 * @return -1 in case of error, 0 in case of EOF, 1 in case of successfull read
 */
int ReadLenPrefixedString(int fd, char **string);

/**
 * @brief Serializes a sequence of strings to a length prefixed format
 *
 * (De)Serialize uses a length prefixed format.
 */
char *SeqStringSerialize(Seq *seq);

/**
 * @brief Serializes a sequence of strings writing them to a Writer object
 *
 * Similar to SeqStringSerialize, but can be used with FileWriter to write
 * line by line to file.
 */
bool SeqStringWrite(Seq *seq, Writer *w);

/**
 * @brief Serializes a sequence of strings writing them to a file
 *
 * Similar to SeqStringWrite, but opens and closes the file(name) for you.
 */
bool SeqStringWriteFile(Seq *seq, const char *file);

/**
 * @brief Serializes a sequence of strings writing them to a file stream
 *
 * Similar to SeqStringWriteFile, but accepts an open file stream
 */
bool SeqStringWriteFileStream(Seq *seq, FILE *file);

/**
 * @brief Reads a file deserializing it into a Seq
 *
 * @return NULL on any error, empty sequence for empty file
 */
Seq *SeqStringReadFile(const char *file);

/**
 * @brief Create a sequence of strings from the serialized format
 *
 * @param[in] serialized The input string, contents are copied
 * @return A sequence of new allocated strings
 */
Seq *SeqStringDeserialize(const char *const serialized);

#endif // __STRING_SEQUENCE_H__
