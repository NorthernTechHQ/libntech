#ifndef CFENGINE_JSON_PCRE_H
#define CFENGINE_JSON_PCRE_H

// Parts of json.h which require PCRE
// Should only be included inside a WITH_PCRE ifdef guard

#include <json.h>
#include <regex.h>

JsonElement *StringCaptureData(
    pcre *pattern, const char *regex, const char *data);

#endif
