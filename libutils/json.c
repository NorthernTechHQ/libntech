/*
  Copyright 2024 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <platform.h>
#include <logging.h>
#include <json.h>
#include <json-priv.h>
#include <json-yaml.h>

#include <alloc.h>
#include <sequence.h>
#include <stdint.h>
#include <inttypes.h>
#include <string_lib.h>
#include <misc_lib.h>
#include <file_lib.h>
#include <printsize.h>
#ifdef WITH_PCRE2
#include <regex.h>
#endif
#include <buffer.h>

static const int SPACES_PER_INDENT = 2;
const int DEFAULT_CONTAINER_CAPACITY = 64;

static const char *const JSON_TRUE = "true";
static const char *const JSON_FALSE = "false";
static const char *const JSON_NULL = "null";

struct JsonElement_
{
    JsonElementType type;

    // We don't have a separate struct for the key-value pairs in a JSON
    // Object. Instead, a JSON Object has a JsonElement Seq, where each element
    // has a propertyName (the key). A JSON Object key-value pair is sometimes
    // called a JSON Object property.
    char *propertyName;

    union
    {
        struct JsonContainer
        {
            JsonContainerType type;
            Seq *children;
        } container;
        struct JsonPrimitive
        {
            JsonPrimitiveType type;
            const char *value;
        } primitive;
    };
};

// *******************************************************************************************
// JsonElement Functions
// *******************************************************************************************

const char *JsonPrimitiveTypeToString(const JsonPrimitiveType type)
{
    switch (type)
    {
    case JSON_PRIMITIVE_TYPE_STRING:
        return "string";
    case JSON_PRIMITIVE_TYPE_REAL:
    case JSON_PRIMITIVE_TYPE_INTEGER:
        return "number";
    case JSON_PRIMITIVE_TYPE_BOOL:
        return "boolean";
    default:
        UnexpectedError("Unknown JSON primitive type: %d", type);
        return "(null)";
    }
}

static void JsonElementSetPropertyName(
    JsonElement *const element, const char *const propertyName)
{
    assert(element != NULL);

    if (element->propertyName != NULL)
    {
        free(element->propertyName);
        element->propertyName = NULL;
    }

    if (propertyName != NULL)
    {
        element->propertyName = xstrdup(propertyName);
    }
}

const char *JsonElementGetPropertyName(const JsonElement *const element)
{
    assert(element != NULL);

    return element->propertyName;
}

static JsonElement *JsonElementCreateContainer(
    const JsonContainerType containerType,
    const char *const propertyName,
    const size_t initialCapacity)
{
    JsonElement *element = xcalloc(1, sizeof(JsonElement));

    element->type = JSON_ELEMENT_TYPE_CONTAINER;

    JsonElementSetPropertyName(element, propertyName);

    element->container.type = containerType;
    element->container.children = SeqNew(initialCapacity, JsonDestroy);

    return element;
}

static JsonElement *JsonElementCreatePrimitive(
    JsonPrimitiveType primitiveType, const char *value)
{
    JsonElement *element = xcalloc(1, sizeof(JsonElement));

    element->type = JSON_ELEMENT_TYPE_PRIMITIVE;

    element->primitive.type = primitiveType;
    element->primitive.value = value;

    return element;
}

static JsonElement *JsonArrayCopy(const JsonElement *array)
{
    assert(array != NULL);
    assert(array->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(array->container.type == JSON_CONTAINER_TYPE_ARRAY);

    JsonElement *copy = JsonArrayCreate(JsonLength(array));

    JsonIterator iter = JsonIteratorInit(array);
    const JsonElement *child;
    while ((child = JsonIteratorNextValue(&iter)) != NULL)
    {
        JsonArrayAppendElement(copy, JsonCopy(child));
    }

    return copy;
}

static JsonElement *JsonObjectCopy(const JsonElement *const object)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(object->container.type == JSON_CONTAINER_TYPE_OBJECT);

    JsonElement *copy = JsonObjectCreate(JsonLength(object));

    JsonIterator iter = JsonIteratorInit(object);
    const JsonElement *child;
    while ((child = JsonIteratorNextValue(&iter)) != NULL)
    {
        JsonObjectAppendElement(
            copy, JsonIteratorCurrentKey(&iter), JsonCopy(child));
    }

    return copy;
}


static JsonElement *JsonContainerCopy(const JsonElement *const container)
{
    assert(container != NULL);
    assert(container->type == JSON_ELEMENT_TYPE_CONTAINER);

    const JsonContainerType type = container->container.type;
    switch (type)
    {
    case JSON_CONTAINER_TYPE_ARRAY:
        return JsonArrayCopy(container);

    case JSON_CONTAINER_TYPE_OBJECT:
        return JsonObjectCopy(container);

    default:
        UnexpectedError("Unknown JSON container type: %d", type);
        return NULL;
    }
}

static JsonElement *JsonPrimitiveCopy(const JsonElement *const primitive)
{
    assert(primitive != NULL);
    assert(primitive->type == JSON_ELEMENT_TYPE_PRIMITIVE);

    const JsonPrimitiveType type = primitive->primitive.type;

    switch (type)
    {
    case JSON_PRIMITIVE_TYPE_BOOL:
        return JsonBoolCreate(JsonPrimitiveGetAsBool(primitive));

    case JSON_PRIMITIVE_TYPE_INTEGER:
        return JsonIntegerCreate(JsonPrimitiveGetAsInteger(primitive));

    case JSON_PRIMITIVE_TYPE_NULL:
        return JsonNullCreate();

    case JSON_PRIMITIVE_TYPE_REAL:
        return JsonRealCreate(JsonPrimitiveGetAsReal(primitive));

    case JSON_PRIMITIVE_TYPE_STRING:
        return JsonStringCreate(JsonPrimitiveGetAsString(primitive));
    }

    UnexpectedError("Unknown JSON primitive type: %d", type);
    return NULL;
}

JsonElement *JsonCopy(const JsonElement *const element)
{
    assert(element != NULL);
    switch (element->type)
    {
    case JSON_ELEMENT_TYPE_CONTAINER:
        return JsonContainerCopy(element);
    case JSON_ELEMENT_TYPE_PRIMITIVE:
        return JsonPrimitiveCopy(element);
    }

    UnexpectedError("Unknown JSON element type: %d", element->type);
    return NULL;
}

static int JsonArrayCompare(
    const JsonElement *const a, const JsonElement *const b)
{
    assert(a != NULL);
    assert(b != NULL);
    assert(a->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(b->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(a->container.type == JSON_CONTAINER_TYPE_ARRAY);
    assert(b->container.type == JSON_CONTAINER_TYPE_ARRAY);

    int ret = JsonLength(a) - JsonLength(b);
    if (ret != 0)
    {
        return ret;
    }

    JsonIterator iter_a = JsonIteratorInit(a);
    JsonIterator iter_b = JsonIteratorInit(a);

    for (size_t i = 0; i < JsonLength(a); i++)
    {
        const JsonElement *child_a = JsonIteratorNextValue(&iter_a);
        const JsonElement *child_b = JsonIteratorNextValue(&iter_b);

        ret = JsonCompare(child_a, child_b);
        if (ret != 0)
        {
            return ret;
        }
    }

    return ret;
}

static int JsonObjectCompare(
    const JsonElement *const a, const JsonElement *const b)
{
    assert(a != NULL);
    assert(b != NULL);
    assert(a->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(b->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(a->container.type == JSON_CONTAINER_TYPE_OBJECT);
    assert(b->container.type == JSON_CONTAINER_TYPE_OBJECT);

    int ret = JsonLength(a) - JsonLength(b);
    if (ret != 0)
    {
        return ret;
    }

    JsonIterator iter_a = JsonIteratorInit(a);
    JsonIterator iter_b = JsonIteratorInit(a);

    for (size_t i = 0; i < JsonLength(a); i++)
    {
        const JsonElement *child_a = JsonIteratorNextValue(&iter_a);
        const JsonElement *child_b = JsonIteratorNextValue(&iter_b);

        const char *const key_a = JsonIteratorCurrentKey(&iter_a);
        const char *const key_b = JsonIteratorCurrentKey(&iter_b);

        ret = strcmp(key_a, key_b);
        if (ret != 0)
        {
            return ret;
        }

        ret = JsonCompare(child_a, child_b);
        if (ret != 0)
        {
            return ret;
        }
    }

    return ret;
}


static int JsonContainerCompare(
    const JsonElement *const a, const JsonElement *const b)
{
    assert(a != NULL);
    assert(b != NULL);
    assert(a->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(b->type == JSON_ELEMENT_TYPE_CONTAINER);

    const JsonContainerType type_a = a->container.type;
    const JsonContainerType type_b = b->container.type;

    if (type_a != type_b)
    {
        return type_a - type_b;
    }

    switch (type_a)
    {
    case JSON_CONTAINER_TYPE_ARRAY:
        return JsonArrayCompare(a, b);

    case JSON_CONTAINER_TYPE_OBJECT:
        return JsonObjectCompare(a, b);

    default:
        UnexpectedError("Unknown JSON container type: %d", type_a);
        return -1;
    }
}

int JsonCompare(const JsonElement *const a, const JsonElement *const b)
{
    assert(a != NULL);
    assert(b != NULL);

    const JsonElementType type_a = a->type;
    const JsonElementType type_b = b->type;

    if (type_a != type_b)
    {
        return type_a - type_b;
    }

    switch (type_a)
    {
    case JSON_ELEMENT_TYPE_CONTAINER:
        return JsonContainerCompare(a, b);

    case JSON_ELEMENT_TYPE_PRIMITIVE:
        return strcmp(a->primitive.value, b->primitive.value);

    default:
        UnexpectedError("Unknown JSON element type: %d", type_a);
        return -1;
    }
}


void JsonDestroy(JsonElement *const element)
{
    if (element != NULL)
    {
        switch (element->type)
        {
        case JSON_ELEMENT_TYPE_CONTAINER:
            assert(element->container.children);
            SeqDestroy(element->container.children);
            element->container.children = NULL;
            break;

        case JSON_ELEMENT_TYPE_PRIMITIVE:
            assert(element->primitive.value);

            if (element->primitive.type != JSON_PRIMITIVE_TYPE_NULL
                && element->primitive.type != JSON_PRIMITIVE_TYPE_BOOL)
            {
                free((void *) element->primitive.value);
            }
            element->primitive.value = NULL;
            break;

        default:
            UnexpectedError("Unknown JSON element type: %d", element->type);
        }

        if (element->propertyName != NULL)
        {
            free(element->propertyName);
        }

        free(element);
    }
}

void JsonDestroyMaybe(JsonElement *const element, const bool allocated)
{
    if (allocated)
    {
        JsonDestroy(element);
    }
}

JsonElement *JsonArrayMergeArray(
    const JsonElement *const a, const JsonElement *const b)
{
    assert(JsonGetElementType(a) == JsonGetElementType(b));
    assert(JsonGetElementType(a) == JSON_ELEMENT_TYPE_CONTAINER);
    assert(JsonGetContainerType(a) == JsonGetContainerType(b));
    assert(JsonGetContainerType(a) == JSON_CONTAINER_TYPE_ARRAY);

    JsonElement *result = JsonArrayCreate(JsonLength(a) + JsonLength(b));
    for (size_t i = 0; i < JsonLength(a); i++)
    {
        JsonArrayAppendElement(result, JsonCopy(JsonAt(a, i)));
    }

    for (size_t i = 0; i < JsonLength(b); i++)
    {
        JsonArrayAppendElement(result, JsonCopy(JsonAt(b, i)));
    }

    return result;
}

JsonElement *JsonObjectMergeArray(
    const JsonElement *const a, const JsonElement *const b)
{
    assert(JsonGetElementType(a) == JsonGetElementType(b));
    assert(JsonGetElementType(a) == JSON_ELEMENT_TYPE_CONTAINER);
    assert(JsonGetContainerType(a) == JSON_CONTAINER_TYPE_OBJECT);
    assert(JsonGetContainerType(b) == JSON_CONTAINER_TYPE_ARRAY);

    JsonElement *result = JsonObjectCopy(a);
    for (size_t i = 0; i < JsonLength(b); i++)
    {
        char *key = StringFromLong(i);
        JsonObjectAppendElement(result, key, JsonCopy(JsonAt(b, i)));
        free(key);
    }

    return result;
}

JsonElement *JsonObjectMergeObject(
    const JsonElement *const a, const JsonElement *const b)
{
    assert(JsonGetElementType(a) == JsonGetElementType(b));
    assert(JsonGetElementType(a) == JSON_ELEMENT_TYPE_CONTAINER);
    assert(JsonGetContainerType(a) == JSON_CONTAINER_TYPE_OBJECT);
    assert(JsonGetContainerType(b) == JSON_CONTAINER_TYPE_OBJECT);

    JsonElement *result = JsonObjectCopy(a);
    JsonIterator iter = JsonIteratorInit(b);
    const char *key;
    while ((key = JsonIteratorNextKey(&iter)))
    {
        JsonObjectAppendElement(
            result, key, JsonCopy(JsonIteratorCurrentValue(&iter)));
    }

    return result;
}

JsonElement *JsonMerge(const JsonElement *const a, const JsonElement *const b)
{
    assert(JsonGetElementType(a) == JsonGetElementType(b));
    assert(JsonGetElementType(a) == JSON_ELEMENT_TYPE_CONTAINER);

    switch (JsonGetContainerType(a))
    {
    case JSON_CONTAINER_TYPE_ARRAY:
        switch (JsonGetContainerType(b))
        {
        case JSON_CONTAINER_TYPE_OBJECT:
            return JsonObjectMergeArray(b, a);
        case JSON_CONTAINER_TYPE_ARRAY:
            return JsonArrayMergeArray(a, b);
        }
        UnexpectedError(
            "Unknown JSON container type: %d", JsonGetContainerType(b));
        break;

    case JSON_CONTAINER_TYPE_OBJECT:
        switch (JsonGetContainerType(b))
        {
        case JSON_CONTAINER_TYPE_OBJECT:
            return JsonObjectMergeObject(a, b);
        case JSON_CONTAINER_TYPE_ARRAY:
            return JsonObjectMergeArray(a, b);
        }
        UnexpectedError(
            "Unknown JSON container type: %d", JsonGetContainerType(b));
        break;

    default:
        UnexpectedError(
            "Unknown JSON container type: %d", JsonGetContainerType(a));
    }

    return NULL;
}

JsonElement *JsonObjectMergeDeepInplace(JsonElement *const base, const JsonElement *const extra)
{
    assert(base != NULL);
    assert(extra != NULL);

    assert(JsonGetType(base) == JSON_TYPE_OBJECT);
    assert(JsonGetType(extra) == JSON_TYPE_OBJECT);

    JsonIterator iter = JsonIteratorInit(extra);
    while (JsonIteratorHasMore(&iter))
    {
        const char *const key = JsonIteratorNextKey(&iter);
        assert(key != NULL);

        const JsonElement *const extra_value = JsonIteratorCurrentValue(&iter);
        assert(key != NULL);

        JsonElement *const base_value = JsonObjectGet(base, key);
        if (base_value == NULL)
        {
            /* Key is unique, copy element into base */
            JsonElement *const element = JsonCopy(extra_value);
            assert(element != NULL);
            JsonObjectAppendElement(base, key, element);
            continue;
        }

        const JsonType base_type = JsonGetType(base_value);
        const JsonType extra_type = JsonGetType(extra_value);

        if (base_type == JSON_TYPE_OBJECT && extra_type == JSON_TYPE_OBJECT)
        {
            /* Both are objects, recursively merge them */
            JsonObjectMergeDeepInplace(base_value, extra_value);
        }
        else if (base_type == JSON_TYPE_ARRAY && extra_type == JSON_TYPE_ARRAY)
        {
            /* Both are arrays, concatenate them into base */
            JsonElement *const element = JsonCopy(extra_value);
            assert(element != NULL);
            JsonArrayExtend(base_value, element);
        }
        else
        {
            /* Otherwise, overwrite value in base */
            JsonElement *const element = JsonCopy(extra_value);
            assert(element != NULL);
            JsonObjectAppendElement(base, key, element);
        }
    }

    return base;
}

