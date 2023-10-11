#ifndef CF_VERSION_COMPARISON_H
#define CF_VERSION_COMPARISON_H

#include <stdbool.h> // bool

typedef enum VersionComparison
{
    VERSION_SMALLER,
    VERSION_EQUAL,
    VERSION_GREATER,
    VERSION_ERROR,
} VersionComparison;

typedef enum BooleanOrError
{
    BOOLEAN_ERROR = -1,
    BOOLEAN_FALSE = false,
    BOOLEAN_TRUE = true,
} BooleanOrError;

VersionComparison CompareVersion(const char *a, const char *b);

/**
  @brief Compare 2 version numbers using an operator.
  @note This function is just a wrapper around CompareVersion().
  @see CompareVersion()
  @param [in] a The first version number of the expression.
  @param [in] operator One of: [">", "<", "=", "==", "!=", ">=", "<="].
  @param [in] b The second version number of the expression.
  @return true or false or -1 (invalid operator or version numbers).
*/
BooleanOrError CompareVersionExpression(
    const char *a,
    const char *operator,
    const char *b);

#endif
