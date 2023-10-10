#include <assert.h>     // assert()
#include <stdio.h>      // sscanf
#include <string_lib.h> // StringEqual()
#include <version_comparison.h>

VersionComparison CompareVersion(const char *a, const char *b)
{
    int a_major = 0;
    int a_minor = 0;
    int a_patch = 0;

    const int a_num = sscanf(a, "%d.%d.%d", &a_major, &a_minor, &a_patch);
    if (a_num < 1 || a_num > 3)
    {
        return VERSION_ERROR;
    }

    int b_major = 0;
    int b_minor = 0;
    int b_patch = 0;

    const int b_num = sscanf(b, "%d.%d.%d", &b_major, &b_minor, &b_patch);
    if (b_num < 1 || b_num > 3)
    {
        return VERSION_ERROR;
    }

    if (a_major > b_major)
    {
        return VERSION_GREATER;
    }

    if (a_major < b_major)
    {
        return VERSION_SMALLER;
    }

    assert(a_major == b_major);

    if (a_num == 1 || b_num == 1)
    {
        return VERSION_EQUAL;
    }

    assert(a_num >= 2 && b_num >= 2);

    if (a_minor < b_minor)
    {
        return VERSION_SMALLER;
    }

    if (a_minor > b_minor)
    {
        return VERSION_GREATER;
    }

    assert(a_minor == b_minor);

    if (a_num == 2 || b_num == 2)
    {
        return VERSION_EQUAL;
    }

    assert(a_num == 3 && b_num == 3);

    if (a_patch < b_patch)
    {
        return VERSION_SMALLER;
    }

    if (a_patch > b_patch)
    {
        return VERSION_GREATER;
    }

    assert(a_patch == b_patch);

    return VERSION_EQUAL;
}

BooleanOrError CompareVersionExpression(
    const char *const a, const char *const operator, const char * const b)
{
    const VersionComparison r = CompareVersion(a, b);
    if (r == VERSION_ERROR)
    {
        return BOOLEAN_ERROR;
    }
    if (StringEqual(operator, "=") || StringEqual(operator, "=="))
    {
        return (BooleanOrError) (r == VERSION_EQUAL);
    }
    if (StringEqual(operator, ">"))
    {
        return (BooleanOrError) (r == VERSION_GREATER);
    }
    if (StringEqual(operator, "<"))
    {
        return (BooleanOrError) (r == VERSION_SMALLER);
    }
    if (StringEqual(operator, ">="))
    {
        return (BooleanOrError) (r == VERSION_GREATER || r == VERSION_EQUAL);
    }
    if (StringEqual(operator, "<="))
    {
        return (BooleanOrError) (r == VERSION_SMALLER || r == VERSION_EQUAL);
    }
    if (StringEqual(operator, "!="))
    {
        return (BooleanOrError) (r != VERSION_EQUAL);
    }
    return BOOLEAN_ERROR;
}