size_t JsonLength(const JsonElement *const element)
{
    assert(element != NULL);

    switch (element->type)
    {
    case JSON_ELEMENT_TYPE_CONTAINER:
        return SeqLength(element->container.children);

    case JSON_ELEMENT_TYPE_PRIMITIVE:
        return strlen(element->primitive.value);

    default:
        UnexpectedError("Unknown JSON element type: %d", element->type);
        return (size_t) -1; // appease gcc
    }
}

JsonIterator JsonIteratorInit(const JsonElement *const container)
{
    assert(container != NULL);
    assert(container->type == JSON_ELEMENT_TYPE_CONTAINER);

    return (JsonIterator){container, 0};
}

const char *JsonIteratorNextKey(JsonIterator *const iter)
{
    assert(iter != NULL);
    assert(iter->container != NULL);
    assert(iter->container->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(iter->container->container.type == JSON_CONTAINER_TYPE_OBJECT);

    const JsonElement *child = JsonIteratorNextValue(iter);
    return child ? child->propertyName : NULL;
}

JsonElement *JsonIteratorNextValue(JsonIterator *const iter)
{
    assert(iter != NULL);
    assert(iter->container != NULL);
    assert(iter->container->type == JSON_ELEMENT_TYPE_CONTAINER);

    if (iter->index >= JsonLength(iter->container))
    {
        return NULL;
    }

    Seq *const children = iter->container->container.children;
    return SeqAt(children, iter->index++);
}

JsonElement *JsonIteratorNextValueByType(
    JsonIterator *const iter, const JsonElementType type, const bool skip_null)
{
    JsonElement *e = NULL;
    while ((e = JsonIteratorNextValue(iter)))
    {
        if (skip_null && JsonGetType(e) == JSON_TYPE_NULL)
        {
            continue;
        }

        if (e->type == type)
        {
            return e;
        }
    }

    return NULL;
}

JsonElement *JsonIteratorCurrentValue(const JsonIterator *const iter)
{
    assert(iter != NULL);
    assert(iter->container != NULL);
    assert(iter->container->type == JSON_ELEMENT_TYPE_CONTAINER);

    if (iter->index == 0 || iter->index > JsonLength(iter->container))
    {
        return NULL;
    }

    Seq *const children = iter->container->container.children;
    return SeqAt(children, iter->index - 1);
}

const char *JsonIteratorCurrentKey(const JsonIterator *const iter)
{
    assert(iter != NULL);
    assert(iter->container != NULL);
    assert(iter->container->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(iter->container->container.type == JSON_CONTAINER_TYPE_OBJECT);

    const JsonElement *child = JsonIteratorCurrentValue(iter);

    return child ? child->propertyName : NULL;
}

JsonElementType JsonIteratorCurrentElementType(const JsonIterator *const iter)
{
    assert(iter != NULL);

    const JsonElement *child = JsonIteratorCurrentValue(iter);
    return child->type;
}

JsonContainerType JsonIteratorCurrentContainerType(
    const JsonIterator *const iter)
{
    assert(iter != NULL);

    const JsonElement *child = JsonIteratorCurrentValue(iter);
    assert(child->type == JSON_ELEMENT_TYPE_CONTAINER);

    return child->container.type;
}

JsonPrimitiveType JsonIteratorCurrentPrimitiveType(
    const JsonIterator *const iter)
{
    assert(iter != NULL);

    const JsonElement *child = JsonIteratorCurrentValue(iter);
    assert(child->type == JSON_ELEMENT_TYPE_PRIMITIVE);

    return child->primitive.type;
}

bool JsonIteratorHasMore(const JsonIterator *const iter)
{
    assert(iter != NULL);

    return iter->index < JsonLength(iter->container);
}

JsonElementType JsonGetElementType(const JsonElement *const element)
{
    assert(element != NULL);

    return element->type;
}

JsonType JsonGetType(const JsonElement *element)
{
    if (JsonGetElementType(element) == JSON_ELEMENT_TYPE_CONTAINER)
    {
        return (JsonType) JsonGetContainerType(element);
    }

    assert(JsonGetElementType(element) == JSON_ELEMENT_TYPE_PRIMITIVE);
    return (JsonType) JsonGetPrimitiveType(element);
}

JsonContainerType JsonGetContainerType(const JsonElement *const container)
{
    assert(container != NULL);
    assert(container->type == JSON_ELEMENT_TYPE_CONTAINER);

    return container->container.type;
}

JsonPrimitiveType JsonGetPrimitiveType(const JsonElement *const primitive)
{
    assert(primitive != NULL);
    assert(primitive->type == JSON_ELEMENT_TYPE_PRIMITIVE);

    return primitive->primitive.type;
}

const char *JsonPrimitiveGetAsString(const JsonElement *const primitive)
{
    assert(primitive != NULL);
    assert(primitive->type == JSON_ELEMENT_TYPE_PRIMITIVE);

    return primitive->primitive.value;
}

char *JsonPrimitiveToString(const JsonElement *const primitive)
{
    if (JsonGetElementType(primitive) != JSON_ELEMENT_TYPE_PRIMITIVE)
    {
        return NULL;
    }

    switch (JsonGetPrimitiveType(primitive))
    {
    case JSON_PRIMITIVE_TYPE_BOOL:
        return xstrdup(JsonPrimitiveGetAsBool(primitive) ? "true" : "false");
        break;

    case JSON_PRIMITIVE_TYPE_INTEGER:
        return StringFromLong(JsonPrimitiveGetAsInteger(primitive));
        break;

    case JSON_PRIMITIVE_TYPE_REAL:
        return StringFromDouble(JsonPrimitiveGetAsReal(primitive));
        break;

    case JSON_PRIMITIVE_TYPE_STRING:
        return xstrdup(JsonPrimitiveGetAsString(primitive));
        break;

    case JSON_PRIMITIVE_TYPE_NULL: // redundant
        break;
    }

    return NULL;
}

bool JsonPrimitiveGetAsBool(const JsonElement *const primitive)
{
    assert(primitive != NULL);
    assert(primitive->type == JSON_ELEMENT_TYPE_PRIMITIVE);
    assert(primitive->primitive.type == JSON_PRIMITIVE_TYPE_BOOL);

    return StringEqual(JSON_TRUE, primitive->primitive.value);
}

long JsonPrimitiveGetAsInteger(const JsonElement *const primitive)
{
    assert(primitive != NULL);
    assert(primitive->type == JSON_ELEMENT_TYPE_PRIMITIVE);
    assert(primitive->primitive.type == JSON_PRIMITIVE_TYPE_INTEGER);

    return StringToLongExitOnError(primitive->primitive.value);
}


int JsonPrimitiveGetAsInt64(const JsonElement *primitive, int64_t *value_out)
{
    assert(primitive != NULL);
    assert(primitive->type == JSON_ELEMENT_TYPE_PRIMITIVE);
    assert(primitive->primitive.type == JSON_PRIMITIVE_TYPE_INTEGER);

    return StringToInt64(primitive->primitive.value, value_out);
}

int64_t JsonPrimitiveGetAsInt64DefaultOnError(const JsonElement *primitive, int64_t default_return)
{
    assert(primitive != NULL);
    assert(primitive->type == JSON_ELEMENT_TYPE_PRIMITIVE);
    assert(primitive->primitive.type == JSON_PRIMITIVE_TYPE_INTEGER);

    return StringToInt64DefaultOnError(primitive->primitive.value, default_return);
}

int64_t JsonPrimitiveGetAsInt64ExitOnError(const JsonElement *primitive)
{
    assert(primitive != NULL);
    assert(primitive->type == JSON_ELEMENT_TYPE_PRIMITIVE);
    assert(primitive->primitive.type == JSON_PRIMITIVE_TYPE_INTEGER);

    return StringToInt64ExitOnError(primitive->primitive.value);
}

double JsonPrimitiveGetAsReal(const JsonElement *const primitive)
{
    assert(primitive != NULL);
    assert(primitive->type == JSON_ELEMENT_TYPE_PRIMITIVE);
    assert(primitive->primitive.type == JSON_PRIMITIVE_TYPE_REAL);

    return StringToDouble(primitive->primitive.value);
}

const char *JsonGetPropertyAsString(const JsonElement *const element)
{
    assert(element != NULL);

    return element->propertyName;
}

void JsonSort(
    const JsonElement *const container,
    JsonComparator *const Compare,
    void *const user_data)
{
    assert(container != NULL);
    assert(container->type == JSON_ELEMENT_TYPE_CONTAINER);

    Seq *const children = container->container.children;
    SeqSort(children, (SeqItemComparator) Compare, user_data);
}

JsonElement *JsonAt(const JsonElement *container, const size_t index)
{
    assert(container != NULL);
    assert(container->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(index < JsonLength(container));

    return SeqAt(container->container.children, index);
}

JsonElement *JsonSelect(
    JsonElement *const element, const size_t num_indices, char **const indices)
{
    if (num_indices == 0)
    {
        return element;
    }
    else
    {
        if (JsonGetElementType(element) != JSON_ELEMENT_TYPE_CONTAINER)
        {
            return NULL;
        }

        const char *index = indices[0];

        switch (JsonGetContainerType(element))
        {
        case JSON_CONTAINER_TYPE_OBJECT:
        {
            JsonElement *child = JsonObjectGet(element, index);
            if (child != NULL)
            {
                return JsonSelect(child, num_indices - 1, indices + 1);
            }
        }
            return NULL;

        case JSON_CONTAINER_TYPE_ARRAY:
            if (StringIsNumeric(index))
            {
                size_t i = StringToLongExitOnError(index);
                if (i < JsonLength(element))
                {
                    JsonElement *child = JsonArrayGet(element, i);
                    if (child != NULL)
                    {
                        return JsonSelect(child, num_indices - 1, indices + 1);
                    }
                }
            }
            break;

        default:
            UnexpectedError(
                "Unknown JSON container type: %d",
                JsonGetContainerType(element));
        }
        return NULL;
    }
}

// *******************************************************************************************
// JsonObject Functions
// *******************************************************************************************

JsonElement *JsonObjectCreate(const size_t initialCapacity)
{
    return JsonElementCreateContainer(
        JSON_CONTAINER_TYPE_OBJECT, NULL, initialCapacity);
}

void JsonEncodeStringWriter(
    const char *const unescaped_string, Writer *const writer)
{
    assert(unescaped_string != NULL);

    for (const char *c = unescaped_string; *c != '\0'; c++)
    {
        switch (*c)
        {
        case '\"':
        case '\\':
            WriterWriteChar(writer, '\\');
            WriterWriteChar(writer, *c);
            break;
        case '\b':
            WriterWriteChar(writer, '\\');
            WriterWriteChar(writer, 'b');
            break;
        case '\f':
            WriterWriteChar(writer, '\\');
            WriterWriteChar(writer, 'f');
            break;
        case '\n':
            WriterWriteChar(writer, '\\');
            WriterWriteChar(writer, 'n');
            break;
        case '\r':
            WriterWriteChar(writer, '\\');
            WriterWriteChar(writer, 'r');
            break;
        case '\t':
            WriterWriteChar(writer, '\\');
            WriterWriteChar(writer, 't');
            break;
        default:
            WriterWriteChar(writer, *c);
        }
    }
}

char *JsonEncodeString(const char *const unescaped_string)
{
    Writer *writer = StringWriter();

    JsonEncodeStringWriter(unescaped_string, writer);

    return StringWriterClose(writer);
}

static void JsonDecodeStringWriter(
    const char *const escaped_string, Writer *const w)
{
    assert(escaped_string != NULL);

    for (const char *c = escaped_string; *c != '\0'; c++)
    {
        if (*c == '\\')
        {
            switch (c[1])
            {
            case '\"':
            case '\\':
                WriterWriteChar(w, c[1]);
                c++;
                break;
            case 'b':
                WriterWriteChar(w, '\b');
                c++;
                break;
            case 'f':
                WriterWriteChar(w, '\f');
                c++;
                break;
            case 'n':
                WriterWriteChar(w, '\n');
                c++;
                break;
            case 'r':
                WriterWriteChar(w, '\r');
                c++;
                break;
            case 't':
                WriterWriteChar(w, '\t');
                c++;
                break;
            default:
                WriterWriteChar(w, *c);
                break;
            }
        }
        else
        {
            WriterWriteChar(w, *c);
        }
    }
}

char *JsonDecodeString(const char *const escaped_string)
{
    Writer *writer = StringWriter();

    JsonDecodeStringWriter(escaped_string, writer);

    return StringWriterClose(writer);
}

void Json5EscapeDataWriter(const Slice unescaped_data, Writer *const writer)
{
    // See: https://spec.json5.org/#strings

    const char *const data = unescaped_data.data;
    assert(data != NULL);

    const size_t size = unescaped_data.size;

    for (size_t index = 0; index < size; index++)
    {
        const char byte = data[index];
        switch (byte)
        {
        case '\0':
            WriterWrite(writer, "\\0");
            break;
        case '\"':
        case '\\':
            WriterWriteChar(writer, '\\');
            WriterWriteChar(writer, byte);
            break;
        case '\b':
            WriterWrite(writer, "\\b");
            break;
        case '\f':
            WriterWrite(writer, "\\f");
            break;
        case '\n':
            WriterWrite(writer, "\\n");
            break;
        case '\r':
            WriterWrite(writer, "\\r");
            break;
        case '\t':
            WriterWrite(writer, "\\t");
            break;
        default:
        {
            if (CharIsPrintableAscii(byte))
            {
                WriterWriteChar(writer, byte);
            }
            else
            {
                // unsigned char behaves better when implicitly cast to int:
                WriterWriteF(writer, "\\x%2.2X", (unsigned char) byte);
            }
            break;
        }
        }
    }
}

char *Json5EscapeData(Slice unescaped_data)
{
    Writer *writer = StringWriter();

    Json5EscapeDataWriter(unescaped_data, writer);

    return StringWriterClose(writer);
}

void JsonObjectAppendString(
    JsonElement *const object, const char *const key, const char *const value)
{
    JsonElement *child = JsonStringCreate(value);
    JsonObjectAppendElement(object, key, child);
}

void JsonObjectAppendInteger(
    JsonElement *const object, const char *const key, const int value)
{
    JsonElement *child = JsonIntegerCreate(value);
    JsonObjectAppendElement(object, key, child);
}

void JsonObjectAppendInteger64(
    JsonElement *const object, const char *const key, const int64_t value)
{
    JsonElement *child = JsonIntegerCreate64(value);
    JsonObjectAppendElement(object, key, child);
}

void JsonObjectAppendBool(
    JsonElement *const object, const char *const key, const _Bool value)
{
    JsonElement *child = JsonBoolCreate(value);
    JsonObjectAppendElement(object, key, child);
}

void JsonObjectAppendReal(
    JsonElement *const object, const char *const key, const double value)
{
    JsonElement *child = JsonRealCreate(value);
    JsonObjectAppendElement(object, key, child);
}

void JsonObjectAppendNull(JsonElement *const object, const char *const key)
{
    JsonElement *child = JsonNullCreate();
    JsonObjectAppendElement(object, key, child);
}

void JsonObjectAppendArray(
    JsonElement *const object, const char *const key, JsonElement *const array)
{
    assert(array != NULL);
    assert(array->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(array->container.type == JSON_CONTAINER_TYPE_ARRAY);

    JsonObjectAppendElement(object, key, array);
}

void JsonObjectAppendObject(
    JsonElement *const object,
    const char *const key,
    JsonElement *const childObject)
{
    assert(childObject != NULL);
    assert(childObject->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(childObject->container.type == JSON_CONTAINER_TYPE_OBJECT);

    JsonObjectAppendElement(object, key, childObject);
}

void JsonObjectAppendElement(
    JsonElement *const object,
    const char *const key,
    JsonElement *const element)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(object->container.type == JSON_CONTAINER_TYPE_OBJECT);
    assert(key != NULL);
    assert(element != NULL);

    JsonObjectRemoveKey(object, key);

    JsonElementSetPropertyName(element, key);
    SeqAppend(object->container.children, element);
}

static int JsonElementHasProperty(
    const void *const propertyName,
    const void *const jsonElement,
    ARG_UNUSED void *const user_data)
{
    assert(propertyName != NULL);

    const JsonElement *element = jsonElement;

    assert(element->propertyName != NULL);

    if (strcmp(propertyName, element->propertyName) == 0)
    {
        return 0;
    }
    return -1;
}

static int CompareKeyToPropertyName(
    const void *const a, const void *const b, ARG_UNUSED void *const user_data)
{
    assert(b != NULL);
    const char *const key_a = a;
    const JsonElement *const json_b = b;
    return StringSafeCompare(key_a, json_b->propertyName);
}

static ssize_t JsonElementIndexInParentObject(
    JsonElement *const parent, const char *const key)
{
    assert(parent != NULL);
    assert(parent->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(parent->container.type == JSON_CONTAINER_TYPE_OBJECT);
    assert(key != NULL);

    Seq *const children = parent->container.children;
    return SeqIndexOf(children, key, CompareKeyToPropertyName);
}

bool JsonObjectRemoveKey(JsonElement *const object, const char *const key)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(object->container.type == JSON_CONTAINER_TYPE_OBJECT);
    assert(key != NULL);

    const ssize_t index = JsonElementIndexInParentObject(object, key);
    if (index != -1)
    {
        SeqRemove(object->container.children, index);
        return true;
    }
    return false;
}

JsonElement *JsonObjectDetachKey(
    JsonElement *const object, const char *const key)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(object->container.type == JSON_CONTAINER_TYPE_OBJECT);
    assert(key != NULL);

    JsonElement *detached = NULL;

    ssize_t index = JsonElementIndexInParentObject(object, key);
    if (index != -1)
    {
        Seq *const children = object->container.children;
        detached = SeqLookup(children, key, JsonElementHasProperty);
        SeqSoftRemove(children, index);
    }

    return detached;
}

const char *JsonObjectGetAsString(
    const JsonElement *const object, const char *const key)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(object->container.type == JSON_CONTAINER_TYPE_OBJECT);
    assert(key != NULL);

    JsonElement *childPrimitive =
        SeqLookup(object->container.children, key, JsonElementHasProperty);

    if (childPrimitive != NULL)
    {
        assert(childPrimitive->type == JSON_ELEMENT_TYPE_PRIMITIVE);
        return childPrimitive->primitive.value;
    }

    return NULL;
}

bool JsonObjectGetAsBool(
    const JsonElement *const object, const char *key)
{
    assert(object != NULL);
    assert(JsonGetType(object) == JSON_TYPE_OBJECT);
    assert(key != NULL);

    JsonElement *childPrimitive = JsonObjectGet(object, key);

    if (childPrimitive != NULL)
    {
        assert(JsonGetType(childPrimitive) == JSON_TYPE_BOOL);
        return StringEqual(childPrimitive->primitive.value, "true");
    }

    return false;
}

JsonElement *JsonObjectGetAsObject(
    JsonElement *const object, const char *const key)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(object->container.type == JSON_CONTAINER_TYPE_OBJECT);
    assert(key != NULL);

    JsonElement *childPrimitive =
        SeqLookup(object->container.children, key, JsonElementHasProperty);

    if (childPrimitive != NULL)
    {
        assert(childPrimitive->type == JSON_ELEMENT_TYPE_CONTAINER);
        assert(childPrimitive->container.type == JSON_CONTAINER_TYPE_OBJECT);
        return childPrimitive;
    }

    return NULL;
}

JsonElement *JsonObjectGetAsArray(
    JsonElement *const object, const char *const key)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(object->container.type == JSON_CONTAINER_TYPE_OBJECT);
    assert(key != NULL);

    JsonElement *childPrimitive =
        SeqLookup(object->container.children, key, JsonElementHasProperty);

    if (childPrimitive != NULL)
    {
        assert(childPrimitive->type == JSON_ELEMENT_TYPE_CONTAINER);
        assert(childPrimitive->container.type == JSON_CONTAINER_TYPE_ARRAY);
        return childPrimitive;
    }

    return NULL;
}

