/*
  Copyright 2022 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; version 3.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#ifndef CFENGINE_HASH_H
#define CFENGINE_HASH_H

/**
  @brief Hash implementations
  */

#include <openssl/rsa.h>
#include <openssl/evp.h>

#include <stdbool.h>
#include <hash_method.h>                            /* HashMethod, HashSize */


typedef struct Hash Hash;

/**
  @brief Creates a new structure of type Hash.
  @param data String to hash.
  @param length Length of the string to hash.
  @param method Hash method.
  @return A structure of type Hash or NULL in case of error.
  */
Hash *HashNew(const char *data, const unsigned int length, HashMethod method);

/**
  @brief Creates a new structure of type Hash.
  @param descriptor Either file descriptor or socket descriptor.
  @param method Hash method.
  @return A structure of type Hash or NULL in case of error.
  */
Hash *HashNewFromDescriptor(const int descriptor, HashMethod method);

/**
  @brief Creates a new structure of type Hash.
  @param rsa RSA key to be hashed.
  @param method Hash method.
  @return A structure of type Hash or NULL in case of error.
  */
Hash *HashNewFromKey(const RSA *rsa, HashMethod method);

/**
  @brief Destroys a structure of type Hash.
  @param hash The structure to be destroyed.
  */
void HashDestroy(Hash **hash);

/**
  @brief Copy a hash
  @param origin Hash to be copied.
  @param destination Hash to be copied to.
  @return 0 if successful, -1 in any other case.
  */
int HashCopy(Hash *origin, Hash **destination);

/**
  @brief Checks if two hashes are equal.
  @param a 1st hash to be compared.
  @param b 2nd hash to be compared.
  @return True if both hashes are equal and false in any other case.
  */
bool HashEqual(const Hash *a, const Hash *b);

/**
  @brief Pointer to the raw digest data.
  @note Notice that this is a binary representation and not '\0' terminated.
  @param hash Hash structure.
  @param length Pointer to an unsigned int to hold the length of the data.
  @return A pointer to the raw digest data.
  */
const unsigned  char *HashData(const Hash *hash, unsigned int *length);

/**
  @brief Printable hash representation.
  @param hash Hash structure.
  @return A pointer to the printable digest representation.
  */
const char *HashPrintable(const Hash *hash);

/**
  @brief Hash type.
  @param hash Hash structure
  @return The hash method used by this hash structure.
  */
HashMethod HashType(const Hash *hash);

/**
  @brief Hash length in bytes.
  @param hash Hash structure
  @return The hash length in bytes.
  */
HashSize HashLength(const Hash *hash);

/**
  @brief Returns the ID of the hash based on the name
  @param hash_name Name of the hash.
  @return Returns the ID of the hash from the name.
  */
HashMethod HashIdFromName(const char *hash_name);

/**
  @brief Returns the name of the hash based on the ID.
  @param hash_id Id of the hash.
  @return Returns the name of the hash.
  */
const char *HashNameFromId(HashMethod hash_id);


/**
  @brief Returns pointer to an openssl digest struct

  Equivalent to EVP_get_digestbyname(HashNameFromId(type)),
  but with added error checking.

  Returns NULL in case of error.
  */
const EVP_MD *HashDigestFromId(HashMethod type);

/**
  @brief Size of the hash
  @param method Hash method
  @return Returns the size of the hash or 0 in case of error.
  */
HashSize HashSizeFromId(HashMethod hash_id);

/* Enough room for "SHA=asdfasdfasdf". */
#define CF_HOSTKEY_STRING_SIZE (4 + 2 * EVP_MAX_MD_SIZE + 1)


void HashFile(const char *filename, unsigned char digest[EVP_MAX_MD_SIZE + 1], HashMethod type, bool text_mode);
void HashString(const char *buffer, int len, unsigned char digest[EVP_MAX_MD_SIZE + 1], HashMethod type);
bool HashesMatch(
    const unsigned char digest1[EVP_MAX_MD_SIZE + 1],
    const unsigned char digest2[EVP_MAX_MD_SIZE + 1],
    HashMethod type);
char *HashPrintSafe(char *dst, size_t dst_size, const unsigned char *digest,
                    HashMethod type, bool use_prefix);
char *SkipHashType(char *hash);
void HashPubKey(const RSA *key, unsigned char digest[EVP_MAX_MD_SIZE + 1], HashMethod type);

/**
 * @brief Copy a string from src to dst, if src is too big, truncate and hash.
 *
 * If the src string (including NUL terminator) does not fit in dst
 * (according to dst_size), the last part of dst is a hash of the full src
 * string, before truncation.
 *
 * This function is primarily intended to limit the length of keys in a
 * key-value store, like LMDB, while still keeping the strings readable AND
 * unique.
 *
 * Examples:
 * "short_string" -> "short_string"
 * "string_which_is_too_long_for_size" -> "string_which_is#MD5="
 *
 * If this function returns dst_size, the string was truncated and hashed,
 * the destination string is exactly dst_size - 1 bytes long in this case.
 *
 * @param src[in] String to copy from, must be '\0'-terminated
 * @param dst[out] Destination to copy to, will always be '\0'-terminated
 * @param dst_size[in] Size of destination buffer (including '\0'-terminator)
 * @return dst_size if string was truncated, string length (src/dst) otherwise
 * @note dst must always be of size dst_size or bigger, regardless of src
 * @see StringCopy()
 */
size_t StringCopyTruncateAndHashIfNecessary(
    const char *src, char *dst, size_t dst_size);

#endif // CFENGINE_HASH_H
