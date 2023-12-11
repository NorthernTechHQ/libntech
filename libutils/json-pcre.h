#ifndef CFENGINE_JSON_PCRE_H
#define CFENGINE_JSON_PCRE_H

// Parts of json.h which require PCRE2
// Should only be included inside a WITH_PCRE2 ifdef guard

#include <json.h>
#include <regex.h>

JsonElement *StringCaptureData(
    const Regex *regex, const char *pattern, const char *data);

#endif