JsonElement *JsonObjectGet(
    const JsonElement *const object, const char *const key)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(object->container.type == JSON_CONTAINER_TYPE_OBJECT);
    assert(key != NULL);

    return SeqLookup(object->container.children, key, JsonElementHasProperty);
}

// *******************************************************************************************
// JsonArray Functions
// *******************************************************************************************

JsonElement *JsonArrayCreate(const size_t initialCapacity)
{
    return JsonElementCreateContainer(
        JSON_CONTAINER_TYPE_ARRAY, NULL, initialCapacity);
}

void JsonArrayAppendString(JsonElement *const array, const char *const value)
{
    JsonElement *child = JsonStringCreate(value);
    JsonArrayAppendElement(array, child);
}

void JsonArrayAppendBool(JsonElement *const array, const bool value)
{
    JsonElement *child = JsonBoolCreate(value);
    JsonArrayAppendElement(array, child);
}

void JsonArrayAppendInteger(JsonElement *const array, const int value)
{
    JsonElement *child = JsonIntegerCreate(value);
    JsonArrayAppendElement(array, child);
}

void JsonArrayAppendReal(JsonElement *const array, const double value)
{
    JsonElement *child = JsonRealCreate(value);
    JsonArrayAppendElement(array, child);
}

void JsonArrayAppendNull(JsonElement *const array)
{
    JsonElement *child = JsonNullCreate();
    JsonArrayAppendElement(array, child);
}

void JsonArrayAppendArray(
    JsonElement *const array, JsonElement *const childArray)
{
    assert(childArray != NULL);
    assert(childArray->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(childArray->container.type == JSON_CONTAINER_TYPE_ARRAY);

    JsonArrayAppendElement(array, childArray);
}

void JsonArrayAppendObject(JsonElement *const array, JsonElement *const object)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(object->container.type == JSON_CONTAINER_TYPE_OBJECT);

    JsonArrayAppendElement(array, object);
}

void JsonArrayAppendElement(
    JsonElement *const array, JsonElement *const element)
{
    assert(array != NULL);
    assert(array->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(array->container.type == JSON_CONTAINER_TYPE_ARRAY);
    assert(element != NULL);

    SeqAppend(array->container.children, element);
}

void JsonArrayExtend(JsonElement *a, JsonElement *b)
{
    assert(a != NULL);
    assert(a->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(a->container.type == JSON_CONTAINER_TYPE_ARRAY);
    assert(b != NULL);
    assert(b->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(b->container.type == JSON_CONTAINER_TYPE_ARRAY);

    SeqAppendSeq(a->container.children, b->container.children);
    SeqSoftDestroy(b->container.children);
    if (b->propertyName != NULL)
    {
        free(b->propertyName);
    }
    free(b);
}

void JsonArrayRemoveRange(
    JsonElement *const array, const size_t start, const size_t end)
{
    assert(array != NULL);
    assert(array->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(array->container.type == JSON_CONTAINER_TYPE_ARRAY);
    assert(end < SeqLength(array->container.children));
    assert(start <= end);

    SeqRemoveRange(array->container.children, start, end);
}

const char *JsonArrayGetAsString(JsonElement *const array, const size_t index)
{
    assert(array != NULL);
    assert(array->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(array->container.type == JSON_CONTAINER_TYPE_ARRAY);
    assert(index < SeqLength(array->container.children));

    JsonElement *childPrimitive = SeqAt(array->container.children, index);

    if (childPrimitive != NULL)
    {
        assert(childPrimitive->type == JSON_ELEMENT_TYPE_PRIMITIVE);
        assert(childPrimitive->primitive.type == JSON_PRIMITIVE_TYPE_STRING);
        return childPrimitive->primitive.value;
    }

    return NULL;
}

JsonElement *JsonArrayGetAsObject(JsonElement *const array, const size_t index)
{
    assert(array != NULL);
    assert(array->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(array->container.type == JSON_CONTAINER_TYPE_ARRAY);
    assert(index < SeqLength(array->container.children));

    JsonElement *child = SeqAt(array->container.children, index);

    if (child != NULL)
    {
        assert(child->type == JSON_ELEMENT_TYPE_CONTAINER);
        assert(child->container.type == JSON_CONTAINER_TYPE_OBJECT);
        return child;
    }

    return NULL;
}

JsonElement *JsonArrayGet(const JsonElement *const array, const size_t index)
{
    assert(array != NULL);
    assert(array->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(array->container.type == JSON_CONTAINER_TYPE_ARRAY);

    return JsonAt(array, index);
}

bool JsonArrayContainsOnlyPrimitives(JsonElement *const array)
{
    assert(array != NULL);
    assert(array->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(array->container.type == JSON_CONTAINER_TYPE_ARRAY);


    for (size_t i = 0; i < JsonLength(array); i++)
    {
        JsonElement *child = JsonArrayGet(array, i);

        if (child->type != JSON_ELEMENT_TYPE_PRIMITIVE)
        {
            return false;
        }
    }

    return true;
}

void JsonContainerReverse(JsonElement *const array)
{
    assert(array != NULL);
    assert(array->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(array->container.type == JSON_CONTAINER_TYPE_ARRAY);

    SeqReverse(array->container.children);
}

// *******************************************************************************************
// Primitive Functions
// *******************************************************************************************

JsonElement *JsonStringCreate(const char *const value)
{
    assert(value != NULL);

    return JsonElementCreatePrimitive(
        JSON_PRIMITIVE_TYPE_STRING, xstrdup(value));
}

JsonElement *JsonIntegerCreate(const int value)
{
    char *buffer;
    xasprintf(&buffer, "%d", value);

    return JsonElementCreatePrimitive(JSON_PRIMITIVE_TYPE_INTEGER, buffer);
}

JsonElement *JsonIntegerCreate64(const int64_t value)
{
    char *buffer;
    xasprintf(&buffer, "%" PRIi64, value);

    return JsonElementCreatePrimitive(JSON_PRIMITIVE_TYPE_INTEGER, buffer);
}

JsonElement *JsonRealCreate(double value)
{
    if (isnan(value) || !isfinite(value))
    {
        value = 0.0;
    }

    char *buffer = xcalloc(32, sizeof(char));
    snprintf(buffer, 32, "%.4f", value);

    return JsonElementCreatePrimitive(JSON_PRIMITIVE_TYPE_REAL, buffer);
}

JsonElement *JsonBoolCreate(const bool value)
{
    const char *const as_string = value ? JSON_TRUE : JSON_FALSE;
    return JsonElementCreatePrimitive(JSON_PRIMITIVE_TYPE_BOOL, as_string);
}

JsonElement *JsonNullCreate()
{
    return JsonElementCreatePrimitive(JSON_PRIMITIVE_TYPE_NULL, JSON_NULL);
}

// *******************************************************************************************
// Printing
// *******************************************************************************************

static void JsonContainerWrite(
    Writer *writer, const JsonElement *containerElement, size_t indent_level);
static void JsonContainerWriteCompact(
    Writer *writer, const JsonElement *containerElement);

static bool IsWhitespace(const char ch)
{
    return (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
}

static bool IsSeparator(const char ch)
{
    return IsWhitespace(ch) || ch == ',' || ch == ']' || ch == '}';
}

static bool IsDigit(const char ch)
{
    // [1,9]
    return (ch >= 49 && ch <= 57);
}

static void PrintIndent(Writer *const writer, const int num)
{
    int i = 0;

    for (i = 0; i < num * SPACES_PER_INDENT; i++)
    {
        WriterWriteChar(writer, ' ');
    }
}

static void JsonPrimitiveWrite(
    Writer *const writer,
    const JsonElement *const primitiveElement,
    const size_t indent_level)
{
    assert(primitiveElement != NULL);
    assert(primitiveElement->type == JSON_ELEMENT_TYPE_PRIMITIVE);

    const char *const value = primitiveElement->primitive.value;

    if (primitiveElement->primitive.type == JSON_PRIMITIVE_TYPE_STRING)
    {
        PrintIndent(writer, indent_level);
        {
            char *encoded = JsonEncodeString(value);
            WriterWriteF(writer, "\"%s\"", encoded);
            free(encoded);
        }
    }
    else
    {
        PrintIndent(writer, indent_level);
        WriterWrite(writer, value);
    }
}

static void JsonArrayWrite(
    Writer *const writer,
    const JsonElement *const array,
    const size_t indent_level)
{
    assert(array != NULL);
    assert(array->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(array->container.type == JSON_CONTAINER_TYPE_ARRAY);
    assert(array->container.children != NULL);

    if (JsonLength(array) == 0)
    {
        WriterWrite(writer, "[]");
        return;
    }

    WriterWrite(writer, "[\n");

    Seq *const children = array->container.children;
    const size_t length = SeqLength(children);
    for (size_t i = 0; i < length; i++)
    {
        JsonElement *const child = SeqAt(children, i);

        switch (child->type)
        {
        case JSON_ELEMENT_TYPE_PRIMITIVE:
            JsonPrimitiveWrite(writer, child, indent_level + 1);
            break;

        case JSON_ELEMENT_TYPE_CONTAINER:
            PrintIndent(writer, indent_level + 1);
            JsonContainerWrite(writer, child, indent_level + 1);
            break;

        default:
            UnexpectedError("Unknown JSON element type: %d", child->type);
        }

        if (i < length - 1)
        {
            WriterWrite(writer, ",\n");
        }
        else
        {
            WriterWrite(writer, "\n");
        }
    }

    PrintIndent(writer, indent_level);
    WriterWriteChar(writer, ']');
}

int JsonElementPropertyCompare(
    const void *const e1,
    const void *const e2,
    ARG_UNUSED void *const user_data)
{
    assert(e1 != NULL);
    assert(e2 != NULL);

    return strcmp(
        ((JsonElement *) e1)->propertyName,
        ((JsonElement *) e2)->propertyName);
}

#ifndef NDEBUG // gcc would complain about unused function

static bool all_children_have_keys(const JsonElement *const object)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);

    Seq *const children = object->container.children;
    const size_t length = SeqLength(children);
    for (size_t i = 0; i < length; i++)
    {
        const JsonElement *const child = SeqAt(children, i);
        if (child->propertyName == NULL)
        {
            return false;
        }
    }
    return true;
}

#endif

void JsonObjectWrite(
    Writer *const writer,
    const JsonElement *const object,
    const size_t indent_level)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(object->container.type == JSON_CONTAINER_TYPE_OBJECT);
    assert(object->container.children != NULL);

    WriterWrite(writer, "{\n");

    assert(all_children_have_keys(object));

    // sort the children Seq so the output is canonical (keys are sorted)
    // we've already asserted that the children have a valid propertyName
    JsonSort(object, (JsonComparator *) JsonElementPropertyCompare, NULL);

    Seq *const children = object->container.children;
    const size_t length = SeqLength(children);
    for (size_t i = 0; i < length; i++)
    {
        JsonElement *child = SeqAt(children, i);

        PrintIndent(writer, indent_level + 1);

        assert(child->propertyName != NULL);
        WriterWriteF(writer, "\"%s\": ", child->propertyName);

        switch (child->type)
        {
        case JSON_ELEMENT_TYPE_PRIMITIVE:
            JsonPrimitiveWrite(writer, child, 0);
            break;

        case JSON_ELEMENT_TYPE_CONTAINER:
            JsonContainerWrite(writer, child, indent_level + 1);
            break;

        default:
            UnexpectedError("Unknown JSON element type: %d", child->type);
        }

        if (i < length - 1)
        {
            WriterWriteChar(writer, ',');
        }
        WriterWrite(writer, "\n");
    }

    PrintIndent(writer, indent_level);
    WriterWriteChar(writer, '}');
}

static void JsonContainerWrite(
    Writer *const writer,
    const JsonElement *const container,
    const size_t indent_level)
{
    assert(container != NULL);
    assert(container->type == JSON_ELEMENT_TYPE_CONTAINER);

    switch (container->container.type)
    {
    case JSON_CONTAINER_TYPE_OBJECT:
        JsonObjectWrite(writer, container, indent_level);
        break;

    case JSON_CONTAINER_TYPE_ARRAY:
        JsonArrayWrite(writer, container, indent_level);
    }
}

void JsonWrite(
    Writer *const writer,
    const JsonElement *const element,
    const size_t indent_level)
{
    assert(writer != NULL);
    assert(element != NULL);

    switch (element->type)
    {
    case JSON_ELEMENT_TYPE_CONTAINER:
        JsonContainerWrite(writer, element, indent_level);
        break;

    case JSON_ELEMENT_TYPE_PRIMITIVE:
        JsonPrimitiveWrite(writer, element, indent_level);
        break;

    default:
        UnexpectedError("Unknown JSON element type: %d", element->type);
    }
}

static void JsonArrayWriteCompact(
    Writer *const writer, const JsonElement *const array)
{
    assert(array != NULL);
    assert(array->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(array->container.type == JSON_CONTAINER_TYPE_ARRAY);
    assert(array->container.children != NULL);

    if (JsonLength(array) == 0)
    {
        WriterWrite(writer, "[]");
        return;
    }

    WriterWrite(writer, "[");
    Seq *const children = array->container.children;
    const size_t length = SeqLength(children);
    for (size_t i = 0; i < length; i++)
    {
        JsonElement *const child = SeqAt(children, i);
        assert(child != NULL);

        switch (child->type)
        {
        case JSON_ELEMENT_TYPE_PRIMITIVE:
            JsonPrimitiveWrite(writer, child, 0);
            break;

        case JSON_ELEMENT_TYPE_CONTAINER:
            JsonContainerWriteCompact(writer, child);
            break;

        default:
            UnexpectedError("Unknown JSON element type: %d", child->type);
        }

        if (i < length - 1)
        {
            WriterWrite(writer, ",");
        }
    }

    WriterWriteChar(writer, ']');
}

static void JsonObjectWriteCompact(
    Writer *const writer, const JsonElement *const object)
{
    assert(object != NULL);
    assert(object->type == JSON_ELEMENT_TYPE_CONTAINER);
    assert(object->container.type == JSON_CONTAINER_TYPE_OBJECT);

    WriterWrite(writer, "{");

    assert(all_children_have_keys(object));

    // sort the children Seq so the output is canonical (keys are sorted)
    // we've already asserted that the children have a valid propertyName
    JsonSort(object, (JsonComparator *) JsonElementPropertyCompare, NULL);

    Seq *const children = object->container.children;
    const size_t length = SeqLength(children);
    for (size_t i = 0; i < length; i++)
    {
        JsonElement *child = SeqAt(children, i);

        WriterWriteF(writer, "\"%s\":", child->propertyName);

        switch (child->type)
        {
        case JSON_ELEMENT_TYPE_PRIMITIVE:
            JsonPrimitiveWrite(writer, child, 0);
            break;

        case JSON_ELEMENT_TYPE_CONTAINER:
            JsonContainerWriteCompact(writer, child);
            break;

        default:
            UnexpectedError("Unknown JSON element type: %d", child->type);
        }

        if (i < length - 1)
        {
            WriterWriteChar(writer, ',');
        }
    }

    WriterWriteChar(writer, '}');
}

static void JsonContainerWriteCompact(
    Writer *const writer, const JsonElement *const container)
{
    assert(container != NULL);
    assert(container->type == JSON_ELEMENT_TYPE_CONTAINER);

    switch (container->container.type)
    {
    case JSON_CONTAINER_TYPE_OBJECT:
        JsonObjectWriteCompact(writer, container);
        break;

    case JSON_CONTAINER_TYPE_ARRAY:
        JsonArrayWriteCompact(writer, container);
    }
}

void JsonWriteCompact(Writer *const w, const JsonElement *const element)
{
    assert(w != NULL);
    assert(element != NULL);

    switch (element->type)
    {
    case JSON_ELEMENT_TYPE_CONTAINER:
        JsonContainerWriteCompact(w, element);
        break;

    case JSON_ELEMENT_TYPE_PRIMITIVE:
        JsonPrimitiveWrite(w, element, 0);
        break;

    default:
        UnexpectedError("Unknown JSON element type: %d", element->type);
    }
}

// *******************************************************************************************
// Parsing
// *******************************************************************************************

static JsonParseError JsonParseAsObject(
    void *lookup_context,
    JsonLookup *lookup_function,
    const char **data,
    JsonElement **json_out);

static JsonElement *JsonParseAsBoolean(const char **const data)
{
    assert(data != NULL);

    if (StringStartsWith(*data, "true"))
    {
        char next = *(*data + 4);
        if (IsSeparator(next) || next == '\0')
        {
            *data += 3;
            return JsonBoolCreate(true);
        }
    }
    else if (StringStartsWith(*data, "false"))
    {
        char next = *(*data + 5);
        if (IsSeparator(next) || next == '\0')
        {
            *data += 4;
            return JsonBoolCreate(false);
        }
    }

    return NULL;
}

static JsonElement *JsonParseAsNull(const char **const data)
{
    assert(data != NULL);

    if (StringStartsWith(*data, "null"))
    {
        char next = *(*data + 4);
        if (IsSeparator(next) || next == '\0')
        {
            *data += 3;
            return JsonNullCreate();
        }
    }

    return NULL;
}

const char *JsonParseErrorToString(const JsonParseError error)
{
    assert(error < JSON_PARSE_ERROR_MAX);

    static const char *const parse_errors[JSON_PARSE_ERROR_MAX] = {
        [JSON_PARSE_OK] = "Success",

        [JSON_PARSE_ERROR_STRING_NO_DOUBLEQUOTE_START] =
            "Unable to parse json data as string, did not start with doublequote",
        [JSON_PARSE_ERROR_STRING_NO_DOUBLEQUOTE_END] =
            "Unable to parse json data as string, did not end with doublequote",

        [JSON_PARSE_ERROR_NUMBER_EXPONENT_NEGATIVE] =
            "Unable to parse json data as number, - not at the start or not after exponent",
        [JSON_PARSE_ERROR_NUMBER_EXPONENT_POSITIVE] =
            "Unable to parse json data as number, + without preceding exponent",
        [JSON_PARSE_ERROR_NUMBER_DUPLICATE_ZERO] =
            "Unable to parse json data as number, started with 0 before dot or exponent, duplicate 0 seen",
        [JSON_PARSE_ERROR_NUMBER_NO_DIGIT] =
            "Unable to parse json data as number, dot not preceded by digit",
        [JSON_PARSE_ERROR_NUMBER_MULTIPLE_DOTS] =
            "Unable to parse json data as number, two or more dots (decimal points)",
        [JSON_PARSE_ERROR_NUMBER_EXPONENT_DUPLICATE] =
            "Unable to parse json data as number, duplicate exponent",
        [JSON_PARSE_ERROR_NUMBER_EXPONENT_DIGIT] =
            "Unable to parse json data as number, exponent without preceding digit",
        [JSON_PARSE_ERROR_NUMBER_EXPONENT_FOLLOW_LEADING_ZERO] =
            "Unable to parse json data as number, dot or exponent must follow leading 0",
        [JSON_PARSE_ERROR_NUMBER_BAD_SYMBOL] =
            "Unable to parse json data as number, invalid symbol",
        [JSON_PARSE_ERROR_NUMBER_DIGIT_END] =
            "Unable to parse json data as string, did not end with digit",

        [JSON_PARSE_ERROR_ARRAY_START] =
            "Unable to parse json data as array, did not start with '['",
        [JSON_PARSE_ERROR_ARRAY_END] =
            "Unable to parse json data as array, did not end with ']'",
        [JSON_PARSE_ERROR_ARRAY_COMMA] =
            "Unable to parse json data as array, extraneous commas",

        [JSON_PARSE_ERROR_OBJECT_BAD_SYMBOL] =
            "Unable to parse json data as object, unrecognized token beginning entry",
        [JSON_PARSE_ERROR_OBJECT_START] =
            "Unable to parse json data as object, did not start with '{'",
        [JSON_PARSE_ERROR_OBJECT_END] =
            "Unable to parse json data as string, did not end with '}'",
        [JSON_PARSE_ERROR_OBJECT_COLON] =
            "Unable to parse json data as object, ':' seen without having specified an l-value",
        [JSON_PARSE_ERROR_OBJECT_COMMA] =
            "Unable to parse json data as object, ',' seen without having specified an r-value",
        [JSON_PARSE_ERROR_OBJECT_ARRAY_LVAL] =
            "Unable to parse json data as object, array not allowed as l-value",
        [JSON_PARSE_ERROR_OBJECT_OBJECT_LVAL] =
            "Unable to parse json data as object, object not allowed as l-value",
        [JSON_PARSE_ERROR_OBJECT_OPEN_LVAL] =
            "Unable to parse json data as object, tried to close object having opened an l-value",

        [JSON_PARSE_ERROR_INVALID_START] =
            "Unwilling to parse json data starting with invalid character",
        [JSON_PARSE_ERROR_INVALID_END] =
            "Unwilling to parse json data with trailing non-whitespace characters",
        [JSON_PARSE_ERROR_TRUNCATED] =
            "Unable to parse JSON without truncating",
        [JSON_PARSE_ERROR_NO_LIBYAML] =
            "CFEngine was not built with libyaml support",
        [JSON_PARSE_ERROR_LIBYAML_FAILURE] = "libyaml internal failure",
        [JSON_PARSE_ERROR_NO_SUCH_FILE] = "No such file or directory",
        [JSON_PARSE_ERROR_NO_DATA] = "No data"};

    return parse_errors[error];
}

static JsonParseError JsonParseAsString(
    const char **const data, char **const str_out)
{
    assert(data != NULL);
    assert(*data != NULL);
    assert(str_out != NULL);

    /* NB: although JavaScript supports both single and double quotes
     * as string delimiters, JSON only supports double quotes. */
    if (**data != '"')
    {
        *str_out = NULL;
        return JSON_PARSE_ERROR_STRING_NO_DOUBLEQUOTE_START;
    }

    Writer *writer = StringWriter();

    for (*data = *data + 1; **data != '\0'; *data = *data + 1)
    {
        switch (**data)
        {
        case '"':
            *str_out = StringWriterClose(writer);
            return JSON_PARSE_OK;

        case '\\':
            *data = *data + 1;
            switch (**data)
            {
            case '\\':
                break;
            case '"':
                break;
            case '/':
                break;

            case 'b':
                WriterWriteChar(writer, '\b');
                continue;
            case 'f':
                WriterWriteChar(writer, '\f');
                continue;
            case 'n':
                WriterWriteChar(writer, '\n');
                continue;
            case 'r':
                WriterWriteChar(writer, '\r');
                continue;
            case 't':
                WriterWriteChar(writer, '\t');
                continue;

            default:
                /* Unrecognised escape sequence.
                 *
                 * For example, we fail to handle Unicode escapes -
                 * \u{hex digits} - we have no way to represent the
                 * character they denote.  So keep them verbatim, for
                 * want of any other way to handle them; but warn. */
                Log(LOG_LEVEL_DEBUG,
                    "Keeping verbatim unrecognised JSON escape '%.6s'",
                    *data - 1); // Include the \ in the displayed escape
                WriterWriteChar(writer, '\\');
                break;
            }
            /* fall through */
        default:
            WriterWriteChar(writer, **data);
            break;
        }
    }

    WriterClose(writer);
    *str_out = NULL;
    return JSON_PARSE_ERROR_STRING_NO_DOUBLEQUOTE_END;
}

JsonParseError JsonParseAsNumber(
    const char **const data, JsonElement **const json_out)
{
    assert(data != NULL);
    assert(*data != NULL);
    assert(json_out != NULL);

    Writer *writer = StringWriter();

    bool zero_started = false;
    bool seen_dot = false;
    bool seen_exponent = false;

    char prev_char = 0;

    for (*data = *data; **data != '\0' && !IsSeparator(**data);
         prev_char = **data, *data = *data + 1)
    {
        switch (**data)
        {
        case '-':
            if (prev_char != 0 && prev_char != 'e' && prev_char != 'E')
            {
                *json_out = NULL;
                WriterClose(writer);
                return JSON_PARSE_ERROR_NUMBER_EXPONENT_NEGATIVE;
            }
            break;

        case '+':
            if (prev_char != 'e' && prev_char != 'E')
            {
                *json_out = NULL;
                WriterClose(writer);
                return JSON_PARSE_ERROR_NUMBER_EXPONENT_POSITIVE;
            }
            break;

        case '0':
            if (zero_started && !seen_dot && !seen_exponent)
            {
                *json_out = NULL;
                WriterClose(writer);
                return JSON_PARSE_ERROR_NUMBER_DUPLICATE_ZERO;
            }
            if (prev_char == 0)
            {
                zero_started = true;
            }
            break;

        case '.':
            if (seen_dot)
            {
                *json_out = NULL;
                WriterClose(writer);
                return JSON_PARSE_ERROR_NUMBER_MULTIPLE_DOTS;
            }
            if (prev_char != '0' && !IsDigit(prev_char))
            {
                *json_out = NULL;
                WriterClose(writer);
                return JSON_PARSE_ERROR_NUMBER_NO_DIGIT;
            }
            seen_dot = true;
            break;

        case 'e':
        case 'E':
            if (seen_exponent)
            {
                *json_out = NULL;
                WriterClose(writer);
                return JSON_PARSE_ERROR_NUMBER_EXPONENT_DUPLICATE;
            }
            else if (!IsDigit(prev_char) && prev_char != '0')
            {
                *json_out = NULL;
                WriterClose(writer);
                return JSON_PARSE_ERROR_NUMBER_EXPONENT_DIGIT;
            }
            seen_exponent = true;
            break;

        default:
            if (zero_started && !seen_dot && !seen_exponent)
            {
                *json_out = NULL;
                WriterClose(writer);
                return JSON_PARSE_ERROR_NUMBER_EXPONENT_FOLLOW_LEADING_ZERO;
            }

            if (!IsDigit(**data))
            {
                *json_out = NULL;
                WriterClose(writer);
                return JSON_PARSE_ERROR_NUMBER_BAD_SYMBOL;
            }
            break;
        }

        WriterWriteChar(writer, **data);
    }

    if (prev_char != '0' && !IsDigit(prev_char))
    {
        *json_out = NULL;
        WriterClose(writer);
        return JSON_PARSE_ERROR_NUMBER_DIGIT_END;
    }

    // rewind 1 char so caller will see separator next
    *data = *data - 1;

    if (seen_dot)
    {
        *json_out = JsonElementCreatePrimitive(
            JSON_PRIMITIVE_TYPE_REAL, StringWriterClose(writer));
        return JSON_PARSE_OK;
    }
    else
    {
        *json_out = JsonElementCreatePrimitive(
            JSON_PRIMITIVE_TYPE_INTEGER, StringWriterClose(writer));
        return JSON_PARSE_OK;
    }
}

static JsonParseError JsonParseAsPrimitive(
    const char **const data, JsonElement **const json_out)
{
    assert(json_out != NULL);
    assert(data != NULL);
    assert(*data != NULL);

    if (**data == '"')
    {
        char *value = NULL;
        const JsonParseError err = JsonParseAsString(data, &value);
        if (err != JSON_PARSE_OK)
        {
            return err;
        }
        *json_out = JsonElementCreatePrimitive(
            JSON_PRIMITIVE_TYPE_STRING, JsonDecodeString(value));
        free(value);
        return JSON_PARSE_OK;
    }
    else
    {
        if (**data == '-' || **data == '0' || IsDigit(**data))
        {
            const JsonParseError err = JsonParseAsNumber(data, json_out);
            if (err != JSON_PARSE_OK)
            {
                return err;
            }
            return JSON_PARSE_OK;
        }

        JsonElement *const child_bool = JsonParseAsBoolean(data);
        if (child_bool != NULL)
        {
            *json_out = child_bool;
            return JSON_PARSE_OK;
        }

        JsonElement *const child_null = JsonParseAsNull(data);
        if (child_null != NULL)
        {
            *json_out = child_null;
            return JSON_PARSE_OK;
        }

        *json_out = NULL;
        return JSON_PARSE_ERROR_OBJECT_BAD_SYMBOL;
    }
}

static JsonParseError JsonParseAsArray(
    void *const lookup_context,
    JsonLookup *const lookup_function,
    const char **const data,
    JsonElement **const json_out)
{
    assert(json_out != NULL);
    assert(data != NULL);
    assert(*data != NULL);

    if (**data != '[')
    {
        *json_out = NULL;
        return JSON_PARSE_ERROR_ARRAY_START;
    }

    JsonElement *array = JsonArrayCreate(DEFAULT_CONTAINER_CAPACITY);
    char prev_char = '[';

    for (*data = *data + 1; **data != '\0'; *data = *data + 1)
    {
        if (IsWhitespace(**data))
        {
            continue;
        }

        switch (**data)
        {
        case '"':
        {
            char *value = NULL;
            JsonParseError err = JsonParseAsString(data, &value);
            if (err != JSON_PARSE_OK)
            {
                return err;
            }
            JsonArrayAppendElement(
                array,
                JsonElementCreatePrimitive(
                    JSON_PRIMITIVE_TYPE_STRING, JsonDecodeString(value)));
            free(value);
        }
        break;

        case '[':
        {
            if (prev_char != '[' && prev_char != ',')
            {
                JsonDestroy(array);
                return JSON_PARSE_ERROR_ARRAY_START;
            }
            JsonElement *child_array = NULL;
            JsonParseError err = JsonParseAsArray(
                lookup_context, lookup_function, data, &child_array);
            if (err != JSON_PARSE_OK)
            {
                JsonDestroy(array);
                return err;
            }
            assert(child_array);

            JsonArrayAppendArray(array, child_array);
        }
        break;

        case '{':
        {
            if (prev_char != '[' && prev_char != ',')
            {
                JsonDestroy(array);
                return JSON_PARSE_ERROR_ARRAY_START;
            }
            JsonElement *child_object = NULL;
            JsonParseError err = JsonParseAsObject(
                lookup_context, lookup_function, data, &child_object);
            if (err != JSON_PARSE_OK)
            {
                JsonDestroy(array);
                return err;
            }
            assert(child_object);

            JsonArrayAppendObject(array, child_object);
        }
        break;

        case ',':
            if (prev_char == ',' || prev_char == '[')
            {
                JsonDestroy(array);
                return JSON_PARSE_ERROR_ARRAY_COMMA;
            }
            break;

        case ']':
            *json_out = array;
            return JSON_PARSE_OK;

        default:
            if (**data == '-' || **data == '0' || IsDigit(**data))
            {
                JsonElement *child = NULL;
                JsonParseError err = JsonParseAsNumber(data, &child);
                if (err != JSON_PARSE_OK)
                {
                    JsonDestroy(array);
                    return err;
                }
                assert(child);

                JsonArrayAppendElement(array, child);
                break;
            }

            JsonElement *child_bool = JsonParseAsBoolean(data);
            if (child_bool != NULL)
            {
                JsonArrayAppendElement(array, child_bool);
                break;
            }

            JsonElement *child_null = JsonParseAsNull(data);
            if (child_null != NULL)
            {
                JsonArrayAppendElement(array, child_null);
                break;
            }

            if (lookup_function != NULL)
            {
                JsonElement *child_ref =
                    (*lookup_function)(lookup_context, data);
                if (child_ref != NULL)
                {
                    JsonArrayAppendElement(array, child_ref);
                    break;
                }
            }

            *json_out = NULL;
            JsonDestroy(array);
            return JSON_PARSE_ERROR_OBJECT_BAD_SYMBOL;
        }

        prev_char = **data;
    }

    *json_out = NULL;
    JsonDestroy(array);
    return JSON_PARSE_ERROR_ARRAY_END;
}

static bool wc(char c)
{
    // word character, \w in regex
    return (isalnum(c) || c == '_');
}

static const char *unquoted_key_with_colon(const char *data)
{
    // Implements the regex: ^\w[-\w]*\s*:
    if (!wc(data[0]))
    {
        return NULL;
    }
    int i;
    for (i = 1; data[i] != '\0' && (data[i] == '-' || wc(data[i])); ++i)
    {
        // Skip past word characters (\w) and dashes
    }
    for (; data[i] != '\0' && IsWhitespace(data[i]); ++i)
    {
        // Skip past whitespace
    }
    if (data[i] != ':')
    {
        return NULL;
    }
    return data + i;
}

static JsonParseError JsonParseAsObject(
    void *const lookup_context,
    JsonLookup *const lookup_function,
    const char **const data,
    JsonElement **const json_out)
{
    assert(json_out != NULL);
    assert(data != NULL);
    assert(*data != NULL);

    if (**data != '{')
    {
        *json_out = NULL;
        return JSON_PARSE_ERROR_ARRAY_START;
    }

    JsonElement *object = JsonObjectCreate(DEFAULT_CONTAINER_CAPACITY);
    char *property_name = NULL;
    char prev_char = '{';

    for (*data = *data + 1; **data != '\0'; *data = *data + 1)
    {
        if (IsWhitespace(**data))
        {
            continue;
        }

        switch (**data)
        {
        case '"':
            if (property_name != NULL)
            {
                char *property_value = NULL;
                JsonParseError err = JsonParseAsString(data, &property_value);
                if (err != JSON_PARSE_OK)
                {
                    free(property_name);
                    JsonDestroy(object);
                    return err;
                }
                assert(property_value);

                JsonObjectAppendElement(
                    object,
                    property_name,
                    JsonElementCreatePrimitive(
                        JSON_PRIMITIVE_TYPE_STRING,
                        JsonDecodeString(property_value)));
                free(property_value);
                free(property_name);
                property_name = NULL;
            }
            else
            {
                property_name = NULL;
                JsonParseError err = JsonParseAsString(data, &property_name);
                if (err != JSON_PARSE_OK)
                {
                    JsonDestroy(object);
                    return err;
                }
                assert(property_name);
            }
            break;

        case ':':
            if (property_name == NULL || prev_char == ':' || prev_char == ',')
            {
                *json_out = NULL;
                free(property_name);
                JsonDestroy(object);
                return JSON_PARSE_ERROR_OBJECT_COLON;
            }
            break;

        case ',':
            if (property_name != NULL || prev_char == ':' || prev_char == ',')
            {
                free(property_name);
                JsonDestroy(object);
                return JSON_PARSE_ERROR_OBJECT_COMMA;
            }
            break;

        case '[':
            if (property_name != NULL)
            {
                JsonElement *child_array = NULL;
                JsonParseError err = JsonParseAsArray(
                    lookup_context, lookup_function, data, &child_array);
                if (err != JSON_PARSE_OK)
                {
                    free(property_name);
                    JsonDestroy(object);
                    return err;
                }

                JsonObjectAppendArray(object, property_name, child_array);
                free(property_name);
                property_name = NULL;
            }
            else
            {
                free(property_name);
                JsonDestroy(object);
                return JSON_PARSE_ERROR_OBJECT_ARRAY_LVAL;
            }
            break;

        case '{':
            if (property_name != NULL)
            {
                JsonElement *child_object = NULL;
                JsonParseError err = JsonParseAsObject(
                    lookup_context, lookup_function, data, &child_object);
                if (err != JSON_PARSE_OK)
                {
                    free(property_name);
                    JsonDestroy(object);
                    return err;
                }

                JsonObjectAppendObject(object, property_name, child_object);
                free(property_name);
                property_name = NULL;
            }
            else
            {
                *json_out = NULL;
                free(property_name);
                JsonDestroy(object);
                return JSON_PARSE_ERROR_OBJECT_OBJECT_LVAL;
            }
            break;

        case '}':
            if (property_name != NULL)
            {
                *json_out = NULL;
                free(property_name);
                JsonDestroy(object);
                return JSON_PARSE_ERROR_OBJECT_OPEN_LVAL;
            }
            free(property_name);
            *json_out = object;
            return JSON_PARSE_OK;

        default:
        {
            const char *colon = NULL;
            // Note the character class excludes ':'.
            // This will match the key from { foo : 2 } but not { -foo: 2 }
            if (property_name == NULL
                && (colon = unquoted_key_with_colon(*data)) != NULL)
            {
                // Step backwards until we are on the last whitespace.

                // Note that this is safe because the above function guarantees
                // we will find at least one non-whitespace character as we
                // go backwards.
                const char *ws = colon;
                while (IsWhitespace(*(ws - 1)))
                {
                    ws -= 1;
                }

                property_name = xstrndup(*data, ws - *data);
                *data = colon;

                break;
            }
            else if (property_name != NULL)
            {
                if (**data == '-' || **data == '0' || IsDigit(**data))
                {
                    JsonElement *child = NULL;
                    JsonParseError err = JsonParseAsNumber(data, &child);
                    if (err != JSON_PARSE_OK)
                    {
                        free(property_name);
                        JsonDestroy(object);
                        return err;
                    }
                    JsonObjectAppendElement(object, property_name, child);
                    free(property_name);
                    property_name = NULL;
                    break;
                }

                JsonElement *child_bool = JsonParseAsBoolean(data);
                if (child_bool != NULL)
                {
                    JsonObjectAppendElement(object, property_name, child_bool);
                    free(property_name);
                    property_name = NULL;
                    break;
                }

                JsonElement *child_null = JsonParseAsNull(data);
                if (child_null != NULL)
                {
                    JsonObjectAppendElement(object, property_name, child_null);
                    free(property_name);
                    property_name = NULL;
                    break;
                }

                if (lookup_function != NULL)
                {
                    JsonElement *child_ref =
                        (*lookup_function)(lookup_context, data);
                    if (child_ref != NULL)
                    {
                        JsonObjectAppendElement(
                            object, property_name, child_ref);
                        free(property_name);
                        property_name = NULL;
                        break;
                    }
                }
            }

            *json_out = NULL;
            free(property_name);
            JsonDestroy(object);
            return JSON_PARSE_ERROR_OBJECT_BAD_SYMBOL;
        } // default
        } // switch

        prev_char = **data;
    }

    *json_out = NULL;
    free(property_name);
    JsonDestroy(object);
    return JSON_PARSE_ERROR_OBJECT_END;
}

JsonParseError JsonParse(const char **const data, JsonElement **const json_out)
{
    return JsonParseWithLookup(NULL, NULL, data, json_out);
}

JsonParseError JsonParseAll(const char **const data, JsonElement **const json_out)
{
    JsonParseError error = JsonParseWithLookup(NULL, NULL, data, json_out);
    if (error == JSON_PARSE_OK && **data != '\0')
    {
        /* The parser has advanced the data pointer up to the last byte
         * processed. Thus every subsequent character until the nullbyte
         * should be whitespace. */
        for (const char *ch = (*data) + 1; *ch != '\0'; ch++)
        {
            if (isspace(*ch))
            {
                continue;
            }
            JsonDestroy(*json_out);
            *json_out = NULL;
            return JSON_PARSE_ERROR_INVALID_END;
        }
    }
    return error;
}

JsonParseError JsonParseWithLookup(
    void *const lookup_context,
    JsonLookup *const lookup_function,
    const char **const data,
    JsonElement **const json_out)
{
    assert(data != NULL);
    assert(*data != NULL);
    if (data == NULL || *data == NULL)
    {
        return JSON_PARSE_ERROR_NO_DATA;
    }

    while (**data)
    {
        if (**data == '{')
        {
            return JsonParseAsObject(
                lookup_context, lookup_function, data, json_out);
        }
        else if (**data == '[')
        {
            return JsonParseAsArray(
                lookup_context, lookup_function, data, json_out);
        }
        else if (IsWhitespace(**data))
        {
            (*data)++;
        }
        else
        {
            return JsonParseAsPrimitive(data, json_out);
        }
    }

    return JSON_PARSE_ERROR_NO_DATA;
}

JsonParseError JsonParseAnyFile(
    const char *const path,
    const size_t size_max,
    JsonElement **const json_out,
    const bool yaml_format)
{
    assert(json_out != NULL);

    bool truncated = false;
    Writer *contents = FileRead(path, size_max, &truncated);
    if (contents == NULL)
    {
        return JSON_PARSE_ERROR_NO_SUCH_FILE;
    }
    else if (truncated)
    {
        return JSON_PARSE_ERROR_TRUNCATED;
    }
    assert(json_out);
    *json_out = NULL;
    const char *data = StringWriterData(contents);
    JsonParseError err;

    if (yaml_format)
    {
        err = JsonParseYamlString(&data, json_out);
    }
    else
    {
        err = JsonParse(&data, json_out);
    }

    WriterClose(contents);
    return err;
}

JsonParseError JsonParseFile(
    const char *const path,
    const size_t size_max,
    JsonElement **const json_out)
{
    return JsonParseAnyFile(path, size_max, json_out, false);
}

bool JsonWalk(JsonElement *element,
              JsonElementVisitor object_visitor,
              JsonElementVisitor array_visitor,
              JsonElementVisitor primitive_visitor,
              void *data)
{
    assert(element != NULL);

    if (element->type == JSON_ELEMENT_TYPE_PRIMITIVE)
    {
        if (primitive_visitor != NULL)
        {
            return primitive_visitor(element, data);
        }
        else
        {
            return true;
        }
    }

    assert(element->type == JSON_ELEMENT_TYPE_CONTAINER);
    if (element->container.type == JSON_CONTAINER_TYPE_ARRAY)
    {
        if (array_visitor != NULL)
        {
            bool keep_going = array_visitor(element, data);
            if (!keep_going)
            {
                return false;
            }
        }
    }
    else
    {
        assert(element->container.type == JSON_CONTAINER_TYPE_OBJECT);
        if (object_visitor != NULL)
        {
            bool keep_going = object_visitor(element, data);
            if (!keep_going)
            {
                return false;
            }
        }
    }

    bool keep_going = true;
    JsonIterator iter = JsonIteratorInit(element);
    while (keep_going && JsonIteratorHasMore(&iter))
    {
        JsonElement *child = JsonIteratorNextValue(&iter);
        keep_going = JsonWalk(child, object_visitor, array_visitor, primitive_visitor, data);
    }
    return keep_going;
}

bool JsonErrorVisitor(ARG_UNUSED JsonElement *element, ARG_UNUSED void *data)
{
    return false;
}

/*******************************************************************/

#ifdef WITH_PCRE2
#include <json-pcre.h>

// returns NULL on any failure
// takes either a pre-compiled pattern OR a regex (one of the two shouldn't be
// NULL)
JsonElement *StringCaptureData(
    const Regex *const regex, const char *const pattern, const char *const data)
{
    assert(regex != NULL || pattern != NULL);
    assert(data != NULL);

    Seq *s;

    if (regex != NULL)
    {
        s = StringMatchCapturesWithPrecompiledRegex(regex, data, true);
    }
    else
    {
        s = StringMatchCaptures(pattern, data, true);
    }

    const size_t length = (s != NULL) ? SeqLength(s) : 0;

    if (length == 0)
    {
        SeqDestroy(s);
        return NULL;
    }

    JsonElement *json = JsonObjectCreate(length / 2);

    for (size_t i = 1; i < length; i += 2)
    {
        Buffer *key = SeqAt(s, i - 1);
        Buffer *value = SeqAt(s, i);

        JsonObjectAppendString(json, BufferData(key), BufferData(value));
    }

    SeqDestroy(s);

    JsonObjectRemoveKey(json, "0");
    return json;
}

#endif // WITH_PCRE2
